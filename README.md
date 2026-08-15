# this is my own personal project, and im working on it solo.
So don't expect this to be done soon. I'm quite busy with it,
but it is an entire kernel.


That being said, if you want to try it,

# Compiling
Required tools:

>make

>x86_64-elf cross compiler (GCC), that supports C23

>mtools (for disk image)

>dd (floppy creation)

>fat32 support in mkfs.fat, should already be present in your distro

>grub (bootloader, grub-mkstandalone)

>OVMF_CODE.fd and OVMF_VARS.fd, uefi implementations for QEMU. put in uefi/ yourself if you use QEMU.

>optional: QEMU to test it, it *may* run on bare hardware right now, but I would not recommend.

This probably won't compile on windows, obviously.
