/* BIOS interface for reading sectors */
l_disk_address_packet:
size:
	.byte	0
unused:
	.byte	0
nsectors:
	.word	0
offset:
	.word	0
segment:
	.word	0
lba:
	.long	0
	.long	0

