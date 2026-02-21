#!/bin/bash
# run.sh - Boot MIPS64 EFI kernel with QEMU loongson3-virt
#
# Usage:
#   ./run.sh              # Boot vmlinux.efi via UEFI firmware
#   ./run.sh --kernel     # Boot vmlinux ELF directly (bypass EFI)
#   ./run.sh --rootfs [path]  # Attach rootfs disk (default: ./rootfs.ext4)
#   ./run.sh --debug      # Same as default but with GDB server on :1234
#
# Examples:
#   ./run.sh --kernel --rootfs              # Direct boot with rootfs
#   ./run.sh --rootfs                       # EFI boot with rootfs
#   ./run.sh --rootfs /path/to/rootfs.ext4  # Custom rootfs path
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIOS="${SCRIPT_DIR}/bios_loongson3.bin"
VMLINUX="${SCRIPT_DIR}/vmlinux"
VMLINUX_EFI="${SCRIPT_DIR}/arch/mips/boot/vmlinux.efi"
EFI_DISK="/tmp/mips64-efi-boot.img"
ROOTFS=""

# Default: EFI boot
MODE="efi"
SMP=1
EXTRA_QEMU_ARGS=""

ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    arg="${ARGS[$i]}"
    case "$arg" in
        --kernel) MODE="kernel" ;;
        --rootfs)
            # --rootfs takes an optional path argument
            next_i=$((i + 1))
            if [ $next_i -lt ${#ARGS[@]} ] && [[ "${ARGS[$next_i]}" != --* ]]; then
                ROOTFS="${ARGS[$next_i]}"
                i=$next_i
            else
                ROOTFS="${SCRIPT_DIR}/rootfs.ext4"
            fi
            ;;
        --smp)
            next_i=$((i + 1))
            if [ $next_i -lt ${#ARGS[@]} ] && [[ "${ARGS[$next_i]}" =~ ^[0-9]+$ ]]; then
                SMP="${ARGS[$next_i]}"
                i=$next_i
            else
                echo "ERROR: --smp requires a number argument"; exit 1
            fi
            ;;
        --debug)  EXTRA_QEMU_ARGS="-s -S" ; echo "GDB server on :1234, waiting for connection..." ;;
        --help|-h)
            echo "Usage: $0 [--kernel] [--rootfs [path]] [--smp N] [--debug]"
            echo "  (default)     Boot vmlinux.efi via UEFI firmware"
            echo "  --kernel      Boot vmlinux ELF directly (bypass EFI, uses -kernel)"
            echo "  --rootfs [p]  Attach rootfs disk image (default: ./rootfs.ext4)"
            echo "  --smp N       Number of CPUs (default: 4)"
            echo "  --debug       Start with GDB server on :1234 (paused)"
            echo ""
            echo "Options can be combined: $0 --kernel --rootfs --smp 2 --debug"
            echo ""
            echo "To create a rootfs image, run: ./create-rootfs.sh"
            exit 0
            ;;
        *) echo "Unknown option: $arg"; exit 1 ;;
    esac
    i=$((i + 1))
done

# ── Sanity checks ──────────────────────────────────────────────────

if [ ! -f "$BIOS" ]; then
    echo "ERROR: Loongson BIOS not found at: $BIOS"
    echo "       Place bios_loongson3.bin in the kernel tree root."
    exit 1
fi

if [ "$MODE" = "kernel" ] && [ ! -f "$VMLINUX" ]; then
    echo "ERROR: vmlinux not found. Build with:"
    echo "  make ARCH=mips LLVM=1 LLVM_IAS=0 CROSS_COMPILE=mips64el-linux-gnuabi64- -j\$(nproc) vmlinux"
    exit 1
fi

if [ "$MODE" = "efi" ] && [ ! -f "$VMLINUX_EFI" ]; then
    echo "ERROR: vmlinux.efi not found. Build with:"
    echo "  make ARCH=mips LLVM=1 LLVM_IAS=0 CROSS_COMPILE=mips64el-linux-gnuabi64- vmlinux.efi"
    exit 1
fi

if [ -n "$ROOTFS" ] && [ ! -f "$ROOTFS" ]; then
    echo "ERROR: Rootfs image not found at: $ROOTFS"
    echo "       Create one with: ./create-rootfs.sh"
    exit 1
fi

# ── Create FAT disk image with EFI binary ─────────────────────────
#
# Build a raw FAT32 disk image containing:
#   /EFI/BOOT/BOOTMIPS64.EFI   (standard UEFI default boot path)
#   /vmlinux.efi                (fallback, accessible from UEFI shell)
#
# The image is 64MB to comfortably fit the ~17MB kernel.

create_efi_disk() {
    local img="$1"
    local efi_bin="$2"

    echo "Creating EFI boot disk at ${img}..."

    # Create a raw image and format as FAT32
    dd if=/dev/zero of="${img}" bs=1M count=64 status=none

    if command -v mkfs.vfat &>/dev/null; then
        mkfs.vfat -F 32 -n "EFI" "${img}" >/dev/null 2>&1
    elif command -v mkfs.fat &>/dev/null; then
        mkfs.fat -F 32 -n "EFI" "${img}" >/dev/null 2>&1
    else
        echo "ERROR: mkfs.vfat or mkfs.fat not found."
        echo "       Install dosfstools: sudo apt install dosfstools"
        exit 1
    fi

    # Create startup.nsh that auto-boots the kernel
    local nsh_tmp
    nsh_tmp="$(mktemp)"
    local rootfs_args=""
    if [ -n "$ROOTFS" ]; then
        rootfs_args=" root=/dev/vda rw"
    fi
    printf 'echo "Starting vmlinux.efi via startup.nsh..."\r\n' > "${nsh_tmp}"
    printf 'fs0:\\vmlinux.efi earlycon console=ttyS0%s\r\n' "${rootfs_args}" >> "${nsh_tmp}"
    printf 'echo "vmlinux.efi returned, dropping to shell"\r\n' >> "${nsh_tmp}"

    # Copy EFI binary and startup.nsh into the FAT image
    if command -v mmd &>/dev/null && command -v mcopy &>/dev/null; then
        # mtools available — use directly, no mount needed, no root needed
        mmd -i "${img}" ::EFI
        mmd -i "${img}" ::EFI/BOOT
        mcopy -i "${img}" "${efi_bin}" ::EFI/BOOT/BOOTMIPS64.EFI
        mcopy -i "${img}" "${efi_bin}" ::vmlinux.efi
        mcopy -i "${img}" "${nsh_tmp}" ::startup.nsh
    elif [ "$(id -u)" = "0" ]; then
        # Running as root — can use loop mount
        local mnt
        mnt="$(mktemp -d)"
        mount -o loop "${img}" "${mnt}"
        mkdir -p "${mnt}/EFI/BOOT"
        cp "${efi_bin}" "${mnt}/EFI/BOOT/BOOTMIPS64.EFI"
        cp "${efi_bin}" "${mnt}/vmlinux.efi"
        cp "${nsh_tmp}" "${mnt}/startup.nsh"
        sync
        umount "${mnt}"
        rmdir "${mnt}"
    else
        rm -f "${nsh_tmp}"
        echo "ERROR: Cannot populate FAT image."
        echo "       Install mtools: sudo apt install mtools"
        echo "       Or run as root to use loop mount."
        exit 1
    fi

    rm -f "${nsh_tmp}"

    echo "  -> $(du -h "${img}" | cut -f1) disk with vmlinux.efi"
}

# ── Launch QEMU ───────────────────────────────────────────────────

COMMON_ARGS=(
    -M loongson3-virt,accel=tcg
    -cpu Loongson-3A4000
    -smp "$SMP"
    -m 2G
    -serial stdio
    -display none
    -no-reboot
    -device virtio-scsi-pci,id=scsi0
    -device qemu-xhci
    -device usb-kbd
    -netdev user,id=net0
    -device virtio-net-pci,netdev=net0
)

# ── Rootfs disk arguments ─────────────────────────────────────────

ROOTFS_QEMU_ARGS=()
if [ -n "$ROOTFS" ]; then
    ROOTFS_QEMU_ARGS=(
        -drive "file=${ROOTFS},id=rootdisk,if=none,format=raw"
        -device "virtio-blk-pci,drive=rootdisk"
    )
fi

case "$MODE" in
    efi)
        create_efi_disk "$EFI_DISK" "$VMLINUX_EFI"

        echo ""
        echo "Booting vmlinux.efi via UEFI firmware..."
        echo "  BIOS:   ${BIOS}"
        echo "  Kernel: ${VMLINUX_EFI}"
        echo "  Disk:   ${EFI_DISK}"
        [ -n "$ROOTFS" ] && echo "  Rootfs: ${ROOTFS}"
        echo ""
        echo "If you reach a UEFI shell, type:"
        echo "  fs0:\\vmlinux.efi earlycon console=ttyS0"
        echo ""
        echo "Press Ctrl-A X to quit QEMU."
        echo "─────────────────────────────────────────"

        # shellcheck disable=SC2086
        qemu-system-mips64el \
            "${COMMON_ARGS[@]}" \
            -bios "$BIOS" \
            -drive file="${EFI_DISK}",id=disk0,if=none,format=raw \
            -device scsi-hd,drive=disk0,bus=scsi0.0 \
            "${ROOTFS_QEMU_ARGS[@]}" \
            ${EXTRA_QEMU_ARGS}
        ;;

    kernel)
        KERNEL_APPEND="earlycon console=ttyS0"
        if [ -n "$ROOTFS" ]; then
            KERNEL_APPEND="${KERNEL_APPEND} root=/dev/vda rw"
        fi

        echo ""
        echo "Booting vmlinux ELF directly (no EFI)..."
        echo "  BIOS:   ${BIOS}"
        echo "  Kernel: ${VMLINUX}"
        [ -n "$ROOTFS" ] && echo "  Rootfs: ${ROOTFS}"
        echo "  Append: ${KERNEL_APPEND}"
        echo ""
        echo "Press Ctrl-A X to quit QEMU."
        echo "─────────────────────────────────────────"

        # shellcheck disable=SC2086
        qemu-system-mips64el \
            "${COMMON_ARGS[@]}" \
            -bios "$BIOS" \
            -kernel "$VMLINUX" \
            -append "$KERNEL_APPEND" \
            "${ROOTFS_QEMU_ARGS[@]}" \
            ${EXTRA_QEMU_ARGS}
        ;;
esac
