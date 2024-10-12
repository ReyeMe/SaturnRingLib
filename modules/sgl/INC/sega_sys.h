/*
 * ����:        �V�X�e�� �v���O����
 *
 * �t�@�C����:  sega_sys.h
 *
 * �ړI:        BOOT ROM �̃T�[�r�X���[�`�����`���܂��B
 *
 *              (1) SCU ���荞�݃T�[�r�X���[�`���̓o�^�Q��
 *              (2) SH2 ���荞�݃x�N�^�̓o�^�Q��
 *              (3) �V�X�e���Z�}�t�H�̑���  (BOOT ROM V0.90�ȍ~)
 *              (4) SCU ���荞�݃}�X�N�̑���(BOOT ROM V0.90�ȍ~)
 *              (5) �V�X�e���N���b�N�̐ؑւ�(BOOT ROM V0.92�ȍ~)
 *              (6) SCU ���荞�݃��[�`���D��x�̕ύX(SCU2.1�ȍ~)
 *              (7) CD  �}���`�v���[���̋N�����s(CD�t�����f���p)
 *              (8) �p���[�I���N���A�������̑��� (ROM V1.00�ȍ~)
 *              (9) MPEG �`�F�b�N(�������A�^)    (ROM V1.00�ȍ~)
 *
 *              sega_sys.lib�Ŋ֐����`���܂��B�iVer.2.01�ȍ~�j
 *
 * �g�p���@:
 *
 *      void  Vfunc(void)
 *      {
 *           // ... ���s�� ... //
 *      }
 *
 *      extern void  Hihdr(void);
 *
 *      void  test()
 *      {
 *           void (*tmp)();
 *           int32_t rep;
 *           uint32_t msk, clk, lval1;
 *           uint8_t  p0;
 *           const uint32_t PriTab[32] = {
 *              0x00f0ffff, 0x00e0fffe, 0x00d0fffc, 0x00c0fff8,
 *              0x00b0fff0, 0x00a0ffe0, 0x0090ffc0, 0x0080ff80,
 *              0x0080ff80, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *              0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
 *           };
 *
 *           SYS_SETUINT(0x40, Vfunc);  // Vfunc() �� VB-In �����ݗp�ɓo�^   //
 *           tmp = SYS_GETUINT(0x41);   // ���݂� VB-Out ���[�`���o�^���Q��  //
 *           SYS_SETUINT(0x41, 0);      // VB-Out ���[�`���̓o�^������       //
 *
 *           SYS_SETSINT(0x42, Hihdr);  // Hihdr() �� HB-In �x�N�^�ɓo�^     //
 *           tmp = SYS_GETSINT(0x6e);   // ���݂� DIVU �x�N�^�o�^���Q��      //
 *           SYS_SETSINT(0x6e, 0);      // DIVU �x�N�^���ď�����             //
 *
 *           rep = SYS_TASSEM(0x5);     // �Z�}�t�H 5 ���e�X�g��A���h��Z�b�g //
 *           SYS_CLRSEM(0x5);           // �Z�}�t�H 5 ���N���A               //
 *
 *           SYS_SETSCUIM(0xfffffffe);  // VB-In �����݂݂̂����ɐݒ�      //
 *           SYS_CHGSCUIM(0xfffffffc, 0); // VB-In, VB-Out �����ɕύX      //
 *           SYS_CHGSCUIM(0xffffffff, 2); // VB-Out  ��s���ɕύX          //
 *           msk = SYS_GETSCUIM;        // ���݂� SCU �����݃}�X�N�l���Q��   //
 *
 *           SYS_CHGSYSCK(1);           // �V�X�e���N���b�N���Q�W�l�ɐؑւ�  //
 *           clk = SYS_GETSYSCK;        // ���݂� �V�X�e���N���b�N�l���Q��   //
 *                                      //   0: 26 MHz       1: 28 MHz       //
 *                                      //     320/640 pix/H   352/704 pix/H //
 *
 *           SYS_CHGUIPR(PriTab);       // �r�b�t���荞�݃��[�`���D��x�ύX  //
 *
 *           SYS_EXECDMP();             // �b�c�}���`�v���[���N�����s        //
 *
 *           p0 = SYS_PCLRMEM;          // �p���[�I���N���A�������擪�o�C�g  //
 *           lval1 = *((uint32_t*)&SYS_PCLRMEM+1); // �ƌ㔼�����O���[�h���Q�� //
 *
 *           rep = SYS_CHKMPEG(0);      // �����͕K�� 0. MANSYS.DOC �̎g�p�� //
 *                                      // �ɏ]���Ă�������                  //
 *      }
 *
 *      // ... �ʂ̃t�@�C�� ... //
 *      #pragma interrupt(Hihdr)
 *      void  Hihdr(void)
 *      {
 *           // ... ���s�� ... //
 *      }
 *
 *
 * ���t:        1994/05/06  for BOOT 0.90,0.901,0.91  by T. S.
 *              1994/07/16  for BOOT 0.92x,0.95       by T. S.
 *              1994/07/29  for BOOT 0.96x,1.00       by T. S.
 *              1994/08/31  how to exec CD multi-player
 *              1994/09/21  for Ver.1.00 BOOT ROM
 *              1995/03/31  MPEG check service
 *              1995/09/26  sega_sys.lib (Ver.2.01)   by M. M.
 *
 * �o�[�W����:  Ver.2.01
 *
 */

#ifndef SEGA_SYS_H
#define SEGA_SYS_H

/***********************************************************************
 *      �C���N���[�h�t�@�C��
 ***********************************************************************/
#include "sega_xpt.h"

/***********************************************************************
 *      �����}�N��
 ***********************************************************************/
#define SYS_SETUINT(_Num_, _Hdr_) \
        ((**(volatile void(**)(uint32_t, void*))0x6000300)((_Num_), (_Hdr_)))
#define SYS_GETUINT(_Num_) \
        ((void*)(**(void(*(**)(uint32_t))(uint32_t))0x6000304)(_Num_))

#define SYS_SETSINT(_Num_, _Hdr_) \
        ((**(volatile void(**)(uint32_t, void*))0x6000310)((_Num_), (_Hdr_)))
#define SYS_GETSINT(_Num_) \
        ((void*)(**(void(*(**)(uint32_t))(uint32_t))0x6000314)(_Num_))

#define SYS_TASSEM(_Num_) \
        ((**(volatile uint32_t(**)(uint32_t))0x6000330)(_Num_))
#define SYS_CLRSEM(_Num_) \
        ((**(volatile void(**)(uint32_t))0x6000334)(_Num_))

#define SYS_SETSCUIM(_MaskPat_) \
        ((**(volatile void(**)(uint32_t))0x6000340)(_MaskPat_))
#define SYS_CHGSCUIM(_AndMask_, _OrMask_) \
        ((**(volatile void(**)(uint32_t, uint32_t))0x6000344)((_AndMask_), (_OrMask_)))
#define SYS_GETSCUIM \
        (*(volatile uint32_t*)0x6000348)

#define SYS_CHGSYSCK(_CkMode_) \
        ((**(volatile void(**)(uint32_t))0x6000320)(_CkMode_))
#define SYS_GETSYSCK \
        (*(volatile uint32_t*)0x6000324)

#define SYS_CHGUIPR(_IprTab_) \
        ((**(volatile void(**)(uint32_t*))0x6000280)(_IprTab_))

#define SYS_EXECDMP \
        (**(volatile void(**)(void))0x600026C)

#define SYS_PCLRMEM \
        (*(volatile uint8_t*)0x6000210)

#define SYS_CHKMPEG(_dummy_) \
        ((**(volatile int32_t(**)(int32_t))0x6000274)(_dummy_))

/***********************************************************************
 *      �֐��̐錾
 ***********************************************************************/
void SYS_CheckTrack(int32_t tno);
void SYS_Exit(int32_t code);

#endif
