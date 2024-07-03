/*-----------------------------------------------------------------------------
 *  FILE: snd_main.h
 *
 *      Copyright (c) by SEGA.
 *
 *  PURPOSE:
 *
 *      �T�E���hI/F���C�u�����w�b�_�t�@�C���B
 *
 *  DESCRIPTION:
 *
 *      �T�E���h�h���C�o�Ƃ̃C���^�t�F�[�X����������B
 *
 *  AUTHOR(S)
 *
 *      1994-05-18  N.T Ver.0.90
 *
 *  MOD HISTORY:
 *
 *      1994-09-23  N.T Ver.1.04
 *      1994-12-01  N.T Ver.1.05
 *      1996-07-10  N.T Ver.1.08
 *
 *-----------------------------------------------------------------------------
 */

#ifndef SEGA_SND_H
#define SEGA_SND_H

/*
 * C VIRTUAL TYPES DEFINITIONS
 */
#include <machine.h>
#include "sega_xpt.h"

/*
 * USER SUPPLIED INCLUDE FILES
 */

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/**** �萔 *******************************************************************/
/******** ����ގ��s��� ******************************************************/
#define SND_RET_SET     0                       /* ����I��                     */
#define SND_RET_NSET    1                       /* �ُ�I��                     */
/******** ����޺��۰ِݒ� ****************************************************/
#define SND_CTRL_OUTPUT_STEREO  (0 << 7)        /* Sound output mode : STEREO*/
#define SND_CTRL_OUTPUT_MONO    (1 << 7)        /* Sound output mode : MONO  */

/******** Sequence PAN ���۰ٽ��� ********************************************/
#define SND_SEQ_PAN_OFF (0 << 7)                /* Control OFF               */
#define SND_SEQ_PAN_ON  (1 << 7)                /* Control ON                */

/******** ʰ�޳���������Ұ� **************************************************/
#define SND_DRAM4           0                   /* DRAM 4Mbit r/w            */
#define SND_DRAM8           1                   /* DRAM 8Mbit r/w            */
#define SND_SCSP_MIDI       2                   /* SCSP MIDI                 */
#define SND_SOUND_SRC_LR    3                   /* �����o��(L/R)            */
#define SND_SOUND_SRC_L     4                   /* �����o��(L)              */
#define SND_SOUND_SRC_R     5                   /* �����o��(R)              */
/******** ʰ�޳�������ð�� ***************************************************/
#define SND_HARD_OK     0x8000                  /* ����I��                  */
#define SND_HARD_ERR    0x7fff                  /* �ُ�I��                  */
/******** ���荞�݋��r�b�g *************************************************/
#define SND_INT_PCM_ADR     (1 <<  7)           /* PCM play address �X�V   */
/******** ��Host���荞�ݎ�� *************************************************/
#define SND_FCT_PCM_ADR     0x01                /* PCM play address �X�V   */
/******** PCM���ڽ�X�V���荞�ݽð�� ******************************************/
#define SND_PCM_PLAY0   (1 <<  0)               /* PCM�Đ��ԍ�0              */
#define SND_PCM_PLAY1   (1 <<  1)               /* PCM�Đ��ԍ�1              */
#define SND_PCM_PLAY2   (1 <<  2)               /* PCM�Đ��ԍ�2              */
#define SND_PCM_PLAY3   (1 <<  3)               /* PCM�Đ��ԍ�3              */
#define SND_PCM_PLAY4   (1 <<  4)               /* PCM�Đ��ԍ�4              */
#define SND_PCM_PLAY5   (1 <<  5)               /* PCM�Đ��ԍ�5              */
#define SND_PCM_PLAY6   (1 <<  6)               /* PCM�Đ��ԍ�6              */
#define SND_PCM_PLAY7   (1 <<  7)               /* PCM�Đ��ԍ�7              */

/******** SndSeqStat(�V�[�P���X�X�e�[�^�X) ***********************************/
/************* SND_SEQ_STAT_MODE(song mode) **********************************/
#define SND_MD_STOP     0x000                           /* stop              */
#define SND_MD_PLAY     0x100                           /* play              */
#define SND_MD_FADE     0x200                           /* fade              */
#define SND_MD_PLAY_PS  0x300                           /* play pause        */
#define SND_MD_FADE_PS  0x400                           /* fade pause        */
/******** SndPcmStartPrm(PCM�J�n�p�����[�^) **********************************/
/************* SND_PRM_MODE(��ڵ��ɸ�+�����ݸ�ڰ�) ***************************/
#define SND_MD_MONO     (0 <<  7)                   /* �ɸ�                  */
#define SND_MD_STEREO   (1 <<  7)                   /* ��ڵ                  */
#define SND_MD_16       (0 <<  4)                   /* 16bitPCM              */
#define SND_MD_8        (1 <<  4)                   /* 8bitPCM               */
/******** SND_MoveData(�T�E���h�f�[�^�]��) ***********************************/
#define SND_KD_TONE         0x0                         /* ���F              */
#define SND_KD_SEQ          0x1                         /* �V�[�P���X        */
#define SND_KD_DSP_PRG      0x2                         /* DSP�v���O����     */
#define SND_KD_DSP_RAM      0x3                         /* DSPܰ�RAM         */
/**** �A�N�Z�X�}�N�� *********************************************************/
/******** SndIniDt(�V�X�e���N��) *********************************************/
#define SND_INI_PRG_ADR(sys_ini)    ((sys_ini).prg_adr) /* 68K��۸��ъi�[... */
#define SND_INI_PRG_SZ(sys_ini)     ((sys_ini).prg_sz)  /* 68K��۸��ѻ���... */
#define SND_INI_ARA_ADR(sys_ini)    ((sys_ini).ara_adr) /* ����޴رϯ�ߊi�[. */
#define SND_INI_ARA_SZ(sys_ini)     ((sys_ini).ara_sz)  /* ����޴رϯ�߻���. */
/******** SndSeqStat(�V�[�P���X�X�e�[�^�X) ***********************************/
#define SND_SEQ_STAT_MODE(status)   ((status).mode)     /* song mode         */
#define SND_SEQ_STAT_STAT(status)   ((status).stat)     /* song status       */
/******** SndPcmStartPrm(PCM�J�n�p�����[�^) **********************************/
#define SND_PRM_MODE(prm)       ((prm).mode)        /* ��ڵ��ɸ�+�����ݸ�ڰ� */
#define SND_PRM_SADR(prm)       ((prm).sadr)        /* PCM��ذ��ޯ̧���ı��ڽ*/
#define SND_PRM_SIZE(prm)       ((prm).size)        /* PCM��ذ��ޯ̧����   */
#define SND_PRM_OFSET(prm)      ((prm).ofset)       /* PCM��ذэĐ��J�n�̾�� */
/******** SndPcmChgPrm(PCM�ύX�p�����[�^) ************************************/
#define SND_PRM_NUM(prm)        ((prm).num)         /* PCM��ذэĐ��ԍ�       */
#define SND_PRM_LEV(prm)        ((prm).lev)         /* �޲ڸĉ�Level         */
#define SND_PRM_PAN(prm)        ((prm).pan)         /* �޲ڸĉ�pan           */
#define SND_PRM_PICH(prm)       ((prm).pich)        /* PICHܰ��              */
#define SND_R_EFCT_IN(prm)      ((prm).r_efct_in)   /* Efect in select(�E)   */
#define SND_R_EFCT_LEV(prm)     ((prm).r_efct_lev)  /* Efect send Level(�E)  */
#define SND_L_EFCT_IN(prm)      ((prm).l_efct_in)   /* Efect in select(��)   */
#define SND_L_EFCT_LEV(prm)     ((prm).l_efct_lev)  /* Efect send Level(��)  */
#define SND_PRM_TL(prm)     	((prm).lev)  		/* Master Level			 */
/******** SndPcmPlayAdr(PCM�Đ����ڽ) ****************************************/
#define SND_PCM_RADR(prm)       ((prm).radr)        /* �E�o��(�ɸ�)          */
#define SND_PCM_LADR(prm)       ((prm).ladr)        /* ���o��                 */
/******** SndCdHzSrVl(���g���ш�ʽ�ڵ��͉���) ******************************/
#define SND_CD_LHIGH(hz_vl)     ((hz_vl).high.left) /* ���o�� ������         */
#define SND_CD_LMID(hz_vl)      ((hz_vl).mid.left)  /* ���o�� ������         */
#define SND_CD_LLOW(hz_vl)      ((hz_vl).low.left)  /* ���o�� �ቹ��         */
#define SND_CD_RHIGH(hz_vl)     ((hz_vl).high.right)/* �E�o�� ������         */
#define SND_CD_RMID(hz_vl)      ((hz_vl).mid.right) /* �E�o�� ������         */
#define SND_CD_RLOW(hz_vl)      ((hz_vl).low.right) /* �E�o�� �ቹ��         */

#define SND_POKE_B(adr, data)   (*((volatile uint8_t *)(adr)) = ((uint8_t)(data))) /* �޲�*/
#define SND_POKE_W(adr, data)   (*((volatile uint16_t *)(adr)) = ((uint16_t)(data)))
#define SND_PEEK_B(adr)         (*((volatile uint8_t *)(adr)))                   /* �޲�*/

/**** �V�X�e���C���^�t�F�[�X���[�N�I�t�Z�b�g *********************************/
#define SND_ADR_INTR_CTRL_WORD  (0x00)          /* Interrupt control word    */
#define SND_ADR_INTR_MODE       (0x01)          /* Interrupt mode            */
#define SND_ADR_PCM_PLAY_NUM    (0x02)          /* PCM play num              */
#define SND_ADR_SND_CTRL        (0x03)          /* Sound control             */
/**** �A�h���X ***************************************************************/
#define SND_ADR_INTR_RESET  ((volatile uint16_t *)0x25b0042e)  /* SCSP���荞��ؾ��ڼ޽� */

/*���荞�� *******************************************************************/
/******************************************************************************
 *
 * NAME:    SND_SET_ENA_INT()       - ���荞�݋��ݒ�
 *
 * PARAMETERS :
 *      (1) uint8_t ena_bit   - <i>   ���荞�݋��r�b�g
 *
 * DESCRIPTION:
 *      ���荞�݋���ݒ肵�܂��B
 *
 * PRECONDITIONS:
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 * CAVEATS:
 *      �Ȃ��B
 *
 ******************************************************************************
 */

#define SND_SET_ENA_INT(ena_bit)\
    do{\
        snd_msk_work_work = get_imask();           /* ����ފ��荞�݂�Disable*/\
        set_imask(15);\
        SND_POKE_B(snd_adr_sys_int_work + SND_ADR_INTR_CTRL_WORD, (ena_bit));\
        set_imask(snd_msk_work_work);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    SND_GET_ENA_INT()       - ���荞�݋��擾
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * DESCRIPTION:
 *      ���荞�݋����擾���܂��B
 *
 * PRECONDITIONS:
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      (1) uint8_t           - <o>   ���荞�݋��r�b�g
 *
 * CAVEATS:
 *      �Ȃ��B
 *
 ******************************************************************************
 */

#define SND_GET_ENA_INT()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_INTR_CTRL_WORD))

/******************************************************************************
 *
 * NAME:    SND_GET_FCT_INT()       - ���荞�ݗv���擾
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * DESCRIPTION:
 *      ���荞�ݗv�����擾���܂��B
 *
 * PRECONDITIONS:
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      (1) uint8_t       - <o>   ���荞�ݗv��
 *
 * CAVEATS:
 *      �Ȃ��B
 *
 ******************************************************************************
 */

#define SND_GET_FCT_INT()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_INTR_MODE))

/******************************************************************************
 *
 * NAME:    SND_RESET_INT()     - ���荞��ؾ��
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * DESCRIPTION:
 *      ���荞�݂�ؾ�Ă��܂��B
 *
 * PRECONDITIONS:
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 * CAVEATS:
 *      �Ȃ��B
 *
 ******************************************************************************
 */

#define SND_RESET_INT()\
        (SND_POKE_W(SND_ADR_INTR_RESET, (1 << 5)))

/******************************************************************************
 *
 * NAME:    SND_GET_INT_STAT()  - PCM���s�A�h���X�X�V���荞�݃X�e�[�^�X�擾
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * DESCRIPTION:
 *      PCM���s�A�h���X�X�V���荞�݃X�e�[�^�X���擾���܂��B
 *
 * PRECONDITIONS:
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      (1) SndPcmIntStat *     - <o>   PCM���s�A�h���X�X�V���荞�݃X�e�[�^�X
 *
 * CAVEATS:
 *      �Ȃ��B
 *
 ******************************************************************************
 */
 
#define SND_GET_INT_STAT()\
    (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_PCM_PLAY_NUM))

/*���̑� *********************************************************************/
/******************************************************************************
 *
 * NAME:    SND_SET_SND_CTRL        - �T�E���h�R���g���[�����ݒ�
 *
 * PARAMETERS :
 *      (1) uint8_t   ctrl_bit   - <i>   ���荞�݋��r�b�g
 *
 * DESCRIPTION:
 *      �T�E���h�R���g���[������ݒ肵�܂��B
 *
 * PRECONDITIONS:
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 * CAVEATS:
 *      �Ȃ��B
 *
 ******************************************************************************
 */

#define SND_SET_SND_CTRL(ctrl_bit)\
    do{\
        SND_POKE_B(snd_adr_sys_int_work + SND_ADR_SND_CTRL, (ctrl_bit));\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    SND_GET_SND_CTRL        - �T�E���h�R���g���[�����擾
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * DESCRIPTION:
 *      �T�E���h�R���g���[�������擾���܂��B
 *
 * PRECONDITIONS:
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 * CAVEATS:
 *      �Ȃ��B
 *
 ******************************************************************************
 */

#define SND_GET_SND_CTRL()\
        (SND_PEEK_B(snd_adr_sys_int_work + SND_ADR_SND_CTRL))

/*
 * STRUCTURE DECLARATIONS
 */

/*
 * TYPEDEFS
 */

/**** ���� *******************************************************************/
typedef struct{
    uint16_t *prg_adr;                        /* 68K��۸��ъi�[�擪���ڽ       */
    uint16_t prg_sz;                          /* 68K��۸��ѻ��޻���           */
    uint16_t *ara_adr;                        /* ����޴رϯ�ߊi�[�擪���ڽ     */
    uint16_t ara_sz;                          /* ����޴رϯ�߻���(ܰ�ޒP��)    */
}SndIniDt;                                  /* �V�X�e���N���f�[�^�^          */

typedef uint8_t SndAreaMap;                   /* ����޴رϯ�߃f�[�^�^          */
typedef uint8_t SndTlVl;                      /* �S�̉��ʃf�[�^�^              */
typedef uint8_t SndEfctBnkNum;                /* Effct bank number�f�[�^�^     */
typedef uint8_t SndToneBnkNum;                /* ���F bank number�f�[�^�^      */
typedef uint8_t SndMixBnkNum;                 /* Mixer number�f�[�^�^          */
typedef uint8_t SndEfctOut;                   /* Effect out select�f�[�^�^     */
typedef uint8_t SndLev;                       /* Level�f�[�^�^                 */
typedef int8_t SndPan;                       /* PAN�f�[�^�^                   */
typedef uint8_t SndRet;                       /* ����ގ��s��ԃf�[�^�^         */
typedef uint8_t SndHardPrm;                   /* ʰ�޳���������Ұ��f�[�^�^     */
typedef uint16_t SndHardStat;                 /* ʰ�޳�������ð���f�[�^�^      */

/**** �V�[�P���X *************************************************************/
typedef uint8_t SndSeqNum;                    /* �����Ǘ��ԍ�                  */
typedef uint8_t SndSeqBnkNum;                 /* Sequence bank number          */
typedef uint8_t SndSeqSongNum;                /* Sequence song number          */
typedef uint8_t SndSeqPri;                    /* Priorty level                 */
typedef uint8_t SndSeqVl;                     /* Sequence Volume               */
typedef uint8_t SndFade;                      /* fade Rate                     */
typedef int16_t SndTempo;                    /* Tempo                         */
typedef struct{
    uint16_t mode;                            /* song mode                     */
    uint8_t stat;                             /* song status                   */
}SndSeqStat;                                /* �V�[�P���X�X�e�[�^�X          */
typedef uint16_t SndSeqPlayPos;               /* �����Ǘ��ԍ��Đ��ʒu          */

/**** PCM ********************************************************************/
typedef struct{
    uint8_t mode;                             /* ��ڵ��ɸ�+�����ݸ�ڰ�         */
    uint16_t sadr;                            /* PCM��ذ��ޯ̧���ı��ڽ        */
    uint16_t size;                            /* PCM��ذ��ޯ̧����             */
}SndPcmStartPrm;                            /* PCM�J�n�p�����[�^          */

typedef uint8_t SndPcmNum;                    /* PCM��ذэĐ��ԍ�               */
typedef uint8_t SndEfctIn;                    /* Efect in select               */
typedef struct{
    SndPcmNum num;                          /* PCM��ذэĐ��ԍ�               */
    SndLev lev;                             /* �޲ڸĉ�Level                 */
    SndPan pan;                             /* �޲ڸĉ�pan                   */
    uint16_t pich;                            /* PICHܰ��                      */
    SndEfctIn r_efct_in;                    /* Efect in select(�E�o��)       */
    SndLev r_efct_lev;                      /* Efect send Level(�E�o��)      */
    SndEfctIn l_efct_in;                    /* Efect in select(���o��)       */
    SndLev l_efct_lev;                      /* Efect send Level(���o��)      */
}SndPcmChgPrm;                              /* PCM�ύX�p�����[�^           */

typedef struct{
    uint8_t radr;                             /* �E�o��(�ɸ�)                  */
    uint8_t ladr;                             /* ���o��                         */
}SndPcmPlayAdr;                             /* PCM�Đ����ڽ               */

typedef uint8_t SndPcmIntStat;                /* PCM���ڽ�X�V���荞�ݽð��     */

/**** CD *********************************************************************/
typedef uint16_t SndCdVlAnl;                  /* ��͉���                      */

typedef struct{
    SndCdVlAnl left;                        /* ���o��                        */
    SndCdVlAnl right;                       /* �E�o��                        */
}SndCdHzVl;                                 /* ���g���ш�ʽ�ڵ��͉���      */

typedef struct{
    SndCdHzVl high;                         /* ������                        */
    SndCdHzVl mid;                          /* ������                        */
    SndCdHzVl low;                          /* �ቹ��                        */
}SndCdHzSrVl;                               /* ���g���ш�ʉ�͉���          */

/*
 * EXTERNAL VARIABLE DECLARATIONS
 */
extern uint8_t *snd_adr_sys_int_work;             /*���Ѳ��̪��ܰ��擪���ڽ�i�[*/
extern uint32_t snd_msk_work_work;                /* sound priority msk        */

/*
 * EXTERNAL FUNCTION PROTOTYPE  DECLARATIONS
 */
void SND_Init(SndIniDt *);
void SND_MoveData(uint16_t *, uint32_t, uint16_t, uint16_t);
SndRet SND_ChgMap(SndAreaMap);
SndRet SND_SetTlVl(SndTlVl);
SndRet SND_ChgEfct(SndEfctBnkNum);
SndRet SND_ChgMix(SndToneBnkNum, SndMixBnkNum);
SndRet SND_ChgMixPrm(SndEfctOut, SndLev, SndPan);
SndRet SND_ChkHard(SndHardStat *, SndHardPrm);
SndRet SND_StopDsp(void);
SndRet SND_OffAllSound(void);
SndRet SND_StartSeq(SndSeqNum, SndSeqBnkNum, SndSeqSongNum, SndSeqPri);
SndRet SND_StopSeq(SndSeqNum);
SndRet SND_PauseSeq(SndSeqNum);
SndRet SND_ContSeq(SndSeqNum);
SndRet SND_SetSeqVl(SndSeqNum, SndSeqVl, SndFade);
SndRet SND_ChgTempo(SndSeqNum, SndTempo);
SndRet SND_CtrlDirMidi(SndSeqNum, SndSeqPri, uint8_t, uint8_t, uint8_t, uint8_t);
SndRet SND_SetSeqPan(SndSeqNum, uint8_t, uint8_t );
SndRet SND_StartPcm(SndPcmStartPrm *, SndPcmChgPrm *);
SndRet SND_StopPcm(SndPcmNum);
SndRet SND_ChgPcm(SndPcmChgPrm *);
SndRet SND_StartVlAnl(void);
SndRet SND_StopVlAnl(void);
SndRet SND_SetCdDaLev(SndLev, SndLev);
SndRet SND_SetCdDaPan(SndPan, SndPan);
void SND_GetSeqStat(SndSeqStat *, SndSeqNum);
void SND_GetSeqPlayPos(SndSeqPlayPos *, SndSeqNum);
void SND_GetPcmPlayAdr(SndPcmPlayAdr *, SndPcmNum);
void SND_GetAnlTlVl(SndCdVlAnl *, SndCdVlAnl *);
void SND_GetAnlHzVl(SndCdHzSrVl *);

#endif  /* ifndef SEGA_SND_H */
