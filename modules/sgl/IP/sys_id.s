! The Rockin'-B, 2005
! Had to adapt the source to sh-coff-as
! Note:
! Using .section shifts this stuff to the ip.bin file end
! the CODE keyword is not recognized
	.align 4
	.ascii	"SEGA SEGASATURN "
	.ascii	"Powered by SRL   "
	.ascii	"         V1.","0","0","0"
	.ascii	"20240714CD-1/1  "	!30
	.ascii	"JTUE            "	!40
	.ascii	"J               "	!50
	.ascii	"SRL APP         "	!60
	.ascii	"                "	!70
	.ascii	"                "	!80
	.ascii	"                "	!90
	.ascii	"                "	!A0
	.ascii	"                "	!B0
	.ascii	"                "	!C0
	.int	0X00000000,0X00000000,0X00000000,0X00000000	!D0
	.int	0X00001800,0X00000000,0X00000000,0X00000000	!E0
	.int	0X06004000,0X00000000,0X00000000,0X00000000	!F0
	.end
