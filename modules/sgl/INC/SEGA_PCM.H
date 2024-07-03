/******************************************************************************
 *	�\�t�g�E�F�A���C�u����
 *
 *	Copyright (c) 1994,1995 SEGA
 *
 * Library	:�o�b�l�E�`�c�o�b�l�Đ����C�u����
 * Module 	:���C�u�����w�b�_
 * File		:sega_pcm.h
 * Date		:1995-03-31
 * Version	:1.16
 * Auther	:Y.T
 *
 ****************************************************************************/
#if !defined(SEGA_PCM_H)
#define SEGA_PCM_H

/* from Ver1.10, adpcm function support */
#define ADPCM_FUNCTION

/*-------------------- �s�C���N���[�h�t�@�C���t --------------------*/
#include "sega_xpt.h"
#include "sega_gfs.h"
#include "sega_stm.h"

/*------------------------- �s�}�N���萔�t -------------------------*/

/* �o�[�W���� */
#define PCM_VERSION				("PCM Version 1.16 1995-03-31")
							/*	           1         2         3         4	*/
							/*	  1234567890123456789012345678901234567890	*/
							/*	  ++++++++++++++++++++++++++++++++			*/
#define PCM_VERSION_LEN			(32)

/* �n���h������ */
#define PCM_HN_MAX				(32)

/* 1024, 2048, 4096,... */
#define PCM_SIZE_1K				(1024)
#define PCM_SIZE_2K				(2 * PCM_SIZE_1K)
#define PCM_SIZE_3K				(3 * PCM_SIZE_1K)
#define PCM_SIZE_4K				(4 * PCM_SIZE_1K)
#define PCM_SIZE_8K				(8 * PCM_SIZE_1K)

/* �t���O */
typedef enum {
    PCM_OFF = 0,
    PCM_ON  = 1
} PcmFlag;

/* �G���[�R�[�h	*/
typedef enum {
	PCM_ERR_OK					= 0x00000000,/* �G���[�͔������Ă��Ȃ�		*/

	/* PCM_Create �̃G���[ */
	PCM_ERR_OUT_OF_HANDLE		= 0x00000101,/* �n���h������؂� 			*/
	PCM_ERR_NO_INIT				= 0x00000103,/* �������֐�����ق���ĂȂ� 	*/

	/* PCM_Destroy �̃G���[ */
	PCM_ERR_INVALID_HN			= 0x00000201,/* �s���ȃn���h���ŏ���		*/
	PCM_ERR_ILL_CREATE_MODE		= 0x00000202,/* Create ���̃��[�h�ƈႤ		*/

	/* pcm_HeaderProcess �̃G���[ */
	PCM_ERR_TOO_LARGE_HEADER	= 0x00000302,/* �w�b�_�����傷����			*/
											/* (�o�b�t�@�T�C�Y������������)	*/
	PCM_ERR_HEADER_DATA			= 0x00000303,/* �w�b�_�f�[�^�G���[ 			*/
	PCM_ERR_TOO_SMALL_PCMBUF	= 0x00000304,/* PCM�ޯ̧���ނ�����������	*/
	PCM_ERR_ILL_SIZE_PCMBUF		= 0x00000305,/* PCM�ޯ̧���ޕs�� 4096*2�`16	*/
	PCM_ERR_AFI_NO_COMMON		= 0x00000306,/* CommonChunk ������ 			*/
	PCM_ERR_AFI_COMMPRESS		= 0x00000307,/* ���Ή����k�^�C�v			*/
	PCM_ERR_NOT_DECLARE_ADPCM	= 0x00000308,/* ADPCM�g�p�錾���Ȃ�			*/

	/*  */
	PCM_ERR_ILLEGAL_PARA		= 0x00000503,/* �������̎w�肪�Ԉ���Ă��� 	*/
	PCM_ERR_ILLEGAL_HANDLE		= 0x00000504,/* �n���h�����ُ� 				*/

	/* �A���Đ��Ɋւ���G���[ */
	PCM_ERR_NEXT_HN_STATUS		= 0x00000601,/* �A���Đ��n���h���̏�Ԉُ� 	*/
	PCM_ERR_NEXT_HN_AUDIO		= 0x00000602,/* �I�[�f�B�I�̏�������v���Ȃ� */
	PCM_ERR_CHANGE_NO_ENTRY		= 0x00000605,/* �G���g�����Ȃ���ԂŃ`�F���W */

	/* �|�[�Y�Ɋւ���G���[ */
	PCM_ERR_PAUSE_STATUS		= 0x00000701,/* PCM_STAT_PLAY_TIME, �܂��́A
											 * PCM_STAT_PLAY_PAUSE �ȊO��
											 * ��ԂŃ|�[�Y�֐����R�[�������B
											 */
	PCM_ERR_PAUSE_WORK_NULL		= 0x00000702,/* �|�[�Y�����p���[�N�ُ�		*/
	PCM_ERR_PAUSE_WORK_SIZE		= 0x00000703,/* �|�[�Y�����p���[�N�ُ�		*/
	PCM_ERR_PAUSE_WORK_SET		= 0x00000704,/* �|�[�Y�����p���[�N�w�肪�s��*/

	/* DMA�]���Ɋւ���G���[ */
	PCM_ERR_DMA_MODE			= 0x00000801,/* ���Ή��̓]������ 			*/
	PCM_ERR_DMA_CPU_PCM			= 0x00000802,/* �c�l�`���ُ�I�� 			*/

	/* �t�@�C���ǂݍ��݃G���[ */
	PCM_ERR_GFS_READ			= 0x00000901,/* �f�e�r�̓ǂݍ��݂Ŏ��s 		*/

	/* �����O�o�b�t�@�ւ̃f�[�^�����Ɋւ���G���[ */
	PCM_ERR_RING_SUPPLY			= 0x00000a01,/* �Đ�������ɋ�����������    */
											 /* �i�������Ԃɍ��킷���f�����j*/

	PCM_ERR_END
} PcmErrCode;

/* �Đ��X�e�[�^�X */
typedef enum {
	PCM_STAT_PLAY_ERR_STOP		= -1,		/* �ُ��~					*/
	PCM_STAT_PLAY_CREATE		= 0,		/* �������					*/
	PCM_STAT_PLAY_PAUSE			= 1,		/* �ꎞ��~		 			*/
	PCM_STAT_PLAY_START			= 2,		/* �J�n						*/
	PCM_STAT_PLAY_HEADER		= 3,		/* �w�b�_�ݒ�@				*/
	PCM_STAT_PLAY_TIME			= 4,		/* �Đ�(�^�C�}�X�^�[�g)		*/
	PCM_STAT_PLAY_END			= 5			/* �Đ����� 				*/
} PcmPlayStatus;

/* �t�@�C���^�C�v */
typedef enum {
	PCM_FILE_TYPE_NO_HEADER		= 1,	/* �w�b�_�Ȃ��B
										 * (PCMײ���؂����Ή���̧�ٌ`��
										 *  ����؍Đ�������ꍇ���܂�)
										 */
	PCM_FILE_TYPE_AIFF			= 2,	/* AIFF							*/
	PCM_FILE_TYPE_AIFC			= 3 	/* AIFC							*/
} PcmFileType;
/* [����] AIFC�͎g�p�ł��܂���B*/

/* �f�[�^�^�C�v */
typedef enum {
	PCM_DATA_TYPE_LRLRLR		= 1,	/* �P�T���v���Â��E���E����	*/
	PCM_DATA_TYPE_RLRLRL		= 2,	/* �P�T���v���ÂE���E������	*/
	PCM_DATA_TYPE_LLLRRR		= 3,	/* �u���b�N�P�ʂō����E�E����	*/
	PCM_DATA_TYPE_RRRLLL		= 4, 	/* �u���b�N�P�ʂŉE�E��������	*/
	PCM_DATA_TYPE_ADPCM_SG		= 5, 	/* ADPCM,����޸�ٰ�߂̘A���f�[�^*/
	PCM_DATA_TYPE_ADPCM_SCT		= 6 	/* ADPCM,1���18����޸�ٰ��		*/
} PcmDataType;

/* �ꎞ��~�̐���R�}���h */
typedef enum {
	PCM_PAUSE_ON_AT_ONCE,				/* �����ꎞ��~ 			*/
	PCM_PAUSE_OFF						/* �ꎞ��~���� 			*/
} PcmPauseCmd;

/* �����؂�ւ��\���`�F�b�N�l */
typedef enum {
	PCM_CHANGE_OK_AT_ONCE		= 0,	/* �����؂�ւ��\�B			*/
	PCM_CHANGE_NO_DATA			= 1,	/* �f�[�^�����s���B				*/
	PCM_CHANGE_NO_ENTRY			= 2,	/* �G���g���������B				*/
	PCM_CHANGE_TOO_LARGE_HEADER	= 3		/* �w�b�_�����傷����B			*/
										/* (�o�b�t�@�T�C�Y������������)	*/
} PcmChangeStatus;

/* �f�[�^�̓]������ */
typedef enum {
	PCM_TRMODE_CPU = 0,					/* �\�t�g�E�F�A�]�� 			*/
	PCM_TRMODE_SDMA,					/* �c�l�`�T�C�N���X�`�[�� 		*/
	PCM_TRMODE_SCU						/* �r�b�t�̂c�l�`				*/
} PcmTrMode;

/* [����] 
	�����O�o�b�t�@���o�b�l�o�b�t�@�̓]���́A�\�t�g�E�F�A�]���ɌŒ�ł��B
*/

/*------------------------- �s�f�t�H���g�l�t -------------------------*/

/* �P��̃^�X�N�ŏ�������ʂ̏�� [sample/1ch] */
#define PCM_DEFAULT_SAMPLE_1TASK		(PCM_SIZE_1K)

/* �Đ��J�n�g���K�T�C�Y[byte] */
#define PCM_DEFAULT_SIZE_START_TRG		(0)

/* �Đ��J�n�g���K�T���v��[sample/1ch] */
#define PCM_DEFAULT_SAMPLE_START_TRG	(PCM_SIZE_4K)

/* �Đ���~�g���K�T���v��[sample/1ch] */
#define PCM_DEFAULT_SAMPLE_STOP_TRG		(PCM_SIZE_2K)


/*------------------------- �s�����}�N���t -------------------------*/

/* �S�a�x�s�d�����R�[�h�� uint32_t �^�ŕ\������B
 *   ('cvid'�ȂǂƏ�����ܰ�ݸނ��o�Ă��܂��̂ŁA�����������邽�߂�)
 */
#define PCM_4BYTE_CHAR(a,b,c,d)					\
	(	((uint32_t)((a) & 0x000000FF) << 24) | 	\
		((uint32_t)((b) & 0x000000FF) << 16) | 	\
		((uint32_t)((c) & 0x000000FF) <<  8) | 	\
		 (uint32_t)((d) & 0x000000FF)				\
	)

/* SEGA_PCM conpatible */
#define PCM_PARA_WORK_ADDR(para)	PCM_PARA_WORK(para)
#define PCM_PARA_BUF_ADDR(para)		PCM_PARA_RING_ADDR(para)
#define PCM_PARA_BUF_SIZE(para)		PCM_PARA_RING_SIZE(para)
#define PCM_CreateMemMovie			PCM_CreateMemHandle
#define PCM_DestroyMemMovie			PCM_DestroyMemHandle

/*-------------------- �s�\���̃A�N�Z�X�}�N���t --------------------*/

/* �쐬�p�����[�^�ւ̃A�N�Z�X�}�N�� */
#define PCM_PARA_WORK(para)			((para)->work)
#define PCM_PARA_RING_ADDR(para)	((para)->ring_addr)
#define PCM_PARA_RING_SIZE(para)	((para)->ring_size)
#define PCM_PARA_PCM_ADDR(para)		((para)->pcm_addr)
#define PCM_PARA_PCM_SIZE(para)		((para)->pcm_size)

/* �Đ����̐ݒ�p�����[�^�ւ̃A�N�Z�X�}�N�� */
#define PCM_INFO_FILE_TYPE(info)		((info)->file_type)
#define PCM_INFO_DATA_TYPE(info)		((info)->data_type)
#define PCM_INFO_FILE_SIZE(info)		((info)->file_size)
#define PCM_INFO_CHANNEL(info)			((info)->channel)
#define PCM_INFO_SAMPLING_BIT(info)		((info)->sampling_bit)
#define PCM_INFO_SAMPLING_RATE(info)	((info)->sampling_rate)
#define PCM_INFO_SAMPLE_FILE(info)		((info)->sample_file)
#define PCM_INFO_COMPRESSION_TYPE(info)	((info)->compression_type)

/* �n���h��(���[�N)�̃A�N�Z�X */
#define PCM_HN_START_TRG_SIZE(hn)	(((*(PcmWork **)hn))->para.start_trg_size)
#define PCM_HN_START_TRG_SAMPLE(hn)	(((*(PcmWork **)hn))->para.start_trg_sample)
#define PCM_HN_STOP_TRG_SAMPLE(hn)	(((*(PcmWork **)hn))->para.stop_trg_sample)
#define PCM_HN_CNT_LOOP(hn)			(((*(PcmWork **)hn))->status.cnt_loop)


/*-------------------------- �s�f�[�^�^�t --------------------------*/
/* �n���h�� */
typedef void *PcmHn;

/* �G���[�o�^�֐� */
typedef void (*PcmErrFunc)(void *obj, int32_t err_code);

typedef struct {
	int8_t		command;
	int8_t		resered;
	uint8_t		pad[14];				/* P1-14	*/
} PcmCmdBlk;

typedef struct {
	int8_t		command;
	int8_t		resered;
	int8_t		channel_bit_stm_no;		/* P1 		*/
	int8_t		level_pan;				/* P2 		*/
	int16_t		buf_addr;				/* P3-4 	*/
	int16_t		buf_size;				/* P5-6 	*/
	int16_t		pitch_word;				/* P7-8 	*/
	uint8_t		effect_r_mono;			/* P9 		*/
	uint8_t		effect_l;				/* P10 		*/
	uint8_t		pad[4];					/* P11-14 	*/
} PcmStartPcm;

typedef struct {
	int8_t		command;
	int8_t		resered;
	int8_t		stm_no;					/* P1 		*/
	uint8_t		pad[13];				/* P2-14	*/
} PcmStopPcm;

typedef struct {
	int8_t		command;
	int8_t		resered;
	int8_t		stm_no;					/* P1 		*/
	int8_t		level_pan;				/* P2 		*/
	int16_t		pitch_word;				/* P3-4 	*/
	uint8_t		effect_r_mono;			/* P5 		*/
	uint8_t		effect_l;				/* P6 		*/
	uint8_t		pad[8];					/* P7-14 	*/
} PcmChangePcmPara;

typedef struct {
	int8_t		command;
	int8_t		resered;
	int8_t		area_no;				/* P1 		*/
	uint8_t		pad[13];				/* P2-14 	*/
} PcmChangeMap;

typedef struct {
	int8_t		command;
	int8_t		resered;
	int8_t		total_volume;			/* P1 		*/
	uint8_t		pad[13];				/* P2-14 	*/
} PcmTotalVolume;

typedef struct {
	int8_t		command;
	int8_t		resered;
	int8_t		effect_bank_no;			/* P1 		*/
	uint8_t		pad[13];				/* P2-14 	*/
} PcmChangeEffect;

typedef struct {
	int8_t		command;
	int8_t		resered;
	int8_t		neiro_bank_no;			/* P1 		*/
	int8_t		mixer_no;				/* P2 		*/
	uint8_t		pad[12];				/* P3-14 	*/
} PcmChangeMixer;

typedef struct {
	int8_t		command;
	int8_t		resered;
	int8_t		effect_out_select;		/* P1 		*/
	int8_t		effect_level_pan;		/* P2 		*/
	uint8_t		pad[12];				/* P3-14 	*/
} PcmChangeMixerPara;

/* �Ō�ɔ��s�����e�R�}���h�̓��e */
typedef struct {
	PcmStartPcm			start_pcm;
	PcmStopPcm			stop_pcm;
	PcmChangePcmPara	change_pcm_para;

/*	PcmChangeMap		change_map;
	PcmTotalVolume		total_volume;
	PcmChangeEffect		change_effect;
	PcmChangeMixer		change_mixer;
	PcmChangeMixerPara	change_mixer_para;
*/
} PcmCommand;

/* �Đ���� */
typedef struct {
	PcmFileType		file_type;			/* �t�@�C���^�C�v 					*/
	PcmDataType		data_type;			/* �f�[�^�^�C�v 					*/
	int32_t			file_size;			/* �t�@�C���T�C�Y[byte]
										 * ����ȏ���ݸ��ޯ̧�ւ̋���������
										 * ���A���̕����͏������Ȃ��B
										 */
	int32_t			channel;			/* �`���l���� 						*/
	int32_t			sampling_bit;		/* �T���v�����O�r�b�g�� 			*/
	int32_t			sampling_rate;		/* �T���v�����O���[�g[Hz] 			*/
	int32_t			sample_file;		/* �t�@�C���̃T���v����[sample/1ch]	*/
	int32_t			compression_type;	/* ���k�^�C�v 						*/
} PcmInfo;

/* PCM�]����� */
typedef struct {
	int8_t		*dst1;				/* �]����A�h���X1 						*/
	int8_t		*dst2;				/* �]����A�h���X2 						*/
	int8_t		*src;				/* �]�����A�h���X 						*/
	int32_t		size;				/* �]���o�C�g�� 						*/
} PcmCopyTbl;

/* �e�퐧���� */
typedef struct {
	/* �S�̐����� */
	int32_t			stat_start;		/* 'STAT' �X�e�[�^�X�̊J�n 				*/
	PcmPlayStatus	play;			/* �Đ���� 							*/
	PcmFlag			ignore_header;	/* 1:�w�b�_��񖳎� 					*/
	PcmFlag			need_ci;		/* 1:�T�u�w�b�_����ci��ǂ� 			*/
	int32_t			cnt_loop;		/* ���[�v��(�J�E���g�_�E��) 			*/

	/* �Đ���� */
	int32_t		info_start;			/* 'INFO' �Đ����̊J�n 				*/
	PcmInfo		info;				/* �Đ���� 							*/

	/* �����O�o�b�t�@������ */
	int32_t		ring_start;			/* 'RING' �����O�o�b�t�@������̊J�n 	*/
	int32_t		ring_write_offset;	/* �����O�o�b�t�@�������݈ʒu[byte] 	*/
									/*	(̧�ِ擪����̃I�t�Z�b�g)			*/
	int32_t		ring_read_offset;	/* �����O�o�b�t�@�ǂݍ��݃I�t�Z�b�g 	*/
									/*	(̧�ِ擪����̃I�t�Z�b�g)			*/
	int8_t		*ring_end_addr;		/* �����O�o�b�t�@�I���A�h���X 			*/
	int8_t		*ring_write_addr;	/* �����O�o�b�t�@�������݃A�h���X 		*/
	int8_t		*ring_read_addr;	/* �����O�o�b�t�@�ǂݎ��A�h���X 		*/
	int32_t		media_offset;		/* �t�@�C���擪����f�[�^�܂ł̵̾�� 	*/

	/* PCM�Đ���� */
	int32_t		pcm_start;			/* 'PCM ' PCM�Đ����̊J�n 			*/
	int8_t		*pcm2_addr;			/* PCM��Q����كo�b�t�@�J�n�A�h���X 	*/
	int32_t		pcm_bsize;			/* �������؂�PCM�o�b�t�@�T�C�Y[byte/1ch]*/
	int32_t		pcm_write_offset;	/* PCM�o�b�t�@�������݈ʒu[byte]		*/
									/*  (PCM�o�b�t�@�̐擪����̃I�t�Z�b�g)	*/
	int32_t		cnt_4ksample;		/* PCM play address 4k����ْP�ʍX�V���� */
	int32_t		sample_write_file;	/* ���t�@�C���̋����T���v����[sample/1ch]*/
									/* (���t�@�C���̌��݂܂ł̑�������)		*/
	int32_t		sample_write;		/* �����T���v����[sample/1ch]			*/
									/* (�擪̨�т���̌��݂܂ł̑�������)	*/
	int32_t		sample_film_start;	/* �t�B�����J�n���T���v����[sample/1ch]	*/
									/* (�擪̨�т��猻��̨�ъJ�n���_�܂ł�)	*/
	int32_t		sample_pause;		/* �|�[�Y���T���v����[sample/1ch]		*/
									/* (�擪̨�т���ŋ߂̃|�[�Y���_�܂ł�)	*/
	int32_t		count_start;		/* �b�o�t�N���b�N�^�C�}�J�n���� 		*/
									/* 						[CPU�ۯ�>>15] 	*/
	int32_t		clock_scale;		/* �b�o�t�N���b�N�^�C�}�̃X�P�[�� 		*/
	PcmFlag		fill_silence;		/* �����ݒ�ς݃t���O					*/
	int32_t		*pwork_addr;		/* �|�[�Y�����p���[�N					*/
	int32_t		pwork_size;			/* �|�[�Y�����p���[�N�T�C�Y[byte] 		*/
									/*  (�|�[�Y�@�\���g�p����ꍇ�ɕK�v)	*/
	int32_t		cnt_sg;				/* �T�E���h�O���[�v�J�E���^				*/
	int32_t		onetask_sample;		/* �P��̃^�X�N�ŏ��������[sample/1ch]	*/
	int32_t		onetask_size;		/* �P��̃^�X�N�ŏ��������[byte/1ch]	*/
	int32_t		onetask_sg;			/* �P��̃^�X�N�ŏ��������[sg/1ch]		*/

	/* �T�E���h�h���C�o�֘A */
	int32_t		sddrv_start;		/* 'SDRV' �T�E���h�h���C�o�֘A�̊J�n 	*/
	PcmCommand	command;			/* �Ō�ɔ��s�����e�R�}���h�̓��e 		*/

	/* PCM�f�[�^�]���Ɋւ����� */
	int32_t		copy_start;			/* 'COPY' PCM�f�[�^�]���֘A�̊J�n 		*/
	PcmTrMode	copy_mode_pcm;		/* �f�[�^�̓]������ 					*/
	PcmFlag		dma_hold;			/* 1:���̃n���h����DMA������ێ����Ă���*/
	uint32_t		pcm_renew_size;		/* DMA�I����pcm�ޯ̧���������߲���X�V����*/
	uint32_t		buf_renew_size;		/* DMA�I�����ޯ̧�ǂݎ���߲���X�V���� 	*/
	int32_t		copy_idx;			/* PCM�]�����e�[�u���̃C���f�b�N�X 	*/
	PcmCopyTbl 	copy_tbl[4];		/* PCM�]�����e�[�u�� 					*/

	/* ���ԊǗ���� */
	int32_t		vbl_start;			/* 'VBL ' ���ԊǗ����̊J�n 			*/
	int32_t		vbl_film_start;		/* �t�B�����J�n����[vbl] 				*/
	int32_t		vbl_pause;			/* �|�[�Y�J�n����[vbl] 					*/
	int32_t		vbl_pcm_start;		/* PCM�J�n����[vbl]
									 * �ŏ��̍��� cnt_4ksample ������Ķ���
									 * ���߂��Ȃ��悤�ɁB
									 */

	/* ADPCM�؂蕪���p�֐��|�C���^ */
	void (*audio_process_fp)(PcmHn hn);
									/* �P�I�[�f�B�I�T���v������
									 * pcm_AudioProcess      �܂��́A
									 * pcm_AudioProcessAdpcm ���o�^
									 * �����
									 */

	/* �G���[��� */
	uint32_t		err_start;			/* 'ERR ' �G���[���̊J�n 				*/
	uint32_t		cnt_load_miss;		/* PCM���[�h�~�X�J�E���^ 				*/
	uint32_t		cnt_task_call;		/* �^�X�N�R�[���J�E���^ 				*/
	uint32_t		cnt_over_task_call;	/* �ߏ�^�X�N�R�[���J�E���^ 			*/
	uint32_t		cnt_buf_empty;		/* �o�b�t�@��^�X�N�J�E���^ 			*/
	uint32_t		max_late_time;		/* �ő�x�ꎞ��[sample] 				*/
	uint32_t		max_late_sample;	/* �ő�x�ꎞ��[sample] 				*/
	int32_t		cnt_fill_silence;	/* �����ݒ�J�E���^						*/
} PcmStatus;

/* �t�@�C���A�N�Z�X�֌W */
typedef struct {
	GfsHn		gfs;			/* �t�@�C���n���h�� */
	int32_t		load_sect;		/* �]���ő�Z�N�^�� */
	bool		called_cdread;	/* GFS_NwCdRead ���R�[�������� TRUE */
	bool		exec_one_state;	/* ExecOne ���s���Ȃ� TRUE */
	int32_t		exec_load_size;	/* ExecOne �œǂݍ��ރo�C�g�� */
	int32_t		now_load_size;	/* ExecOne �œǂݍ���ł���o�C�g�� */
	int32_t		load_total_sect;/* �ǂݍ��ݑ��Z�N�^�� */
	int32_t		file_sect;		/* �t�@�C���̃Z�N�^�� */
	PcmTrMode	tr_mode;		/* �]�����[�h */
} PcmFileGfsPara;

typedef struct {
	StmHn		stm;			/* �X�g���[���n���h�� */
	int32_t		load_sect;		/* �]���ő�Z�N�^�� */
	int32_t		old_cd_buf_num;	/* �O��̂b�c�o�b�t�@�̐� */
	bool		dma_state;		/* �c�l�`�]�����Ă���Ȃ� TRUE */
	int32_t		dma_sect;		/* �c�l�`�]������Z�N�^�� */
	uint32_t		*write_addr;	/* �������ރA�h���X */
	int32_t		buf_bsize;		/* �󂫃o�b�t�@�̃o�C�g�� */
	int32_t		write_bsize;	/* �]�������o�C�g�� */
	int32_t		sect_bsize;		/* �P�Z�N�^�̃o�C�g�� */
	int32_t		audio_1st_sect;	/* �擪�I�[�f�B�I�Z�N�^ */
	StmTrFunc	load_func;		/* �]���֐� */
} PcmFileStmPara;

typedef struct {
	int32_t		faccess_type;		/* �t�@�C���A�N�Z�X�^�C�v */
	/* �t�@�C�����[�J���f�[�^ */
	union	{
		PcmFileGfsPara	gfs;
		PcmFileStmPara	stm;
	} data;
} PcmFilePara;

/* ���s�֐� */
typedef struct {
	void	(*start)(PcmHn pcm);
	void	(*task)(PcmHn pcm);
	int32_t	(*preload_file)(PcmHn pcm, int32_t size);
	void	(*set_load_num)(PcmHn pcm, int32_t load_sct);
	void	(*set_trmode)(PcmHn pcm, PcmTrMode mode);
} PcmExecFunc;

/* ���ݎQ�Ƃ̂��߂̋�錾 */
struct PcmWork;

/* �쐬�p�����[�^ */
typedef struct {
	struct PcmWork *work;		/* ���[�N�̃A�h���X 						*/
								/*  (���C�u�������g�p��������ٖ��̍�Ɨ̈�) */
	int8_t		*ring_addr;		/* �t�@�C�����i�[����o�b�t�@�̐擪�A�h���X */
								/*  (�������̃t�@�C���̎��͂��̃A�h���X) 	*/
								/*  (CD�̃t�@�C���̎����ݸ��ޯ̧�̱��ڽ) 	*/
	int32_t		ring_size;		/* �o�b�t�@�T�C�Y[byte] 					*/
								/*  (�������̃t�@�C���̎��̓t�@�C���T�C�Y) 	*/
								/*  (CD�̃t�@�C���̎����ݸ��ޯ̧�̃T�C�Y 	*/
	int8_t		*pcm_addr;		/* �T�E���h��������PCM�o�b�t�@�A�h���X 		*/
	int32_t		pcm_size;		/* �T�E���h��������PCM�o�b�t�@�T�C�Y[sample]*/
								/*  (1ch���̃T���v���� 4096*1..4096*16) 	*/
} PcmCreatePara;

/* �쐬�p�����[�^ */
typedef struct {
	int8_t		*ring_addr;		/* �t�@�C�����i�[����o�b�t�@�̐擪�A�h���X */
								/*  (�������̃t�@�C���̎��͂��̃A�h���X) 	*/
								/*  (CD�̃t�@�C���̎����ݸ��ޯ̧�̱��ڽ) 	*/
	int32_t		ring_size;		/* �o�b�t�@�T�C�Y[byte] 					*/
								/*  (�������̃t�@�C���̎��̓t�@�C���T�C�Y) 	*/
								/*  (CD�̃t�@�C���̎����ݸ��ޯ̧�̃T�C�Y 	*/
	int8_t		*pcm_addr;		/* �T�E���h��������PCM�o�b�t�@�A�h���X 		*/
	int32_t		pcm_size;		/* �T�E���h��������PCM�o�b�t�@�T�C�Y 		*/
								/*  (1ch���̃T���v���� 4096*1..4096*16) 	*/

	int32_t		command_blk_no;		/* �R�}���h�u���b�N�ԍ� 				*/
	int32_t		pcm_stream_no;		/* �o�b�l�X�g���[���Đ��ԍ� 			*/
	int32_t		pcm_pan;			/* �o�`�m     [0..31] 					*/
	int32_t		pcm_level;			/* �k�d�u�d�k [0..7] 					*/

	int32_t		start_trg_size;		/* �Đ��J�n�g���K�T�C�Y[byte] 			*/
	int32_t		start_trg_sample;	/* �Đ��J�n�g���K�T���v��[sample/1ch] 	*/
	int32_t		stop_trg_sample;	/* �Đ���~�g���K�T���v��[sample/1ch] 	*/
} PcmPara;

/* �o�b�l���[�N */
typedef struct {
	int32_t			work_start;		/* 'WORK' ���[�N�̊J�n 			*/
	PcmPara 		para;			/* �p�����[�^ 					*/
	PcmStatus 		status;			/* �e�퐧���� 				*/
	PcmFilePara 	filepara;		/* �t�@�C���A�N�Z�X�p�����[�^ 	*/
	PcmExecFunc 	execfunc;		/* ���s�֐� 					*/
} PcmWork;

/*-------------------------- �s�֐��錾�t --------------------------*/
/* ���C�u�����̏����� */
bool PCM_Init(void);

/* ���C�u�����̏I������ */
void PCM_Finish(void);

/* �`�c�o�b�l�g�p�錾 */
void PCM_DeclareUseAdpcm(void);

/* �n���h���̍쐬�i�������j */
PcmHn PCM_CreateMemHandle(PcmCreatePara *para);

/* �n���h���̏����i�������j */
void PCM_DestroyMemHandle(PcmHn pcm);

/* �n���h���̍쐬�i�t�@�C���V�X�e���j */
PcmHn PCM_CreateGfsHandle(PcmCreatePara *para, GfsHn gfs);

/* �n���h���̏����i�t�@�C���V�X�e���j */
void PCM_DestroyGfsHandle(PcmHn pcm);

/* �n���h���̍쐬�i�X�g���[���V�X�e���j */
PcmHn PCM_CreateStmHandle(PcmCreatePara *para, StmHn stm);

/* �n���h���̏����i�X�g���[���V�X�e���j */
void PCM_DestroyStmHandle(PcmHn pcm);

/* �Đ��^�X�N */
void PCM_Task(PcmHn pcm);

/* �u�u�����N�h�m�����֐� */
void PCM_VblIn(void);

/* �Đ��J�n */
void PCM_Start(PcmHn pcm);

/* �Đ���~ */
void PCM_Stop(PcmHn pcm);

/* �ꎞ��~ */
void PCM_Pause(PcmHn pcm, PcmPauseCmd cmd);

/* ���[�v�񐔂̎w�� */
void PCM_SetLoop(PcmHn pcm, int32_t cnt_loop);

/* ���ɍĐ�����n���h���̓o�^ */
void PCM_EntryNext(PcmHn pcm);

/* �n���h���̋����؂�ւ� */
void PCM_Change(void);

/* �n���h���̐؂�ւ���Ԃ̎擾 */
PcmChangeStatus PCM_CheckChange(void);

/* �|�[�Y�����p���[�N�A�h���X�A�T�C�Y�̐ݒ� */
void PCM_SetPauseWork(int32_t *addr, int32_t size);

/* �ő�]���Z�N�^���̐ݒ� */
void PCM_SetLoadNum(PcmHn pcm, int32_t read_sct);

/* �Đ��o�`�m�̐ݒ� */
void PCM_SetPan(PcmHn pcm, int32_t pan);

/* �Đ��{�����[���̐ݒ� */
void PCM_SetVolume(PcmHn pcm, int32_t volume);

/* �o�b�l�Đ��p�����[�^�̕ύX */
void PCM_ChangePcmPara(PcmHn pcm);

/* �o�b�l�Đ����̐ݒ� */
void PCM_SetInfo(PcmHn hn, PcmInfo *info);

/* �P��̃^�X�N�ŏ�������ʂ̏���̐ݒ� [sample/1ch] */
void PCM_Set1TaskSample(PcmHn pcm, int32_t sample);

/* �Đ��J�n�g���K�T�C�Y�̐ݒ� [byte] */
void PCM_SetStartTrgSize(PcmHn pcm, int32_t size);

/* �Đ��J�n�g���K�T���v���̐ݒ� [sample/1ch] */
void PCM_SetStartTrgSample(PcmHn pcm, int32_t sample);

/* �Đ���~�g���K�T���v���̐ݒ� [sample/1ch] */
void PCM_SetStopTrgSample(PcmHn pcm, int32_t sample);

/* �o�b�l�R�}���h�u���b�N�ԍ��̐ݒ� */
/* void PCM_SetPcmCmdBlockNo(PcmHn pcm, int32_t blk_no); */

/* �o�b�l�X�g���[���ԍ��̐ݒ� */
void PCM_SetPcmStreamNo(PcmHn pcm, int32_t stream_no);

/* �f�[�^�̓]�������̐ݒ�i�b�c�u���b�N�������O�o�b�t�@�j */
void PCM_SetTrModeCd(PcmHn pcm, PcmTrMode mode);

/* �f�[�^�̓]�������̐ݒ�i�����O�o�b�t�@���o�b�l�o�b�t�@�j */
/* void PCM_SetTrModePcm(PcmHn pcm, PcmTrMode mode); */

/* �Đ������̎擾 */
int32_t PCM_GetTime(PcmHn pcm);

/* �Đ��X�e�[�^�X�̎擾 */
PcmPlayStatus PCM_GetPlayStatus(PcmHn pcm);

/* �������݃o�b�t�@�A�h���X�̎擾 */
uint32_t *PCM_GetWriteBuf(PcmHn pcm, int32_t *free_size, int32_t *free_total);

/* �o�b�t�@�ւ̏������݃T�C�Y�̒ʒm */
void PCM_NotifyWriteSize(PcmHn pcm, int32_t write_size);

/* �������Ƀt�@�C����ǂݍ��� */
int32_t PCM_PreloadFile(PcmHn pcm, int32_t size);

/* �������Ƀt�@�C���w�b�_��ǂݍ��� */
/* int32_t PCM_PreloadHeader(PcmHn pcm); */

/* �G���[�֐��̓o�^ */
void PCM_SetErrFunc(PcmErrFunc func, void *obj);

/* �G���[���̎擾 */
PcmErrCode PCM_GetErr(void);

/* ����J */
bool PCM_IsDeath(PcmHn pcm);

#endif	/* SEGA_PCM_H */
