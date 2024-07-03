/*****************************************************************************
 *      �\�t�g�E�F�A���C�u����
 *
 *      Copyright (c) 1994,1995,1996 SEGA
 *
 * Library: �X�g���[���V�X�e��
 * Module : ���J�p�w�b�_�t�@�C��
 * File   : sega_stm.h
 * Date   : 1996-03-21
 * Version: 2.11
 * Auther : H.T
 *
 *****************************************************************************/
#if !defined(SEGA_STM_H)
#define     SEGA_STM_H

/*****************************************************************************
 *      �C���N���[�h�t�@�C��
 *****************************************************************************/
#include        "sega_gfs.h"

/* �o�[�W����   */
                            /*             1         2         3         4  */
                            /*    1234567890123456789012345678901234567890  */
                            /*    ++++++++++++++++++++++++++++++++          */
#if defined(USE_SGL)
    #define STM_VERSION_STR     ("STM_SGL Version 2.11 1996-03-21")
#else
    #define STM_VERSION_STR     ("STM_SBL Version 2.11 1996-03-21")
#endif
#define STM_VERSION_LEN 32

/*****************************************************************************
 *      �萔�}�N��
 *****************************************************************************/

#define STM_LONG_MAX    0x7fffffff

/* ���[�v�Đ����̓ǂݍ��ݎw��           */
enum StmLoopRead {
    STM_LOOP_READ       = TRUE,         /* ���[�v�Đ����ǂݍ��݂�����   */
    STM_LOOP_NOREAD     = FALSE,        /* ���[�v�Đ����ǂݍ��݂����Ȃ� */
    STM_LOOP_END
};

/* ���[�v�X�g���[����擪�X�g���[���ɐݒ肷��   */
#define STM_LOOP_DFL    ((StmHn)(0xffffffff))

/* �����񃋁[�v�̎w��                   */
#define STM_LOOP_ENDLESS        STM_LONG_MAX

/* �X�g���[���A�N�Z�X���(acstat)       */
enum StmAcStat {
    STM_EXEC_COMPLETED  = 0x100,        /* �A�N�Z�X�I��                 */
    STM_EXEC_PAUSE      = 0x101,        /* �A�N�Z�X�ꎞ��~��           */
    STM_EXEC_DOING      = 0x102,        /* �A�N�Z�X��                   */
    STM_EXEC_WAIT       = 0x103,        /* �]���Q�[�g�̊J���҂����     */
    STM_EXEC_TSKEND     = 0x104,        /* �������A��������             */
    STM_EXEC_END
};


/* �]�����j�b�g�T�C�Y           */
enum StmUnitSize {
    STM_UNIT_FORM1      = 2048,         /* MODE1,MODE2Form1�Z�N�^�T�C�Y */
    STM_UNIT_FORM2      = 2324,         /* MODE2Form2�Z�N�^�T�C�Y       */
    STM_UNIT_WORD       = 2,            /* ���[�h                       */
    STM_UNIT_BYTE       = 1,            /* �o�C�g                       */
    STM_UNIT_END
};


/* �X�g���[�����                       */
#define STM_KEY_NONE    (-1)            /* �e�X�g���[���L�[�I���𖳎�   */

/* �ǂݍ��݃Z�N�^�͈�                   */
enum StmFad {
    STM_FAD_CDTOP       = 150,          /* �f�B�X�N�擪FAD              */
    STM_FAD_CDEND       = STM_LONG_MAX  /* �f�B�X�N�I�����ǂޏꍇ�̃Z�N�^��*/
};


/* CD�o�b�t�@�Z�N�^�ʒu                 */
enum StmSctPos {
    STM_CDBUF_TOP       = 0,            /* �o�b�t�@���擪             */
    STM_CDBUF_END       = 0xffff        /* �S�Z�N�^��                   */
};

/* �I�[�v�����ǂݍ��݃U�u���[�h�ݒ�p   */
enum StmSctType {
    STM_SM_EOR          = CDC_SM_EOR,           /* ���R�[�h�Ō�̃Z�N�^ */
    STM_SM_VIDEO        = CDC_SM_VIDEO,         /* �r�f�I�Z�N�^         */
    STM_SM_AUDIO        = CDC_SM_AUDIO,         /* �I�[�f�B�I�Z�N�^     */
    STM_SM_DATA         = CDC_SM_DATA,          /* �f�[�^�Z�N�^         */
    STM_SM_TRIG         = CDC_SM_TRIG,          /* �g���K�[�r�b�g       */
    STM_SM_FORM         = CDC_SM_FORM,          /* �t�H�[���r�b�g       */
    STM_SM_RT           = CDC_SM_RT,            /* ���A���^�C���Z�N�^   */
    STM_SM_EOF          = CDC_SM_EOF,           /* �t�@�C���Ō�̃Z�N�^ */
    STM_SM_END
};


/* �ő�]���Z�N�^��                     */
#define STM_TR_ALL      STM_LONG_MAX


/* �]���Q�[�g���                       */
enum StmGate {
    STM_GATE_OPEN       = 0x300,                /* �J��                 */
    STM_GATE_CLOSE      = 0x301,                /* ��                 */
    STM_GATE_END
};


/* �]�����[�h                   */
enum StmTrMode {
    STM_TR_SCU          = GFS_TMODE_SCU,        /* SCU�ɂ��DMA         */
    STM_TR_SDMA0        = GFS_TMODE_SDMA0,      /* CPU�T�C�N���X�`�[��  */
    STM_TR_SDMA1        = GFS_TMODE_SDMA1,      /* CPU�T�C�N���X�`�[��  */
    STM_TR_CPU          = GFS_TMODE_CPU,        /* �\�t�g�E�G�A�]��     */
    STM_TR_END
};


/* �֐�ID                       */
enum StmFuncId {
    STM_INIT            = 0x300,        /* �X�g���[���V�X�e��������     */
    STM_OPENGRP         = 0x301,        /* �X�g���[���O���[�v�̃I�[�v�� */
    STM_CLOSEGRP        = 0x302,        /* �X�g���[���O���[�v�̃N���[�Y */
    STM_GETSTMNUM       = 0x303,        /* �X�g���[�����̎擾           */
    STM_GETSTMHNDL      = 0x304,        /* �X�g���[���n���h���̎擾     */
    STM_SETCDBUFFUNC    = 0x305,        /* �o�b�t�@�T�C�Y�Ď��֐��̓o�^ */
    STM_OPENFID         = 0x306,        /* ���ʎq�ɂ��I�[�v��         */
    STM_OPENFRANGE      = 0x307,        /* �Z�N�^�ʒu�ɂ��I�[�v��     */
    STM_OPENRESI        = 0x308,        /* �풓�X�g���[���̃I�[�v��     */
    STM_CLOSE           = 0x309,        /* �X�g���[���̃N���[�Y         */
    STM_SETKEY          = 0x310,        /* �X�g���[���L�[�̐ݒ�         */
    STM_GETINFO         = 0x311,        /* �X�g���[�����̎擾         */
    STM_SCTTOWORD       = 0x312,        /* �Z�N�^�����烏�[�h���ւ̕ϊ� */
    STM_WORDTOSCT       = 0x313,        /* ���[�h������Z�N�^���ւ̕ϊ� */
    STM_SETALIAS        = 0x314,        /* �G�C���A�X�̐ݒ�             */
    STM_UNSETALIAS      = 0x315,        /* �G�C���A�X�̉���             */
    STM_SETTRBUF        = 0x316,        /* �]���̈�̐ݒ�               */
    STM_SETTRFUNC       = 0x317,        /* �]���֐��̐ݒ�               */
    STM_STARTTRANS      = 0x318,        /* �]���֐��ɂ�����]���J�n     */
    STM_SETTRGATE       = 0x319,        /* �]���Q�[�g�̊J��             */
    STM_SETTRPARA       = 0x320,        /* �ő�]���Z�N�^���̐ݒ�       */
    STM_SETTRFAD        = 0x321,        /* �]���J�nFAD�̐ݒ�            */
    STM_SETTRMODE       = 0x322,        /* �]�����[�h�̐ݒ�             */
    STM_RESETTRBUF      = 0x323,        /* �]���̈�̃��Z�b�g           */
    STM_GETNUMCDBUF     = 0x324,        /* CD�o�b�t�@�L���Z�N�^���̎擾 */
    STM_GETSCTINFO      = 0x325,        /* �ǂݍ��݃Z�N�^���̎擾     */
    STM_GETLENTRBUF     = 0x326,        /* �]���̈�̃f�[�^���̎擾     */
    STM_ISTRBUFFULL     = 0x327,        /* �]���̈�t���̃`�F�b�N       */
    STM_ISTRANS         = 0x328,        /* �]�����̃`�F�b�N             */
    STM_SETEXECGRP      = 0x329,        /* �T�[�o���s�O���[�v�̎w��     */
    STM_EXECSERVER      = 0x330,        /* �T�[�o�̎��s                 */
    STM_MOVEPICKUP      = 0x331,        /* �s�b�N�A�b�v�̈ړ�           */
    STM_SETLOOP         = 0x332,        /* ���[�v�X�g���[���̎w��       */
    STM_GETLOOPCNT      = 0x333,        /* ���[�v�J�E���^�̎擾         */
    STM_GETEXECSTAT     = 0x334,        /* ���s��Ԃ̎擾               */
    STM_ISCOMPLETE      = 0x335,        /* ���s�I���̃`�F�b�N           */
    STM_EXECTRANS       = 0x336,        /* �X�g���[���f�[�^�̓]��       */
    STM_CONNECTCDBUF    = 0x337,        /* �i���CD�o�b�t�@���̐ڑ�   */
    STM_MOVECDBUF       = 0x338,        /* CD�o�b�t�@�f�[�^�̈ړ�       */
    STM_ERASECDBUF      = 0x339,        /* CD�o�b�t�@�f�[�^�̏���       */
    STM_SETERRFUNC      = 0x340,        /* �G���[�֐��̓o�^             */
    STM_GETERRSTAT      = 0x341,        /* �G���[��Ԃ̎擾             */
    STM_FUNC_END
};


/* �G���[�R�[�h         */
enum StmErrCode {
    STM_ERR_OK          = GFS_ERR_OK,           /* ����I��             */
    STM_ERR_CDRD        = GFS_ERR_CDRD,         /* CD���[�h�G���[       */
    STM_ERR_CDNODISC    = GFS_ERR_CDNODISC,     /* CD���Z�b�g���Ă��Ȃ� */
    STM_ERR_FID         = GFS_ERR_FID,          /* �t�@�C�����ʎq�s��   */
    STM_ERR_HNDL        = GFS_ERR_HNDL,         /* �n���h�����s��       */
    STM_ERR_NUM         = GFS_ERR_NUM,          /* �o�C�g���Ȃǂ���     */
    STM_ERR_PARA        = GFS_ERR_PARA,         /* �p�����[�^���s��     */
    STM_ERR_NOHNDL      = GFS_ERR_NOHNDL,       /* �n���h���ɋ󂫂��Ȃ� */
    STM_ERR_PUINUSE     = GFS_ERR_PUINUSE,      /* �s�b�N�A�b�v���쒆   */
    STM_ERR_TMOUT       = GFS_ERR_TMOUT,        /* �^�C���A�E�g         */
    STM_ERR_CDOPEN      = GFS_ERR_CDOPEN,       /* �g���C���J���Ă���   */
    STM_ERR_FATAL       = GFS_ERR_FATAL,        /* CD��FATAL���        */
    STM_ERR_END
};


/*****************************************************************************
 *      �f�[�^�^�̒�`
 *****************************************************************************/

/* �X�g���[���n���h��           */
typedef struct stmobj {
    void            *grp;           /* ��������X�g���[���O���[�v           */
    struct stmobj   *alias;         /* ���ۂɑ��삷��X�g���[��             */
    struct stmobj   *sfadlst;       /* �J�nFAD�������X�g                    */
    struct stmobj   *fltlst;        /* �i��̐ڑ������X�g                   */
    GfsHn           gfs;            /* �t�@�C���n���h��                     */
    GfsFlow         *flow;          /* �ǂݍ��݊Ǘ�                         */
    int32_t          (*trfunc)(void *obj, struct stmobj *stm, int32_t nsct);
                                    /* �]���֐�(StmTrFunc)                  */
    void            *obj;           /* �]���֐��̑�����   �@�@�@�@        */
    uint32_t          *trsrc;         /* �]�����A�h���X                       */
    int32_t          adlt;           /* �]�����A�h���X�ω���                 */
    int32_t          trfad;          /* �]���J�nFAD                          */
    uint8_t           atr;            /* �A�g���r���[�g                       */
    uint8_t           svrstat;
} StmObj;

typedef StmObj      *StmHn;


/* �]���֐�     */
typedef int32_t  (*StmTrFunc)(void *obj, StmHn stm, int32_t nsct);
#define STM_TR_NULLFUNC         ((StmTrFunc)0)

/* �G���[�֐�   */
typedef void    (*StmErrFunc)(void *obj, int32_t ec);
#define STM_ERR_NULLFUNC        ((StmErrFunc)0)

/* �o�b�t�@�Ď��֐�  */
typedef void    (*StmCdbufFunc)(void *obj);
#define STM_CDBUF_NULLFUNC      ((StmCdbufFunc)0)
#define STM_CDBUF_REST          NULL

/* �X�g���[���O���[�v           */
typedef struct {
    StmHn       sfadlst;        /* �ǂݍ��݊J�nFAD�ɂ�郊�X�g          */
    StmHn       fltlst;         /* �i��ڑ������X�g                     */
    StmHn       loopstm;        /* ���[�v�X�g���[��                     */
    StmHn       lststm;         /* �ŏI�X�g���[��                       */
    StmHn       nexttrns;       /* ���̓]���X�g���[��           */
    int32_t      fad;            /* �ǂݍ��ݎ��s���A�ꎞ��~����FAD      */
    int32_t      acstat;         /* �A�N�Z�X���                         */
                                /* (COMPLETED, PAUSE, DOING, WAIT)      */
    StmHn       chkstm;         /* �o�b�t�@�T�C�Y�Ď��X�g���[��         */
    int32_t      chksctnum;      /* �o�b�t�@�T�C�Y�Ď��֐��Ăт����Z�N�^�� */
    StmCdbufFunc  chkfunc;      /* �o�b�t�@�T�C�Y�Ď��֐�               */
    void        *chkobj;        /* �o�b�t�@�T�C�Y�Ď��I�u�W�F�N�g       */
    int32_t      loopmax;        /* ���[�v��                           */
    int32_t      loopcnt;        /* ���[�v�J�E���^                       */
    int32_t      puid;           /* �s�b�N�A�b�v���[�UID                 */
    uint16_t      atr;            /* �A�g���r���[�g                       */
    uint8_t       svrstat;
} StmGrp;

typedef StmGrp  *StmGrpHn;

/* �X�g���[���L�[               */
typedef struct {
    int16_t      fn;             /* �t�@�C���ԍ�                         */
    int16_t      cn;             /* �`���l���ԍ�                         */
    int16_t      smmsk;          /* �T�u���[�h�}�X�N�p�^�[��             */
    int16_t      smval;          /* �T�u���[�h��r�l                     */
    int16_t      cimsk;          /* �R�[�f�B���O���}�X�N�p�^�[��       */
    int16_t      cival;          /* �R�[�f�B���O����r�l               */
} StmKey;

#define STM_KEY_FN(stmkey)      ((stmkey)->fn)
#define STM_KEY_CN(stmkey)      ((stmkey)->cn)
#define STM_KEY_SMMSK(stmkey)   ((stmkey)->smmsk)
#define STM_KEY_SMVAL(stmkey)   ((stmkey)->smval)
#define STM_KEY_CIMSK(stmkey)   ((stmkey)->cimsk)
#define STM_KEY_CIVAL(stmkey)   ((stmkey)->cival)


/* �X�g���[���Đ��͈�           */
typedef struct {
    int32_t      sfad;           /* �Đ��J�nFAD                          */
    int32_t      fasnum;         /* �Đ��Z�N�^��                         */
} StmFrange;

#define STM_FRANGE_SFAD(frange)         ((frange)->sfad)
#define STM_FRANGE_FASNUM(frange)       ((frange)->fasnum)


/* �Z�N�^���                   */
typedef struct {
    int32_t      fad;            /* �t���[���A�h���X                     */
    int32_t      fn;             /* �t�@�C���ԍ�                         */
    int32_t      cn;             /* �`���l���ԍ�                         */
    uint8_t       sm;             /* �T�u���[�h                           */
    uint8_t       ci;             /* �R�[�f�B���O���                     */
} StmSct;

#define STM_SCT_FAD(sct)        ((sct)->fad)
#define STM_SCT_FN(sct)         ((sct)->fn)
#define STM_SCT_CN(sct)         ((sct)->cn)
#define STM_SCT_SM(sct)         ((sct)->sm)
#define STM_SCT_CI(sct)         ((sct)->ci)


/* �G���[�Ǘ�                   */
typedef struct {
    int32_t      code;           /* �G���[�R�[�h                         */
    int32_t      where;          /* �G���[�����ꏊ                       */
    StmErrFunc  func;           /* �G���[�������̌Ăт����֐�           */
    void        *obj;           /* �Ăт����֐��̑�����               */
} StmErrStat;

#define STM_ERR_CODE(err)       ((err)->code)
#define STM_ERR_WHERE(err)      ((err)->where)
#define STM_ERR_FUNC(err)       ((err)->func)
#define STM_ERR_OBJ(err)        ((err)->obj)


/* �X�g���[���V�X�e���Ǘ�       */
typedef struct {
    StmGrp      *stmgrptbl;     /* �X�g���[���O���[�v�����ė̈�         */
    int32_t      stmgrpmax;      /* �X�g���[���O���[�v�ő吔             */
    StmObj      *stmtbl;        /* �X�g���[�������ė̈�                 */
    int32_t      stmmax;         /* �X�g���[���ő吔                     */
    StmGrpHn    curgrp;         /* ���ݏ������X�g���[���O���[�v         */
    int32_t      sfad;           /* �Đ��J�nFAD                          */
    int32_t      efad;           /* �Đ��I��FAD                          */
    StmErrStat  err;            /* �G���[�Ǘ�                           */
    bool        svrexec;        /* STM_ExecServer������                 */
} StmMng;


/*****************************************************************************
 *      �����}�N��
 *****************************************************************************/

/* ��Ɨ̈�T�C�Y�̎擾                 */
#define STM_WORK_SIZE(grpmax, stmmax)   \
    (sizeof(StmMng) + sizeof(StmGrp) * (grpmax) + sizeof(StmObj) * (stmmax))

/*****************************************************************************
 *      �֐��̐錾
 *****************************************************************************/

/* �X�g���[���V�X�e��������             */
bool    STM_Init(int32_t grpmax, int32_t stmmax, void *work);

/* �X�g���[���V�X�e���̃��Z�b�g         */
void    STM_Reset(void);

/* �X�g���[���O���[�v�̃I�[�v��         */
StmGrpHn        STM_OpenGrp(void);

/* �X�g���[���O���[�v�̃N���[�Y         */
void    STM_CloseGrp(StmGrpHn grp);

/* �X�g���[�����̎擾                   */
int32_t  STM_GetStmNum(StmGrpHn grp);

/* �X�g���[���n���h���̎擾             */
StmHn   STM_GetStmHndl(StmGrpHn grp, int32_t nstm);

/* �o�b�t�@�T�C�Y�Ď��֐��̓o�^         */
void    STM_SetCdbufFunc(StmGrpHn grp, StmHn stm, int32_t nsct, 
                                       StmCdbufFunc func, void *obj);

/* ���ʎq�ɂ��t�@�C���̃I�[�v��       */
StmHn   STM_OpenFid(StmGrpHn grp, int32_t fid, StmKey *key, int32_t lpread);

/* �Z�N�^�ʒu�ɂ��t�@�C���̃I�[�v��   */
StmHn   STM_OpenFrange(StmGrpHn grp, StmFrange *frange, StmKey *key,
                                                           int32_t lpread);

/* �풓�X�g���[���̃I�[�v��             */
StmHn   STM_OpenResi(StmGrpHn grp, int32_t fid, StmKey *key, int32_t fad);

/* �X�g���[���̃N���[�Y                 */
void    STM_Close(StmHn stm);

/* �X�g���[���L�[�̐ݒ�                 */
void    STM_SetKey(StmHn stm, StmKey *stmkey);

/* �X�g���[�����̎擾                 */
StmGrpHn        STM_GetInfo(StmHn stm, int32_t *fid, StmFrange *range, 
                                                int32_t *bn, StmKey *stmkey);

/* �Z�N�^�����烏�[�h���ւ̕ϊ�         */
int32_t  STM_SctToWord(StmHn stm, int32_t nsct);

/* ���[�h������Z�N�^���ւ̕ϊ�         */
int32_t  STM_WordToSct(StmHn stm, int32_t nword);

/* �X�g���[���n���h���̃G�C���A�X�ݒ�   */
bool    STM_SetAlias(StmHn alias, StmHn stm);

/* �X�g���[���n���h���̃G�C���A�X����   */
void    STM_UnsetAlias(StmHn alias);

/* �]���̈�̐ݒ�                       */
void    STM_SetTrBuf(StmHn stm, void *buffer, int32_t nunit, int32_t unitsize);

/* �]���֐��̐ݒ�                       */
void    STM_SetTrFunc(StmHn stm, StmTrFunc func, void *obj);

/* �]���֐��ɂ�����]���J�n             */
uint32_t  *STM_StartTrans(StmHn stm, int32_t *dadr);

/* �]���Q�[�g�̊J��                     */
void    STM_SetTrGate(StmHn stm, int32_t gate);

/* �ő�]���Z�N�^���̐ݒ�               */
void    STM_SetTrPara(StmHn stm, int32_t tsct);

/* �]���J�nFAD�̐ݒ�                    */
void    STM_SetTrFad(StmHn stm, int32_t fad);

/* �]�����[�h�̐ݒ�                     */
void    STM_SetTrMode(StmHn stm, int32_t tmode);

/* �]���̈�̃��Z�b�g                   */
void    STM_ResetTrBuf(StmHn stm);

/* �b�c�o�b�t�@�L���Z�N�^���̎擾       */
int32_t  STM_GetNumCdbuf(StmHn stm);

/* �ǂݍ��݃Z�N�^���̎擾             */
bool    STM_GetSctInfo(StmHn stm, int32_t spos, StmSct *sinfo);

/* �]���̈�̃f�[�^��(���[�h)�̎擾     */
int32_t  STM_GetLenTrBuf(StmHn stm);

/* �]���̈�t���̃`�F�b�N               */
bool    STM_IsTrBufFull(StmHn stm);

/* �X�g���[���f�[�^�]�����̃`�F�b�N     */
bool    STM_IsTrans(StmHn stm);

/* �T�[�o���s�O���[�v�̎w��(�������A)   */
bool    STM_NwSetExecGrp(StmGrpHn grp);

/* �T�[�o���s�O���[�v�̎w��(�������A)   */
bool    STM_SetExecGrp(StmGrpHn grp);

/* �T�[�o�̎��s                         */
int32_t  STM_ExecServer(void);

/* �s�b�N�A�b�v�̈ړ�                   */
void    STM_MovePickup(StmHn stm, int32_t off);

/* ���[�h�G���[�������̃��J�o��         */
int32_t  STM_Recover(void);

/* ���[�v�X�g���[���̎w��               */
bool    STM_SetLoop(StmGrpHn grp, StmHn loopstm, int32_t loopmax);

/* ���[�v�J�E���^�̎擾                 */
int32_t  STM_GetLoopCnt(StmGrpHn grp);

/* ���s��Ԃ̎擾                       */
int32_t  STM_GetExecStat(StmGrpHn grp, int32_t *fad);

/* ���s�I���̃`�F�b�N                   */
bool    STM_IsComplete(StmHn stm);

/* �X�g���[���f�[�^�̓]��               */
bool    STM_ExecTrans(StmHn stm);

/* �X�g���[���̐ڑ�                     */
void    STM_ConnectCdbuf(StmHn keystm, StmHn bufstm);

/* CD�o�b�t�@�f�[�^�̈ړ�               */
void    STM_MoveCdbuf(StmHn src, int32_t spos, int32_t snum, StmHn dst);

/* CD�o�b�t�@�f�[�^�̏���               */
void    STM_EraseCdbuf(StmHn stm, int32_t spos, int32_t snum);

/* �G���[�������̌Ăт����֐��̓o�^     */
void    STM_SetErrFunc(StmErrFunc func, void *obj);

/* �G���[��Ԃ̎擾                     */
void    STM_GetErrStat(StmErrStat *stat);

#endif  /* SEGA_STM_H   */

/* end of sega_stm.h            */
