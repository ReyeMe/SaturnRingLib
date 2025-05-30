/*
 *     ソフトウェアライブラリ
 *
 *     Copyright (c) 1994 SEGA
 *
 *  Library: ＣＤ通信インタフェース（CDC）
 *  Module : CDCライブラリ用ヘッダファイル
 *  File   : sega_cdc.h
 *  Date   : 1995-03-31
 *  Version: 1.20
 *  Author : M.M.
 *
 */

/* 多重インクルードへの対応 */
#ifndef SEGA_CDC_H
#define SEGA_CDC_H

/*******************************************************************
*       インクルードファイル
*******************************************************************/
#include    "sega_xpt.h"

/*******************************************************************
*       定数マクロ
*******************************************************************/

/* 割り込み要因レジスタ関係（HIRQREQ, HIRQMSK）のビット名 */
#define CDC_HIRQ_CMOK   0x0001  /* bit0 ：コマンド発行可能 */
#define CDC_HIRQ_DRDY   0x0002  /* bit1 ：データ転送準備完了 */
#define CDC_HIRQ_CSCT   0x0004  /* bit2 ：１セクタ読み込み完了 */
#define CDC_HIRQ_BFUL   0x0008  /* bit3 ：ＣＤバッファフル */
#define CDC_HIRQ_PEND   0x0010  /* bit4 ：ＣＤ再生の終了 */
#define CDC_HIRQ_DCHG   0x0020  /* bit5 ：ディスク交換の発生 */
#define CDC_HIRQ_ESEL   0x0040  /* bit6 ：セレクタ設定処理の終了 */
#define CDC_HIRQ_EHST   0x0080  /* bit7 ：ホスト入出力処理の終了 */
#define CDC_HIRQ_ECPY   0x0100  /* bit8 ：複写／移動処理の終了 */
#define CDC_HIRQ_EFLS   0x0200  /* bit9 ：ファイルシステム処理の終了 */
#define CDC_HIRQ_SCDQ   0x0400  /* bit10：サブコードＱの」更新完了 */
#define CDC_HIRQ_MPED   0x0800  /* bit11：MPEG関連処理の終了 */
#define CDC_HIRQ_MPCM   0x1000  /* bit12：MPEG動作不定区間の終了 */
#define CDC_HIRQ_MPST   0x2000  /* bit13：MPEG割り込みステータスの通知 */
    
/* バッファ区画のセクタ範囲（セクタ位置とセクタ数）の特殊指定 */
/* （いずれも下位16ビットが有効で、内部的には0xffffと同値） */
#define CDC_SPOS_END    -1  /* 区画最後のセクタ位置を示す */
#define CDC_SNUM_END    -1  /* 指定ｾｸﾀ位置から区画最後までのｾｸﾀ数を示す */

/* その他の特殊指定 */
#define CDC_PARA_DFL    0       /* パラメータの省略値指定 */
#define CDC_PARA_NOCHG  -1      /* パラメータの未変更指定 */
#define CDC_NUL_SEL     0xff    /* セレクタ番号の特殊値 */
#define CDC_NUL_FID     -1      /* ファイル識別子の特殊値 (0xffffff) */

/* ＣＤフラグ */
#define CDC_CDFLG_ROM   0x80    /* CD-ROMデコード中 */

/* ハードウェアフラグ（ハードウェア情報内） */
#define CDC_HFLAG_MPEG  0x02    /* MPEGあり */
#define CDC_HFLAG_HERR  0x80    /* ハードウェアエラー発生 */

/* 再生モード（ＣＤ再生パラメータ内） */
#define CDC_PM_DFL          0x00    /* 再生モードの省略値 */
#define CDC_PM_REP_NOCHG    0x7f    /* 最大リピート回数を変更しない */
#define CDC_PM_PIC_NOCHG    0x80    /* ピックアップ位置を変更しない */
#define CDC_PM_NOCHG        -1      /* 再生モードを変更しない (0xff) */

/* サブモード */
#define CDC_SM_EOR      0x01    /* レコード最後のセクタ */
#define CDC_SM_VIDEO    0x02    /* ビデオセクタ */
#define CDC_SM_AUDIO    0x04    /* オーディオセクタ */
#define CDC_SM_DATA     0x08    /* データセクタ */
#define CDC_SM_TRIG     0x10    /* トリガＯＮ */
#define CDC_SM_FORM     0x20    /* フォームビット（1:Form2,  0:Form1）*/
#define CDC_SM_RT       0x40    /* リアルタイムセクタ */
#define CDC_SM_EOF      0x80    /* ファイル最後のセクタ */

/* ファイルアトリビュート（ファイル情報内） */
#define CDC_ATR_DIRFG   0x02    /* ディレクトリである */
#define CDC_ATR_FORM1   0x08    /* Form1セクタを含む */
#define CDC_ATR_FORM2   0x10    /* Form2セクタを含む */
#define CDC_ATR_INTLV   0x20    /* インタリーブセクタを含む */
#define CDC_ATR_CDDA    0x40    /* CD-DAファイル */
#define CDC_ATR_DIRXA   0x80    /* ディレクトリファイル */

/* スタンバイタイム */
#define CDC_STNBY_MIN   60      /* 最小値 */
#define CDC_STNBY_MAX   900     /* 最大値 */

/* サブコードフラグ */
#define CDC_SCD_PACK    0x01    /* パックデータエラー */
#define CDC_SCD_OVER    0x02    /* オーバーランエラー */

/* ＣＤブロックの転送ワード数 */
#define CDC_DEND_ERR    0xffffff    /* データ転送でエラーが発生 */

/* 実データサイズ */
#define CDC_ACTSIZ_ERR  0xffffff    /* 計算を実行できなかった */

/* フレームアドレス検索結果（セクタ位置とフレームアドレス） */
#define CDC_SPOS_ERR    0xffff      /* 検索でエラーが発生 */
#define CDC_FAD_ERR     0xffffff    /* 検索を実行できなかった */

/* マスクビットパターン */
#define CDC_STC_MSK     0x0f    /* ステータスコード */

/*******************************************************************
*       列挙定数
*******************************************************************/

/* エラーコード */
enum CdcErrCode {
    CDC_ERR_OK = OK,    /* 関数が正常終了した */

    CDC_ERR_CMDBUSY=-1, /* コマンド終了フラグが１になっていない */
    CDC_ERR_CMDNG  =-2, /* コマンド発行時、CMOKフラグが１になっていない */
    CDC_ERR_TMOUT  =-3, /* タイムアウト（ﾚｽﾎﾟﾝｽ待ち、ﾃﾞｰﾀ転送準備待ち）*/
    CDC_ERR_PUT    =-4, /* 書き込み準備で空きセクタを確保できなかった */
    CDC_ERR_REJECT =-5, /* コマンドに対するレスポンスがREJECTとなった */
    CDC_ERR_WAIT   =-6, /* コマンドに対するレスポンスがWAITとなった */
    CDC_ERR_TRNS   =-7, /* データ転送サイズが異常である */
    CDC_ERR_PERI   =-8  /* 定期レスポンスを取得できなかった */
};

/* ステータス */
enum CdcStatus {
    /* ステータスコード（ＣＤドライブ状態） */
    CDC_ST_BUSY     = 0x00,     /* 状態遷移中 */
    CDC_ST_PAUSE    = 0x01,     /* ポーズ中（一時停止） */
    CDC_ST_STANDBY  = 0x02,     /* スタンバイ（ドライブ停止状態） */
    CDC_ST_PLAY     = 0x03,     /* ＣＤ再生中 */
    CDC_ST_SEEK     = 0x04,     /* シーク中 */
    CDC_ST_SCAN     = 0x05,     /* スキャン再生中 */
    CDC_ST_OPEN     = 0x06,     /* トレイが開いている */
    CDC_ST_NODISC   = 0x07,     /* ディスクがない */
    CDC_ST_RETRY    = 0x08,     /* リードリトライ処理中 */
    CDC_ST_ERROR    = 0x09,     /* リードデータエラーが発生した */
    CDC_ST_FATAL    = 0x0a,     /* 致命的エラーが発生した */

    /* その他 */
    CDC_ST_PERI     = 0x20,     /* 定期レスポンス */
    CDC_ST_TRNS     = 0x40,     /* データ転送要求あり */
    CDC_ST_WAIT     = 0x80,     /* WAIT */
    CDC_ST_REJECT   = 0xff      /* REJECT */
};

/* 位置タイプ（再生、シーク時におけるＣＤ位置パラメータで指定） */
enum CdcPosType {
    CDC_PTYPE_DFL,          /* 省略値の指定 */
    CDC_PTYPE_FAD,          /* フレームアドレス指定 */
    CDC_PTYPE_TNO,          /* トラック／インデックス指定 */
    CDC_PTYPE_NOCHG,        /* 未変更の指定（設定値を変更しない） */

    CDC_PTYPE_END
};

/* 転送準備待ちタイプ（データ転送の準備待ちで指定） */
enum CdcDrdyType {
    CDC_DRDY_GET,           /* データを取り出す（CDﾌﾞﾛｯｸ→ﾎｽﾄ） */
    CDC_DRDY_PUT,           /* データを書き込む（ﾎｽﾄ→CDﾌﾞﾛｯｸ） */

    CDC_DRDY_END
};

/* スキャン方向 */
enum CdcScanDir {
    CDC_SCAN_FWD = 0x00,    /* 早送り再生 */
    CDC_SCAN_RVS = 0x01     /* 早戻し再生 */
};

/* セクタ長 */
enum CdcSctLen {
    CDC_SLEN_2048  = 0,     /* 2048バイトと2324バイト（ユーザデータ）*/
    CDC_SLEN_2336  = 1,     /* 2336バイト（サブヘッダまで）*/
    CDC_SLEN_2340  = 2,     /* 2340バイト（ヘッダまで）*/
    CDC_SLEN_2352  = 3,     /* 2352バイト（セクタ全体）*/
    CDC_SLEN_NOCHG = -1     /* 設定を変更しない (0xff) */
};

/* 複写／移動エラー情報 */
enum CdcCopyErr {
    CDC_COPY_OK   = 0,      /* 正常終了 */
    CDC_COPY_NG   = 1,      /* エラー発生 */
    CDC_COPY_BUSY = 0xff    /* 複写／移動処理中 */
};

/*******************************************************************
*       構造体アクセス処理マクロ
*******************************************************************/

/* ＣＤステータス情報 */
#define CDC_STAT_STATUS(stat)       ((stat)->status)
#define CDC_STAT_FLGREP(stat)       ((stat)->report.flgrep)
#define CDC_STAT_CTLADR(stat)       ((stat)->report.ctladr)
#define CDC_STAT_TNO(stat)          ((stat)->report.tno)
#define CDC_STAT_IDX(stat)          ((stat)->report.idx)
#define CDC_STAT_FAD(stat)          ((stat)->report.fad)

/* ハードウェア情報 */
#define CDC_HW_HFLAG(hw)            ((hw)->hflag)
#define CDC_HW_VER(hw)              ((hw)->ver)
#define CDC_HW_MPVER(hw)            ((hw)->mpver)
#define CDC_HW_DRV(hw)              ((hw)->drv)
#define CDC_HW_REV(hw)              ((hw)->rev)

/* ＣＤ位置パラメータ */
#define CDC_POS_PTYPE(pos)          ((pos)->ptype)
#define CDC_POS_FAD(pos)            ((pos)->pbody.fad)
#define CDC_POS_TNO(pos)            ((pos)->pbody.trkidx.tno)
#define CDC_POS_IDX(pos)            ((pos)->pbody.trkidx.idx)

/* ＣＤ再生パラメータ */
#define CDC_PLY_START(ply)          ((ply)->start)
#define CDC_PLY_END(ply)            ((ply)->end)
#define CDC_PLY_PMODE(ply)          ((ply)->pmode)

#define CDC_PLY_STYPE(ply)          CDC_POS_PTYPE(&CDC_PLY_START(ply))
#define CDC_PLY_SFAD(ply)           CDC_POS_FAD(&CDC_PLY_START(ply))
#define CDC_PLY_STNO(ply)           CDC_POS_TNO(&CDC_PLY_START(ply))
#define CDC_PLY_SIDX(ply)           CDC_POS_IDX(&CDC_PLY_START(ply))

#define CDC_PLY_ETYPE(ply)          CDC_POS_PTYPE(&CDC_PLY_END(ply))
#define CDC_PLY_EFAS(ply)           CDC_POS_FAD(&CDC_PLY_END(ply))
#define CDC_PLY_ETNO(ply)           CDC_POS_TNO(&CDC_PLY_END(ply))
#define CDC_PLY_EIDX(ply)           CDC_POS_IDX(&CDC_PLY_END(ply))

/* サブヘッダ条件 */
#define CDC_SUBH_FN(subh)           ((subh)->fn)
#define CDC_SUBH_CN(subh)           ((subh)->cn)
#define CDC_SUBH_SMMSK(subh)        ((subh)->smmsk)
#define CDC_SUBH_SMVAL(subh)        ((subh)->smval)
#define CDC_SUBH_CIMSK(subh)        ((subh)->cimsk)
#define CDC_SUBH_CIVAL(subh)        ((subh)->cival)

/* セクタ情報 */
#define CDC_SCT_FAD(sct)            ((sct)->fad)
#define CDC_SCT_FN(sct)             ((sct)->fn)
#define CDC_SCT_CN(sct)             ((sct)->cn)
#define CDC_SCT_SM(sct)             ((sct)->sm)
#define CDC_SCT_CI(sct)             ((sct)->ci)

/* ファイル情報 */
#define CDC_FILE_FAD(file)          ((file)->fad)
#define CDC_FILE_SIZE(file)         ((file)->size)
#define CDC_FILE_UNIT(file)         ((file)->unit)
#define CDC_FILE_GAP(file)          ((file)->gap)
#define CDC_FILE_FN(file)           ((file)->fn)
#define CDC_FILE_ATR(file)          ((file)->atr)

/*******************************************************************
*       処理マクロ
*******************************************************************/

/* ＣＤステータス情報からステータスコードを取得 */
#define CDC_GET_STC(stat)       (CDC_STAT_STATUS(stat) & CDC_STC_MSK)

/* ＣＤステータス情報からリピート回数を取得 */
#define CDC_GET_REPEAT(stat)    (CDC_STAT_FLGREP(stat) & 0x0f)

/* MPEGが使用可能か調べる（TRUE：使用可能、FALSE：使用不可） */
/* 注意：このマクロはマニュアルに記述しない */
#define CDC_IS_MPEG_ENA(hw)     (CDC_HW_MPVER(hw) != 0)

/*******************************************************************
*       データ型の宣言
*******************************************************************/

/* ＣＤステータス情報（ステータス＋ＣＤレポート） */
typedef struct {
    uint8_t   status;         /* ステータス */
    struct {                /* ＣＤレポート */
        uint8_t   flgrep;     /* ＣＤフラグとリピート回数 */
        uint8_t   ctladr;     /* CONTROL/ADRバイト */
        uint8_t   tno;        /* トラック番号 */
        uint8_t   idx;        /* インデックス番号 */
        int32_t  fad;        /* フレームアドレス */
    } report;
} CdcStat;

/* ハードウェア情報 */
typedef struct {
    uint8_t   hflag;          /* ハードウェアフラグ */
    uint8_t   ver;            /* ＣＤブロックのバージョン情報 */
    uint8_t   mpver;          /* MPEGのバージョン情報 */
    uint8_t   drv;            /* ＣＤドライブ情報 */
    uint8_t   rev;            /* ＣＤブロックのリビジョン情報 */
} CdcHw;

/* ＣＤ位置パラメータ */
typedef struct {
    int32_t ptype;           /* 位置タイプ（位置パラメータの種類の指定）*/
    union {
        int32_t fad;         /* フレームアドレス、セクタ数 */
        struct {
            uint8_t tno;      /* トラック番号 */
            uint8_t idx;      /* インデックス番号 */
        } trkidx;
    } pbody;
} CdcPos;

/* ＣＤ再生パラメータ */
typedef struct {
    CdcPos  start;          /* 開始位置 */
    CdcPos  end;            /* 終了位置 */
    uint8_t   pmode;          /* 再生モード（ﾋﾟｯｸｱｯﾌﾟ移動、繰り返し回数）*/
} CdcPly;

/* サブヘッダ条件 */
typedef struct {
    uint8_t   fn;             /* ファイル番号 */
    uint8_t   cn;             /* チャネル番号 */
    uint8_t   smmsk;          /* サブモードのマスクパターン */
    uint8_t   smval;          /* サブモードの比較値 */
    uint8_t   cimsk;          /* コーディング情報のマスクパターン */
    uint8_t   cival;          /* コーディング情報の比較値 */
} CdcSubh;

/* セクタ情報 */
typedef struct {
    int32_t  fad;            /* フレームアドレス */
    uint8_t   fn;             /* ファイル番号 */
    uint8_t   cn;             /* チャネル番号 */
    uint8_t   sm;             /* サブモード */
    uint8_t   ci;             /* コーディング情報 */
} CdcSct;

/* ファイル情報 */
typedef struct {
    int32_t  fad;            /* ファイル先頭フレームアドレス */
    int32_t  size;           /* ファイルサイズ（バイト数） */
    uint8_t   unit;           /* ファイルユニットサイズ */
    uint8_t   gap;            /* ギャップサイズ */
    uint8_t   fn;             /* ファイル番号 */
    uint8_t   atr;            /* ファイルアトリビュート */
} CdcFile;

/*******************************************************************
*       変数宣言
*******************************************************************/

/*******************************************************************
*       関数宣言
*******************************************************************/

/* cdc_cmn.c */
int32_t  CDC_GetCurStat(CdcStat *stat);
int32_t  CDC_GetLastStat(CdcStat *stat);
int32_t  CDC_GetHwInfo(CdcHw *hw);
int32_t  CDC_TgetToc(uint32_t *toc);
int32_t  CDC_GetSes(int32_t sesno, uint32_t *ses);
int32_t  CDC_CdInit(int32_t iflag, int32_t stnby, int32_t ecc, int32_t retry);
int32_t  CDC_CdOpen(void);
int32_t  CDC_DataReady(int32_t dtype);
int32_t  CDC_DataEnd(int32_t *cdwnum);
int32_t  CDC_GetPeriStat(CdcStat *stat);

/* cdc_drv.c */
int32_t  CDC_CdPlay(CdcPly *ply);
int32_t  CDC_CdSeek(CdcPos *pos);
int32_t  CDC_CdScan(int32_t scandir);

/* cdc_scd.c */
int32_t  CDC_TgetScdQch(uint16_t *qcode);
int32_t  CDC_TgetScdRwch(uint16_t *rwcode, int32_t *scdflag);

/* cdc_dev.c */
int32_t  CDC_CdSetCon(int32_t filtno);
int32_t  CDC_CdGetCon(int32_t *filtno);
int32_t  CDC_CdGetLastBuf(int32_t *bufno);

/* cdc_sel.c */
int32_t  CDC_SetFiltRange(int32_t filtno, int32_t fad, int32_t fasnum);
int32_t  CDC_GetFiltRange(int32_t filtno, int32_t *fad, int32_t *fasnum);
int32_t  CDC_SetFiltSubh(int32_t filtno, CdcSubh *subh);
int32_t  CDC_GetFiltSubh(int32_t filtno, CdcSubh *subh);
int32_t  CDC_SetFiltMode(int32_t filtno, int32_t fmode);
int32_t  CDC_GetFiltMode(int32_t filtno, int32_t *fmode);
int32_t  CDC_SetFiltCon(int32_t filtno, int32_t cflag, int32_t bufno,
                       int32_t flnout);
int32_t  CDC_GetFiltCon(int32_t filtno, int32_t *bufno, int32_t *flnout);
int32_t  CDC_ResetSelector(int32_t rflag, int32_t bufno);

/* cdc_bif.c */
int32_t  CDC_GetBufSiz(int32_t *totalsiz, int32_t *bufnum, int32_t *freesiz);
int32_t  CDC_GetSctNum(int32_t bufno, int32_t *snum);
int32_t  CDC_CalActSiz(int32_t bufno, int32_t spos, int32_t snum);
int32_t  CDC_GetActSiz(int32_t *actwnum);
int32_t  CDC_GetSctInfo(int32_t bufno, int32_t spos, CdcSct *sct);
int32_t  CDC_ExeFadSearch(int32_t bufno, int32_t spos, int32_t fad);
int32_t  CDC_GetFadSearch(int32_t *bufno, int32_t *spos, int32_t *fad);

/* cdc_bio.c */
int32_t  CDC_SetSctLen(int32_t getslen, int32_t putslen);
int32_t  CDC_GetSctData(int32_t bufno, int32_t spos, int32_t snum);
int32_t  CDC_DelSctData(int32_t bufno, int32_t spos, int32_t snum);
int32_t  CDC_GetdelSctData(int32_t bufno, int32_t spos, int32_t snum);
int32_t  CDC_PutSctData(int32_t filtno, int32_t snum);
int32_t  CDC_CopySctData(int32_t srcbn, int32_t spos, int32_t snum,
                        int32_t dstfln);
int32_t  CDC_MoveSctData(int32_t srcbn, int32_t spos, int32_t snum,
                        int32_t dstfln);
int32_t  CDC_GetCopyErr(int32_t *cpyerr);

/* cdc_cfs.c */
int32_t  CDC_ChgDir(int32_t filtno, int32_t fid);
int32_t  CDC_ReadDir(int32_t filtno, int32_t fid);
int32_t  CDC_GetFileScope(int32_t *fid, int32_t *infnum, bool *drend);
int32_t  CDC_TgetFileInfo(int32_t fid, CdcFile *file);
int32_t  CDC_ReadFile(int32_t filtno, int32_t fid, int32_t offset);
int32_t  CDC_AbortFile(void);

/* cdc_reg.c */
uint32_t  *CDC_GetDataPtr(void);
int32_t  CDC_GetHirqReq(void);
void    CDC_ClrHirqReq(int32_t bitpat);
int32_t  CDC_GetHirqMsk(void);
void    CDC_SetHirqMsk(int32_t bitpat);
uint32_t  *CDC_GetMpegPtr(void);



/********************************************************************/
/********************************************************************/
/********************************************************************/
/*------------------------------------------------------------------*/
/*------------------------- 《MPEGパート》 -------------------------*/
/*------------------------------------------------------------------*/
/********************************************************************/
/********************************************************************/
/********************************************************************/



/*******************************************************************
*       定数マクロ
*******************************************************************/

/* ウィンドウサイズと座標の最大値 */
#define CDC_MPNT_NSX    352           /* NTSC通常Ｘ方向サイズ(352) */
#define CDC_MPNT_NSY    240           /* NTSC通常Ｙ方向サイズ(240) */
#define CDC_MPNT_NPX (CDC_MPNT_NSX-1) /* NTSC通常Ｘ方向座標  (351) */
#define CDC_MPNT_NPY (CDC_MPNT_NSY-1) /* NTSC通常Ｙ方向座標  (239) */

#define CDC_MPNT_HSX (2*CDC_MPNT_NSX) /* NTSC高精細Ｘ方向サイズ(704) */
#define CDC_MPNT_HSY (2*CDC_MPNT_NSY) /* NTSC高精細Ｙ方向サイズ(480) */
#define CDC_MPNT_HPX (CDC_MPNT_HSX-1) /* NTSC高精細Ｘ方向座標  (703) */
#define CDC_MPNT_HPY (CDC_MPNT_HSY-1) /* NTSC高精細Ｙ方向座標  (479) */

#define CDC_MPPL_NSX    352           /* PAL 通常Ｘ方向サイズ(352) */
#define CDC_MPPL_NSY    288           /* PAL 通常Ｙ方向サイズ(288) */
#define CDC_MPPL_NPX (CDC_MPPL_NSX-1) /* PAL 通常Ｘ方向座標  (351) */
#define CDC_MPPL_NPY (CDC_MPPL_NSY-1) /* PAL 通常Ｙ方向座標  (287) */

#define CDC_MPPL_HSX (2*CDC_MPPL_NSX) /* PAL 高精細Ｘ方向サイズ(704) */
#define CDC_MPPL_HSY (2*CDC_MPPL_NSY) /* PAL 高精細Ｙ方向サイズ(576) */
#define CDC_MPPL_HPX (CDC_MPPL_HSX-1) /* PAL 高精細Ｘ方向座標  (703) */
#define CDC_MPPL_HPY (CDC_MPPL_HSY-1) /* PAL 高精細Ｙ方向座標  (575) */

/* MPEG/Video動作状態（MPEG動作ステータスのbit0～2） */
#define CDC_MPASTV_STOP     0x01    /* 停止 */
#define CDC_MPASTV_PRE1     0x02    /* 準備１ */
#define CDC_MPASTV_PRE2     0x03    /* 準備２ */
#define CDC_MPASTV_TRNS     0x04    /* 転送（再生） */
#define CDC_MPASTV_CHG      0x05    /* 切り替え */
#define CDC_MPASTV_RCV      0x06    /* 復活処理 */

/* MPEGデコード状態（MPEG動作ステータスのbit3） */
#define CDC_MPASTD_STOP     0x08    /* MPEGデコード停止 */

/* MPEG/Audio動作状態（MPEG動作ステータスのbit4～6） */
#define CDC_MPASTA_STOP     0x10    /* 停止 */
#define CDC_MPASTA_PRE1     0x20    /* 準備１ */
#define CDC_MPASTA_PRE2     0x30    /* 準備２ */
#define CDC_MPASTA_TRNS     0x40    /* 転送（再生） */
#define CDC_MPASTA_CHG      0x50    /* 切り替え */
#define CDC_MPASTA_RCV      0x60    /* 復活処理 */

/* MPEG/Audioステータス */
#define CDC_MPSTA_DEC       0x01    /* オーディオデコード動作フラグ */
#define CDC_MPSTA_ILG       0x08    /* オーディオイリーガルフラグ */
#define CDC_MPSTA_BEMPTY    0x10    /* オーディオ用バッファ区画空フラグ */
#define CDC_MPSTA_ERR       0x20    /* オーディオエラーフラグ */
#define CDC_MPSTA_OUTL      0x40    /* 左チャンネル出力フラグ */
#define CDC_MPSTA_OUTR      0x80    /* 右チャンネル出力フラグ */

/* MPEG/Videoステータス */
#define CDC_MPSTV_DEC       0x0001  /* ビデオデコード動作フラグ */
#define CDC_MPSTV_DISP      0x0002  /* 表示フラグ */
#define CDC_MPSTV_PAUSE     0x0004  /* ポーズフラグ */
#define CDC_MPSTV_FREEZE    0x0008  /* フリーズフラグ */
#define CDC_MPSTV_LSTPIC    0x0010  /* 最後ピクチャ表示フラグ */
#define CDC_MPSTV_FIELD     0x0020  /* 奇数フィールドフラグ */
#define CDC_MPSTV_UPDPIC    0x0040  /* ピクチャ更新フラグ */
#define CDC_MPSTV_ERR       0x0080  /* ビデオエラーフラグ */
#define CDC_MPSTV_RDY       0x0100  /* 出力準備完了フラグ */
#define CDC_MPSTV_1STPIC    0x0800  /* 先頭ピクチャ表示フラグ */
#define CDC_MPSTV_BEMPTY    0x1000  /* ビデオ用バッファ区画空フラグ */

/* MPEG割り込み要因 */
#define CDC_MPINT_VSRDY  0x00000001 /* ビデオストリーム準備完了 */
#define CDC_MPINT_VSCHG  0x00000002 /* ビデオストリーム切り替え完了 */
#define CDC_MPINT_VORDY  0x00000004 /* ビデオ出力準備完了 */
#define CDC_MPINT_VOSTRT 0x00000008 /* ビデオ出力開始 */
#define CDC_MPINT_VDERR  0x00000010 /* ビデオデコードエラー */
#define CDC_MPINT_VSERR  0x00000020 /* ビデオストリームデータエラー */
#define CDC_MPINT_VBERR  0x00000040 /* ビデオバッファ区画接続エラー */
#define CDC_MPINT_VNERR  0x00000080 /* 次発ﾋﾞﾃﾞｵｽﾄﾘｰﾑのデータエラー */
#define CDC_MPINT_PSTRT  0x00000100 /* ピクチャスタート検出 */
#define CDC_MPINT_GSTRT  0x00000200 /* GOPスタート検出 */
#define CDC_MPINT_SQEND  0x00000400 /* シーケンスエンド検出 */
#define CDC_MPINT_SQSTRT 0x00000800 /* シーケンススタート検出 */
#define CDC_MPINT_VTRG   0x00001000 /* ビデオセクタのトリガビット検出 */
#define CDC_MPINT_VEOR   0x00002000 /* ビデオセクタのEORビット検出 */
#define CDC_MPINT_ATRG   0x00004000 /* オーディオセクタのﾄﾘｶﾞﾋﾞｯﾄ検出 */
#define CDC_MPINT_AEOR   0x00008000 /* オーディオセクタのEORビット検出 */
#define CDC_MPINT_ASRDY  0x00010000 /* オーディオストリーム準備完了 */
#define CDC_MPINT_ASCHG  0x00020000 /* オーディオストリーム切り替え完了 */
#define CDC_MPINT_AORDY  0x00040000 /* オーディオ出力準備完了 */
#define CDC_MPINT_AOSTRT 0x00080000 /* オーディオ出力開始 */
#define CDC_MPINT_ADERR  0x00100000 /* オーディオデコードエラー */
#define CDC_MPINT_ASERR  0x00200000 /* オーディオストリームデータエラー */
#define CDC_MPINT_ABERR  0x00400000 /* オーディオバッファ区画接続エラー */
#define CDC_MPINT_ANERR  0x00800000 /* 次発ｵｰﾃﾞｨｵｽﾄﾘｰﾑのデータエラー */

/* 接続モード（MPEGデコーダ接続先パラメータ内） */
#define CDC_MPCMOD_EOR      0x01    /* EORビットで切り替える */
#define CDC_MPCMOD_SEC      0x02    /* システムエンド(SEC)で切り替える */
#define CDC_MPCMOD_DEL      0x04    /* セクタの消去 */
#define CDC_MPCMOD_IGPTS    0x08    /* PTS識別をしない */
#define CDC_MPCMOD_VCLR     0x10    /* VBVのクリア */
#define CDC_MPCMOD_VWCLR    0x20    /* VBV+WBCのクリア */
#define CDC_MPCMOD_BEF      0x40    /* 後方絞りの前で終了条件判定 */

/* レイヤ指定（MPEGデコーダ接続先パラメータ内） */
#define CDC_MPLAY_SYS       0x00    /* システムレイヤ */
#define CDC_MPLAY_AUDIO     0x01    /* オーディオレイヤ */
#define CDC_MPLAY_VIDEO     0x01    /* ビデオレイヤ */

/* MPEG/Videoのピクチャサーチ指定（MPEGデコーダ接続先パラメータ内） */
#define CDC_MPSRCH_OFF      0x00    /* ピクチャサーチをしない */
#define CDC_MPSRCH_VIDEO    0x80    /* ピクチャサーチをする */
#define CDC_MPSRCH_AV       0xc0    /* ﾋﾟｸﾁｬｻｰﾁに合わせｵｰﾃﾞｨｵﾃﾞｰﾀを破棄 */

/* ストリームモード（MPEGストリームパラメータ内） */
#define CDC_MPSMOD_SNSET    0x01    /* ストリーム番号の設定を行う */
#define CDC_MPSMOD_SNIDF    0x02    /* ストリーム番号を識別する */
#define CDC_MPSMOD_CNSET    0x10    /* チャネル番号の設定を行う */
#define CDC_MPSMOD_CNIDF    0x20    /* チャネル番号を識別する */

/* 補間モード（画面特殊効果パラメータ内） */
#define CDC_MPITP_YH        0x01    /* Ｙの水平補間をする */
#define CDC_MPITP_CH        0x02    /* Ｃの水平補間をする */
#define CDC_MPITP_YV        0x04    /* Ｙの垂直補間をする */
#define CDC_MPITP_CV        0x08    /* Ｃの垂直補間をする */

/* 透明ビットモード（画面特殊効果パラメータ内） */
#define CDC_MPTRP_DFL       0x00    /* 通常（透明ビット処理をしない） */
#define CDC_MPTRP_64        0x01    /* 輝度64 */
#define CDC_MPTRP_128       0x02    /* 輝度128 */
#define CDC_MPTRP_256       0x03    /* 輝度256 */
#define CDC_MPTRP_MAG       0x04    /* 透明領域の拡大をする */

/* ぼかしモード（画面特殊効果パラメータ内） */
#define CDC_MPSOFT_ON       0x01    /* ぼかしをかける */

/* MPEG/Audioのミュート */
#define CDC_MPMUT_DFL       0x04    /* 省略値（ミュートしない） */
#define CDC_MPMUT_R         0x01    /* 右チャンネルのミュートをする */
#define CDC_MPMUT_L         0x02    /* 左チャンネルのミュートをする */

/*******************************************************************
*       列挙定数
*******************************************************************/

/* エラーコード */
enum CdcMpErrCode {
    CDC_ERR_MP_COMU = -20   /* MPCMフラグが１になっていない */
};

/* 次ストリームフラグ（現ストリームと次ストリームの指定） */
enum CdcMpStf {
    CDC_MPSTF_CUR  = 0,     /* 現在の接続先／ストリームの指定（先発） */
    CDC_MPSTF_NEXT = 1      /* 　次の接続先／ストリームの指定（次発） */
};

/* ピクチャタイプ（下位３ビットが有効） */
enum CdcMpPict {
    CDC_MPPICT_I = 0x01,    /* Ｉピクチャ */
    CDC_MPPICT_P = 0x02,    /* Ｐピクチャ */
    CDC_MPPICT_B = 0x03,    /* Ｂピクチャ */
    CDC_MPPICT_D = 0x04     /* Ｄピクチャ */
};

/* MPEG動作モード */
enum CdcMpAct {
    CDC_MPACT_NMOV = 0,     /* 動画再生モード */
    CDC_MPACT_NSTL = 1,     /* 静止画再生モード */
    CDC_MPACT_HMOV = 2,     /* 高精細の動画再生モード（未対応） */
    CDC_MPACT_HSTL = 3,     /* 高精細の静止画再生モード */
    CDC_MPACT_SBUF = 4      /* MPEGセクタバッファモード */
};

/* デコードタイミング */
enum CdcMpDec {
    CDC_MPDEC_VSYNC = 0,    /* VSYNC同期によるデコード */
    CDC_MPDEC_HOST  = 1     /* ホスト同期によるデコード */
};

/* 画像データの出力先 */
enum CdcMpOut {
    CDC_MPOUT_VDP2 = 0,     /* VDP2へ出力 */
    CDC_MPOUT_HOST = 1      /* ホストへ出力 */
};

/* MPEG再生モード */
enum CdcMpPly {
    CDC_MPPLY_SYNC = 0,     /* 同期再生モード */
    CDC_MPPLY_INDP = 1      /* 独立再生モード */
};

/* MPEGデコーダの転送モード */
enum CdcMpTrn {
    CDC_MPTRN_AUTO  = 0,    /* 自動転送モード */
    CDC_MPTRN_FORCE = 1     /* 強制転送モード */
};

/* MPEGデコーダの接続切り替えフラグ */
enum CdcMpCof {
    CDC_MPCOF_ABT = 0,      /* 切り離し（強制終了） */
    CDC_MPCOF_CHG = 1       /* 強制切り替え */
};

/* MPEG/Audioのクリア方式 */
enum CdcMpCla {
    CDC_MPCLA_OFF = 0,      /* クリアしない */
    CDC_MPCLA_ON  = 1       /* 即座に１セクタのバッファをクリアする */
};

/* MPEG/Videoのクリア方式 */
enum CdcMpClv {
    CDC_MPCLV_FRM = 0,      /* 即座にVBVとMPEGﾌﾚｰﾑﾊﾞｯﾌｧをクリアする */
    CDC_MPCLV_VBV = 2       /* 次のIまたはPﾋﾟｸﾁｬｽﾀｰﾄでVBVをクリアする */
};

/*******************************************************************
*       構造体アクセス処理マクロ
*******************************************************************/

/* MPEGステータス情報 */
#define CDC_MPSTAT_STS(mpstat)      ((mpstat)->status)
#define CDC_MPSTAT_AST(mpstat)      ((mpstat)->report.actst)
#define CDC_MPSTAT_PICT(mpstat)     ((mpstat)->report.pict)
#define CDC_MPSTAT_STA(mpstat)      ((mpstat)->report.stat_a)
#define CDC_MPSTAT_STV(mpstat)      ((mpstat)->report.stat_v)
#define CDC_MPSTAT_VCNT(mpstat)     ((mpstat)->report.vcnt)

/* タイムコード */
#define CDC_MPTC_HOUR(mptc)         ((mptc)->hour)
#define CDC_MPTC_MIN(mptc)          ((mptc)->min)
#define CDC_MPTC_SEC(mptc)          ((mptc)->sec)
#define CDC_MPTC_PIC(mptc)          ((mptc)->pic)

/* MPEGデコーダ接続先パラメータ */
#define CDC_MPCON_CMOD(mpcon)       ((mpcon)->cmod)
#define CDC_MPCON_LAY(mpcon)        ((mpcon)->lay)
#define CDC_MPCON_BN(mpcon)         ((mpcon)->bn)

/* MPEGストリームパラメータ */
#define CDC_MPSTM_SMOD(mpstm)       ((mpstm)->smod)
#define CDC_MPSTM_ID(mpstm)         ((mpstm)->id)
#define CDC_MPSTM_CN(mpstm)         ((mpstm)->cn)

/* 画面特殊効果パラメータ */
#define CDC_MPVEF_ITP(mpvef)        ((mpvef)->itp)
#define CDC_MPVEF_TRP(mpvef)        ((mpvef)->trp)
#define CDC_MPVEF_MOZH(mpvef)       ((mpvef)->moz_h)
#define CDC_MPVEF_MOZV(mpvef)       ((mpvef)->moz_v)
#define CDC_MPVEF_SOFTH(mpvef)      ((mpvef)->soft_h)
#define CDC_MPVEF_SOFTV(mpvef)      ((mpvef)->soft_v)

/*******************************************************************
*       処理マクロ
*******************************************************************/

/* MPEGステータス情報からステータスコードを取得する */
#define CDC_MPGET_STC(mpstat)   (CDC_MPSTAT_STS(mpstat) & CDC_STC_MSK)

/* MPEGステータス情報からMPEG/Video動作状態を取得する */
#define CDC_MPGET_ASTV(mpstat)   (CDC_MPSTAT_AST(mpstat) & 0x07)

/* MPEGステータス情報からMPEG/Audio動作状態を取得する */
#define CDC_MPGET_ASTA(mpstat)   (CDC_MPSTAT_AST(mpstat) & 0x70)

/*******************************************************************
*       データ型の宣言
*******************************************************************/

/* MPEGステータス情報（ステータス＋MPEGレポート） */
typedef struct {
    uint8_t   status;         /* ステータス */
    struct {                /* MPEGレポート */
        uint8_t   actst;      /* MPEG動作ステータス */
        uint8_t   pict;       /* ピクチャ情報 */
        uint8_t   stat_a;     /* MPEG/Audioステータス */
        uint16_t  stat_v;     /* MPEG/Videoステータス */
        uint16_t  vcnt;       /* 動作区間（VSYNC）カウンタ */
    } report;
} CdcMpStat;

/* タイムコード */
typedef struct {
    uint8_t   hour;           /* 時間 */
    uint8_t   min;            /* 分 */
    uint8_t   sec;            /* 秒 */
    uint8_t   pic;            /* ピクチャ */
} CdcMpTc;

/* MPEGデコーダ接続先パラメータ */
typedef struct {
    uint8_t   cmod;           /* 接続モード */
    uint8_t   lay;            /* レイヤ指定とピクチャサーチ指定 */
    uint8_t   bn;             /* バッファ区画番号 */
} CdcMpCon;

/* MPEGストリームパラメータ */
typedef struct {
    uint8_t   smod;           /* ストリームモード */
    uint8_t   id;             /* ストリーム番号 */
    uint8_t   cn;             /* チャネル番号 */
} CdcMpStm;

/* 画面特殊効果パラメータ */
typedef struct {
    uint8_t   itp;            /* 補間モード */
    uint8_t   trp;            /* 透明ビットモード */
    uint8_t   moz_h;          /* 水平方向モザイクモード */
    uint8_t   moz_v;          /* 垂直方向モザイクモード */
    uint8_t   soft_h;         /* 水平方向ぼかしモード */
    uint8_t   soft_v;         /* 垂直方向ぼかしモード */
} CdcMpVef;

/*******************************************************************
*       変数宣言
*******************************************************************/

/*******************************************************************
*       関数宣言
*******************************************************************/

/* cdc_mdc.c */
int32_t  CDC_MpGetCurStat(CdcMpStat *mpstat);
int32_t  CDC_MpGetLastStat(CdcMpStat *mpstat);
int32_t  CDC_MpGetInt(int32_t *intreq);
int32_t  CDC_MpSetIntMsk(int32_t intmsk);
int32_t  CDC_MpInit(bool sw);
int32_t  CDC_MpSetMode(int32_t actmod, int32_t dectim, int32_t out,
                      int32_t scnmod);
int32_t  CDC_MpPlay(int32_t plymod, int32_t tmod_a, int32_t tmod_v,
                   int32_t dec_v);
int32_t  CDC_MpSetDec(int32_t mute, int32_t pautim, int32_t frztim);
int32_t  CDC_MpOutDsync(int32_t fbn);
int32_t  CDC_MpGetTc(int32_t *bnk, int32_t *pictyp, int32_t *tr, CdcMpTc *mptc);
int32_t  CDC_MpGetPts(int32_t *pts_a);

/* cdc_mst.c */
int32_t  CDC_MpSetCon(int32_t next, CdcMpCon *mpcon_a, CdcMpCon *mpcon_v);
int32_t  CDC_MpGetCon(int32_t next, CdcMpCon *mpcon_a, CdcMpCon *mpcon_v);
int32_t  CDC_MpChgCon(int32_t chg_a, int32_t chg_v,
                     int32_t clr_a, int32_t clr_v);
int32_t  CDC_MpSetStm(int32_t next, CdcMpStm *mpstm_a, CdcMpStm *mpstm_v);
int32_t  CDC_MpGetStm(int32_t next, CdcMpStm *mpstm_a, CdcMpStm *mpstm_v);
int32_t  CDC_MpGetPictSiz(int32_t *siz_h, int32_t *siz_v);

/* cdc_mwn.c */
int32_t  CDC_MpDisp(bool dspsw, int32_t fbn);
int32_t  CDC_MpSetWinFpos(bool chgflg, int32_t fpx, int32_t fpy);
int32_t  CDC_MpSetWinFrat(bool chgflg, int32_t frx, int32_t fry);
int32_t  CDC_MpSetWinDpos(bool chgflg, int32_t dpx, int32_t dpy);
int32_t  CDC_MpSetWinDsiz(bool chgflg, int32_t dsx, int32_t dsy);
int32_t  CDC_MpSetWinDofs(bool chgflg, int32_t dox, int32_t doy);
int32_t  CDC_MpSetBcolor(int32_t bcolor);
int32_t  CDC_MpSetFade(int32_t gain_y, int32_t gain_c);
int32_t  CDC_MpSetVeff(CdcMpVef *mpvef);

/* cdc_mfb.c */
int32_t  CDC_MpGetImg(int32_t *dwnum);
int32_t  CDC_MpSetImgPos(int32_t fbn, int32_t ipx, int32_t ipy);
int32_t  CDC_MpSetImgSiz(int32_t fbn, int32_t isx, int32_t isy);
int32_t  CDC_MpReadImg(int32_t srcfbn,
                      int32_t fln_y, int32_t fln_cr, int32_t fln_cb);
int32_t  CDC_MpWriteImg(int32_t bn_y, int32_t bn_cr, int32_t bn_cb,
                       int32_t dstfbn, int32_t clrmod);

/* cdc_mbu.c */
int32_t  CDC_MpReadSct(int32_t srcmsp, int32_t snum, int32_t dstfln);
int32_t  CDC_MpWriteSct(int32_t srcbn, int32_t sp, int32_t snum,
                       int32_t dstmsp);

/* cdc_mls.c */
/* 注意：以下の関数は非公開とする */
int32_t  CDC_MpGetLsi(int32_t r_sw, int32_t reg_no, int32_t *rdata);
int32_t  CDC_MpSetLsi(int32_t rwsw, int32_t reg_no, int32_t wdata,
                     int32_t *rdata);

#endif  /* ifndef SEGA_CDC_H */

