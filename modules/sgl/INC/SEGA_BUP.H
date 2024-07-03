/*------------------------------------------------------------------------
 *  FILE:	sega_bup.h
 *
 *	Copyright(c) 1994 SEGA
 *
 *  PURPOSE:
 *	Back Up Library
 *
 *  AUTHOR(S):
 *	K.M
 *		
 *  MOD HISTORY:
 *	Written by K.M on 1994-07-13 Ver.1.00
 *	Updated by K.M on 1994-07-29 Ver.1.00
 * 
 *------------------------------------------------------------------------
 */


#ifndef	SEGA_BUP_H
#define SEGA_BUP_H

#include	<sega_xpt.h>

#define	BUP_LIB_ADDRESS		(*(volatile uint32_t *)(0x6000350+8))
#define	BUP_VECTOR_ADDRESS	(*(volatile uint32_t *)(0x6000350+4))

/* ���j�b�g�h�c */
#define	BUP_MAIN_UNIT	(1)
#define	BUP_CURTRIDGE	(2)

/* ������ */
#define	BUP_JAPANESE	(0)
#define	BUP_ENGLISH	(1)
#define	BUP_FRANCAIS	(2)
#define	BUP_DEUTSCH	(3)
#define	BUP_ESPANOL	(4)
#define	BUP_ITALIANO	(5)

/* ���u��� */
#define	BUP_NON			(1)
#define	BUP_UNFORMAT		(2)
#define	BUP_WRITE_PROTECT	(3)
#define	BUP_NOT_ENOUGH_MEMORY	(4)
#define	BUP_NOT_FOUND		(5)
#define	BUP_FOUND		(6)
#define	BUP_NO_MATCH		(7)
#define	BUP_BROKEN		(8)

/******************************************
 * �L�����u�ڑ����e�[�u��               *
 ******************************************/
typedef struct BupConfig{
	uint16_t	unit_id;	/* ���j�b�g�h�c */
	uint16_t	partition;	/* �p�[�e�B�V������ */
} BupConfig;

/******************************************
 * �X�e�C�^�X���e�[�u��                 *
 ******************************************/
typedef	struct BupStat{
	uint32_t	totalsize;	/* �S�e��(Byte) */
	uint32_t	totalblock;	/* �S�u���b�N�� */
	uint32_t	blocksize;	/* 1�u���b�N�̃T�C�Y(Byte) */
	uint32_t	freesize;	/* �󂫗e�� */
	uint32_t	freeblock;	/* ��u���b�N�� */
	uint32_t	datanum;
} BupStat;

/******************************************
 * �f�B���N�g�����e�[�u��               *
 ******************************************/
typedef struct BupDir{
	uint8_t	filename[12];	/* �t�@�C���� */
	uint8_t	comment[11];	/* �R�����g */
	uint8_t	language;	/* �R�����g�̌����� */
	uint32_t	date;		/* �^�C���X�^���v */
	uint32_t	datasize;	/* �f�[�^�T�C�Y�iByte�j */
	uint16_t	blocksize;	/* �f�[�^�T�C�Y�i�u���b�N���j */
} BupDir;

typedef struct BupDate {
	uint8_t	year;		/* �^�C���X�^���v�i�N -1980�j */
	uint8_t	month;		/* �^�C���X�^���v�i�� 1�`12�j */
	uint8_t	day;		/* �^�C���X�^���v�i�� 1�`31�j */
	uint8_t	time;		/* �^�C���X�^���v�i�� 0�`23�j */
	uint8_t	min;		/* �^�C���X�^���v�i�� 0�`59�j */
	uint8_t	week;		/* �^�C���X�^���v (�j�� ��0�`�y6)*/
} BupDate;

#endif /* ifndef SEGA_BUP_H */

#ifndef	SEGA_BUP_PROTO
#define SEGA_BUP_PROTO

/* #if !(__GNUC__) */
#ifndef __GNUC__
#define	BUP_Init	((void (*)(uint32_t *lib,uint32_t *work,BupConfig tp[3])) (BUP_LIB_ADDRESS))
#else
#define	BUP_Init	((void (*)(volatile uint32_t *lib,uint32_t *work,BupConfig tp[3])) (BUP_LIB_ADDRESS))
#endif

#define	BUP_SelPart	((int32_t (*)(uint32_t device,uint16_t num)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+4)))

#define	BUP_Format	((int32_t (*)(uint32_t device)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+8)))

#define	BUP_Stat	((int32_t (*)(uint32_t device,uint32_t datasize,BupStat *tb)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+12)))

/* #if !(__GNUC__) */
#ifndef __GNUC__
#define	BUP_Write	((int32_t (*)(uint32_t device,BupDir *tb,uint8_t *data,uint8_t wmode)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+16)))
#else
#define	BUP_Write	((int32_t (*)(uint32_t device,BupDir *tb,volatile uint8_t *data,uint8_t wmode)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+16)))
#endif

/* #if !(__GNUC__) */
#ifndef __GNUC__
#define	BUP_Read	((int32_t (*)(uint32_t device,uint8_t *filename,uint8_t *data)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+20)))
#else
#define	BUP_Read	((int32_t (*)(uint32_t device,uint8_t *filename,volatile uint8_t *data)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+20)))
#endif

#define	BUP_Delete	((int32_t (*)(uint32_t device,uint8_t *filename)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+24)))

#define	BUP_Dir 	((int32_t (*)(uint32_t device,uint8_t *filename,uint16_t tbsize,BupDir *tb)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+28)))

/* #if !(__GNUC__) */
#ifndef __GNUC__
#define	BUP_Verify	((int32_t (*)(uint32_t device,uint8_t *filename,uint8_t *data)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+32)))
#else
#define	BUP_Verify	((int32_t (*)(uint32_t device,uint8_t *filename,volatile uint8_t *data)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+32)))
#endif

#define	BUP_GetDate	((void (*)(uint32_t date,BupDate *tb)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+36)))

#define	BUP_SetDate	((uint32_t (*)(BupDate *tb)) (*(uint32_t *)(BUP_VECTOR_ADDRESS+40)))

#endif /* SEGA_BUP_PROTO */
