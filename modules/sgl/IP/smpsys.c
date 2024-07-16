/*                                                              */
/*    アプリケーション初期化プログラムサンプル                  */
/*                              V1.10  '94.11.11  S.U and T.S   */
/*                                                              */
/*    動作タイミングと機能について：                            */
/*       ライセンス画面表示中に、不定になっている各種デバイス   */
/*       状態や、そのメモリのクリアなどを行ないます。           */
/*       処理の所要時間は、0.1 秒程度かかります。               */
/*       終了後、先読みしておいたプログラムの実行に移ります。   */
/*       その実行開始アドレスは APP_ENTRY に定義してください。  */
/*                                                              */
/*    リンケージについて：                                      */
/*       ＩＰの sys_init.obj の直後にリンクしてください。       */
/*       プログラムをこのまま使用した場合、約３６０Ｈバイトに   */
/*       なり、ＩＰサイズは１０００Ｈを越えますので注意して     */
/*       ください。                                             */
/*                                                              */
/*    プログラム変更時の注意：                                  */
/*       main() の前に実行文を含む関数を追加しないでください。  */
/*                                                              */

#include "sega_sys.h"

/* 先読みしたプログラムの実行開始アドレス（メイン処理完了後にジャンプ）*/
#define	APP_ENTRY	(0x6004000)

/* デバイスを初期化するルーチン */
static void vd1Comfil(void);		 /* ＶＤＰ１ クリッピング初期化    */
static void vd2Ramfil(void);		 /* ＶＤＰ２ ＶＲＡＭクリア        */
static void colRamfil(void);		 /* カラーＲＡＭクリア             */
static void sndRamfil(int32_t);		 /* サウンドＲＡＭクリア           */
static void scuDspInit(void);		 /* ＳＣＵ ＤＳＰ 初期化           */
static void msh2PeriInit(void);		 /* マスタＳＨ周辺モジュール初期化 */
static void sndDspInit(void);		 /* サウンドＤＳＰクリア           */

/* その他サブルーチン */
static void vbIrtn(void);		 /* ＶＢ-Ｉｎ 割込み処理           */
static void vbOrtn(void);		 /* ＶＢ-Ｏｕｔ 割込み処理         */
static void syncVbI(void);		 /* ＶＢ-Ｉｎ 同期処理用           */

#ifndef __GNUC__
static void memset_w(int16_t *, int16_t, int32_t); /* ワード ｍｅｍｓｅｔ */
static void memcpy_w(int16_t *, int16_t *, int32_t); /* ワード ｍｅｍｃｐｙ */
static int16_t *blkmfil_w(int16_t *, int16_t, int32_t);
                                         /* ワード・ブロックｆｉｌｌ */
static int32_t *blkmfil_l(int32_t *, int32_t, int32_t);
                                         /* ロングワード・ブロックｆｉｌｌ */
#else
static void memset_w(volatile int16_t *, int16_t, int32_t);
static void memcpy_w(volatile int16_t *, int16_t *, int32_t);
static void blkmfil_w(volatile int16_t *, int16_t, int32_t);
                                         /* ワード・ブロックｆｉｌｌ */
static void blkmfil_l(volatile int32_t *, int32_t, int32_t);
                                         /* ロングワード・ブロックｆｉｌｌ */
#endif

/* 現時点の(ライセンス画面表示中)画面サイズに関する情報 */
#define	XRES			(320)	 /* ライセンス画面の水平サイズ     */
#define	SCLIP_UX		(XRES-1) /*             〃                 */
#define	SCLIP_UY_N		(224-1)	 /*   〃   (ＮＴＳＣの場合)        */
#define	SCLIP_UY_P		(256-1)	 /*   〃   (ＰＡＬの場合)          */

/* 処理対象デバイスのベースアドレス */
#define	SND_RAM			((volatile int32_t *)0x25a00000)
#define	VD1_VRAM		((volatile int16_t *)0x25c00000)
#define	VD1_REG			((volatile int16_t *)0x25d00000)
          /* VD2_VRAM は、ライセンス表示で使用中のＶＲＡＭ領域を除く    */
#define	VD2_VRAM		((volatile int32_t *)0x25e08004)
          /* COL_RAM は、ライセンス表示で使用中のカラーＲＡＭ領域を除く */
#define	COL_RAM			((volatile int16_t *)0x25f00020)
#define	VD2_REG			((volatile int16_t *)0x25f80000)
#define SCSP_DSP_RAM		((volatile int16_t *)0x25b00800)

/* ＳＭＰＣレジスタ */
#define	SMPC_REG(ofs)		(*(volatile uint8_t  *)(0x20100000+ofs))

/* ＳＣＵレジスタ */
#define DSP_PGM_CTRL_PORT	(*(volatile int32_t *)0x25fe0080)
#define DSP_PGM_RAM_PORT	(*(volatile int32_t *)0x25fe0084)
#define DSP_DATA_RAM_ADRS_PORT	(*(volatile int32_t *)0x25fe0088)
#define DSP_DATA_RAM_DATA_PORT	(*(volatile int32_t *)0x25fe008c)

/* ＳＣＳＰ サウンドＲＡＭサイズレジスタ */
#define SCSP_SNDRAMSZ		(*(volatile int8_t *)0x25b00400)

/* ＳＨ２周辺モジュールレジスタ */
#define MSH2_DMAC_SAR(ofs)	(*(volatile int32_t *)(0xffffff80 + ofs))
#define MSH2_DMAC_DAR(ofs)	(*(volatile int32_t *)(0xffffff84 + ofs))
#define MSH2_DMAC_TCR(ofs)	(*(volatile int32_t *)(0xffffff88 + ofs))
#define MSH2_DMAC_CHCR(ofs)	(*(volatile int32_t *)(0xffffff8c + ofs))
#define MSH2_DMAC_DRCR(sel)	(*(volatile int8_t  *)(0xfffffe71 + sel))
#define MSH2_DMAC_DMAOR		(*(volatile int32_t *)(0xffffffb0))
#define MSH2_DIVU_CONT		(*(volatile int32_t *)(0xffffffb8))

/* メインの処理に関する情報 */
 	  /* 現在ライセンス表示中、１６ビット１０２４色モードを使用 */
 	  /* ＶＤＰ２のＶＲＡＭとカラーＲＡＭは４回に分けてクリア   */
	  /* ＶＲＡＭは１回に ２００００Ｈ バイト(表示中部分を除く) */
 	  /* カラーＲＡＭは１回に ２００Ｈ バイト(表示中部分を除く) */
#define	MSETDIV			(4)
#define BLKMSK_VD2_VRAM		(0x1fffc)
#define BLKMSK_COL_RAM		(0x001fe)
	  /* サウンドＲＡＭは３手順でクリア */
#define M68000_VECTBLSZ		(0x00400/sizeof(int32_t))
#define BLKMSK_SND_RAM		(0x003fc)
	  /* サウンドＤＳＰ ＲＡＭサイズ */
#define SCSP_DSP_RAMSZ		(0x00400)

	  /* 割込み処理に関する情報 */
#define	VBI_NUM			(0x40)   /* ＶＢイン割込み番号        */
#define	VBO_NUM			(0x41)   /* ＶＢアウト割込み番号      */
#define	VB_MASK			(0x0003) /* ＳＣＵ割込みマスク２つ分  */

	  /* スタティック変数 */
static int16_t	yBottom, ewBotRight;
static int16_t	vdp1cmds[48];
#ifndef __GNUC__
/* static int16_t	vbIcnt = 0, sequence = 0; */
static int16_t	vbIcnt = 0, sequence = 0;
static int32_t	*vramptr = VD2_VRAM;
static int16_t	*cramptr = COL_RAM;
#else
static volatile int16_t	vbIcnt = 0, sequence = 0;
static volatile int32_t	*vramptr = VD2_VRAM;
static volatile int16_t	*cramptr = COL_RAM;
#endif

/* メイン処理 */
void main(void)
{
	/* 注意：AUTO 変数をとると、APP_ENTRY のプログラムに */
        /*       制御が移るとき、スタックが若干無駄になる    */

	yBottom  = (VD2_REG[2]&1)? SCLIP_UY_P: SCLIP_UY_N;
	ewBotRight = ((XRES/8)<<9)+(yBottom);
	  /* 画面縦上限＝ ２２３(ＮＴＳＣ) or ２５５(ＰＡＬ) */

	SYS_SETUINT(VBI_NUM, vbIrtn); /* ＶＢイン処理登録    */
	SYS_SETUINT(VBO_NUM, vbOrtn); /* ＶＢアウト  〃      */
	SYS_CHGSCUIM( ~VB_MASK, 0);   /* ＶＢ割込み２つ許可  */

	vd1Comfil();                  /* ＶＤＰ１初期化      */
	for (sequence = 0; sequence < MSETDIV; sequence++)
	{
		syncVbI();   /* カラーＲＡＭクリアのため同期 */
		colRamfil();	     /* カラーＲＡＭクリア   */
		vd2Ramfil();	     /* ＶＤＰ２ＲＡＭクリア */
		sndRamfil(sequence); /* サウンドＲＡＭクリア */
	}

	scuDspInit();                /* ＳＣＵのＤＳＰ初期化 */
	msh2PeriInit();              /* ＳＨ周辺モジュール〃 */
	sndDspInit();                /* サウンドＤＳＰ 〃    */

	SYS_CHGSCUIM( -1, VB_MASK);  /* ＶＢ割込み２つ禁止   */
	SYS_SETUINT(VBI_NUM, (void(*)())0 ); /* フック       */
	SYS_SETUINT(VBO_NUM, (void(*)())0 ); /*   再初期化   */

	((void(*)())APP_ENTRY)();    /* 次の実行開始アドレス */
}

#ifdef __GNUC__		/* Add. 96/04/01 K.T */
void __main(){}
#endif

#ifndef __GNUC__
static void memset_w(int16_t *buf, int16_t pattern, int32_t size)
#else
static void memset_w(volatile int16_t *buf, int16_t pattern, int32_t size)
#endif
{
	register int32_t	i;

	for (i = 0; i < size; i+= sizeof(int16_t)) {
		*buf++ = pattern;
	}
}

#ifndef __GNUC__
static void memcpy_w(int16_t *dst, int16_t *src, int32_t size)
#else
static void memcpy_w(volatile int16_t *dst, int16_t *src, int32_t size)
#endif
{
	register int32_t	i;

	for (i = 0; i < size; i+= sizeof(int16_t)) {
		*dst++ = *src++;
	}
}

/* ｂｌｋｍｆｉｌは、区切りのいいアドレス範囲をｍｅｍｓｅｔするもの */
#ifndef __GNUC__
static int16_t *blkmfil_w(int16_t *buf, int16_t pattern, int32_t brkmsk)
#else
static void blkmfil_w(volatile int16_t *buf, int16_t pattern, int32_t brkmsk)
#endif
{
	register int32_t	i;

#ifndef __GNUC__
	i = (int32_t)buf & brkmsk;
#else
	i = (volatile int32_t)buf & brkmsk;
#endif
	for (; i <= brkmsk; i+= sizeof(int16_t)) {
		*buf++ = pattern;
	}
#ifndef __GNUC__
	return (buf);
#endif
}

#ifndef __GNUC__
static int32_t *blkmfil_l(int32_t *buf, int32_t pattern, int32_t brkmsk)
#else
static void blkmfil_l(volatile int32_t *buf, int32_t pattern, int32_t brkmsk)
#endif
{
	register int32_t	i;

#ifndef __GNUC__
	i = (int32_t)buf & brkmsk;
#else
	i = (volatile int32_t)buf & brkmsk;
#endif
	for (; i <= brkmsk; i+= sizeof(int32_t)) {
		*buf++ = pattern;
	}
#ifndef __GNUC__
	return (buf);
#endif
}

/* ＶＢインは、割込みでスタティック変数をインクリメントするのみ */
static void vbIrtn(void)
{
	vbIcnt++;
}

/* ＶＢアウトは、割込みでＶＤＰ１レジスタをコントロールするのみ */
static void vbOrtn(void)
{
#ifndef __GNUC__
	register int16_t	*vdp1r;
#else
	register volatile int16_t *vdp1r;
#endif
                        /* イレースライトでフレームバッファをクリア */
	vdp1r = VD1_REG;
	*vdp1r++ = 0x0;	/* １／６０秒自動描画モード */
	*vdp1r++ = 0x0;
	*vdp1r++ = 0x2;
	*vdp1r++ = 0x0;	/* イレースライトは透明色   */
	*vdp1r++ = 0x0;	        /*  〃   左上座標   */
	*vdp1r   = ewBotRight;  /*  〃   右下座標   */
}

/* ＶＢＩ同期は、呼ばれると待ち、ＶＢＩ直後に抜けて戻る */
static void syncVbI(void)
{
	register int32_t  cur_cnt_value;
                        /* 待つのはカラーＲＡＭクリアのため */
	cur_cnt_value = vbIcnt;
	while (cur_cnt_value == vbIcnt);
}

/* ＶＤＰ１に、システムクリッピングとローカル座標を読ませる */
static void vd1Comfil(void)
{
	register int16_t	*cmdbuf;

	memset_w((cmdbuf=vdp1cmds), 0, sizeof(vdp1cmds));
	cmdbuf[0]  = 0x0009;
	cmdbuf[10] = SCLIP_UX;
	cmdbuf[11] = yBottom;
	cmdbuf[16] = 0x000a;
	cmdbuf[32] = 0x8000;
	memcpy_w(VD1_VRAM, vdp1cmds, sizeof(vdp1cmds));
}

/* ＶＤＰ２のＲＡＭを、１／４ずつクリア */
static void vd2Ramfil(void)
{
#ifndef __GNUC__
	vramptr = blkmfil_l(vramptr, 0, BLKMSK_VD2_VRAM); 
#else
	blkmfil_l(vramptr, 0, BLKMSK_VD2_VRAM); 
#endif
}

/* カラーＲＡＭを、１／４ずつクリア */
static void colRamfil(void)
{
#ifndef __GNUC__
	cramptr = blkmfil_w(cramptr, 0, BLKMSK_COL_RAM);
#else
	blkmfil_w(cramptr, 0, BLKMSK_COL_RAM);
#endif
}

/* サウンドＲＡＭを、３手順でクリア */
static void sndRamfil(int32_t initstep)
{
#ifndef __GNUC__
	register int32_t	*memptr;
#else
	register volatile int32_t *memptr;
#endif

	switch (initstep) {
	case 0:
		SMPC_REG(31) = 7;	/* Ｍ６８０００を停止          */
		break;
	case 1:
		SCSP_SNDRAMSZ = 2;   	/* サウンドＲＡＭサイズ設定    */
					/* サウンドＲＡＭ先頭４００Ｈ  */
		memptr = SND_RAM;	/* (ベクタ)に４００Ｈをフィル  */
		blkmfil_l(memptr, 0x400, BLKMSK_SND_RAM);
		*memptr = 0x0007fffc;	/* ＳＰ初期値をセット          */
		memptr += M68000_VECTBLSZ;
		*memptr = 0x4e7160fc;	/* アドレス４００Ｈに ＮＯＰと */
					/* ＢＲＡ ＠－２ 命令を書込み  */
		break;
	case 2:
		SMPC_REG(31) = 6;	/* Ｍ６８０００起動(無限待ち)  */
		break;
					/* 備考： １イントの間がある   */
	}                               /* ため、ＳＭＰＣステータスの  */
}                                       /* セット／チェックを省略      */

static void msh2PeriInit(void)
{
	register int32_t i, ofs, dummy;

        ofs = 0;
	for(i = 0;i < 2; i++)
        {                               /* ＤＭＡＣ各レジスタを初期化  */
		MSH2_DMAC_SAR(ofs)  = 0x00000000;
		MSH2_DMAC_DAR(ofs)  = 0x00000000;
		MSH2_DMAC_TCR(ofs)  = 0x00000001;
		dummy = MSH2_DMAC_CHCR(ofs);
		MSH2_DMAC_CHCR(ofs) = 0x00000000;
		MSH2_DMAC_DRCR(i)   = 0x00;
		ofs = 0x10;
	}
	dummy  = MSH2_DMAC_DMAOR;
	MSH2_DMAC_DMAOR = 0x00000000;
                                        /* ＤＩＶＵ割込みを不許可      */
	MSH2_DIVU_CONT =  0x00000000;
}

static void scuDspInit(void)
{
	register int32_t i;

	DSP_PGM_CTRL_PORT = 0x0;              /* ＤＳＰ停止           */

	for(i = 0; i < 256; i++)
	  DSP_PGM_RAM_PORT = 0xf0000000;      /* ＥＮＤ命令フィル     */
	
	for(i = 0; i < 256; i++){             /* ＤＳＰ ＲＡＭクリア  */
		DSP_DATA_RAM_ADRS_PORT = i;
		DSP_DATA_RAM_DATA_PORT = 0x0;
	}
}

static void sndDspInit(void)
{
	memset_w(SCSP_DSP_RAM, 0, SCSP_DSP_RAMSZ);
					      /* サウンドＤＳＰ       */
}					      /* プログラム領域クリア */
