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

#define	BUP_LIB_ADDRESS	(*(volatile uint32_t *)(0x6000350+8))
#define	BUP_VECTOR_ADDRESS	(*(volatile uint32_t *)(0x6000350+4))

/* Unit ID */
#define	BUP_MAIN_UNIT	(1)
#define	BUP_CURTRIDGE	(2)

/* Language type */
#define	BUP_JAPANESE	(0)
#define	BUP_ENGLISH	(1)
#define	BUP_FRANCAIS	(2)
#define	BUP_DEUTSCH	(3)
#define	BUP_ESPANOL	(4)
#define	BUP_ITALIANO	(5)

/* Machine state */
#define	BUP_NON		(1)
#define	BUP_UNFORMAT		(2)
#define	BUP_WRITE_PROTECT	(3)
#define	BUP_NOT_ENOUGH_MEMORY	(4)
#define	BUP_NOT_FOUND		(5)
#define	BUP_FOUND		(6)
#define	BUP_NO_MATCH		(7)
#define	BUP_BROKEN		(8)

/******************************************
 * Storage connection information table   *
 ******************************************/
typedef struct BupConfig{
	uint16_t	unit_id;	/* Unit ID */
	uint16_t	partition;	/* Number of partitions */
} BupConfig;

/******************************************
 * Status information table               *
 ******************************************/
typedef	struct BupStat{
	uint32_t	totalsize;	/* Total capacity (Byte) */
	uint32_t	totalblock;	/* Total blocks */
	uint32_t	blocksize;	/* Size of a block (Byte) */
	uint32_t	freesize;	/* Free space */
	uint32_t	freeblock;	/* Number of empty blocks */
	uint32_t	datanum;
} BupStat;

/******************************************
 * Directory information table            *
 ******************************************/
typedef struct BupDir{
	uint8_t	filename[12];	/* File name */
	uint8_t	comment[11];	/* Comments */
	uint8_t	language;	/* The language type of the comment */
	uint32_t	date;		/* Time stamp */
	uint32_t	datasize;	/* Data size (Byte) */
	uint16_t	blocksize;	/* Data size (blocks) */
} BupDir;

typedef struct BupDate {
	uint8_t	year;		/* Timestamp (year-1980) */
	uint8_t	month;		/* Timestamp (month 1-12) */
	uint8_t	day;		/* Time stamp (1–31 days) */
	uint8_t	time;		/* Time stamp (hours 0-23) */
	uint8_t	min;		/* Time stamp (minutes 0-59) */
	uint8_t	week;		/* Timestamp (day of week 0-Saturday 6)*/
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
