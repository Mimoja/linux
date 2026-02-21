#!/bin/bash
# create-rootfs.sh - Create a Debian bookworm mips64el rootfs image
#
# Builds an ext4 disk image containing a minimal Debian bookworm system
# for use with QEMU loongson3-virt and the run.sh script.
#
# Requirements:
#   - sudo (debootstrap needs root)
#   - debootstrap, e2fsprogs (mkfs.ext4)
#   - qemu-user + binfmt-support (for foreign arch chroot)
#
# Usage:
#   ./create-rootfs.sh              # Create rootfs.ext4 in current directory
#   ./create-rootfs.sh /path/to/out.ext4   # Create at custom path
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT="${1:-${SCRIPT_DIR}/rootfs.ext4}"
ROOTFS_SIZE_MB=512
DEBIAN_RELEASE="bookworm"
DEBIAN_MIRROR="http://deb.debian.org/debian"
ARCH="mips64el"
HOSTNAME="mips64el-debian"
QEMU_STATIC="/usr/bin/qemu-mips64el"

# ── Sanity checks ──────────────────────────────────────────────────

if [ "$(id -u)" -ne 0 ]; then
    echo "This script requires root. Re-running with sudo..."
    exec sudo "$0" "$@"
fi

for cmd in debootstrap mkfs.ext4; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "ERROR: '$cmd' not found. Install it first."
        exit 1
    fi
done

if [ ! -f "$QEMU_STATIC" ]; then
    echo "ERROR: $QEMU_STATIC not found."
    echo "       Install: sudo apt install qemu-user qemu-user-binfmt"
    exit 1
fi

if [ ! -f /proc/sys/fs/binfmt_misc/qemu-mips64el ]; then
    echo "ERROR: binfmt_misc not registered for mips64el."
    echo "       Install: sudo apt install qemu-user-binfmt binfmt-support"
    exit 1
fi

# ── Working directory ──────────────────────────────────────────────

WORKDIR="$(mktemp -d)"
MOUNTPOINT="$(mktemp -d)"

cleanup() {
    echo ""
    echo "Cleaning up..."
    # Unmount everything inside the chroot
    umount -lf "${MOUNTPOINT}/proc" 2>/dev/null || true
    umount -lf "${MOUNTPOINT}/sys" 2>/dev/null || true
    umount -lf "${MOUNTPOINT}/dev/pts" 2>/dev/null || true
    umount -lf "${MOUNTPOINT}/dev" 2>/dev/null || true
    umount -lf "${MOUNTPOINT}" 2>/dev/null || true
    rm -rf "${WORKDIR}"
    rmdir "${MOUNTPOINT}" 2>/dev/null || true
}
trap cleanup EXIT

# ── Create ext4 image ─────────────────────────────────────────────

echo "=== Creating ${ROOTFS_SIZE_MB}MB ext4 image at ${OUTPUT} ==="
dd if=/dev/zero of="${OUTPUT}" bs=1M count=${ROOTFS_SIZE_MB} status=progress
mkfs.ext4 -L rootfs -F "${OUTPUT}"

mount -o loop "${OUTPUT}" "${MOUNTPOINT}"

# ── Bootstrap Debian ──────────────────────────────────────────────

echo ""
echo "=== Bootstrapping Debian ${DEBIAN_RELEASE} ${ARCH} (first stage) ==="
debootstrap --arch="${ARCH}" --variant=minbase --foreign \
    "${DEBIAN_RELEASE}" "${MOUNTPOINT}" "${DEBIAN_MIRROR}"

# Copy qemu-user binary into chroot so second-stage works
cp "${QEMU_STATIC}" "${MOUNTPOINT}/usr/bin/"

echo ""
echo "=== Running second stage (this takes a while) ==="
chroot "${MOUNTPOINT}" /debootstrap/debootstrap --second-stage

# ── Configure the rootfs ──────────────────────────────────────────

echo ""
echo "=== Configuring rootfs ==="

# Mount virtual filesystems for chroot commands
mount -t proc proc "${MOUNTPOINT}/proc"
mount -t sysfs sys "${MOUNTPOINT}/sys"
mount --bind /dev "${MOUNTPOINT}/dev"
mount -t devpts devpts "${MOUNTPOINT}/dev/pts"

# Set hostname
echo "${HOSTNAME}" > "${MOUNTPOINT}/etc/hostname"
cat > "${MOUNTPOINT}/etc/hosts" <<EOF
127.0.0.1	localhost
127.0.1.1	${HOSTNAME}
::1		localhost ip6-localhost ip6-loopback
EOF

# Set empty root password (for easy serial login during development)
chroot "${MOUNTPOINT}" passwd -d root

# Configure serial console getty on ttyS0
# minbase uses sysvinit-utils; we set up a simple inittab
cat > "${MOUNTPOINT}/etc/inittab" <<'EOF'
# /etc/inittab - minimal config for serial console on QEMU
id:2:initdefault:
si::sysinit:/etc/init.d/rcS
S0:2345:respawn:/sbin/getty -L ttyS0 115200 vt100
EOF

# Create rcS init script to mount virtual filesystems
mkdir -p "${MOUNTPOINT}/etc/init.d"
cat > "${MOUNTPOINT}/etc/init.d/rcS" <<'EOFRC'
#!/bin/sh
# Mount virtual filesystems
mount -t proc proc /proc
mount -t sysfs sys /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || true
mkdir -p /dev/pts /dev/shm
mount -t devpts devpts /dev/pts
mount -t tmpfs tmpfs /dev/shm
mount -t tmpfs tmpfs /tmp
mount -t tmpfs tmpfs /run

# Remount root as read-write
mount -o remount,rw /

# Set hostname
hostname -F /etc/hostname

echo ""
echo "====================================="
echo "  Debian ${HOSTNAME} booted"
echo "====================================="
echo ""
EOFRC
chmod 755 "${MOUNTPOINT}/etc/init.d/rcS"

# Install sysvinit-core for /sbin/init
chroot "${MOUNTPOINT}" apt-get update -qq
chroot "${MOUNTPOINT}" apt-get install -y --no-install-recommends sysvinit-core

# fstab
cat > "${MOUNTPOINT}/etc/fstab" <<'EOF'
# /etc/fstab - QEMU virtio-blk rootfs
/dev/vda	/	ext4	errors=remount-ro	0	1
proc		/proc	proc	defaults		0	0
sysfs		/sys	sysfs	defaults		0	0
tmpfs		/tmp	tmpfs	defaults		0	0
tmpfs		/run	tmpfs	defaults		0	0
EOF

# Configure apt sources for the chroot (useful if user wants to install more packages later)
cat > "${MOUNTPOINT}/etc/apt/sources.list" <<EOF
deb ${DEBIAN_MIRROR} ${DEBIAN_RELEASE} main
deb ${DEBIAN_MIRROR} ${DEBIAN_RELEASE}-updates main
deb http://security.debian.org/debian-security ${DEBIAN_RELEASE}-security main
EOF

# Set a basic locale
echo "LANG=C.UTF-8" > "${MOUNTPOINT}/etc/default/locale"

# Allow root login on serial console
echo "ttyS0" >> "${MOUNTPOINT}/etc/securetty"

# Clean up apt cache to save space
chroot "${MOUNTPOINT}" apt-get clean
rm -rf "${MOUNTPOINT}/var/lib/apt/lists/"*

# Remove the qemu-user binary from the rootfs (not needed at runtime)
rm -f "${MOUNTPOINT}/${QEMU_STATIC}"

# ── Done ──────────────────────────────────────────────────────────

# Unmount virtual filesystems before final umount
umount "${MOUNTPOINT}/dev/pts" 2>/dev/null || true
umount "${MOUNTPOINT}/dev" 2>/dev/null || true
umount "${MOUNTPOINT}/sys" 2>/dev/null || true
umount "${MOUNTPOINT}/proc" 2>/dev/null || true

sync
umount "${MOUNTPOINT}"
rmdir "${MOUNTPOINT}" 2>/dev/null || true

# Shrink the image to minimum + headroom
e2fsck -f -y "${OUTPUT}" || true
resize2fs -M "${OUTPUT}"
# Then expand back to a round size with some free space
resize2fs "${OUTPUT}" $((ROOTFS_SIZE_MB - 32))M

# Fix ownership so non-root user can use the image with QEMU
if [ -n "${SUDO_UID:-}" ] && [ -n "${SUDO_GID:-}" ]; then
    chown "${SUDO_UID}:${SUDO_GID}" "${OUTPUT}"
fi

echo ""
echo "=== Done ==="
echo "Rootfs image: ${OUTPUT} ($(du -h "${OUTPUT}" | cut -f1))"
echo ""
echo "Boot with:"
echo "  ./run.sh --kernel --rootfs"
echo "  ./run.sh --rootfs              # EFI boot with rootfs"
