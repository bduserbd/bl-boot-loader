The repository of the BL boot loader.

BL is a new boot loader with the following purposes:

1) To learn some low level stuff. Usually, with "normal" operation system development,
   things like system boot, BIOS/UEFI functions aren't present so it is a nice way
   to learn about them.

2) Get familiar with common concepts like USB, pixel graphics, disk operations without
   the need to develop full multitasking, user space system.

3) Eliminate the dependency (hopefully) on other boot loaders like GRUB.

At this moment BL isn't fully functional and more work needs to be done to bring its
components together. Compiling & booting the boot loader may lead to errors & crushes.

The following was written for BL:

1) Booting x86 32-bit systems.

2) Support BIOS & UEFI:
  * Booting.
  * MBR & GPT.
  * Graphics (VBE & GOP).

3) Boot loader modules - For example - No need to precompile BIOS stuff for UEFI loader.

4) USB:
  * UHCI - Control & Bulk & Interrupt transfers.
  * OHCI - Control & Bulk transfers.
  * EHCI - Control transfers.
  * XHCI - Control transfers.

5) Common file systems:
  * FAT (12/16/32).
  * EXT2/3/4- On the work.
  * NTFS.

6) Storage:
  * PATA.
  * AHCI.
  * USB SCSI.

7) Keyboard:
  * PS2.
  * USB HID.

BL was tested on QEMU & Virtual Box. In the past, BL succeeded running on Bochs
in order to test MTRR but not further attempts were done.

