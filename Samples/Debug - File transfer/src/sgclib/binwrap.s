!
!  SD card access module for Saturn Gamer's Cartridge
!  by cafe-alpha
!
!  See LICENSE file for details.
!

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
.global __sgclib_stub_dat
.global __sgclib_stub_end
    .align 1
__sgclib_stub_dat:
    .incbin "sgclib.bin"
__sgclib_stub_end:

