/*-----------------------------------------------------------------------------
 *  FILE: sega_tim.h
 *
 *      Copyright(c) 1994 SEGA.
 *
 *  PURPOSE:
 *
 *      �^�C�}���C�u�����w�b�_�t�@�C���B
 *
 *  DESCRIPTION:
 *
 *      �^�C�}�Ǘ�����������B
 *
 *  AUTHOR(S)
 *
 *      1994-08-10  N.T Ver.1.00
 *      1995-11/14  H.O Ver.1.00a
 *
 *  MOD HISTORY:
 *      1995-11/14  �O���N���b�N�w��̋֎~�B
 *
 *-----------------------------------------------------------------------------
 */

/*
 * C VIRTUAL TYPES DEFINITIONS
 */
#include "sega_xpt.h"

/*
 * USER SUPPLIED INCLUDE FILES
 */
#include "sega_int.h"
#include "sega_sys.h"

#ifndef SEGA_TIM_H
#define SEGA_TIM_H

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/**** SCU ********************************************************************/
/******** ���W�X�^ ***********************************************************/
#define TIM_REG_T0C     ((volatile uint32_t *)0x25fe0090)          /* ��Ϻ��߱ڼ޽�     */
#define TIM_REG_T1S     ((volatile uint32_t *)0x25fe0094)          /* ���1���ڼ޽��@    */
#define TIM_REG_T1MD    ((volatile uint32_t *)0x25fe0098)          /* ���1Ӱ��ڼ޽�     */
/******** �萔 ***************************************************************/
/************ �r�b�g�ʒu *****************************************************/
/**************** ���1Ӱ�� ***************************************************/
#define TIM_B_TENB      (0)                             /* ��ϲȰ���ڼ޽��@�@*/
#define TIM_B_T1MD      (8)                             /* ���1Ӱ��ڼ޽� �@�@*/
/************ �l *************************************************************/
/**************** ���1Ӱ�� ***************************************************/
#define TIM_TENB_OFF    (0 << TIM_B_TENB)               /* ���off            */
#define TIM_TENB_ON     (1 << TIM_B_TENB)               /* ���on         �@�@*/
#define TIM_T1MD_ALL_LINE   (0 << TIM_B_T1MD)           /* �����C�����荞��  */
#define TIM_T1MD_CST_LINE   (1 << TIM_B_T1MD)           /* �w�胉�C�����荞��*/

/**** CPU (�ذ���ݸ����) *****************************************************/
/******** ���W�X�^ ***********************************************************/
#define TIM_REG_TIER    ((volatile uint8_t  *)0xfffffe10)          /* ��ϲ�����ĲȰ���  */
#define TIM_REG_TCSR    ((volatile uint8_t  *)0xfffffe11)          /* ��Ϻ��۰٥�ð��   */
#define TIM_REG_FRC_H   ((volatile uint8_t  *)0xfffffe12)          /* �ذ���ݸ޶��� H   */
#define TIM_REG_FRC_L   ((volatile uint8_t  *)0xfffffe13)          /* �ذ���ݸ޶��� L   */
#define TIM_REG_OCRX_H  ((volatile uint8_t  *)0xfffffe14)          /* ����߯ĺ��߱A,B H */
#define TIM_REG_OCRX_L  ((volatile uint8_t  *)0xfffffe15)          /* ����߯ĺ��߱A,B L */
#define TIM_REG_TCR     ((volatile uint8_t  *)0xfffffe16)          /* ��Ϻ��۰ف@�@�@   */
#define TIM_REG_TOCR    ((volatile uint8_t  *)0xfffffe17)          /* ��ϱ���߯ĺ��߱   */
#define TIM_REG_ICRA_H  ((volatile uint8_t  *)0xfffffe18)          /* ���߯ķ�����A H   */
#define TIM_REG_ICRA_L  ((volatile uint8_t  *)0xfffffe19)          /* ���߯ķ�����A L   */
/******** �萔 ***************************************************************/
/************ �r�b�g�ʒu *****************************************************/
/**************** ��ϲ�����ĲȰ��  *******************************************/
#define TIM_B_ICIAE     (7)                             /* ICAE              */
#define TIM_B_OCIAE     (3)                             /* OCIAE             */
#define TIM_B_OCIBE     (2)                             /* OCIBE             */
#define TIM_B_OVIE      (1)                             /* OVIE              */
/**************** ��Ϻ��۰٥�ð� *********************************************/
#define TIM_B_ICFA      (7)                             /* ICFA              */
#define TIM_B_OCFA      (3)                             /* OCFA              */
#define TIM_B_OCFB      (2)                             /* OCFB              */
#define TIM_B_OVI       (1)                             /* OVI               */
#define TIM_B_CCLRA     (0)                             /* CCLRA             */
/**************** ��Ϻ��۰� **************************************************/
#define TIM_B_IEDGA     (7)                             /* IEDGA             */
#define TIM_B_BUFEA     (3)                             /* BUFEA             */
#define TIM_B_BUFEB     (2)                             /* BUFEB             */
#define TIM_B_CKS1      (1)                             /* CKS1              */
#define TIM_B_CKS0      (0)                             /* CKS0              */
/**************** ��ϱ���߯ĺ��߱*********************************************/
#define TIM_B_OCRS      (4)                             /* OCRS              */
#define TIM_B_OEA       (3)                             /* OEA               */
#define TIM_B_OEB       (2)                             /* OEB               */
#define TIM_B_OLVLA     (1)                             /* OLVLA             */
#define TIM_B_OLVLB     (0)                             /* OLVLB             */
/************ �}�X�N *********************************************************/
/**************** ��ϲ�����ĲȰ��  *******************************************/
#define TIM_M_ICIAE     (1 << TIM_B_ICIAE)              /* ICAE              */
#define TIM_M_OCIAE     (1 << TIM_B_OCIAE)              /* OCIAE             */
#define TIM_M_OCIBE     (1 << TIM_B_OCIBE)              /* OCIBE             */
#define TIM_M_OVIE      (1 << TIM_B_OVIE )              /* OVIE              */
/**************** ��Ϻ��۰٥�ð� *********************************************/
#define TIM_M_ICFA      (1 << TIM_B_ICFA )              /* ICFA              */
#define TIM_M_OCFA      (1 << TIM_B_OCFA )              /* OCFA              */
#define TIM_M_OCFB      (1 << TIM_B_OCFB )              /* OCFB              */
#define TIM_M_OVI       (1 << TIM_B_OVI  )              /* OVI               */
#define TIM_M_CCLRA     (1 << TIM_B_CCLRA)              /* CCLRA             */
/**************** ��Ϻ��۰� **************************************************/
#define TIM_M_IEDGA     (1 << TIM_B_IEDGA)              /* IEDGA             */
#define TIM_M_BUFEA     (1 << TIM_B_BUFEA)              /* BUFEA             */
#define TIM_M_BUFEB     (1 << TIM_B_BUFEB)              /* BUFEB             */
#define TIM_M_CKS       (3 << TIM_B_CKS0 )              /* CKS1,0            */
/**************** ��ϱ���߯ĺ��߱*********************************************/
#define TIM_M_OCRS      (1 << TIM_B_OCRS )              /* OCRS              */
#define TIM_M_OEA       (1 << TIM_B_OEA  )              /* OEA               */
#define TIM_M_OEB       (1 << TIM_B_OEB  )              /* OEB               */
#define TIM_M_OLVLA     (1 << TIM_B_OLVLA)              /* OLVLA             */
#define TIM_M_OLVLB     (1 << TIM_B_OLVLB)              /* OLVLB             */
/************ �l *************************************************************/
/**************** ��ϲ�����ĲȰ�� ********************************************/
/******************** ��ϲ�����ĲȰ�� ****************************************/
#define TIM_ICIAE_DIS   (0 << TIM_B_ICIAE)              /* ICAE�֎~          */
#define TIM_ICIAE_ENA   (1 << TIM_B_ICIAE)              /* ICAE����          */
/******************** ����߯ĺ��߱A ******************************************/
#define TIM_OCIAE_DIS   (0 << TIM_B_OCIAE)              /* OCFAE�֎~         */
#define TIM_OCIAE_ENA   (1 << TIM_B_OCIAE)              /* OCFAE����         */
/******************** ����߯ĺ��߱B ******************************************/
#define TIM_OCIBE_DIS   (0 << TIM_B_OCIBE)              /* OCFAE�֎~         */
#define TIM_OCIBE_ENA   (1 << TIM_B_OCIBE)              /* OCFBE����         */
/******************** ��ϵ����۰������ĲȰ��� ********************************/
#define TIM_OVIE_DIS    (0 << TIM_B_OVIE)               /* FOVI �֎~         */
#define TIM_OVIE_ENA    (1 << TIM_B_OVIE)               /* FOVI ����         */
/**************** ��Ϻ��۰٥�ð� *********************************************/
/******************** ICFA ***************************************************/
#define TIM_ICFA_CLR    (0 << TIM_B_ICFA)               /* ICFA �N���A       */
#define TIM_ICFA_SET    (1 << TIM_B_ICFA)               /* ICFA �Z�b�g       */
/******************** OCFA ***************************************************/
#define TIM_OCFA_CLR    (0 << TIM_B_OCFA)               /* OCFA �N���A       */
#define TIM_OCFA_SET    (1 << TIM_B_OCFA)               /* OCFA �Z�b�g       */
/******************** OCFB ***************************************************/
#define TIM_OCFB_CLR    (0 << TIM_B_OCFB)               /* OCFB �N���A       */
#define TIM_OCFB_SET    (1 << TIM_B_OCFB)               /* OCFB �Z�b�g       */
/******************** OVF ****************************************************/
#define TIM_OVF_CLR     (0 << TIM_B_OVI)                /* OVI  �N���A       */
#define TIM_OVF_SET     (1 << TIM_B_OVI)                /* OVI  �Z�b�g       */
/******************** CCLRA **************************************************/
#define TIM_CCLRA_CLR   (0 << TIM_B_CCLRA)              /* FRC  �N���A�֎~   */
#define TIM_CCLRA_SET   (1 << TIM_B_CCLRA)              /* FRC���N���A       */
/**************** ��Ϻ��۰� **************************************************/
/******************** IEDGA **************************************************/
#define TIM_IEDGA_UP_EG (1 << TIM_B_IEDGA)              /* UP EG ������      */
#define TIM_IEDGA_DW_EG (0 << TIM_B_IEDGA)              /* DW EG ������      */
/******************** BUFEA **************************************************/
#define TIM_BUFEA_NO_US (0 << TIM_B_BUFEA)              /* ICRC���ޯ̧�g�p   */
#define TIM_BUFEA_US    (1 << TIM_B_BUFEA)              /* ICRC���ޯ̧�g�p   */
/******************** BUFEB **************************************************/
#define TIM_BUFEB_NO_US (0 << TIM_B_BUFEB)              /* ICRD���ޯ̧�g�p   */
#define TIM_BUFEB_US    (1 << TIM_B_BUFEB)              /* ICRD���ޯ̧�g�p   */
/******************** CKS1,0 *************************************************/
#define TIM_CKS_8       ( 0 << TIM_B_CKS0)              /* 8 �J�E���g        */
#define TIM_CKS_32      ( 1 << TIM_B_CKS0)              /* 32�J�E���g        */
#define TIM_CKS_128     ( 2 << TIM_B_CKS0)              /* 128�J�E���g       */
#define TIM_CKS_OUT     ( 3 << TIM_B_CKS0)              /* �O���J�E���g      */
/**************** ��ϱ���߯ĺ��߱*********************************************/
/******************** OCRS ***************************************************/
#define TIM_OCRS_OCRA   ( 0 << TIM_B_OCRS)              /* OCRA�I���@        */
#define TIM_OCRS_OCRB   ( 1 << TIM_B_OCRS)              /* OCRB�I��          */
/******************** OEA ****************************************************/
#define TIM_OEA_DIS     ( 0 << TIM_B_OEA)               /* OEA�֎~ �@        */
#define TIM_OEA_ENA     ( 1 << TIM_B_OEA)               /* OEA���� �@        */
/******************** OEB ****************************************************/
#define TIM_OEB_DIS     ( 0 << TIM_B_OEB)               /* OEB�֎~ �@        */
#define TIM_OEB_ENA     ( 1 << TIM_B_OEB)               /* OEB���� �@        */
/******************** OLVLA **************************************************/
#define TIM_OLVLA_0     ( 0 << TIM_B_OLVLA)             /* "0"�o�� �@        */
#define TIM_OLVLA_1     ( 1 << TIM_B_OLVLA)             /* "1"�o�́@         */
/******************** OLVLB **************************************************/
#define TIM_OLVLB_0     ( 0 << TIM_B_OLVLB)             /* "0"�o�� �@        */
#define TIM_OLVLB_1     ( 1 << TIM_B_OLVLB)             /* "1"�o�́@         */
/**** �����}�N�� *************************************************************/
/**** ���������C�g ***********************************************************/
#define TIM_POKE_B(adr, data)   (*((volatile uint8_t *)(adr)) = ((uint8_t)(data)))  /*�޲�*/
#define TIM_POKE_W(adr, data)   (*((volatile uint16_t *)(adr)) = ((uint16_t)(data)))/*ܰ��*/
#define TIM_POKE_L(adr, data)   (*((volatile uint32_t *)(adr)) = ((uint32_t)(data)))/*�ݸ�*/
/**** ���������[�h ***********************************************************/
#define TIM_PEEK_B(adr)         (*((volatile uint8_t *)(adr)))                    /*�޲�*/
#define TIM_PEEK_W(adr)         (*((volatile uint16_t *)(adr)))                   /*ܰ��*/
#define TIM_PEEK_L(adr)         (*((volatile uint32_t *)(adr)))                   /*�ݸ�*/

/*****************************************************************************/
/*****************************************************************************/
/**** �֐��`���}�N�� *********************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/******* SCU *****************************************************************/
/*****************************************************************************/

/******************************************************************************
 *
 * NAME:    TIM_T0_ENABLE           - �^�C�}0���荞�݃C�l�[�u��
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_T0_ENABLE()\
    do{\
        INT_ChgMsk(INT_MSK_TIM0, INT_MSK_NULL);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T0_DISABLE          - �^�C�}0���荞�݃f�B�Z�[�u��
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_T0_DISABLE()\
    do{\
        INT_ChgMsk(INT_MSK_NULL, INT_MSK_TIM0);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T1_ENABLE           - �^�C�}1���荞�݃C�l�[�u��
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_T1_ENABLE()\
    do{\
        INT_ChgMsk(INT_MSK_TIM1, INT_MSK_NULL);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T1_DISABLE          - �^�C�}1���荞�݃f�B�Z�[�u��
 *
 * PARAMETERS :
 *      �Ȃ��B
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_T1_DISABLE()\
    do{\
        INT_ChgMsk(INT_MSK_NULL, INT_MSK_TIM1);\
    }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_T0_SET_CMP          - �^�C�}0�R���y�A���W�X�^�ݒ�
 *
 * PARAMETERS :
 *      (1) uint32_t time_cmp         - <i>   �^�C�}0�R���y�A���W�X�^�ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_T0_SET_CMP(time_cmp)\
            TIM_POKE_L(TIM_REG_T0C, (time_cmp))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_DATA         - �^�C�}1�Z�b�g�f�[�^���W�X�^
 *
 * PARAMETERS :
 *      (1) uint32_t time_data        - <i>   �^�C�}1�Z�b�g�f�[�^���W�X�^�ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_T1_SET_DATA(time_data)\
            TIM_POKE_L(TIM_REG_T1S, (time_data))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_MODE         - �^�C�}1���[�h���W�X�^�ݒ�
 *
 * PARAMETERS :
 *      (1) uint32_t time_mode        - <i>   �^�C�}1���[�h���W�X�^�ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_T1_SET_MODE(time_mode)\
            TIM_POKE_L(TIM_REG_T1MD, (time_mode))

/******************************************************************************
 *
 * NAME:    TIM_T1_SET_MODE         - �^�C�}1���[�h���W�X�^�ݒ�
 *
 * PARAMETERS :
 *      (1) uint32_t time_mode        - <i>   �^�C�}1���[�h���W�X�^�ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_T1_SET_MODE(time_mode)\
            TIM_POKE_L(TIM_REG_T1MD, (time_mode))

/*****************************************************************************/
/******* CPU *****************************************************************/
/*****************************************************************************/
/******************************************************************************
 *
 * NAME:    TIM_FRT_INIT            - FRT������
 *
 * PARAMETERS :
 *      (1) uint32_t mode             - <i>   ������
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 * NOTICE:
 *      �O���N���b�N�I���̋֎~�΍��ŁB
 *
 ******************************************************************************
 */
#if 0
#define TIM_FRT_INIT(mode)\
            TIM_POKE_B(TIM_REG_TCR ,\
                       (TIM_PEEK_B(TIM_REG_TCR) & ~TIM_M_CKS) | (mode))
#else
#define TIM_FRT_INIT(mode)\
	do{\
		unsigned char _tcr = (unsigned char)(TIM_PEEK_B(TIM_REG_TCR) & ~TIM_M_CKS);\
		if( ( ( mode ) & TIM_M_CKS ) != TIM_M_CKS ){\
			_tcr |= ( ( mode ) & TIM_M_CKS );\
            TIM_POKE_B(TIM_REG_TCR , _tcr);\
		}\
	}while(0)
#endif

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_16          - �J�E���^�l�ݒ�(16�r�b�g)
 *
 * PARAMETERS :
 *      (1) uint16_t cnt              - <i>   �J�E���^�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_16(cnt)\
        do{\
            TIM_FRT_SET_FRC(cnt);\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_16          - �J�E���^�l�擾(16�r�b�g)
 *
 * PARAMETERS :
 *      �Ȃ�
 *
 * POSTCONDITIONS:
 *      (1) uint16_t cnt              - <o>   �J�E���^�l
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_16()\
        TIM_FRT_GET_FRC()

/******************************************************************************
 *
 * NAME:    TIM_FRT_DELAY_16        - ���ԑ҂�(16�r�b�g)
 *
 * PARAMETERS :
 *      (1) uint16_t cnt              - <i>   ���ԑ҂��J�E���^�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_DELAY_16(cnt)\
        do{\
            TIM_FRT_SET_16(0);\
            while((cnt) != TIM_FRT_GET_16());\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_CNT_TO_MCR      -   �J�E���^�l->�}�C�N���b�ϊ�
 *
 * PARAMETERS :
 *      (1) uint32_t count            - <i>   �J�E���^�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_CNT_TO_MCR(count)\
    (\
    (((*(uint16_t *)0x25f80004 & 0x1) == 0x1) ?   /* PAL ��? */\
     ((SYS_GETSYSCK == 0) ? (float)0.037470726 : (float)0.035164835 ) :/*PAL 26,28*/\
     ((SYS_GETSYSCK == 0) ? (float)0.037210548 : (float)0.03492059 )) /*NT 26,28*/\
     * (count) * (8 << ((TIM_PEEK_B(TIM_REG_TCR) & TIM_M_CKS) << 1)))

/******************************************************************************
 *
 * NAME:    TIM_FRT_MCR_TO_CNT      -   �}�C�N���b->�J�E���^�l�ϊ�
 *
 * PARAMETERS :
 *      (1) uint32_t mcr              - <i>   �}�C�N���b�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_MCR_TO_CNT(mcr)\
    ((mcr) /\
    (((*(volatile uint16_t *)0x25f80004 & 0x1) == 0x1) ?   /* PAL ��? */\
     ((SYS_GETSYSCK == 0) ? (float)0.037470726 : (float)0.035164835 ) :/*PAL 26,28*/\
     ((SYS_GETSYSCK == 0) ? (float)0.037210548 : (float)0.03492059 )) /*NT 26,28*/\
    / (8 << ((TIM_PEEK_B(TIM_REG_TCR) & TIM_M_CKS) << 1)))
/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TIER        - �^�C�}�C���^���v�g�C�l�[�u�����W�X�^�ݒ�
 *
 * PARAMETERS :
 *      (1) uint8_t reg               - <i>   �ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TIER(reg)\
        TIM_POKE_B(TIM_REG_TIER, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TCSR        - �^�C�}�R���g���[���X�e�[�^�X���W�X�^�ݒ�
 *
 * PARAMETERS :
 *      (1) uint8_t reg               - <i>   �ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TCSR(reg)\
        TIM_POKE_B(TIM_REG_TCSR, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_FRC         - �t���[�����j���O�J�E���^�ݒ�
 *
 * PARAMETERS :
 *      (1) uint16_t reg              - <i>   �ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_FRC(reg)\
        do{\
            TIM_POKE_B(TIM_REG_FRC_H, ((reg) >> 8));\
            TIM_POKE_B(TIM_REG_FRC_L, (reg));\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_OCRA        - �A�E�g�v�b�g�R���y�A���W�X�^A�ݒ�
 *
 * PARAMETERS :
 *      (1) uint16_t reg              - <i>   �ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_OCRA(reg)\
        do{\
            TIM_FRT_SET_TOCR(((TIM_FRT_GET_TOCR()) & ~TIM_M_OCRS) |\
            TIM_OCRS_OCRA);\
            TIM_POKE_B(TIM_REG_OCRX_H, ((reg) >> 8));\
            TIM_POKE_B(TIM_REG_OCRX_L, (reg));\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_OCRB        - �A�E�g�v�b�g�R���y�A���W�X�^B�ݒ�
 *
 * PARAMETERS :
 *      (1) uint16_t reg              - <i>   �ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_OCRB(reg)\
        do{\
            TIM_FRT_SET_TOCR(((TIM_FRT_GET_TOCR()) & ~TIM_M_OCRS) |\
            TIM_OCRS_OCRB);\
            TIM_POKE_B(TIM_REG_OCRX_H, ((reg) >> 8));\
            TIM_POKE_B(TIM_REG_OCRX_L, (reg));\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TCR         - �^�C�}�R���g���[�����W�X�^�ݒ�
 *
 * PARAMETERS :
 *      (1) uint8_t reg               - <i>   �ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 * NOTICE:
 *      �O���N���b�N�I���̋֎~�΍��ŁB
 *
 ******************************************************************************
 */
#if 0
#define TIM_FRT_SET_TCR(reg)\
            TIM_POKE_B(TIM_REG_TCR, (reg))
#else
#define TIM_FRT_SET_TCR(reg)\
	do{\
		unsigned char _tcr;\
		if(((reg) & TIM_M_CKS ) != TIM_M_CKS ){\
			_tcr = (unsigned char)( (reg) & 0x83 );\
		}\
		else{\
			_tcr = (unsigned char)( (reg) & 0x80 );\
		}\
        TIM_POKE_B(TIM_REG_TCR, _tcr);\
	}while(0)
#endif

/******************************************************************************
 *
 * NAME:    TIM_FRT_SET_TOCR        - �^�C�}�A�E�g�v�b�g�R���y�A�R���g���[��
 *                                    ���W�X�^�ݒ�
 * PARAMETERS :
 *      (1) uint8_t reg               - <i>   �ݒ�l
 *
 * POSTCONDITIONS:
 *      �Ȃ�
 *
 ******************************************************************************
 */
#define TIM_FRT_SET_TOCR(reg)\
            TIM_POKE_B(TIM_REG_TOCR, (reg))

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TIER        - �^�C�}�C���^���v�g�C�l�[�u�����W�X�^�擾
 *
 * PARAMETERS :
 *      �Ȃ�
 *
 * POSTCONDITIONS:
 *      (1) uint8_t reg               - <o>   ���W�X�^�l
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TIER()\
            TIM_PEEK_B(TIM_REG_TIER)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TCSR        - �^�C�}�R���g���[���X�e�[�^�X���W�X�^�擾
 *
 * PARAMETERS :
 *      �Ȃ�
 *
 * POSTCONDITIONS:
 *      (1) uint8_t reg               - <o>   ���W�X�^�l
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TCSR()\
            TIM_PEEK_B(TIM_REG_TCSR)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_FRC          - �t���[�����j���O�J�E���g�擾
 *
 * PARAMETERS :
 *      �Ȃ�
 *
 * POSTCONDITIONS:
 *      (1) uint16_t reg               - <o>   ���W�X�^�l
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_FRC()\
            (TIM_PEEK_B(TIM_REG_FRC_H) << 8 | TIM_PEEK_B(TIM_REG_FRC_L))

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_OCRA         - �A�E�g�v�b�g�R���y�A���W�X�^A�擾
 *
 * PARAMETERS :
 *      (1) uint16_t reg               - <o>   ���W�X�^�l
 *
 * POSTCONDITIONS:
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_OCRA(reg)\
        do{\
            TIM_FRT_SET_TOCR(((TIM_FRT_GET_TOCR()) & ~TIM_M_OCRS) |\
            TIM_OCRS_OCRA);\
            (reg) = (TIM_PEEK_B(TIM_REG_OCRX_H) << 8) |\
                                       TIM_PEEK_B(TIM_REG_OCRX_L);\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_OCRB         - �A�E�g�v�b�g�R���y�A���W�X�^B�擾
 *
 * PARAMETERS :
 *      (1) uint16_t reg               - <o>   ���W�X�^�l
 *
 * POSTCONDITIONS:
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_OCRB(reg)\
        do{\
            TIM_FRT_SET_TOCR(((TIM_FRT_GET_TOCR()) & ~TIM_M_OCRS) |\
            TIM_OCRS_OCRB);\
            (reg) = (TIM_PEEK_B(TIM_REG_OCRX_H) << 8) |\
                                       TIM_PEEK_B(TIM_REG_OCRX_L);\
        }while(FALSE)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TCR          - �^�C�}�R���g���[�����W�X�^�擾
 *
 * PARAMETERS :
 *      �Ȃ�
 *
 * POSTCONDITIONS:
 *      (1) uint8_t  reg               - <o>   ���W�X�^�l
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TCR()\
            TIM_PEEK_B(TIM_REG_TCR)
                                                              
/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_TOCR         - �^�C�}�A�E�g�v�b�g�R���y�A�R���g���[��
 *                                     ���W�X�^�擾
 * PARAMETERS :
 *      �Ȃ�
 *
 * POSTCONDITIONS:
 *      (1) uint8_t reg                - <o>   ���W�X�^�l
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_TOCR()\
            TIM_PEEK_B(TIM_REG_TOCR)

/******************************************************************************
 *
 * NAME:    TIM_FRT_GET_ICRA         - �C���v�b�g�L���v�`�����W�X�^A�擾
 *
 * PARAMETERS :
 *      �Ȃ�
 *
 * POSTCONDITIONS:
 *      (1) uint16_t reg               - <o>   ���W�X�^�l
 *
 ******************************************************************************
 */
#define TIM_FRT_GET_ICRA()\
            ((TIM_PEEK_B(TIM_REG_ICRA_H) << 8) | TIM_PEEK_B(TIM_REG_ICRA_L))

/*
 * STRUCTURE DECLARATIONS
 */

/*
 * TYPEDEFS
 */

/*
 * EXTERNAL VARIABLE DECLARATIONS
 */

/*
 * EXTERNAL FUNCTION PROTOTYPE  DECLARATIONS
 */

#endif  /* ifndef SEGA_TIM_H */
