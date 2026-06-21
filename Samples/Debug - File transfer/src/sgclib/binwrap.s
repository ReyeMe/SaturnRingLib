!
!  SD card access module for Saturn Gamer's Cartridge
!  by cafe-alpha
!
!  See LICENSE file for details.
!

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
.global ___sgclib_stub_dat
.global ___sgclib_stub_end
    .align 1
___sgclib_stub_dat:
    .incbin "src/sgclib/sgclib.bin"
___sgclib_stub_end:

