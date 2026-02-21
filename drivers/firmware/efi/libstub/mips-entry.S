/* SPDX-License-Identifier: GPL-2.0 */
/*
 * EFI PE/COFF entry trampoline for MIPS
 *
 * MIPS does not have PC-relative data addressing, so PIC code uses a
 * Global Offset Table (GOT) accessed via the $gp register. The GOT
 * entries are resolved by the static linker to absolute virtual
 * addresses. When UEFI loads the PE image at an arbitrary physical
 * address, those GOT entries are wrong.
 *
 * This trampoline:
 *  1. Determines the actual load address using bal (PC-relative)
 *  2. Computes the delta between load and link addresses
 *  3. Adjusts every GOT entry by adding the delta
 *  4. Sets $t9 = actual address of efi_pe_entry (for $gp setup)
 *  5. Jumps to the C entry point
 *
 * UEFI calling convention for efi_pe_entry:
 *   $a0 = EFI handle
 *   $a1 = EFI system table pointer
 *
 * These registers are preserved across the trampoline.
 */

#include <asm/asm.h>
#include <asm/regdef.h>

	/*
	 * Force non-PIC instruction expansion for 'dla' (address loads).
	 * The C code in libstub is compiled with -fpic -mabicalls, but
	 * this trampoline must use absolute address sequences because it
	 * runs BEFORE the GOT is fixed up. The absolute addresses are
	 * only used to compute deltas, not for direct use.
	 */
	.option	pic0
	.set	noreorder

	.section ".text", "ax"

	.globl	efi_entry
	.type	efi_entry, @function
efi_entry:
	/*
	 * Save the UEFI arguments and return address on the stack.
	 */
	daddiu	sp, sp, -48
	sd	ra, 40(sp)
	sd	a0, 32(sp)
	sd	a1, 24(sp)
	sd	s0, 16(sp)

	/*
	 * Step 1: Get our actual runtime address.
	 *
	 * 'bal .Lhere' writes PC+8 into $ra, then jumps to .Lhere
	 * which is the very next instruction. After execution, $ra
	 * holds the actual runtime address of .Lhere.
	 */
	bal	.Lhere
	 nop
.Lhere:
	move	s0, ra		/* s0 = actual address of .Lhere */

	/*
	 * Step 2: Compute delta = actual - link_time.
	 *
	 * 'dla' loads the link-time (virtual) address of .Lhere.
	 * The difference is the relocation delta.
	 */
	dla	t0, .Lhere	/* t0 = link-time address of .Lhere */
	dsubu	s0, s0, t0	/* s0 = delta (reuse as persistent delta) */

	/* If delta == 0, we're at the link address — skip GOT fixup */
	beqz	s0, .Lgot_done
	 nop

	/*
	 * Step 3: Fix up the GOT.
	 *
	 * Walk from _got_start to _got_end, adding delta to each
	 * 8-byte (MIPS64) GOT entry. The first two GOT entries are
	 * reserved (lazy resolver / module pointer) — we fix them
	 * all uniformly since adding delta to any value is safe.
	 */
	dla	t0, _got_start
	daddu	t0, t0, s0	/* t0 = actual _got_start */
	dla	t1, _got_end
	daddu	t1, t1, s0	/* t1 = actual _got_end */

.Lgot_loop:
	beq	t0, t1, .Lgot_done
	 nop
	ld	t2, 0(t0)	/* load GOT entry */
	daddu	t2, t2, s0	/* add delta */
	sd	t2, 0(t0)	/* store fixed entry */
	b	.Lgot_loop
	 daddiu	t0, t0, 8	/* next entry (in delay slot) */

.Lgot_done:
	/*
	 * Step 4: Call efi_pe_entry with correct $t9.
	 *
	 * With -mabicalls, the C function prologue computes $gp from
	 * $t9: gp = t9 + const. So $t9 must hold the actual address
	 * of efi_pe_entry at the call site.
	 */
	dla	t9, efi_pe_entry
	daddu	t9, t9, s0	/* t9 = actual address of efi_pe_entry */

	/* Restore UEFI arguments */
	ld	a0, 32(sp)
	ld	a1, 24(sp)
	ld	s0, 16(sp)
	ld	ra, 40(sp)
	daddiu	sp, sp, 48

	/* Tail-call into efi_pe_entry via jalr to set $ra for it */
	jr	t9
	 nop
	.size	efi_entry, . - efi_entry
