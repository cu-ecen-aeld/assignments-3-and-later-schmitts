#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
SYS_ROOT=$(${CROSS_COMPILE}gcc --print-sysroot)

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
	git reset --hard

	make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper

	make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig

	sed -i 's/YYLTYPE yylloc;/extern YYLTYPE yylloc;/' ${OUTDIR}/linux-stable/scripts/dtc/dtc-lexer.l

	make -j4 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all
	make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE modules
	make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs
fi

echo "Adding the Image in outdir"

cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
mkdir rootfs
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
	mkdir ${OUTDIR}/rootfs
	cd ${OUTDIR}/rootfs
	mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
	mkdir usr/bin usr/lib usr/sbin
	mkdir -p var/log
	sudo chown -R root.root *
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
	git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
	make distclean
	make defconfig
	sudo env PATH=$PATH make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE install
else
    cd busybox
	sudo cp -ra _install/* ${OUTDIR}/rootfs
fi

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

## Add library dependencies to rootfs

sudo cp -a ${SYS_ROOT}/lib64/ld-2.31.so ${OUTDIR}/rootfs/lib
cd ${OUTDIR}/rootfs/lib
sudo ln -s ld-2.31.so ld-linux-aarch64.so.1

sudo cp -a ${SYS_ROOT}/lib64/libm-2.31.so ${OUTDIR}/rootfs/lib64
sudo cp -a ${SYS_ROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
sudo cp -a ${SYS_ROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
sudo cp -a ${SYS_ROOT}/lib64/libresolv-2.31.so ${OUTDIR}/rootfs/lib64
sudo cp -a ${SYS_ROOT}/lib64/libc-2.31.so ${OUTDIR}/rootfs/lib64
sudo cp -a ${SYS_ROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64

## Make device nodes

cd ${OUTDIR}/rootfs

sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

## Clean and build the writer utility

cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=$CROSS_COMPILE

## Copy the finder related scripts and executables to the /home directory
## on the target rootfs

sudo cp -v autorun-qemu.sh finder.sh finder-test.sh writer ${OUTDIR}/rootfs/home
sudo cp -vr ../conf ${OUTDIR}/rootfs/home
cd ${OUTDIR}/rootfs
sudo ln -s home/conf .

## Chown the root directory
sudo chown -R root.root ${OUTDIR}/rootfs/home

## Create initramfs.cpio.gz

cd ${OUTDIR}/rootfs
find . -print0 | cpio --null -ov --format=newc --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
