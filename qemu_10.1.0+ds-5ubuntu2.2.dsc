-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: qemu
Binary: qemu-system, qemu-block-extra, qemu-block-supplemental, qemu-system-data, qemu-system-common, qemu-system-gui, qemu-system-modules-spice, qemu-system-modules-opengl, qemu-system-misc, qemu-system-arm, qemu-system-mips, qemu-system-ppc, qemu-system-riscv, qemu-system-s390x, qemu-system-sparc, qemu-system-x86-xen, qemu-system-x86, qemu-system-xen, qemu-user, qemu-user-binfmt, qemu-utils, qemu-guest-agent
Architecture: linux-any all
Version: 1:10.1.0+ds-5ubuntu2.2
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Michael Tokarev <mjt@tls.msk.ru>
Homepage: http://www.qemu.org/
Standards-Version: 4.7.2
Vcs-Browser: https://git.launchpad.net/ubuntu/+source/qemu
Vcs-Git: https://git.launchpad.net/ubuntu/+source/qemu
Testsuite: autopkgtest
Build-Depends: debhelper-compat (= 13), python3-venv:any, python3-distlib, python3:any (>> 3.11), meson (>> 1.5.0~), ninja-build, flex, bison, python3-sphinx:native, python3-sphinx-rtd-theme:native
Build-Depends-Arch: pkgconf, libglib2.0-dev, zlib1g-dev, pkgconf:native, libglib2.0-dev:native, libaio-dev [linux-any], libjack-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libpulse-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libasound2-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libpipewire-0.3-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libmpathpersist-dev [linux-any], libmpathcmd-dev [linux-any], libbpf-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libblkio-dev [amd64 arm64 loong64 mips64 mips64el ppc64 ppc64el riscv64 s390x sparc64], libbrlapi-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libcap-ng-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libcurl4-gnutls-dev, libfdt-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libfuse3-dev [linux-any], gnutls-dev, libgtk-3-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libvte-2.91-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libiscsi-dev, libncurses-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libvirglrenderer-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libepoxy-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libdrm-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libgbm-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libnfs-dev, libnuma-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libcacard-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libpixman-1-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], librbd-dev [amd64 arm64 loong64 mips64el ppc64el riscv64 s390x], libglusterfs-dev [amd64 arm64 loong64 mips64 mips64el ppc64 ppc64el riscv64 s390x sparc64], libsasl2-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libsdl2-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libseccomp-dev [amd64 arm64 armel armhf i386 loong64 mips64el mipsel ppc64 ppc64el powerpc riscv64 s390x], libslirp-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libspice-server-dev [amd64 i386 arm64 armel armhf loong64 mips64el mipsel ppc64el riscv64], librdmacm-dev [linux-any], libibverbs-dev [linux-any], libibumad-dev [linux-any], liburing-dev [linux-any], libusb-1.0-0-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libusbredirparser-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libssh-dev, libzstd-dev, libxen-dev [linux-amd64], nettle-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libudev-dev [linux-any], libjpeg-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libpng-dev [amd64 arm arm64 armel armhf i386 loong64 mips mips64 mips64el mipsel powerpc powerpcspe ppc64 ppc64el riscv64 s390x sparc sparc64], libpmem-dev [amd64 arm64]
Build-Depends-Indep: gcc-x86-64-linux-gnu, gcc-i686-linux-gnu, device-tree-compiler, gcc-s390x-linux-gnu, gcc-alpha-linux-gnu, gcc-powerpc-linux-gnu, gcc-powerpc64-linux-gnu, libssl-dev, gcc-sparc64-linux-gnu, fcode-utils, xsltproc, gcc-hppa-linux-gnu, gcc-hppa64-linux-gnu, gcc-riscv64-linux-gnu, gcc-arm-linux-gnueabi, libc6-dev-armel-cross, gcc-aarch64-linux-gnu, libc6-dev-arm64-cross, libc6-dev-hppa-cross <!pkg.qemu.use-upstream-vdso>, libc6-dev-i386-cross <!pkg.qemu.use-upstream-vdso>, gcc-loongarch64-linux-gnu <!pkg.qemu.use-upstream-vdso>, libc6-dev-loong64-cross <!pkg.qemu.use-upstream-vdso>, libc6-dev-ppc64-cross <!pkg.qemu.use-upstream-vdso>, libc6-dev-riscv64-cross <!pkg.qemu.use-upstream-vdso>, libc6-dev-s390x-cross <!pkg.qemu.use-upstream-vdso>
Build-Conflicts: oss4-dev
Package-List:
 qemu-block-extra deb otherosfs optional arch=alpha,amd64,arm,arm64,armel,armhf,hppa,i386,loong64,m68k,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sh4,sparc,sparc64,x32
 qemu-block-supplemental deb otherosfs optional arch=amd64,arm64,ppc64el,riscv64,s390x
 qemu-guest-agent deb otherosfs optional arch=linux-any
 qemu-system deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-arm deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-common deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-data deb otherosfs optional arch=all
 qemu-system-gui deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-mips deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-misc deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-modules-opengl deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-modules-spice deb otherosfs optional arch=amd64,i386,arm64,armel,armhf,loong64,mips64el,mipsel,ppc64el,riscv64 profile=!pkg.qemu.omit-system
 qemu-system-ppc deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-riscv deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-s390x deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-sparc deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-x86 deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-system
 qemu-system-x86-xen deb oldlibs optional arch=amd64
 qemu-system-xen deb otherosfs optional arch=amd64 profile=!pkg.qemu.omit-system,!pkg.qemu.omit-system-xen
 qemu-user deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-user
 qemu-user-binfmt deb otherosfs optional arch=amd64,arm,arm64,armel,armhf,i386,loong64,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sparc,sparc64 profile=!pkg.qemu.omit-user
 qemu-utils deb otherosfs optional arch=alpha,amd64,arm,arm64,armel,armhf,hppa,i386,loong64,m68k,mips,mips64,mips64el,mipsel,powerpc,powerpcspe,ppc64,ppc64el,riscv64,s390x,sh4,sparc,sparc64,x32
Checksums-Sha1:
 f77bd4eca6697b7569b45ce53a0569b112f9efaa 40218072 qemu_10.1.0+ds.orig.tar.xz
 2d35d1f1e216552b5dcd9b39478a315e80388b0a 172576 qemu_10.1.0+ds-5ubuntu2.2.debian.tar.xz
Checksums-Sha256:
 580ebb2760d85859b7b9211aad8cb77b2641efd39016c62dde37a07961b7ed85 40218072 qemu_10.1.0+ds.orig.tar.xz
 83f1a390a81c4792a311f02209cc176acfc4ae7af2e7787015f790c3533c2dc3 172576 qemu_10.1.0+ds-5ubuntu2.2.debian.tar.xz
Files:
 266fe1d037202fcb3081aa21e06fb20c 40218072 qemu_10.1.0+ds.orig.tar.xz
 a859ac9722ba686de51fd575e0a9dd34 172576 qemu_10.1.0+ds-5ubuntu2.2.debian.tar.xz
Debian-Vcs-Browser: https://salsa.debian.org/qemu-team/qemu
Debian-Vcs-Git: https://salsa.debian.org/qemu-team/qemu.git
Original-Maintainer: Debian QEMU Team <pkg-qemu-devel@lists.alioth.debian.org>

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEco7DU8UfXhRO0oCBM4dveyhIiTsFAmlwjPUACgkQM4dveyhI
iTvreBAAuaW2APtfLGEf7rB8NU9sCXzdQbcSBoRqZ0vaUEE3pVsI93WJSMPw1OG/
66RWQ67F8GO+a9PtcVxg8pOxASxNHGYxtCXKQme2J6YUgVZBISgPjcY5JO73giL4
xD+ekucahCn/h8rFX5bUDrGk1dkMb1KDP8XRRdGO9o37g2t13ckuyFoTrGJUSUYy
wDNWMwp3rxc9pcGTvm2CDHYNC7Hh2+UKZXmBMIg1+EgF2421cw6dPq2Bn/2QmNEJ
WebEP7FK7Snuk1PDv6oJhu9JJ6B+9wr5apb9M8W84aN2T0cTmTjvtiSjSPJNfnHS
4eYFwz9WzOkxf8u2iyUcfM9pVrc1URd9Scnpp5kugTX5EjGK06Nz3KQJQx2NmV2c
FKVlyG5L1SDpJgAJp3MinWIrwlzeeuvSuuHgVuxxogJcW/13alZZJSbpnYcsX2T2
nFBgKVXPIrLk2AC/xM+FaYhce4KF243x5tURNIENzLeAs+tbTirDXmRArFdWZ3N1
57+yfx/waSvnKpfpnXNV6oOZTYJmt2ZeExR++TmTBuyDdV6DqxAeKg1cdXrF9e59
AdjcB6BzzHeGUWj1Q3uUwfd0P/OaOgrFeQnJg7YAms5WLcATfmIeMDBT7pnoyUm6
ZhbrdV7Ut5c2/9nJJu9RutxaCsXBESGcwyeZ4JlvMBirrb2Qx5c=
=E/Df
-----END PGP SIGNATURE-----
