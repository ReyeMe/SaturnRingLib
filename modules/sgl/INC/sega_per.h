/*-----------------------------------------------------------------------------
 *  FILE: sega_per.h
 *
 *      Copyright(c) 1994 SEGA.
 *
 *  PURPOSE:
 *
 *      �����ƥࡿ�ڥ�ե����饤�֥����ѥإå��ե����롣
 *
 *  DESCRIPTION:
 *
 *      �����ƥࡿ�ڥ�ե����饤�֥�����Ѥ���ץ������˥��󥯥롼�ɤ���
 *      ����������
 *
 *  AUTHOR(S)
 *
 *      1994-05-19  N.T Ver.0.90
 *
 *  MOD HISTORY:
 *      1994-08-26  N.T Ver.1.02
 *      1994-11-25  N.T Ver.1.05
 *      1996-09-25  K.K Ver.1.09 1byte���ʥ��������Ǻ���
 *      1997-04-14  A.H Ver.1.10 SMPC����߻� MAC�쥸�������˲������Х�����
 *      1997-06-09  A.H Ver.1.11
 *            1.����饤��ޥ˥奢�빹����ȼ������¤�������ľ�����ɲ�(ͽ��)
 *            2.SEGA_SYS�饤�֥��Ver2.50 ���б�
 *            3.�饤�֥�꤬���Ѥ������̤η׻��ѥޥ������ɲ�
 *                �Х���ñ���ѡ�PER_WORK_SIZE
 *                ���ñ���ѡ�PER_WORK_SIZE2
 *                ���󥰥��ñ���ѡ�PER_WORK_SIZE4
 *
 *
 *-----------------------------------------------------------------------------
 */

#ifndef SEGA_PER_H
#define SEGA_PER_H

/*
 * �С�����������
 */
#define PER_VER_111         /* 1997-05-26 �ɲ� A.H(SOJ) */

#include "sega_xpt.h"       /* 1997-05-26 �ɲ� A.H(SOJ) */
#include "sega_int.h"       /* 1997-05-26 �ɲ� A.H(SOJ) */

/********************************* per_xpt.h *********************************/
/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/**** �饤�֥����ѥ�������� *********************************************/
/* for Uint8 */
#define PER_WORK_SIZE(num, size)	\
	  ( ((num) * ((size) + 2) * 2) + (size) )

/* for Uint16 */
#define PER_WORK_SIZE2(num, size)	\
	( ( ((num) * ((size) + 2) * 2) + (size) + sizeof(Uint16) - 1) / sizeof(Uint16) )

/* for Uint32 */
#define PER_WORK_SIZE4(num, size)	\
	( ( ((num) * ((size) + 2) * 2) + (size) + sizeof(Uint32) - 1) / sizeof(Uint32) )

/**** �쥸�������ɥ쥹 *******************************************************/
#ifndef _DEB2
#define PER_REG_COMREG  ((volatile uint8_t *)0x2010001f)   /* ���ޥ�ɥ쥸����          */
#define PER_REG_SR      ((volatile uint8_t *)0x20100061)   /* ���ơ������쥸����        */
#define PER_REG_SF      ((volatile uint8_t *)0x20100063)   /* ���ơ������ե饰          */
#define PER_REG_IREG    ((volatile uint8_t *)0x20100001)   /* IREG                      */
#define PER_REG_OREG    ((volatile uint8_t *)0x20100021)   /* OREG                      */
#else
#define PER_REG_COMREG  ((volatile uint8_t *)0x0603001f)   /* ���ޥ�ɥ쥸����          */
#define PER_REG_SR      ((volatile uint8_t *)0x06030061)   /* ���ơ������쥸����        */
#define PER_REG_SF      ((volatile uint8_t *)0x06030063)   /* ���ơ������ե饰          */
#define PER_REG_IREG    ((volatile uint8_t *)0x06030001)   /* IREG                      */
#define PER_REG_OREG    ((volatile uint8_t *)0x06030021)   /* OREG                      */
#endif

/**** �ӥåȰ��� *************************************************************/
#define PER_B_SF        0x1                     /* ���ơ������ե饰          */

/**** �ؿ������ޥ��� *********************************************************/
#define PER_PokeByte(address,data)  (*(volatile uint8_t *)(address) = (uint8_t)(data))
                                      /* �Х��ȥǡ������ɥ쥹ľ�ܽ񤭹��� */
#define PER_PeekByte(address)   (*(volatile uint8_t *)(address))
                                      /* �Х��ȥǡ������ɥ쥹ľ���ɤ߹���  */

/*
 * TYPEDEFS
 */

/********************************* per_prt.h *********************************/
/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/* �ޥ�����å�ID */
#define PER_MID_NCON_ONE    0xf0                /* ̤��³ or ľ��            */

/* �ޥ�����åץ��ͥ����� */
#define PER_MCON_NCON_UNKNOWN   0x00            /* ̤��³ or UNKNOWN         */

/* �ڥ�ե���륿���� */
#define PER_ID_NCON_UNKNOWN 0xf0                /* ̤��³ or SMPC UNKNOWN    */
#define PER_ID_DGT      0x00                    /* �ǥ�����ǥХ���          */
#define PER_ID_ANL      0x10                    /* ���ʥ����ǥХ���          */
#define PER_ID_PNT      0x20                    /* �ޥ���                    */
#define PER_ID_KBD      0x30                    /* �����ܡ���                */
#define PER_ID_MD       0xe0                   /* �ᥬ�ɥ饤�� PAD(3B or 6B) */

/* �ڥ�ե���륵���� */
#define PER_SIZE_NCON_15    0x0f                /* ̤��³ or 15�Х��ȥǥХ���*/
#define PER_SIZE_DGT    2                       /* �ǥ�����ǥХ���          */
#define PER_SIZE_PNT    3                       /* �ޥ���                    */
#define PER_SIZE_KBD    4                       /* �����ܡ���                */
#define PER_SIZE_M3BP   1                       /* �ᥬ�ɥ饤��3�ܥ���ѥå� */
#define PER_SIZE_M6BP   2                       /* �ᥬ�ɥ饤��6�ܥ���ѥå� */
#define PER_SIZE_ANL    5                       /* �ߥå���󥹥ƥ��å�      */
#if 1
#define PER_SIZE_ANL2   8            /* �ߥå���󥹥ƥ��å�(2���ƥ��å�)    */
#define PER_SIZE_3DPAD  6                       /* �ޥ������ȥ�����        */
#define PER_SIZE_STEERING    3                  /* ���ƥ���󥰥���ȥ�����  */
#endif

/* ����ȥХå����� */
#define PER_KD_SYS      0                       /* �����ƥ�ǡ�������        */
#define PER_KD_PER      1                       /* �ڥ�ե����ǡ�������    */
#define PER_KD_PERTIM   2                       /* �ڥ�ե����ǡ���������  */
                                                /* ����ǡ�������            */
/* �ӥåȰ��� ****************************************************************/
/* �ǥ�����ǥХ����ǡ�����	 */
#define	PER_LDGT_R   (1 <<  7)                  /* RIGHT                     */
#define	PER_LDGT_L   (1 <<  6)                  /* LEFT                      */
#define	PER_LDGT_D   (1 <<  5)                  /* DOWN                      */
#define	PER_LDGT_U   (1 <<  4)                  /* UP                        */
#define	PER_LDGT_S   (1 <<  3)                  /* START                     */
#define	PER_LDGT_A   (1 <<  2)                  /* A                         */
#define	PER_LDGT_C   (1 <<  1)                  /* C                         */
#define	PER_LDGT_B   (1 <<  0)                  /* B                         */
#define	PER_LDGT_TR  (1 <<  7)                  /* �ȥꥬ RIGHT              */
#define	PER_LDGT_X   (1 <<  6)                  /* X                         */
#define	PER_LDGT_Y   (1 <<  5)                  /* Y                         */
#define	PER_LDGT_Z   (1 <<  4)                  /* Z                         */
#define	PER_LDGT_TL  (1 <<  3)                  /* �ȥꥬ LEFT               */
/* �ݥ���ƥ��󥰥ǥХ����ǡ����� */
/***** �ǡ��� */
#define PER_LPNT_YO  (1 << 7)                   /* Y�������Хե���           */
#define PER_LPNT_XO  (1 << 6)                   /* X�������Хե���           */
#define PER_LPNT_YS  (1 << 5)                   /* Y�����                   */
#define PER_LPNT_XS  (1 << 4)                   /* X�����                   */
#define PER_LPNT_CNT (1 << 3)                   /* CENTER                    */
#define PER_LPNT_MID (1 << 2)                   /* MIDDLE                    */
#define PER_LPNT_R   (1 << 1)                   /* RIGHT                     */
#define PER_LPNT_L   (1 << 0)                   /* LEFT                      */

/* �����ܡ��ɥǥХ����ǡ����� */
/***** �ü쥭�� */
#define PER_LKBD_CL  (1 << 6)                   /* Caps Lock                 */
#define PER_LKBD_NL  (1 << 5)                   /* Num Lock                  */
#define PER_LKBD_SL  (1 << 4)                   /* Scrool Lock               */
#define PER_LKBD_MK  (1 << 3)                   /* Make                      */
#define PER_LKBD_BR  (1 << 0)                   /* Break                     */

/* �ᥬ�ɥ饤��3�ܥ���ѥåɥǡ����� */
#define	PER_LM3BP_R  PER_LDGT_R                 /* RIGHT                     */
#define	PER_LM3BP_L  PER_LDGT_L                 /* LEFT                      */
#define	PER_LM3BP_D  PER_LDGT_D                 /* DOWN                      */
#define	PER_LM3BP_U  PER_LDGT_U                 /* UP                        */
#define	PER_LM3BP_S  PER_LDGT_S                 /* START                     */
#define	PER_LM3BP_A  PER_LDGT_A                 /* A                         */
#define	PER_LM3BP_C  PER_LDGT_C                 /* C                         */
#define	PER_LM3BP_B  PER_LDGT_B                 /* B                         */

/* �ᥬ�ɥ饤��6�ܥ���ѥåɥǡ����� */
#define	PER_LM6BP_R  PER_LDGT_R                 /* RIGHT                     */
#define	PER_LM6BP_L  PER_LDGT_L                 /* LEFT                      */
#define	PER_LM6BP_D  PER_LDGT_D                 /* DOWN                      */
#define	PER_LM6BP_U  PER_LDGT_U                 /* UP                        */
#define	PER_LM6BP_S  PER_LDGT_S                 /* START                     */
#define	PER_LM6BP_A  PER_LDGT_A                 /* A                         */
#define	PER_LM6BP_C  PER_LDGT_C                 /* C                         */
#define	PER_LM6BP_B  PER_LDGT_B                 /* B                         */
#define	PER_LM6BP_MD PER_LDGT_TR                /* MODE                      */
#define	PER_LM6BP_X  PER_LDGT_X                 /* X                         */
#define	PER_LM6BP_Y  PER_LDGT_Y                 /* Y                         */
#define	PER_LM6BP_Z  PER_LDGT_Z                 /* Z                         */

/* �����ƥ�ǡ������� */
/* �����ƥॹ�ơ����� */
#define PER_SS_DOTSEL   (1 <<  6)               /* DOTSEL�������            */
#define PER_SS_SYSRES   (1 <<  1)               /* SYSRES�������            */
#define PER_SS_MSHNMI   (1 <<  3)               /* MSHNMI�������            */
#define PER_SS_SNDRES   (1 <<  0)               /* SNDRES�������            */
#define PER_SS_CDRES    (1 << 14)               /* CDRES�������             */

/* SMPC���� */
/***** �ޥ��� */
#define PER_MSK_LANGU   (0xf << 0)              /* ����ޥ���                */
#define PER_MSK_SE      (0x1 << 8)              /* SE                        */
#define PER_MSK_STEREO  (0x1 << 9)              /* STEREO or MONO            */
#define PER_MSK_HELP    (0x1 << 10)             /* HELP                      */
/***** ���� */
#define PER_ENGLISH     0x0                     /* English(�Ѹ�)             */
#define PER_DEUTSCH     0x1                     /* Deutsch(�ɥ��ĸ�)         */
#define PER_FRANCAIS    0x2                     /* francais(�ե�󥹸�)      */
#define PER_ESPNOL      0x3                     /* Espnol(���ڥ����)        */
#define PER_ITALIANO    0x4                     /* Italiano(�����ꥢ��)      */
#define PER_JAPAN       0x5                     /* Japan(���ܸ�)             */

/* SMPC���ơ����� */
#define PER_SS_RESET    (1 <<  6)               /* �ꥻ�åȥޥ�������        */
#define PER_SS_SETTIM   (1 <<  7)               /* �����������              */

/* �����ƥ�ǡ������� */
#define PER_GS_CC(data)         ((data)->cc)    /* �����ȥ�å�������        */
#define PER_GS_AC(data)         ((data)->ac)    /* ���ꥢ������              */
#define PER_GS_SS(data)         ((data)->ss)    /* �����ƥॹ�ơ�����        */
#define PER_GS_SM(data)         ((data)->sm)    /* SMPC����                */
#define PER_GS_SMPC_STAT(data)  ((data)->stat)  /* SMPC���ơ�����            */

/* ��� **********************************************************************/
#define PER_HOT_RES_ON  0x1                     /* �ۥåȥꥻ�å�ON          */
#define PER_HOT_RES_OFF 0x0                     /* �ۥåȥꥻ�å�OFF         */

/* ����ȥХå��¹Ծ���      */
#define PER_INT_OK      0x0                     /* ����                      */
#define PER_INT_ERR     0x1                     /* ���顼                    */

/******************************************************************************
 *
 * NAME:    PER_GET_TIM             -   �������
 *
 ******************************************************************************
 */
#define PER_GET_TIM()  (per_get_time_adr)

/******************************************************************************
 *
 * NAME:    PER_GET_SYS             -   �����ƥ�ǡ�������
 *
 ******************************************************************************
 */

#define PER_GET_SYS()   ((per_set_sys_flg == OFF) ? NULL : &per_get_sys_data)


#ifndef _PER_HOT_ENA
/******************************************************************************
 *
 * NAME:    PER_GET_HOT_RES         -   �ۥåȥꥻ�åȼ���
 *
 ******************************************************************************
 */
#define PER_GET_HOT_RES()  (per_hot_res)
#endif  /* _PER_HOT_ENA */

/*
 * TYPEDEFS
 */
typedef uint8_t   PerId;                  /* �ڥ�ե����ID                    */
typedef uint8_t   PerSize;                /* �ڥ�ե���륵����                */
typedef uint8_t   PerKind;                /* ����ȥХå�����          */
typedef uint16_t  PerNum;                 /* ɬ�ץڥ�ե�����        */
typedef uint8_t   PerMulId;               /* �ޥ�����å�ID                    */
typedef uint8_t   PerMulCon;              /* �ޥ�����åץ��ͥ�����            */

typedef struct  {     /* �ޥ�����å׾���             */
    PerMulId    id;       /* �ޥ�����å�ID           */
    PerMulCon   con;      /* �ޥ�����åץ��ͥ�����   */
}PerMulInfo;

typedef struct  {     /* �����ƥ����             */
    uint8_t   cc;           /* �����ȥ�å�������   */
    uint8_t   ac;           /* ���ꥢ������         */
    uint16_t  ss;           /* �����ƥॹ�ơ�����   */
    uint32_t  sm;           /* SMPC����           */
    uint8_t   stat;         /* SMPC���ơ�����       */
}PerGetSys;

#ifndef PER_VER_111
typedef void PerGetPer;                         /* �ڥ�ե����ǡ�������    */
#else
typedef volatile void PerGetPer;                /* �ڥ�ե����ǡ�������    */
#endif

/*
 * EXTERNAL VARIABLE DECLARATIONS
 */
#ifndef _PER_HOT_ENA
extern uint8_t per_hot_res;                       /* �ۥåȥꥻ�åȾ���        */
#endif  /* _PER_HOT_ENA */
extern uint8_t *per_get_time_adr;                 /* ����ǡ���������Ƭ���ɥ쥹*/
extern PerGetSys per_get_sys_data;              /* �����ƥ�ǡ���������Ǽ    */
#ifdef __GNUC__
extern volatile uint8_t per_set_sys_flg;          /* �����ƥ�ǡ�������ե饰  */
#else
extern uint8_t per_set_sys_flg;                   /* �����ƥ�ǡ�������ե饰  */
#endif
extern uint8_t per_time_out_flg;                  /* �����ॢ���ȥե饰        */

/*
 * EXTERNAL FUNCTION PROTOTYPE  DECLARATIONS
 */
uint32_t PER_LInit(PerKind, PerNum, PerSize, uint8_t *, uint8_t);
                                                /* ����ȥХå������        */
uint32_t PER_LGetPer(PerGetPer **, PerMulInfo **);/* �ڥ�ե����ǡ�������    */
#if 0   /* 1997-05-27 A.H(SOJ) ��������¸�ߤ��ʤ��ΤǺ�� */
PerGetSys * PER_GetSys(void);                   /* �����ƥ�ǡ�������        */
#endif
void PER_IntFunc(void);                         /* SMPC�����߽���          */
/********************************* per_smp.h *********************************/
/*
 * GLOBAL DEFINESMACROS DEFINES
 */

/**** ���ޥ�� ***************************************************************/
#define PER_SM_MSHON    0x00                    /* �ޥ���SH ON               */
#define PER_SM_SSHON    0x02                    /* ���졼��SH ON             */
#define PER_SM_SSHOFF   0x03                    /* ���졼��SH OFF            */
#define PER_SM_SNDON    0x06                    /* �������ON                */
#define PER_SM_SNDOFF   0x07                    /* �������OFF               */
#define PER_SM_CDON     0x08                    /* CD ON                     */
#define PER_SM_CDOFF    0x09                    /* CD OFF                    */
#define PER_SM_SYSRES   0x0d                    /* �����ƥ����Υꥻ�å�      */
#define PER_SM_NMIREQ   0x18                    /* NMI�ꥯ������             */
#define PER_SM_RESENA   0x19                    /* �ۥåȥꥻ�åȥ��͡��֥�  */
#define PER_SM_RESDIS   0x1a                    /* �ۥåȥꥻ�åȥǥ������֥�*/
#define PER_SM_SETSM    0x17                    /* SMPC��������            */
#define PER_SM_SETTIM   0x16                    /* ��������                  */

/*****************************************************************************/
/*****************************************************************************/
/**** ����ޥ��� ***********************************************************/
/*****************************************************************************/
/*****************************************************************************/

/******************************************************************************
 *
 * NAME:    PER_SMPC_WAIT() - SMPC�Ԥ����ֽ���
 *
 * PARAMETERS :
 *      �ʤ���
 *
 * DESCRIPTION:
 *      SMPC���������¹Ԥ��뤿��Ρ��Ԥ����֤��롣
 *
 * PRECONDITIONS:
 *      �ʤ���
 *
 * POSTCONDITIONS:
 *      �ʤ���
 *
 * CAVEATS:
 *      PER_SMPC_SET_IREG(),PER_SMPCCmdGo()������ɬ���¹Ԥ��뤳�ȡ�
 *
 ******************************************************************************
 */

#define PER_SMPC_WAIT()                        /* SMPC�Ԥ����ֽ���         */\
            do{                                                               \
                while((PER_PeekByte(PER_REG_SF) & PER_B_SF) == PER_B_SF);\
                                                /* SF��"1"�δַ����֤�      */\
                PER_PokeByte(PER_REG_SF, PER_B_SF);\
                                                /* SF<-"1"                  */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_GO_CMD() - SMPC���ޥ�ɼ¹�
 *
 * PARAMETERS :
 *      (1) Uint8   smpc_cmd    - <i>   SMPC�ޥ���̾
 *
 * DESCRIPTION:
 *      ��������SMPC�ޥ���̾��SMPC��COMREG�ʥ��ޥ�ɥ쥸�����ˤإ��åȤ��롣
 *  �ʥ��åȤ��뤳�Ȥˤ�ꡢSMPC�ϡ��ɤ�COMREG�˽񤫤줿���ޥ�ɤ�¹Ԥ����
 *
 * PRECONDITIONS:
 *      �ʤ���
 *
 * POSTCONDITIONS:
 *      �ʤ���
 *
 * CAVEATS:
 *      �ʤ���
 *
 ******************************************************************************
 */

#define PER_SMPC_GO_CMD(smpc_cmd)                 /* SMPC���ޥ�ɼ¹�         */\
            do{                                                               \
             PER_PokeByte(PER_REG_COMREG, smpc_cmd);/* COMREG�˥��ޥ��WRITE*/\
             while(PER_PeekByte(PER_REG_SF) & PER_B_SF);\
                                                    /* SF��"1"�Ǥʤ��ʤ�ޤ�*/\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_STATS_GET()  - SMPC���ơ���������
 *
 * PARAMETERS :
 *      (1) Uint8   stats_reg   - <o>   ���ޥ�ɼ¹ԥ��ơ�����
 *
 * DESCRIPTION:
 *      ���ޥ�ɼ¹Ը�Υ��ơ��������������
 *
 * PRECONDITIONS:
 *      �ʤ���
 *
 * POSTCONDITIONS:
 *      �ʤ���
 *
 * CAVEATS:
 *      �ʤ���
 *
 ******************************************************************************
 */

#define PER_SMPC_STATS_GET(stats_reg)           /* SMPC���ơ���������       */\
            do{                                                               \
                (stats_reg) = PER_PeekByte(PER_SR);                           \
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_SET_IREG()   - SMPC IREG���å�
 *
 * PARAMETERS :
 *      (1) Uint32  ireg_no     - <o>   IREG�ֹ�
 *      (2) Uint8   ireg_prm    - <i>   IREG���å���
 *
 * DESCRIPTION:
 *      ���ꤵ�줿IREG�ֹ楢�ɥ쥹��IREG�ͤ򥻥åȤ��롣
 *
 * PRECONDITIONS:
 *      �ʤ���
 *
 * POSTCONDITIONS:
 *      �ʤ���
 *
 * CAVEATS:
 *      �ʤ���
 *
 ******************************************************************************
 */

#define PER_SMPC_SET_IREG(ireg_no, ireg_prm)    /* SMPC IREG���å�          */\
            do{                                                               \
                PER_PokeByte((PER_REG_IREG + ((ireg_no) * 2)), (ireg_prm));   \
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_NO_IREG()    - IREG����̵�����ޥ�ɼ¹�
 *
 ******************************************************************************
 */

#define PER_SMPC_NO_IREG(com)\
            do{                                                               \
                PER_SMPC_WAIT();                /* SMPC�Ԥ����ֽ���         */\
                PER_SMPC_GO_CMD(com);           /* SMPC���ޥ�ɼ¹�         */\
            }while(FALSE)

/*****************************************************************************/
/*****************************************************************************/
/**** ����ޥ��� ***********************************************************/
/*****************************************************************************/
/*****************************************************************************/

/******************************************************************************
 *
 * NAME:    PER_SMPC_MSH_ON()     - �ޥ���SH ON
 *
 ******************************************************************************
 */

#define PER_SMPC_MSH_ON()\
            do{                                                               \
              PER_SMPC_NO_IREG(PER_SM_MSHON);   /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_SSH_ON()     - ���졼��SH ON
 *
 ******************************************************************************
 */

#define PER_SMPC_SSH_ON()\
            do{                                                               \
              PER_SMPC_NO_IREG(PER_SM_SSHON);   /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_SSH_OFF()    - ���졼��SH OFF
 *
 ******************************************************************************
 */

#define PER_SMPC_SSH_OFF()\
            do{                                                               \
              PER_SMPC_NO_IREG(PER_SM_SSHOFF);  /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_SND_ON()     - �������ON
 *
 ******************************************************************************
 */

#define PER_SMPC_SND_ON()\
            do{                                                               \
                PER_SMPC_NO_IREG(PER_SM_SNDON);   /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_SND_OFF()    - �������OFF
 *
 ******************************************************************************
 */

#define PER_SMPC_SND_OFF()\
            do{                                                               \
                PER_SMPC_NO_IREG(PER_SM_SNDOFF);  /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_CD_ON()      - CD ON
 *
 ******************************************************************************
 */

#define PER_SMPC_CD_ON()\
            do{                                                               \
                PER_SMPC_NO_IREG(PER_SM_CDON);    /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_CD_OFF()     - CD OFF
 *
 ******************************************************************************
 */

#define PER_SMPC_CD_OFF()\
            do{                                                               \
                PER_SMPC_NO_IREG(PER_SM_CDOFF);   /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_SYS_RES()    - �����ƥ����Υꥻ�å�
 *
 ******************************************************************************
 */

#define PER_SMPC_SYS_RES()\
            do{                                                               \
                PER_SMPC_NO_IREG(PER_SM_SYSRES);  /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_NMI_REQ()    - NMI�ꥯ������
 *
 ******************************************************************************
 */

#define PER_SMPC_NMI_REQ()\
            do{                                                               \
                PER_SMPC_NO_IREG(PER_SM_NMIREQ);  /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_RES_ENA()    - �ۥåȥꥻ�åȥ��͡��֥�
 *
 ******************************************************************************
 */

#define PER_SMPC_RES_ENA()\
            do{                                                               \
                PER_SMPC_NO_IREG(PER_SM_RESENA);  /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_RES_DIS()    - �ۥåȥꥻ�åȥǥ������֥�
 *
 ******************************************************************************
 */

#define PER_SMPC_RES_DIS()\
            do{                                                               \
                PER_SMPC_NO_IREG(PER_SM_RESDIS);  /* IREG����̵�����ޥ�ɼ¹� */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_SET_SM()     - SMPC��������
 *
 ******************************************************************************
 */

#define PER_SMPC_SET_SM(ireg)\
            do{                                                               \
                PER_SMPC_WAIT();                 /* SMPC�Ԥ����ֽ���         */\
                PER_SMPC_SET_IREG(0, (ireg) >> 24); /* IREG0���å�          */\
                PER_SMPC_SET_IREG(1, (ireg) >> 16); /* IREG0���å�          */\
                PER_SMPC_SET_IREG(2, (ireg) >>  8); /* IREG0���å�          */\
                PER_SMPC_SET_IREG(3, (ireg) >>  0); /* IREG0���å�          */\
                PER_SMPC_GO_CMD(PER_SM_SETSM);    /* SMPC���ޥ�ɼ¹�         */\
            }while(FALSE)

/******************************************************************************
 *
 * NAME:    PER_SMPC_SET_TIM()    - ��������
 *
 ******************************************************************************
 */

#define PER_SMPC_SET_TIM(ireg)\
            do{                                                               \
                PER_SMPC_WAIT();                 /* SMPC�Ԥ����ֽ���         */\
                PER_SMPC_SET_IREG(6, *(ireg));    /* IREG6���å�              */\
                PER_SMPC_SET_IREG(5, *((ireg) + 1));/* IREG5���å�              */\
                PER_SMPC_SET_IREG(4, *((ireg) + 2));/* IREG4���å�              */\
                PER_SMPC_SET_IREG(3, *((ireg) + 3));/* IREG3���å�              */\
                PER_SMPC_SET_IREG(2, *((ireg) + 4));/* IREG2���å�              */\
                PER_SMPC_SET_IREG(1, *((ireg) + 5));/* IREG1���å�              */\
                PER_SMPC_SET_IREG(0, *((ireg) + 6));/* IREG0���å�              */\
                PER_SMPC_GO_CMD(PER_SM_SETTIM);   /* SMPC���ޥ�ɼ¹�         */\
            }while(FALSE)

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

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */
/* �ӥåȰ��� ****************************************************************/
/* �ǥ�����ǥХ����ǡ�����	 */
#define	PER_DGT_R   (1 << 15)                   /* RIGHT                     */
#define	PER_DGT_L   (1 << 14)                   /* LEFT                      */
#define	PER_DGT_D   (1 << 13)                   /* DOWN                      */
#define	PER_DGT_U   (1 << 12)                   /* UP                        */
#define	PER_DGT_S   (1 << 11)                   /* START                     */
#define	PER_DGT_A   (1 << 10)                   /* A                         */
#define	PER_DGT_C   (1 << 9)                    /* C                         */
#define	PER_DGT_B   (1 << 8)                    /* B                         */
#define	PER_DGT_TR  (1 << 7)                    /* �ȥꥬ RIGHT              */
#define	PER_DGT_X   (1 << 6)                    /* X                         */
#define	PER_DGT_Y   (1 << 5)                    /* Y                         */
#define	PER_DGT_Z   (1 << 4)                    /* Z                         */
#define	PER_DGT_TL  (1 << 3)                    /* �ȥꥬ LEFT               */

/* �ݥ���ƥ��󥰥ǥХ����ǡ����� */
/***** �ǡ��� */
#define PER_PNT_R   (1 << 1)                    /* RIGHT                     */
#define PER_PNT_L   (1 << 0)                    /* LEFT                      */
#define PER_PNT_MID (1 << 2)                    /* MIDDLE                    */
#define PER_PNT_CNT (1 << 3)                    /* CENTER                    */
#define PER_PNT_S   (1 << 3)                    /* START                     */
#define PER_PNT_XO  (1 << 6)                    /* X�������Хե���           */
#define PER_PNT_YO  (1 << 7)                    /* Y�������Хե���           */

/* �����ܡ��ɥǥХ����ǡ����� */
/***** �ü쥭�� */
#define PER_KBD_CL  (1 << 6)                    /* Caps Lock                 */
#define PER_KBD_NL  (1 << 5)                    /* Num Lock                  */
#define PER_KBD_SL  (1 << 4)                    /* Scrool Lock               */
#define PER_KBD_MK  (1 << 3)                    /* Make                      */
#define PER_KBD_BR  (1 << 0)                    /* Break                     */

/********************************* persprt.h *********************************/
#if 0
    �ʲ� DISK Version 1994-11-11���ɲ�ʬ
    
    �������ƥࡿ�ڥ�ե����饤�֥����ѥ���ץ�ץ������ؤ��н�
        �����ƥࡿ�ڥ�ե����饤�֥�����Ѥ��Ƥ���ƥ饤�֥���
        ����ץ�ץ������Τ���˺���Υ����ƥࡿ�ڥ�ե����饤�֥��ˤϡ�
        �����ΥС������δؿ����������ޤ�Ƥ��ޤ���â�������δؿ����������
        ��4thSTEP�ǤϺ�����ޤ���
        �����ΥС������δؿ���������ϻ��Ѥ��ʤ��Ǥ���������
#endif
#if 0   /* ��С������(Ver1.02 ����)�� �ڥ�ե����ǡ�������ޥ��� */
/* �ᥬ�ɥ饤��3�ܥ���ѥåɥǡ����� */
#define	PER_M3BP_U  PER_DGT_U                   /* UP                        */
#define	PER_M3BP_D  PER_DGT_D                   /* DOWN                      */
#define	PER_M3BP_R  PER_DGT_R                   /* RIGHT                     */
#define	PER_M3BP_L  PER_DGT_L                   /* LEFT                      */
#define	PER_M3BP_A  PER_DGT_A                   /* A                         */
#define	PER_M3BP_B  PER_DGT_B                   /* B                         */
#define	PER_M3BP_C  PER_DGT_C                   /* C                         */
#define	PER_M3BP_S  PER_DGT_S                   /* START                     */

/* �ᥬ�ɥ饤��6�ܥ���ѥåɥǡ����� */
#define	PER_M6BP_U  PER_DGT_U                    /* UP                        */
#define	PER_M6BP_D  PER_DGT_D                    /* DOWN                      */
#define	PER_M6BP_R  PER_DGT_R                    /* RIGHT                     */
#define	PER_M6BP_L  PER_DGT_L                    /* LEFT                      */
#define	PER_M6BP_A  PER_DGT_A                    /* A                         */
#define	PER_M6BP_B  PER_DGT_B                    /* B                         */
#define	PER_M6BP_C  PER_DGT_C                    /* C                         */
#define	PER_M6BP_S  PER_DGT_S                    /* START                     */
#define	PER_M6BP_X  PER_DGT_X                    /* X                         */
#define	PER_M6BP_Y  PER_DGT_Y                    /* Y                         */
#define	PER_M6BP_Z  PER_DGT_Z                    /* Z                         */
#define	PER_M6BP_MD PER_DGT_TR                   /* MODE                      */
/* ���������ޥ��� ************************************************************/
/* �ǥ�����ǥХ����ǡ����� */
#define PER_DGT(data)   ((PerDgtInfo *)(data))

/* ���ʥ����ǥХ����ǡ�����  */
#define PER_ANL(data)   ((PerAnlInfo *)(data))

/* �ݥ���ƥ��󥰥ǥХ����ǡ����� */
#define PER_PNT(data)   ((PerPntInfo *)(data))

/* �����ܡ��ɥǥХ����ǡ����� */
#define PER_KBD(data)   ((PerKbdInfo *)(data))

/* �ᥬ�ɥ饤��3�ܥ���ѥåɥǡ����� */
#define PER_M3BP(data)  ((PerM3bpInfo *)(data))

/* �ᥬ�ɥ饤��6�ܥ���ѥåɥǡ����� */
#define PER_M6BP(data)  ((PerM6bpInfo *)(data))

/*
 * TYPEDEFS
 */
/* �ǥХ����ǡ����� */
typedef Uint16  PerDgtData;                     /* �ǥ�����ǥХ����ǡ�����  */
typedef Sint16  PerAnlN;                        /* ���ʥ���n�������ͥǡ����� */
typedef Uint16  PerPntBtnData;                  /* �ݥ���ƥ��󥰥ǥХ����ܥ���ǡ����� */
typedef Sint16  PerPntN;                        /* �ݥ���ƥ���n�������ͥǡ�����*/
typedef Uint8   PerKbdSkey;                     /* �����ܡ����ü쥭���ǡ�����   */
typedef Uint8   PerKbdKey;                      /* �����ܡ��ɥ����ǡ�����       */

typedef struct  {                               /* ���ʥ����ǥХ����ǡ�����  */
    PerDgtData  dgt;                            /* �ǥ�����ǥХ����ǡ�����  */
    PerAnlN     x;                              /* X��������(0��255)         */
    PerAnlN     y;                              /* Y��������(0��255)         */
    PerAnlN     z;                              /* Z��������(0��255)         */
}PerAnlData;

typedef struct  {                       /* �ݥ���ƥ��󥰥ǥХ����ǡ�����    */
    PerDgtData      dgt;                        /* �ǥ�����ǥХ����ǡ�����  */
    PerPntBtnData   data;                       /* �ǡ���                    */
    PerPntN     x;                              /* X����ư��(-128��127)      */
    PerPntN     y;                              /* Y����ư��(-128��127)      */
}PerPntData;

typedef struct  {                           /* �����ܡ��ɥǥХ����ǡ�����    */
    PerDgtData  dgt;                            /* �ǥ�����ǥХ����ǡ�����  */
    PerKbdSkey  skey;                           /* �ü쥭��                  */
    PerKbdKey   key;                            /* ����                      */
}PerKbdData;

typedef Uint8   PerM3bpData;            /* �ᥬ�ɥ饤��3�ܥ���ѥåɥǡ����� */
typedef Uint16  PerM6bpData;            /* �ᥬ�ɥ饤��6�ܥ���ѥåɥǡ����� */

/* �ڥ�ե����ǡ������� */
typedef struct  {                               /* �ǥ�����ǥХ���          */
    PerDgtData  data;                           /* ���ߥڥ�ե����ǡ���    */
    PerDgtData  push;                           /* ���ߥڥ�ե����ǡ���    */
    PerId       id;                             /* �ڥ�ե����ID            */
}PerDgtInfo;

typedef struct  {                               /* ���ʥ����ǥХ���          */
    PerAnlData  data;                           /* ���ߥڥ�ե����ǡ���    */
    PerAnlData  push;                           /* ���ߥڥ�ե����ǡ���    */
    PerId       id;                             /* �ڥ�ե����ID            */
}PerAnlInfo;

typedef struct  {                               /* �ݥ���ƥ��󥰥ǥХ���    */
    PerPntData  data;                           /* ���ߥڥ�ե����ǡ���    */
    PerPntData  push;                           /* ���ߥڥ�ե����ǡ���    */
    PerId       id;                             /* �ڥ�ե����ID            */
}PerPntInfo;

typedef struct  {                               /* �����ܡ��ɥǥХ���        */
    PerKbdData  data;                           /* ���ߥڥ�ե����ǡ���    */
    PerKbdData  push;                           /* ���ߥڥ�ե����ǡ���    */
    PerId       id;                             /* �ڥ�ե����ID            */
}PerKbdInfo;

typedef struct  {                               /* �ᥬ�ɥ饤��3�ܥ���ѥå� */
    PerM3bpData data;                           /* ���ߥڥ�ե����ǡ���    */
    PerM3bpData push;                           /* ���ߥڥ�ե����ǡ���    */
    PerId       id;                             /* �ڥ�ե����ID            */
}PerM3bpInfo;

typedef struct  {                               /* �ᥬ�ɥ饤��6�ܥ���ѥå� */
    PerM6bpData data;                           /* ���ߥڥ�ե����ǡ���    */
    PerM6bpData push;                           /* ���ߥڥ�ե����ǡ���    */
    PerId       id;                             /* �ڥ�ե����ID            */
}PerM6bpInfo;

/*
 * EXTERNAL VARIABLE DECLARATIONS
 */

/*
 * EXTERNAL FUNCTION PROTOTYPE  DECLARATIONS
 */
Uint32 PER_Init(PerKind , PerNum , PerId , PerSize , Uint32 *, Uint8 );
Uint32 PER_GetPer(PerGetPer **);

#endif  /* ��С������(Ver1.02 ����)�� �ڥ�ե����ǡ�������ޥ��� */
#endif  /* ifndef SEGA_PER_H */
/******************************* end of file *********************************/
