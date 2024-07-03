/******************************************************************************
 *      ソフトウェアライブラリ
 *
 *      Copyright (c) 1994,1995,1996 SEGA
 *
 * Library:ファイルシステム
 * Module :ライブラリヘッダ
 * File   :sega_gfs.h
 * Date   :1996-02-18
 * Version:2.11
 * Author :H.T.
 *
 *************************************************************************** */

#if !defined(SEGA_GFS_H)
#define SEGA_GFS_H

/*****************************************************************************
 *      インクルードファイル
 *****************************************************************************/
#include "sega_cdc.h"

/* バージョン   */
                            /*             1         2         3         4  */
                            /*    1234567890123456789012345678901234567890  */
                            /*    ++++++++++++++++++++++++++++++++          */
#if defined(USE_SGL)
    #define GFS_VERSION_STR     ("GFS_SGL Version 2.11 1996-02-18")
#else
    #define GFS_VERSION_STR     ("GFS_SBL Version 2.11 1996-02-18")
#endif
#define GFS_VERSION_LEN 32

/*****************************************************************************
 *      定数マクロ
 *****************************************************************************/
#define GFS_FNAME_LEN   12              /* ファイル名の長さ                 */

/* ファイルの属性 */
#define GFS_ATR_DIR     0x80            /* ディレクトリ                     */
#define GFS_ATR_CDDA    0x40            /* CDDAファイル                     */
#define GFS_ATR_INTLV   0x20            /* インターリーブセクタを含む       */
#define GFS_ATR_FORM2   0x10            /* Form2セクタを含む                */
#define GFS_ATR_FORM1   0x08            /* Form1セクタを含む                */
#define GFS_ATR_END_TBL 0x01            /* ディレクトリテーブルの最後       */

#define GFS_BUFSIZ_INF  -1              /* 読み込み領域サイズに制限はない   */
#define GFS_RPARA_DFL   0x7fffffff      /* 読み込みパラメータの初期値       */

/* デモデモ用   */
#define GFS_DDS_ADDR    0x6000ccc       /* デモデモ情報参照領域のアドレス   */
#define GFS_DDS_EXEC    0x01            /* DDS実行中を示す                  */
#define GFS_DDS_ID()    (*(uint8_t *)(GFS_DDS_ADDR+0))  /* DDS識別子の値      */
#define GFS_DDS_TNO()   (*(uint8_t *)(GFS_DDS_ADDR+1))  /* 先頭CDDAトラックNo */
#define GFS_DDS_FID()   (*(uint16_t *)(GFS_DDS_ADDR+2)) /* ディレクトリ識別子 */
#define GFS_IS_DDS()    (GFS_DDS_ID()==GFS_DDS_EXEC)  /* DDS実行中の判断    */

/* エラーコード */
#define GFS_ERR_OK              0       /* 正常終了                         */
#define GFS_ERR_CDRD            (-1)    /* CDリードエラー                   */
#define GFS_ERR_CDNODISC        (-2)    /* CDがセットされていない           */
#define GFS_ERR_CDROM           (-3)    /* ディスクがCD-ROMでない           */
#define GFS_ERR_DIRTBL          (-4)    /* ディレクトリ管理テーブルが不正   */
#define GFS_ERR_OPENMAX         (-5)    /* 最大オープン数の値が不正         */
#define GFS_ERR_DIR             (-6)    /* 指定ファイルはディレクトリでない */
#define GFS_ERR_CDBFS           (-7)    /* CDブロックファイルシステムの     */
                                        /* 使用宣言していない               */
#define GFS_ERR_NONAME          (-8)    /* ファイル名を扱えない             */
#define GFS_ERR_NEXIST          (-9)    /* 指定されたファイルは存在しない   */
#define GFS_ERR_FID             (-10)   /* 不正なファイル識別子             */
#define GFS_ERR_HNDL            (-11)   /* ファイルハンドルが不正           */
#define GFS_ERR_SEEK            (-12)   /* シーク位置が不正                 */
#define GFS_ERR_ORG             (-13)   /* 基準位値が不正                   */
#define GFS_ERR_NUM             (-14)   /* バイト数が負                     */
#define GFS_ERR_OFS             (-15)   /* オフセットが不正                 */
#define GFS_ERR_FBUSY           (-16)   /* 指定ファイルの処理が残っている   */
#define GFS_ERR_PARA            (-17)   /* パラメータが不正                 */
#define GFS_ERR_BUSY            (-18)   /* ライブラリ関数実行中             */
#define GFS_ERR_NOHNDL          (-19)   /* ファイルハンドルに空きがない     */
#define GFS_ERR_PUINUSE         (-20)   /* ピックアップ動作中               */
#define GFS_ERR_ALIGN           (-21)   /* 作業領域が4byte境界にない        */
#define GFS_ERR_TMOUT           (-22)   /* タイムアウト                     */
#define GFS_ERR_CDOPEN          (-23)   /* トレイが開いている               */
#define GFS_ERR_BFUL            (-24)   /* バッファフルで読み込み中止       */
#define GFS_ERR_FATAL           (-25)   /* CDがFATAL状態                    */

/*****************************************************************************
 *      列挙定数
 *****************************************************************************/
/* アクセスモード(GfsFile:amode) */
enum GfsAccessMode {
    GFS_NWSTAT_NOACT = 0,               /* 何もしていない              */
    GFS_NWSTAT_FREAD,                   /* 読み込み中                  */
    GFS_NWSTAT_CDREAD                   /* ＣＤリード中                */
};


/* シークモード(GFS_Seek)               */
enum GfsSeekMode {
    GFS_SEEK_SET = 0,                   /* 先頭からシーク               */
    GFS_SEEK_CUR,                       /* 現在の位置からシーク         */
    GFS_SEEK_END                        /* 終端からシーク               */
};


/* 取り出しモード(GFS_SetGmode:gmode) */
enum GfsGetMode {
    GFS_GMODE_ERASE = 0,                /* 転送後、バッファから削除     */
    GFS_GMODE_RESIDENT                  /* ＣＤバッファに常駐           */
};


/* 転送モード(GFS_SetTmode) */
enum GfsTransMode {
    GFS_TMODE_SCU = 0,                  /* ＳＣＵによりＤＭＡ転送    */
    GFS_TMODE_SDMA0,                    /* ＤＭＡサイクルスチール転送   cn=0 */
    GFS_TMODE_SDMA1,                    /*                              cn=1 */
    GFS_TMODE_CPU,                      /* ソフトウェア転送 */
    GFS_TMODE_STM,                      /* ストリーム用転送 */
    GFS_TMODE_END
};

/* アクセスサーバ状態(GFS_NwExecServer) */
enum GfsServerStatus {
    GFS_SVR_COMPLETED = 0,              /* 全ファイルのアクセス終了     */
    GFS_SVR_BUSY,                       /* ファイルアクセス中           */
    GFS_SVR_CDPAUSE,                    /* ＣＤバッファフルの為一時停止 */
    GFS_SVR_ERROR                       /* エラー発生 */
};


/* ディレクトリ種別(GFS_LoadDir, GFS_SetDir)     */
enum GfsDirType {
    GFS_DIR_ID          = 0,            /* ファイル名によるアクセス不可 */
    GFS_DIR_NAME                        /* ファイル名によるアクセス可   */
};


/*****************************************************************************
 *      処理マクロ
 *****************************************************************************/
#define GFS_WORK_SIZE(open_max) \
            (sizeof(GfsMng) + ((open_max) - 1) * sizeof(GfsFile))

/*****************************************************************************
 *      構造体アクセス処理マクロ
 *****************************************************************************/

/* ディレクトリレコード構造体  */
#define GFS_DIR_REC(rec)        ((rec)->dirrec)
#define GFS_DIR_FAD(rec)        CDC_FILE_FAD(&GFS_DIR_REC(rec))
#define GFS_DIR_SIZE(rec)       CDC_FILE_SIZE(&GFS_DIR_REC(rec))
#define GFS_DIR_UNIT(rec)       CDC_FILE_UNIT(&GFS_DIR_REC(rec))
#define GFS_DIR_GAP(rec)        CDC_FILE_GAP(&GFS_DIR_REC(rec))
#define GFS_DIR_FN(rec)         CDC_FILE_FN(&GFS_DIR_REC(rec))
#define GFS_DIR_ATR(rec)        CDC_FILE_ATR(&GFS_DIR_REC(rec))
#define GFS_DIR_FNAME(rec)      ((rec)->fname)


/* ディレクトリ情報管理構造体 */
#define GFS_DIRTBL_TYPE(dirtbl)         ((dirtbl)->type)
#define GFS_DIRTBL_NDIR(dirtbl)         ((dirtbl)->ndir)
#define GFS_DIRTBL_DIRID(dirtbl)        ((dirtbl)->dir.dir_i)
#define GFS_DIRTBL_DIRNAME(dirtbl)      ((dirtbl)->dir.dir_n)


/* エラー状態 */
#define GFS_ERR_FUNC(err)       ((err)->func)
#define GFS_ERR_OBJ(err)        ((err)->obj)
#define GFS_ERR_CODE(err)       ((err)->code)


/*****************************************************************************
 *      データ型の定義
 *****************************************************************************/

typedef int32_t (*GfsTransFunc)(void *obj, int32_t nsct);

     /* ファイル情報 */
typedef struct {
    int32_t      fid;                    /* ファイル識別子 */
    CdcFile     finfo;
    int32_t      sctsz;                  /* セクタ長 */
    int32_t      nsct;                   /* セクタ数 */
    int32_t      lstrm;                  /* 最終セクタ中の無効データ数   */
} GfsFinfo;


/* CDバッファの資源管理 */
typedef struct {
    int32_t      bufno;                  /* バッファ区画番号 */
    int32_t      sctsz;                  /* ＣＤバッファのセクタサイズ   */
    int32_t      flt;                    /* 絞り番号 */
    CdcSubh     subh;                   /* サブヘッダ */
    int32_t      fmode;                  /* 絞りモード */
    int32_t      puid;                   /* 取得ピックアップのユーザID */
    int32_t      filepos;                /* 先頭データのファイル上の位置 */
    int32_t      sctpos;                 /* 転送位置 */
    /* 再生範囲 */
    int32_t      sfad;                   /* 開始FAD */
    int32_t      efad;                   /* 終了FAD */
} GfsCdRsrc;


/* SCSIファイルの仮想CDバッファ管理 */
typedef struct {
    int32_t      fid;                    /* SCSIファイル識別子   */
    int32_t      filepos;                /* 先頭データのファイル上の位置 */
    int32_t      sctpos;                 /* バッファ上のセクタ位置 */
    int32_t      sctnum;                 /* バッファ内のセクタ数 */
} GfsScsiRsrc;


/* メモリファイルの仮想CDバッファ管理 */
typedef struct {
    void        *data;                  /* データ領域           */
    int32_t      filepos;                /* 先頭データのファイル上の位置 */
    int32_t      sctpos;                 /* バッファ上のセクタ位置 */
    int32_t      sctnum;                 /* バッファ内のセクタ数 */
} GfsMemRsrc;


/* 読み込み元管理 */
typedef struct {
    /* 読み込み元管理 */
    int32_t      ftype;                  /* ファイル種別         */
    union {
        GfsCdRsrc       cd;             /* CDの資源             */
        GfsScsiRsrc     scsi;           /* SCSIファイルの資源   */
        GfsMemRsrc      mem;            /* メモリファイルの資源 */
    } rsrc;
} GfsDtsrc;


/* CDからの読み込みを管理する */
typedef struct {
    GfsFinfo    finfo;                  /* ファイル情報         */
    GfsDtsrc    dtsrc;                  /* 読み込み元           */
    int32_t      gmode;                  /* 取り出しモード       */
    int32_t      stat;                   /* 実行状態             */
    /* flow in管理 */
    int32_t      sct;                    /* 読み込みセクタ数     */
    int32_t      sctcnt;                 /* 読み込みカウンタ     */
    int32_t      sctmax;                 /* 読み込み最大セクタ数 */
} GfsFlow;


/* データパック構造体 */
typedef struct {
    void        *data;                  /* データ領域           */
    int32_t      adlt;                   /* アドレスの増加分     */
    int32_t      len;                    /* 転送バイト数         */
    int32_t      nsct;                   /* 転送セクタ数         */
    bool        use;                    /* 使用フラグ           */
} GfsDataPack;


typedef GfsDataPack *GfdpHn;

/* 転送管理 */
/* CDバッファからの転送を管理する */
typedef struct {
    void        *buf;                   /* 転送先アドレス */
    int32_t      size;                   /* データ領域のサイズ */
    int32_t      wp;                     /* 書き込みオフセット */
    /* 転送管理 */
    GfdpHn      dps;                    /* 転送元のデータパック */
    GfdpHn      dpd;                    /* 転送先からのデータパック */
    int32_t      tsctmax;                /* １回の転送の最大セクタ数 */
    int32_t      tsct;                   /* 転送目標セクタ数 */
    int32_t      tsctcnt;                /* 現在まで転送セクタ数 */
    int32_t      tbytcnt;                /* セクタ内の転送バイト数 */
    /* 転送関数 */
    void        *obj;                   /* 転送関数への第１引数 */
    GfsTransFunc tfunc;                 /* 転送関数 */
    int32_t      unit;                   /* 転送単位 */
    bool        active;                 /* 動作中フラグ */
    int32_t      stat;                   /* 状態 */
    int32_t      mode;                   /* 転送モード */
} GfsTrans;


typedef GfsTrans *GftrHn;


/* オープンしたファイルを管理する */
typedef struct {
    bool        used;                   /* この構造体が使用中かどうか */
    int32_t      amode;                  /* アクセスモード */
    int32_t      astat;                  /* アクセス状態 */
    GfsFlow     flow;
    GfsTrans    trans;
} GfsFile;


/* ファイルハンドル */
typedef GfsFile *GfsHn;


/* ユーザ登録転送関数 */
typedef int32_t (*GfsTrFunc)(GfsHn gfs, int32_t nsct);
     

/* ディレクトリレコード構造体(ファイル名なし)   */
typedef struct {
    CdcFile     dirrec;
} GfsDirId;


/* ディレクトリレコード構造体(ファイル名あり)   */
typedef struct {
    CdcFile     dirrec;
    int8_t       fname[GFS_FNAME_LEN];   /* ファイル名 */
} GfsDirName;


/* ディレクトリ情報管理構造体 */
typedef struct {
    int32_t      type;                   /* 種別 */
    int32_t      ndir;                   /* 最大要素数 */
    union {
        GfsDirId *dir_i;                /* ファイル名なしディレクトリ情報 */
        GfsDirName *dir_n;              /* ファイル名つきディレクトリ情報 */
    } dir;
} GfsDirTbl;


/* エラー処理関数 */
typedef void (*GfsErrFunc)(void *obj, int32_t ec);

/* エラー状態 */
typedef struct {
    GfsErrFunc  func;                   /* エラー発生時の呼び出し関数   */
    void        *obj;                   /* 呼び出し関数に渡す第一引数   */
    int32_t      code;                   /* エラーコード                 */
} GfsErrStat;


/*****************************************************************************
 *      関数の宣言
 *****************************************************************************/

/* ファイルシステムの初期化 */
int32_t GFS_Init(int32_t open_max, void *work, GfsDirTbl *dirtbl);

/* ディレクトリレコードの読み込み */
int32_t GFS_LoadDir(int32_t fid, GfsDirTbl *dirtbl);

/* カレントディレクトリレコードの設定 */
int32_t GFS_SetDir(GfsDirTbl *dirtbl);

/* 名前からファイル識別子への変換 */
int32_t GFS_NameToId(int8_t *fname);

/* ファイル識別子からファイル名への変換 */
int8_t *GFS_IdToName(int32_t fid);

/* ファイル識別子からディレクトリレコード取得 */
int32_t GFS_GetDirInfo(int32_t fid, GfsDirId *dirrec);

/*  ファイルシステムのリセット  */
void    GFS_Reset(void);

/* ファイルハンドルの割当て */
GfsHn GFS_Open(int32_t fid);

/* ファイルハンドルの割当て解除 */
void GFS_Close(GfsHn gfs);

/* アクセスポインタの移動 */
int32_t GFS_Seek(GfsHn gfs, int32_t ofs, int32_t org);

/* アクセスポインタの取得 */
int32_t GFS_Tell(GfsHn gfs);

/* ファイル終了のチェック */
bool GFS_IsEof(GfsHn gfs);

/* バイトサイズからセクタサイズへの変換 */
int32_t GFS_ByteToSct(GfsHn gfs, int32_t nbyte);

/* ファイルサイズに関する情報の取得 */
void GFS_GetFileSize(GfsHn gfs, int32_t *sctsz, int32_t *nsct, int32_t *lstsz);

/* ファイル情報の取得 */
void GFS_GetFileInfo(GfsHn gfs, int32_t *fid, int32_t *fn, int32_t *fsize,
                     int32_t *atr);

/*  CDバッファ区画のセクタ数の取得      */
int32_t GFS_GetNumCdbuf(GfsHn gfs);

/* 一括読み込み */
int32_t GFS_Load(int32_t fid, int32_t ofs, void *buf, int32_t bsize);

/* 完了復帰型読み込み */
int32_t GFS_Fread(GfsHn gfs, int32_t nsct, void *buf, int32_t bsize);

/* 即時復帰型読み込み */
int32_t GFS_NwFread(GfsHn gfs, int32_t nsct, void *buf, int32_t bsize);

/* 即時復帰型ＣＤバッファ読み込み */
int32_t GFS_NwCdRead(GfsHn gfs, int32_t nsct);

/* アクセス動作完了チェック */
bool GFS_NwIsComplete(GfsHn gfs);

/* アクセス動作の中止 */
int32_t GFS_NwStop(GfsHn gfs);

/* アクセス状態の取得 */
void GFS_NwGetStat(GfsHn gfs, int32_t *amode, int32_t *ndata);

/* ファイル単位でのアクセス動作の実行 */
int32_t GFS_NwExecOne(GfsHn gfs);

/* サーバアクセス動作の実行 */
int32_t GFS_NwExecServer(GfsHn *now_gfs);

/* ＣＤバッファからの取り出しモードの設定 */
int32_t GFS_SetGmode(GfsHn gfs, int32_t gmode);

/* 転送モードの設定 */
int32_t GFS_SetTmode(GfsHn gfs, int32_t tmode);

/* ソースバッファへの読み込みセクタ数の設定 */
int32_t GFS_SetReadPara(GfsHn gfs, int32_t nsct);

/* ソースバッファからホストへの転送セクタ数の設定 */
int32_t GFS_SetTransPara(GfsHn gfs, int32_t tsize);

/* 転送関数の登録       */
void    GFS_SetTrFunc(GfsHn gfs, GfsTrFunc func);

/* 転送関数における転送開始     */
uint32_t  *GFS_StartTrans(GfsHn gfs, int32_t *dadr);

/* ピックアップをアクセスポインタの位置に移動する */
int32_t GFS_CdMovePickup(GfsHn gfs);

/* エラー処理関数の登録 */
void GFS_SetErrFunc(GfsErrFunc func, void *obj);

/* エラー状態の取得 */
void GFS_GetErrStat(GfsErrStat *stat);

/*  トラック番号の変換      */
int32_t  GFS_ConvTno(int32_t logtno);

/* fidとオフセットからフレームアドレスを取得する */
int32_t GFS_GetFad(int32_t fid, int32_t ofs);


/*****************************************************************************
 *      システム用情報
 *****************************************************************************/

/*---定数--------------------------------------------------------------------*/
#define GFS_OPEN_MAX    24              /* 最大オープンファイル数 */
#define GFS_TBUF_NR     2               /* 転送バッファの数 */
#define GFS_FTYPE_NR    3               /* ファイル種別数 */
#define GFS_SCTBUF_SIZ  2048            /* セクタバッファの大きさ */
#define GFS_CDBBUF_NR   24              /* CDブロックの区画数 */
#define GFS_SELQ_MAX    24              /* 絞り設定コマンドキューの最大長 */
#define GFS_FCONQ_MAX   24              /* 接続コマンドキューの最大長 */

/*---データ型----------------------------------------------------------------*/

/* ファイル操作用関数(管理テーブル, GfsMngのメンバ) */
typedef struct {
    int32_t (*flowin)(GfsFlow *);        /* ソースバッファ読み込み関数   */
    void (*stopin)(GfsFlow *, bool);    /* 読み込み中止関数             */
    int32_t (*seek)(GfsFlow *, int32_t);  /* アクセスポインタ設定関数     */
    int32_t (*tell)(GfsFlow *);          /* アクセスポインタ取得関数     */
} GfsFileFunc;

/* アクセスサーバ管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct {
    GfsHn   access_file[GFS_OPEN_MAX];  /* アクセスモード指定済ファイル */
    int32_t  nfile;                      /* 登録ファイル数               */
} GfsSvr;


/* ディレクトリ管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct {
    GfsDirTbl   dirtbl;                 /* ディレクトリ情報管理 */
    int32_t      nfile;                  /* ファイル数 */
} GfsDirMng;


/* 絞り設定コマンドキューの要素(CDブロック管理構造体, GfsCdbMngのメンバ) */
typedef struct {
    uint8_t flt;                          /* 設定先絞り番号 */
    uint8_t fmode;                        /* 絞りモード */
    CdcSubh subh;                       /* サブヘッダ */
    int32_t fad;                         /* 開始FAD */
    int32_t snum;                        /* FADセクタ数 */
} GfcdSelQu;

/* 接続コマンドキューの要素(CDブロック管理構造体, GfsCdbMngのメンバ) */
typedef struct {
    int32_t flt;                         /* 設定絞り */
    int32_t buf;                         /* 真出力接続バッファ */
    int32_t flnout;                      /* 偽出力接続絞り(負の値を使用) */
} GfcdFconQu;    

/* CDブロック管理構造体(管理テーブル, GfsMngのメンバ) */
typedef struct {
    /* 資源管理 */
    int8_t use_buf[GFS_CDBBUF_NR];       /* バッファ区画使用状態 */
    int8_t use_filt[GFS_CDBBUF_NR];      /* 絞り使用状態 */
    bool use_pu;                        /* ピックアップ使用状態 */
    int32_t tr_bufno;                    /* データ転送中のバッファ区画   */
    int32_t puid;                        /* ピックアップの所有者ID */
    int32_t timer;                       /* タイムアウトカウンタ */
    CdcStat stat;                       /* CDブロックの状態 */
    void (*func)(void *);               /* CSCTハンドラ */
    void *obj;                          /* CSCTハンドラの引数 */
    /* GFCD_SetFiltタスク */
    struct {
        int32_t len;                     /* 絞り設定コマンドキューの長さ */
        int32_t stat;                    /* コマンドキュー先頭の状態 */
        GfcdSelQu selq[GFS_SELQ_MAX];   /* 絞り設定コマンドキュー */
    } tsk_setflt;
    /* GFCD_SetFiltConタスク */
    struct {
        int32_t len;                     /* 接続コマンドキューの長さ */
        int32_t stat;                    /* タスク終了フラグ */
        GfcdFconQu fconq[GFS_FCONQ_MAX]; /* 接続コマンドキュー */
    } tsk_fltcon;
    /* GFCD_SetConタスク */
    struct {
        int32_t stat;                    /* タスク終了フラグ */
        int32_t flt;                     /* CD接続先絞り */
    } tsk_setcon;
    /* GFCD_GetLenDataタスク */
    struct {
        int32_t stat;                    /* タスク終了フラグ */
        int32_t bufno;                   /* 対象区画 */
        int32_t spos;                    /* 先頭セクタ位置 */
        int32_t usct;                    /* 要求セクタ数 */
        int32_t cnt;                     /* タスクの実行回数 */
        int32_t *nsct;                   /* 区画内セクタ数格納先 */
        int32_t *nbyte;                  /* 区画内バイト数格納先 */
    }tsk_getlen;
    /* GFCD_ReqDataタスク */
    struct {
        int32_t stat;                    /* タスク終了フラグ */
        int32_t bufno;                   /* 対象区画 */
        int32_t sctpos;                  /* セクタ位置 */
        int32_t nsct;                    /* 要求セクタ数 */
    } tsk_reqdat;
    /* GFCD_DelSctDataタスク */
    struct {
        int32_t stat;                    /* タスク終了フラグ */
        int32_t bufno;                   /* 対象区画 */
        int32_t sctpos;                  /* 削除開始セクタ位置 */
        int32_t nsct;                    /* 削除セクタ数 */
    } tsk_delsct;
    /* GFCD_MoveSctDataタスク */
    struct {
        int32_t stat;                    /* タスク終了フラグ */
        int32_t dst;                     /* 移動先区画 */
        int32_t src;                     /* 移動元区画 */
        int32_t spos;                    /* 移動開始セクタ位置 */
        int32_t snum;                    /* 移動セクタ数 */
        int32_t fmode;                   /* 移動先セレクタの絞りモード保存 */
    } tsk_movsct;
    /* GFCD_ChgDirタスク */
    struct {
        int32_t stat;                    /* タスク終了フラグ */
        int16_t fid;                     /* 設定ファイル識別子 */
        int16_t work;                    /* 作業バッファ */
        int32_t *ndir;                   /* 保持ディレクトリ情報個数 */
    } tsk_chgdir;
} GfsCdbMng;

/* 管理テーブル */
typedef struct {
    int32_t      openmax;                /* オープンファイルの最大数 */
    GfsFileFunc functbl[GFS_FTYPE_NR];  /* ファイル操作用関数 */
    GfsSvr      svr;                    /* アクセスサーバ */
    GfsDirMng   curdir;                 /* カレントディレクトリ */
    GfsHn       pickup;                 /* ピックアップ制御中ファイル */
    int32_t      sfad;                   /* 再生範囲:開始FAD */
    int32_t      efad;                   /* 再生範囲:終了FAD+1 */
    GfsHn       trans;                  /* データ転送中ファイル */
    GfsErrStat  error;                  /* エラー状態 */
    uint32_t      flags;                  /* デバイス接続情報など */
    int32_t      timer;                  /* タイムアウトカウンタ */
    GfsCdbMng   cdb;                    /* CDブロック管理構造体 */
    GfsDataPack srcpk;                  /* 転送元データパック */
    GfsDataPack dstpk;                  /* 転送先データパック */
    uint8_t       sect_buf[GFS_SCTBUF_SIZ]; /* １セクタの読み込み用バッファ */
    GfsFile     file[1];                /* 全ハンドルの配列（可変長） */
} GfsMng;

#endif      /* !defined(SEGA_GFS_H) */

/* end of file */
