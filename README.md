# this is my own personal project, and im working on it solo.
So don't expect this to be done soon. I'm quite busy with it,
but it is an entire kernel.


That being said...

# Compiling
Required tools:

>x86_64-elf cross compiler

>mtools (for boot sector)

>dd (floppy creation)

>fat12 support in mkfs.fat, should already be present in your distro

>objcopy (making flat kernel binary from the debug elf)

>make

This probably won't compile on windows, obviously.
