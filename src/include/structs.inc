%ifndef KERN_ASM_STRUCTS
%define KERN_ASM_STRUCTS

struc bootsector
	.jmp:						resb 3
	.oem:						resb 8
	.bytes_per_sector:			resw 1
	.sectors_per_cluster:		resb 1
	.reserved_sectors:			resw 1
	.fat_count:					resb 1
	.dentries:					resw 1
	.total_sectors:				resw 1
	.media_descriptor_table: 	resb 1
	.sectors_per_fat:			resw 1
	.sectors_per_track:			resw 1
	.heads:						resw 1
	.hidden:					resd 1
	.large: 					resd 1

	.ebr.drive_number:			resb 1
								resb 1
	.ebr.sig:					resb 1
	.ebr.volume_id:				resb 4
	.ebr.volume_label:			resb 11
	.ebr.system_id:		 		resb 8
endstruc

%endif