# Backlog

Items here are not part of the learning steps in the README. Steps 3 to 5 stay Bochs, gem5, and the other architectures.

## Generate the root disk

`rootfs/hdc-0.11.img` is the teaching disk used by QEMU. It is not produced by the kernel build, and it is not in the 1991 kernel tarball. The file in the tree is the image shipped by [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11) as `rootfs/hdc-0.11.img.xz`.

Do not build this image yet. When it is built, a script should create the partition table and the Minix filesystem, then place `/dev`, `/etc`, and the user programs on it. Those programs (`/bin/sh`, `/bin/ls`, and the rest) are a separate source tree. This kernel executes a.out ZMAGIC with a 1024-byte header, so host GCC output cannot be copied onto the disk. The existing ports leave that userspace prebuilt.

Until that script exists, QEMU keeps using the teaching disk above.

[中文](../zh/backlog.md)
