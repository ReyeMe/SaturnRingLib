/* */
/* SEGA Library defines Ver3.00 */
/* */

/** \defgroup sl_def SL_DEF.H
 *  @{
 */

#ifndef SEGA_XPT_H
#define SEGA_XPT_H

#if 0
#define ST_V /* or ST_VE (for Enhanced ST-V) */
#define TITAN
#endif

#define shc 1
#define sh_gcc 2
#define Compiler sh_gcc /* fot Hitachi then "shc" */

#if Compiler == shc
#define STCNS static const
#else

/** @brief Static flag
 */
#define STCNS static
#endif

/*****************************************************************************/
/****************************** Basic declaration *****************************/
/*****************************************************************************/

#include <stdint-gcc.h>

/*****************************************************************************/
/********************************* Constant macro ********************************/
/*****************************************************************************/

#ifndef NULL

/** @brief Null pointer
 */
#define NULL ((void *)0) 
#endif

/*****************************************************************************/
/********************************** Enumeration constant *********************************/
/*****************************************************************************/

/** @brief Logical constant 1 (false, true)
 */
enum booleanLogic
{
    FALSE = 0,
    TRUE = 1
};

/** @brief Logical constant 2 (switch)
 */
enum booleanSwitch
{
    OFF = 0,
    ON = 1
};

/** @brief Result determination constant (success, failure)
 */
enum Judgement
{ 
    /** @brief Success
     */
    OK = 0,

    /** @brief Failure
     */
    NG = -1
};

/*****************************************************************************/
/********************************* 処理マクロ ********************************/
/*****************************************************************************/

/** @brief Maximum value
 */
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/** @brief Minimum value
 */
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/** @brief Absolute value
 */
#define ABS(x) ((x) < 0 ? -(x) : (x))

/*------------------------------------------------------------------------------------*/

/** @brief Disable interrupts
 */
#define DI 15

/** @brief Enable interrupts
 */
#define EI 0

#if 0                 /* Ver2.1 より削除 */
#define MAX_EVENTS 64 /* 使用できるイベントの数 */
#define MAX_WORKS 256 /* 使用できるワークの数 */
#endif

/** @brief Number of nestable matrices
 */
#define MAX_NEST 20

/** @brief Work size
 */
#define WORK_SIZE 0x40

/** @brief Event size
 */
#define EVENT_SIZE 0x80

/** @brief 360 degree angle is represented by 16 bits
 */
typedef int16_t ANGLE;

/** @brief Fixed-point type that represents the integer part in the upper 16 bits and the decimal part in the lower 16 bits
 */
typedef int32_t FIXED;

/** @brief Point component and array size
 */
enum ps
{
    X,
    Y,
    Z,
    XYZ,
    XYZS,
    XYZSS,
    XY = Z,
    S = XYZ,
    Sh = S,
    Sv = XYZS
};

typedef struct work
{
    struct work *next;
    uint8_t user[WORK_SIZE - sizeof(struct work *)];
} WORK;

typedef struct evnt
{
    WORK *work;
    struct evnt *next;
    struct evnt *before;
    void (*exad)();
    uint8_t user[EVENT_SIZE - (sizeof(WORK *) + sizeof(struct evnt *) * 2 + sizeof(void (*)()))];
} EVENT;

/** @brief Matrix data
 */
typedef FIXED MATRIX[4][3];

/** @brief Vector data
 */
typedef FIXED VECTOR[XYZ];

/** @brief Vertex position data
 */
typedef FIXED POINT[XYZ];

/** @brief Current Matrix
 */
#define CURRENT 0

/** @brief Original Scale
 */
#define ORIGINAL (0.99999)

/** @brief Matrix element number
 */
enum mtrx
{
    M00,
    M01,
    M02,
    M10,
    M11,
    M12,
    M20,
    M21,
    M22,
    M30,
    M31,
    M32,
    MTRX
};

/** @brief Screen resolution
 */
enum tvsz
{
    TV_320x224,
    TV_320x240,
    TV_320x256,
    TV_dummy1,
    TV_352x224,
    TV_352x240,
    TV_352x256,
    TV_dummy2,
    TV_640x224,
    TV_640x240,
    TV_640x256,
    TV_dummy3,
    TV_704x224,
    TV_704x240,
    TV_704x256,
    TV_dummy4,

    TV_320x448,
    TV_320x480,
    TV_320x512,
    TV_dummy5,
    TV_352x448,
    TV_352x480,
    TV_352x512,
    TV_dummy6,
    TV_640x448,
    TV_640x480,
    TV_640x512,
    TV_dummy7,
    TV_704x448,
    TV_704x480,
    TV_704x512,
    TV_dummy8
};

#define _SysPause 64
#define _SprPause 2
#define _ScrPause 1

/*----------------------------------------------------------------------------------*/

#define M_PI 3.1415926535897932
#define toFIXED(a) ((FIXED)(65536.0 * (a)))
#define POStoFIXED(x, y, z)                \
    {                                      \
        toFIXED(x), toFIXED(y), toFIXED(z) \
    }
#define ATTRIBUTE(f, s, t, c, g, a, d, o)                                                  \
    {                                                                                      \
        f, (s) | (((d) >> 16) & 0x1c) | (o), t, (a) | (((d) >> 24) & 0xc0), c, g, (d)&0x3f \
    }
#define SPR_ATTRIBUTE(t, c, g, a, d)                    \
    {                                                   \
        t, (a) | (((d) >> 24) & 0xc0), c, g, (d)&0x0f3f \
    }
#define DEGtoANG(d) ((ANGLE)((65536.0 * (d)) / 360.0))
#define RADtoANG(d) ((ANGLE)((65536.0 * (d)) / (2 * M_PI)))
#define RGB(r, g, b) (0x8000 | ((b) << 10) | ((g) << 5) | (r))
#define DGTtoRGB(c) (0x8000 | (((c)&0x0f00) << 3) | (((c)&0x4000) >> 4) | (((c)&0x00f0) << 2) | (((c)&0x2000) >> 8) | (((c)&0x000f) << 1) | (((c)&0x1000) >> 12))
#define SLocate(x, y, p) ((p)*Page + (y)*Line + (x))

/** \defgroup sprite_control Sprite control function
 *  @{
 */

#define FUNC_Sprite 1
#define FUNC_Texture 2
#define FUNC_Polygon 4
#define FUNC_PolyLine 5
#define FUNC_Line 6
#define FUNC_SystemClip 9
#define FUNC_UserClip 8
#define FUNC_BasePosition 10
#define FUNC_End -1

/** @brief Zoom base Left Top
 */
#define _ZmLT (0x05 << 8)

/** @brief Zoom base Left Center
 */
#define _ZmLC (0x09 << 8)

/** @brief Zoom base Left Bottom
 */
#define _ZmLB (0x0d << 8)

/** @brief Zoom base Center Top
 */
#define _ZmCT (0x06 << 8)

/** @brief Zoom base Center Center
 */
#define _ZmCC (0x0a << 8)

/** @brief Zoom base Center Bottom
 */
#define _ZmCB (0x0e << 8)

/** @brief Zoom base Right Top
 */
#define _ZmRT (0x07 << 8)

/** @brief Zoom base Right Center
 */
#define _ZmRC (0x0b << 8)

/** @brief Zoom base Right Bottom
 */
#define _ZmRB (0x0f << 8)

/** @brief Set MSB to be written to frame buffer to 1
 */
#define MSBon (1 << 15)

/** @brief High speed shrink
 */
#define HSSon (1 << 12)

/** @brief High speed shrink disabled (default)
 */
#define HSSoff (0 << 12)

/** @brief No window restrictions (default)
 */
#define No_Window (0 << 9)

/** @brief Display inside window
 */
#define Window_In (2 << 9)

/** @brief Display outside window
 */
#define Window_Out (3 << 9)

/** @brief Usually expressed as (default)
 */
#define MESHoff (0 << 8)

/** @brief Display as mesh
 */
#define MESHon (1 << 8)

/** @brief Use end code as one of the palettes
 */
#define ECdis (1 << 7)

/** @brief End code valid
 */
#define ECenb (0 << 7)

/** @brief Use space code as one of the palettes
 */
#define SPdis (1 << 6)

/** @brief Do not display spaces (default)
 */
#define SPenb (0 << 6)

/** @brief Color bank 16-color mode (default)
 */
#define CL16Bnk (0 << 3)

/** @brief Color lookup 16 color mode
 */
#define CL16Look (1 << 3)

/** @brief Color bank 64 color mode
 */
#define CL64Bnk (2 << 3)

/** @brief Color bank 128 color mode
 */
#define CL128Bnk (3 << 3)

/** @brief Color bank 256 color mode
 */
#define CL256Bnk (4 << 3)

/** @brief RGB32K color mode
 */
#define CL32KRGB (5 << 3)

/** @brief Overwrite mode
 */
#define CL_Replace 0

/** @brief Shadow mode
 */
#define CL_Shadow 1

/** @brief Half-brightness mode
 */
#define CL_Half 2

/** @brief Translucent mode
 */
#define CL_Trans 3

/** @brief Gouraud shading mode
 */
#define CL_Gouraud 4


/** @brief Polygon to put texture
 */
#define UseTexture (1 << 2)

/** @brief Polygon affected by light source
 */
#define UseLight (1 << 3)

/** @brief Polygon color
 */
#define UsePalette (1 << 5)

/** @brief Near clipping
 */
#define UseNearClip (1 << 6)

/** @brief Real gouraud
 */
#define UseGouraud (1 << 7)

/** @brief Depth Cue
 */
#define UseDepth (1 << 4)

/** @brief Perform near clipping
 */
#define UseClip UseNearClip /*  */

#define sprHflip ((1 << 4) | FUNC_Texture | (UseTexture << 16))
#define sprVflip ((1 << 5) | FUNC_Texture | (UseTexture << 16))
#define sprHVflip ((3 << 4) | FUNC_Texture | (UseTexture << 16))
#define sprNoflip ((0) | FUNC_Texture | (UseTexture << 16))
#define sprPolygon (FUNC_Polygon | ((ECdis | SPdis) << 24))
#define sprPolyLine (FUNC_PolyLine | ((ECdis | SPdis) << 24))
#define sprLine (FUNC_Line | ((ECdis | SPdis) << 24))

/** @brief When texture is not used
 */
#define No_Texture 0

/** @brief When option is not used
 */
#define No_Option 0

/** @brief When Gouraud shading is not used
 */
#define No_Gouraud 0

/** @brief When color palette is not required
 */
#define No_Palet 0

/** @brief Gouraud buffer for light effects
 */
#define GouraudRAM (0x00080000 - (32 * 8))

/** @brief Mesh face
 */
typedef struct
{
    /** @brief Normal vector
     */
    VECTOR norm;

    /** @brief Vertex numbers that make up the polygon
     */
    uint16_t Vertices[4];
} POLYGON;

/** @brief Mesh face attributes
 */
typedef struct
{
    /** @brief Single-sided or double-sided flag
     */
    uint8_t flag;

    /** @brief Sort reference position and optional settings
     */
    uint8_t sort;

    /** @brief Texture number
     */
    uint16_t texno;

    /** @brief Attribute data (display mode)
     */
    uint16_t atrb;

    /** @brief Color number
     */
    uint16_t colno;

    /** @brief Gouraud shading table
     */
    uint16_t gstb;

    /** @brief Texture inversion and function number
     */
    uint16_t dir;
} ATTR;

/** @brief Sprite attributes
 */
typedef struct
{
    /** @brief Texture number
     */
    uint16_t texno;

    /** @brief Attribute data (display mode)
     */
    uint16_t atrb;

    /** @brief Color number
     */
    uint16_t colno;

    /** @brief Gouraud shading table
     */
    uint16_t gstb;

    /** @brief Texture inversion and function number
     */
    uint16_t dir;
} SPR_ATTR;

/** @brief Flat shaded model data
 */
typedef struct
{
    /** @brief Vertex position data table
     */
    POINT *pntbl;

    /** @brief Number of vertices
     */
    uint32_t nbPoint;

    /** @brief Polygon definition table
     */
    POLYGON *pltbl;

    /** @brief Number of polygons
     */
    uint32_t nbPolygon;

    /** @brief Polygon attribute table
     */
    ATTR *attbl;
} PDATA;

/** @brief Smooth shaded model data
 */
typedef struct
{
    /** @brief Vertex position data table
     */
    POINT *pntbl;     /*  */

    /** @brief Number of vertices
     */
    uint32_t nbPoint;   /*  */

    /** @brief Polygon definition table
     */
    POLYGON *pltbl;   /*  */

    /** @brief Number of polygons
     */
    uint32_t nbPolygon; /*  */

    /** @brief Polygon attribute table
     */
    ATTR *attbl;
    
    /** @brief Normal line vector data for vertexes
     */
    VECTOR *vntbl;
} XPDATA;

#ifdef ST_VE

typedef struct
{
    uint16_t pmod;   /* Sprite display mode */
    uint16_t colno;  /* Color number */
    uint16_t CGadr;  /* Sprite CG address / 8 */
    uint16_t HVsize; /* H size / 8, V size (for hardware) */
} gxATTR_VDP1;

typedef struct
{
    uint32_t texcel;  /*Texture cell address */
    uint16_t dspmode; /*Display mode flag */
    uint16_t colno;   /*Color data */
    uint8_t TXA, TYA; /*Texture point */
    uint8_t TXB, TYB;
    uint8_t TXC, TYC;
    uint8_t TXD, TYD;
} gxATTR_ENHC;

typedef struct
{
    uint16_t texno; /* Texture number */
    uint16_t atrb;  /* Attribute data (display mode) */
    uint16_t colno; /* Color number */
    uint16_t gstb;  /* Gouraud shading table */
    uint16_t dir;   /* Texture inversion and function number */
} gxSPR_ATTR;

typedef struct
{
    uint16_t texno;      /* Texture offset address */
    uint8_t attr;        /* Material code, sorting mode, etc.*/
    uint8_t nvface;      /* Surface normal vector number */
    uint8_t nvnum[4];    /* Vertex normal vector number */
    uint8_t Vertices[4]; /* Vertex number constituting the polygon */
} gxPOLYGON;

typedef struct
{
    POINT *pntbl;       /*Vertex position data table */
    uint16_t nbPoint;     /*Number of vertices */
    uint16_t nbNVector;   /*Number of normal vectors */
    VECTOR *nvtbl;      /*Normal vector data table */
    gxPOLYGON *pltbl;   /*Polygon definition table */
    uint32_t nbPolygon;   /*Number of polygons */
    gxATTR_VDP1 *attbl; /*Polygon attribute table */
} gxPDATA;

typedef struct
{
    uint32_t texadr; /* Texture data table */
    uint8_t BD;      /* Luminance of each vertex */
    uint8_t BC;
    uint8_t BB;
    uint8_t BA;
    int16_t XA; /*Position of each vertex */
    int16_t YA;
    int16_t XB;
    int16_t YB;
    int16_t XC;
    int16_t YC;
    int16_t XD;
    int16_t YD;
    uint16_t ZA; /* Z value of each vertex */
    uint16_t ZB;
    uint16_t ZC;
    uint16_t ZD;
    uint16_t winno; /* Window number */
    uint16_t ZF;    /* On behalf of Z value */
} gxDIRECT;

#define winEXn 0 /* Enhancer side (near) */
#define winEXf 1 /* Enhancer side (far) */
#define winSTn 2 /* ST-V side near */
#define winSTf 3 /* ST-V side Far (far) */

#define gxPOLYTBL(f, s, t, m, n) \
    {                            \
        ((f) << 16) | ((t) << 20) | ((s) << 12) | ((m) << 8) | (n)
#define gxNormal(a, b, c, d) (a, b, c, d)
#define gxVertices(a, b, c, d) \
    (a, b, c, d)               \
    }
#define gxATTRVDP1(p, c, a, s) \
    {                          \
        p, c, a, s             \
    }
#define gxATTRENHC(cx, cy, sx, sy, cm, cl, an, hf, ms, sp, te, fm) \
    {                                                              \
        ((cx) << 4) | ((cy) << 14) | (sx) | ((sy) << 10) | (cm),   \
            ((ms) >> 5) | (((sp) ^ SPdis) >> 4) | ((te) << 1) |    \
                ((an) << 8) | ((hf) << 6) | (fm),                  \
            (cl)
#endif

/** @brief Object data
 */
typedef struct obj
{
    /** @brief Polygon model data table
     */
    PDATA *pat;

    /** @brief Relative position data
     */
    FIXED pos[XYZ];

    /** @brief Rotation angle data
     */
    ANGLE ang[XYZ];

    /** @brief Scaling data
     */
    FIXED scl[XYZ];

    /** @brief Child object data
     */
    struct obj *child;

    /** @brief Sibling object data
     */
    struct obj *sibling;
} OBJECT;

/** @brief VDP1 texture entry
 */
typedef struct
{
    /** @brief Texture height
     */
    uint16_t Hsize;

    /** @brief Texture width
     */
    uint16_t Vsize;

    /** @brief Texture address
     *  @note Address is divided by 8
     */
    uint16_t CGadr;

    /** @brief Texture data size
     *  @note H size / 8, V size (for hardware)
     */
    uint16_t HVsize;
} TEXTURE;

/** @brief Texture metadata
 */
typedef struct
{
    /** @brief Texture number
     */
    uint16_t texno;

    /** @brief Color mode
     */
    uint16_t cmode;

    /** @brief Location of original data
     */
    void *pcsrc;
} PICTURE;

/** @brief Color bank 16 color mode
 */
#define COL_16 (2 + 1)

/** @brief Color bank 64 color mode
 */
#define COL_64 (2 + 0)

/** @brief Color bank 128 color mode
 */
#define COL_128 (2 + 0)

/** @brief Color bank 256 color mode
 */
#define COL_256 (2 + 0)

/** @brief RGB32K color mode
 */
#define COL_32K (2 - 1)

/** @brief Quad sort function
 */
enum base
{
    /** @brief Use the position of the polygon displayed immediately before
     */
    SORT_BFR,

    /** @brief Use the foremost point out of 4 points
     */
    SORT_MIN,

    /** @brief Use the farthest of the four points
     */
    SORT_MAX,

    /** @brief Use the average position of 4 points
     */
    SORT_CEN
};

/** @brief Quad culling mode
 */
enum pln
{
    /** @brief Single-sided polygon (not visible when rotated away from camera)
     */
    Single_Plane, /*  */

    /** @brief Double-sided polygon
     */
    Dual_Plane
};

/** @brief Sprite entry
 */
typedef struct
{
    /** @brief Control function
     */
    uint16_t CTRL;

    /** @brief Link address
     */
    uint16_t LINK;

    /** @brief Put mode
     */
    uint16_t PMOD;

    /** @brief Color data
     */
    uint16_t COLR;

    /** @brief CG address
     */
    uint16_t SRCA;

    /** @brief Character size
     */
    uint16_t SIZE;

    /** @brief Indicates X position A
     */
    int16_t XA;
    
    /** @brief Indicates Y position A
     */
    int16_t YA;
    
    /** @brief Indicates X position B
     */
    int16_t XB;
    
    /** @brief Indicates Y position B
     */
    int16_t YB;
    
    /** @brief Indicates X position C
     */
    int16_t XC;
    
    /** @brief Indicates Y position C
     */
    int16_t YC;
    
    /** @brief Indicates X position D
     */
    int16_t XD;
    
    /** @brief Indicates Y position D
     */
    int16_t YD;

    /** @brief Gouraud shading table
     */
    uint16_t GRDA;

    /** @brief Dummy data for size adjustment
     */
    uint16_t DMMY;
} SPRITE;

/** @brief Start address of VDP1 texture ram
 */
#define SpriteVRAM 0x25c00000

/** @}*/

/*-----------------------------------------------------------------------*/

/** \defgroup colors RGB mode color sample data
 *  @{
 */

/** @brief No opacity flag
 */
#define RGB_Flag 0x8000

#define CD_Black (0 << 10) | (0 << 5) | (0) | RGB_Flag
#define CD_DarkRed (0 << 10) | (0 << 5) | (8) | RGB_Flag
#define CD_DarkGreen (0 << 10) | (8 << 5) | (0) | RGB_Flag
#define CD_DarkYellow (0 << 10) | (8 << 5) | (8) | RGB_Flag
#define CD_DarkBlue (8 << 10) | (0 << 5) | (0) | RGB_Flag
#define CD_DarkPurple (8 << 10) | (0 << 5) | (8) | RGB_Flag
#define CD_DarkMagenta (8 << 10) | (8 << 5) | (0) | RGB_Flag
#define CD_DarkWhite (8 << 10) | (8 << 5) | (8) | RGB_Flag
#define CD_MediumRed (0 << 10) | (0 << 5) | (16) | RGB_Flag
#define CD_MediumGreen (0 << 10) | (16 << 5) | (0) | RGB_Flag
#define CD_MediumYellow (0 << 10) | (16 << 5) | (16) | RGB_Flag
#define CD_MediumBlue (16 << 10) | (0 << 5) | (0) | RGB_Flag
#define CD_MediumPurple (16 << 10) | (0 << 5) | (16) | RGB_Flag
#define CD_MediumMagenta (16 << 10) | (16 << 5) | (0) | RGB_Flag
#define CD_MediumWhite (16 << 10) | (16 << 5) | (16) | RGB_Flag
#define CD_SemiRed (0 << 10) | (0 << 5) | (24) | RGB_Flag
#define CD_SemiGreen (0 << 10) | (24 << 5) | (0) | RGB_Flag
#define CD_SemiYellow (0 << 10) | (24 << 5) | (24) | RGB_Flag
#define CD_SemiBlue (24 << 10) | (0 << 5) | (0) | RGB_Flag
#define CD_SemiPurple (24 << 10) | (0 << 5) | (24) | RGB_Flag
#define CD_SemiMagenta (24 << 10) | (24 << 5) | (0) | RGB_Flag
#define CD_SemiWhite (24 << 10) | (24 << 5) | (24) | RGB_Flag
#define CD_Red (0 << 10) | (0 << 5) | (31) | RGB_Flag
#define CD_Green (0 << 10) | (31 << 5) | (0) | RGB_Flag
#define CD_Yellow (0 << 10) | (31 << 5) | (31) | RGB_Flag
#define CD_Blue (31 << 10) | (0 << 5) | (0) | RGB_Flag
#define CD_Purple (31 << 10) | (0 << 5) | (31) | RGB_Flag
#define CD_Magenta (31 << 10) | (31 << 5) | (0) | RGB_Flag
#define CD_White (31 << 10) | (31 << 5) | (31) | RGB_Flag

/** @}*/

/*------[ Scroll control ]------------------------------------------------*/

/** \defgroup scroll_control Scroll control
 *  @{
 */

/** \defgroup scroll_control_addr Scroll V-RAM map address
 *  @{
 */

#define VDP2_VRAM_A0 0x25e00000
#define VDP2_VRAM_A1 0x25e20000
#define VDP2_VRAM_B0 0x25e40000
#define VDP2_VRAM_B1 0x25e60000

/** @brief CG data for scroll 0, 1
 */
#define CGN01_RAM VDP2_VRAM_B1

/** @brief Scroll 0 VRAM address
 */
#define NBG0_MAP (CGN01_RAM + 0x16000)

/** @brief Scroll 1 VRAM address
 */
#define NBG1_MAP (CGN01_RAM + 0x18000)

/** @brief Coefficient table for rotary scroll
 */
#define KTBL0_RAM VDP2_VRAM_A1

/** @brief Back color
 */
#define BACK_CRAM (KTBL0_RAM + 0x1fffe)

#define RBG_PARA_ADR (KTBL0_RAM + 0x1ff00)
#define RBG_PARB_ADR (RBG_PARA_ADR + 0x80)

/** @brief CG data for rotary scroll
 */
#define CGR0_RAM VDP2_VRAM_A0

/** @brief Rotating scroll 0 VRAM address
 */
#define RBG0_MAP VDP2_VRAM_B0

#define VDP2_COLRAM 0x25f00000

/** @}*/

/** @brief 1 cell size (CG) 
 */
#define CG_Size (8 * 8)

/** @brief 1 cell size (CG 16Colors)
 */
#define CG16Size (8 * 8 / 2)

/** @brief 1 cell size (CG 256Colors)
 */
#define CG256Size (8 * 8)

/** @brief 1 cell size (CG 2048Colors)
 */
#define CG2048Size (8 * 8 * 2)

/** @brief 1 cell size (CG 32KColors)
 */
#define CG32KSize (8 * 8 * 2)

/** @brief 1 cell size (Pattern name)
 */
typedef uint16_t CELL;

/** @brief 1 line size
 */
#define Line 64

/** @brief 1 page size
 */
#define Page (64 * Line)

/** @brief Rotation and scroll control
 */
typedef struct rdat
{
    /** @brief Screen screen start coordinates Xst
     */
    FIXED XST;

    /** @brief Screen screen start coordinate Yst
     */
    FIXED YST;

    /** @brief Screen screen start coordinate Zst
     */
    FIXED ZST;

    /** @brief Screen screen vertical coordinate increment dXst
     */
    FIXED DXST;

    /** @brief Screen screen vertical coordinate increment dYst
     */
    FIXED DYST;

    /** @brief Screen screen horizontal coordinate increment dX
     */
    FIXED DX;

    /** @brief Screen screen horizontal coordinate increment dY
     */
    FIXED DY;

    /** @brief Rotation matrix parameter A
     */
    FIXED MATA;

    /** @brief Rotation matrix parameter B
     */
    FIXED MATB;

    /** @brief Rotation matrix parameter C
     */
    FIXED MATC;

    /** @brief Rotation matrix parameter D
     */
    FIXED MATD;

    /** @brief Rotation matrix parameter E
     */
    FIXED MATE;

    /** @brief Rotation matrix parameter F
     */
    FIXED MATF;


    /** @brief Viewpoint coordinates Px
     */
    int16_t PX;

    /** @brief Viewpoint coordinates Py
     */
    int16_t PY;

    /** @brief Viewpoint coordinates Pz
     */
    int16_t PZ;
    int16_t dummy0;

    /** @brief Center coordinates Cx
     */
    int16_t CX;

    /** @brief Center coordinates Cy
     */
    int16_t CY;

    /** @brief Center coordinates Cz
     */
    int16_t CZ;
    int16_t dummy1;

    /** @brief  Parallel movement amount Mx
     */
    FIXED MX;

    /** @brief Parallel movement amount My
     */
    FIXED MY;

    /** @brief Zoom out large reduction factor kx
     */
    FIXED KX;

    /** @brief Zoom out large reduction factor ky
     */
    FIXED KY;

    /** @brief Coefficient table start address KAst
     */
    uint32_t KAST;

    /** @brief Coefficient table vertical address increment KAst
     */
    int32_t DKAST;

    /** @brief Coefficient table horizontal address increment KA
     */
    int32_t DKA;
} ROTSCROLL;

/** \defgroup slExtSignal slExtSignal
 *  @{
 */
/** @brief External latch enable
 */
#define EXLTEN 0x0200

/** @brief External synch enable
 */
#define EXSYEN 0x0100

/** @brief Display area select
 */
#define DASEL 0x0002

/** @brief External background screen enable
 */
#define EXBGEN 0x0001
/** @}*/

/** \defgroup slColRAMMode slColRAMMode
 *  @brief Color RAM control
 *  @{
 */

/** @brief Color mode 0. 5 bits for each of R, G and B; total 15bits. 1024 colors out of 32,768 colors.
 */
#define CRM16_1024 0

/** @brief Color mode 1. 5 bits for each of R, G and B; total 15bits. 2048 colors out of 32,768 colors.
 */
#define CRM16_2048 1

/** @brief Color mode 2. 8 bits for each of R, G and B; total 24bits. 1024 colors out of 16,77 million colors.
 */
#define CRM32_1024 2

/** @}*/

/* slChar */
#define COL_TYPE_16 0x00
#define COL_TYPE_256 0x10
#define COL_TYPE_2048 0x20
#define COL_TYPE_32768 0x30
#define COL_TYPE_1M 0x40
#define CHAR_SIZE_1x1 0
#define CHAR_SIZE_2x2 1
/* slPage */
#define PNB_2WORD 0
#define PNB_1WORD 0x8000
#define CN_10BIT 0
#define CN_12BIT 0x4000
/* slPlane */
#define PL_SIZE_1x1 0
#define PL_SIZE_2x1 1
#define PL_SIZE_2x2 3
/* slScrDisp */
#define NBG0ON (1 << 0)
#define NBG1ON (1 << 1)
#define NBG2ON (1 << 2)
#define NBG3ON (1 << 3)
#define RBG0ON (1 << 4)
#define LNCLON (1 << 5)
#define SPRON (1 << 6)
#define BACKON (1 << 5)
#define NBG0OFF (1 << 16)
#define NBG1OFF (1 << 17)
#define NBG2OFF (1 << 18)
#define NBG3OFF (1 << 19)
#define RBG0OFF (1 << 20)
/* slZoomMode */
#define ZOOM_1 0
#define ZOOM_HALF 1
#define ZOOM_QUARTER 2
/* slKtable */
#define K_OFF 0
#define K_ON 1
#define K_2WORD 0
#define K_1WORD 2
#define K_MODE0 0
#define K_MODE1 4
#define K_MODE2 8
#define K_MODE3 12
#define K_LINECOL 0x10
#define K_DOT 0x20
#define K_LINE 0
#define K_FIX 0x40
/* slCurRpara */
/* slRparaMode */
#define RA 0
#define RB 1
#define K_CHANGE 2
#define W_CHANGE 3

#define LnRd_Xst 0x01
#define LnRd_Yst 0x02
#define LnRd_KAst 0x04

/* slColCalc */
#define CC_RATE 0
#define CC_ADD 0x100
#define CC_TOP 0
#define CC_2ND 0x200
#define CC_EXT 0x400

/* Gradation control */
#define grdNBG0 0xa000
#define grdNBG1 0xc000
#define grdNBG2 0xd000
#define grdNBG3 0xe000
#define grdRBG0 0x9000
#define grdSPR 0x8000

/* color */
#define COL_R (1 << 0)
#define COL_G (1 << 5)
#define COL_B (1 << 10)
/* SpriteColorCalc */
#define CC_pr_CN 0 /* Priority <= CCalcNum */
#define CC_PR_CN 1 /* Priority == CCalcNum */
#define CC_PR_cn 2 /* Priority >= CCalcNum */
#define CC_MSB 3   /* Use MSB */

#define SPR_PAL 0     /* Palette code only */
#define SPR_PAL_RGB 1 /* Use Palette and RGB */

enum color_rate
{
    CLRate31_1, /* Top 31 : Second 1 */
    CLRate30_2,
    CLRate29_3,
    CLRate28_4,
    CLRate27_5,
    CLRate26_6,
    CLRate25_7,
    CLRate24_8,
    CLRate23_9,
    CLRate22_10,
    CLRate21_11,
    CLRate20_12,
    CLRate19_13,
    CLRate18_14,
    CLRate17_15,
    CLRate16_16,
    CLRate15_17,
    CLRate14_18,
    CLRate13_19,
    CLRate12_20,
    CLRate11_21,
    CLRate10_22,
    CLRate9_23,
    CLRate8_24,
    CLRate7_25,
    CLRate6_26,
    CLRate5_27,
    CLRate4_28,
    CLRate3_29,
    CLRate2_30,
    CLRate1_31,
    CLRate0_32 /* Top 0 : Second 32 */
};

/* Screen select */
#define scnNBG0 1
#define scnNBG1 0
#define scnNBG2 3
#define scnNBG3 2
#define scnRBG0 5
#define scnRBGA 5
#define scnRBGB 4
#define scnSPR 4
#define scnROT 7
#define scnCCAL 6

#define scnLNCL 7
#define scnBACK 6

#define scnSPR0 -7
#define scnSPR1 -8
#define scnSPR2 -5
#define scnSPR3 -6
#define scnSPR4 -3
#define scnSPR5 -4
#define scnSPR6 -1
#define scnSPR7 -2

#define bmNBG0 1
#define bmNBG1 0
#define bmRBG0 3

/** @}*/

/* Window control */
#define win_OR 0x80
#define win_AND 0x00

#define win0_IN 0x03
#define win0_OUT 0x02

#define win1_IN 0x0c
#define win1_OUT 0x08

#define spw_IN 0x30
#define spw_OUT 0x20

/* Line Scroll control */
#define lineSZ1 0x00
#define lineSZ2 0x10
#define lineSZ4 0x20
#define lineSZ8 0x30

#define lineZoom 0x08
#define lineVScroll 0x04
#define lineHScroll 0x02
#define VCellScroll 0x01

/* BitMap control */
#define BM_512x256 0x02
#define BM_512x512 0x06
#define BM_1024x256 0x0a
#define BM_1024x512 0x0e

/* Special Function Control */
#define spPRI_Scrn 0
#define spPRI_Char 1
#define spPRI_Dot 2

#define spCCAL_Scrn 0
#define spCCAL_Char 1
#define spCCAL_Dot 2
#define spCCAL_MSB 3

#define sfCOL_01 0x01
#define sfCOL_23 0x02
#define sfCOL_45 0x04
#define sfCOL_67 0x08
#define sfCOL_89 0x10
#define sfCOL_ab 0x20
#define sfCOL_cd 0x40
#define sfCOL_ef 0x80

/* V-RAM Division */
#define Div_A 1
#define Div_B 2

/*------[ D.M.A. control ]------------------------------------------------*/
#define Sinc_Dinc_Byte 0
#define Sdec_Dinc_Byte 1
#define Sinc_Ddec_Byte 2
#define Sfix_Dinc_Byte 3
#define Sfix_Ddec_Byte 4
#define Sinc_Dfix_Byte 5
#define Sdec_Dfix_Byte 6

#define Sinc_Dinc_Word 7
#define Sdec_Dinc_Word 8
#define Sinc_Ddec_Word 9
#define Sfix_Dinc_Word 10
#define Sfix_Ddec_Word 11
#define Sinc_Dfix_Word 12
#define Sdec_Dfix_Word 13

#define Sinc_Dinc_Long 14
#define Sdec_Dinc_Long 15
#define Sinc_Ddec_Long 16
#define Sfix_Dinc_Long 17
#define Sfix_Ddec_Long 18
#define Sinc_Dfix_Long 19
#define Sdec_Dfix_Long 20

/*------[ Sound control ]-------------------------------------------------*/

typedef struct
{
    uint8_t mode;    /* Mode */
    uint8_t channel; /* PCM Channel Number */
    uint8_t level;   /* 0 ~ 127 */
    int8_t pan;     /* -128 ~ +127 */
    uint16_t pitch;
    uint8_t eflevelR;  /* Effect level for Right(mono) 0 ~ 7 */
    uint8_t efselectR; /* Effect select for Right(mono) 0 ~ 15 */
    uint8_t eflevelL;  /* Effect level for Left 0 ~ 7 */
    uint8_t efselectL; /* Effect select for Left 0 ~ 15 */
} PCM;

#define _Stereo 0x80
#define _Mono 0x00

#define _PCM16Bit 0x00
#define _PCM8Bit 0x10

#define SND_SEQ_START 0x01     /* Sequence Start */
#define SND_SEQ_STOP 0x02      /* Sequence Stop */
#define SND_SEQ_PAUSE 0x03     /* Sequence Pause */
#define SND_SEQ_CONT 0x04      /* Sequence Continue */
#define SND_SEQ_VOL 0x05       /* Sequence Volume */
#define SND_TEMPO_CHG 0x07     /* Tempo Change */
#define SND_MAP_CHG 0x08       /* map Change */
#define SND_MIDI_CTRL_DIR 0x09 /* MIDI direct control */
#define SND_VOL_ANL_START 0x0a /* Volume analize start */
#define SND_VOL_ANL_STOP 0x0b  /* Volume analize stop */
#define SND_DSP_STOP 0x0c      /* DSP stop */
#define SND_ALL_OFF 0x0d       /* Sound all OFF */
#define SND_SEQ_PAN 0x0e       /* Sequence PAN */
#define SND_CDDA_LEV 0x80      /* CD-DA Level */
#define SND_CDDA_PAN 0x81      /* CD-DA pan */
#define SND_TTL_VOL 0x82       /* Total Volume */
#define SND_EFCT_CHG 0x83      /* Effect Change */
#define SND_PCM_START 0x85     /* PCM start */
#define SND_PCM_STOP 0x86      /* PCM stop */
#define SND_MIX_CHG 0x87       /* Mixer change */
#define SND_MIX_PRM_CHG 0x88   /* Mixer parameter change */
#define SND_HARD_CHK 0x89      /* Hard check */
#define SND_PCM_PRM_CHG 0x8a   /* PCM parameter change */

#define SoundRAM 0x25a00000

/*------[ System Work Assignments ]---------------------------------------*/

/** @brief Stack area End
 */
extern const void *MasterStack;

/** @brief Calculatable vertice numbers
 */
extern const uint16_t MaxVertices;

/** @brief Calculatable polygon numbers
 */
extern const uint16_t MaxPolygons;

/** @brief sizeof(EVENT)
 */
extern const uint16_t EventSize;

/** @brief sizeof(WORK)
 */
extern const uint16_t WorkSize;

/** @brief Useful Event count
 */
extern const uint16_t MaxEvents;

/** @brief Useful Work count
 */
extern const uint16_t MaxWorks;

/** @brief SCU D.M.A. Table
 */
extern const void *SortList;

/** @brief Sprite transfer request table size
 */
extern const uint32_t SortListSize;

/** @brief SCU D.M.A. Table
 */
extern const void *TransList;

/** @brief Sort buffer
 *  @note (128 + 128 + 256) * 4 Bytes fix
 */
extern const void *Zbuffer;

/** @brief Sprite control data buffer
 */
extern const void *SpriteBuf;

/** @brief Sprite control data buffer size
 */
extern const uint32_t SpriteBufSize;

/** @brief Point calculate buffer
 */
extern const void *Pbuffer;

/** @brief Offset buffer
 *  @note 32 * 32 Bytes fix
 */
extern const void *CLOfstBuf;

/** @brief Command for slave CPU buffer
 */
extern const void *CommandBuf;

/** @brief PCM data buffer
 */
extern const void *PCM_Work;

/** @brief PCM data buffer size
 */
extern const uint32_t PCM_WkSize;

/** @brief Buffer for Event use
 */
extern EVENT EventBuf[];

/** @brief Buffer for Work use
 */
extern WORK WorkBuf[];

/** @brief Remain Event address buffer
 */
extern EVENT *RemainEvent[];

/** @brief Remain Work address buffer
 */
extern WORK *RemainWork[];

/*------[ Prottype ]------------------------------------------------------*/

/* Math library	*/

extern FIXED slSin(ANGLE);
extern FIXED slCos(ANGLE);
extern FIXED slTan(ANGLE);
extern ANGLE slAtan(FIXED, FIXED);
extern uint32_t slSquart(uint32_t);
extern FIXED slSquartFX(FIXED);
extern FIXED slMulFX(FIXED, FIXED);
extern FIXED slDivFX(FIXED, FIXED);
extern FIXED slInnerProduct(VECTOR vct1, VECTOR vct2);
extern FIXED slCalcPoint(FIXED, FIXED, FIXED, FIXED ans[XYZ]);
extern FIXED slCalcVector(VECTOR vct, VECTOR ans);
extern FIXED slRandom(void);
extern void slNormalVector(VECTOR, VECTOR, VECTOR, VECTOR);
extern void slRevNormalVector(VECTOR, VECTOR, VECTOR, VECTOR);

extern void slInitMatrix(void);
extern void slUnitMatrix(MATRIX);
extern bool slPushMatrix(void);
extern bool slPopMatrix(void);
extern bool slPushUnitMatrix(void);
extern bool slIncMatrixPtr(void);
extern bool slDecMatrixPtr(void);
extern void slLoadMatrix(MATRIX);
extern void slGetMatrix(MATRIX);
extern bool slCopyMatrix();
extern void slGetTranslate(FIXED *);
extern void slLoadTranslate(FIXED, FIXED, FIXED);
extern void slRegistMatrix(MATRIX);
extern void slInversMatrix(void);
extern void slTransposeMatrix(void);
extern void slUnitAngle(MATRIX);
extern void slUnitTranslate(MATRIX);

extern void slTranslate(FIXED, FIXED, FIXED);
extern void slRegistTranslate(FIXED, FIXED, FIXED);
extern void slMultiMatrix(MATRIX);
extern void slScale(FIXED, FIXED, FIXED);
extern void slRotX(ANGLE);
extern void slRotY(ANGLE);
extern void slRotZ(ANGLE);
extern void slRotXSC(FIXED, FIXED);
extern void slRotYSC(FIXED, FIXED);
extern void slRotZSC(FIXED, FIXED);
extern void slRotAX(FIXED, FIXED, FIXED, ANGLE);
extern void slLookAt(FIXED *camera, FIXED *target, ANGLE);
extern FIXED slBallCollision(FIXED *pos1, FIXED size1, FIXED *pos2, FIXED size2);
extern FIXED slCheckOnScreen(FIXED *pos, FIXED size);
extern FIXED slCheckOnScreen0(FIXED size);
extern void slBezier(VECTOR p1, VECTOR p2, VECTOR p3, VECTOR p4, FIXED time, VECTOR ans);
extern FIXED slConvert3Dto2D(FIXED pos[XYZ], int32_t ans[XY]);
extern FIXED slConvert3Dto2DFX(FIXED pos[XYZ], FIXED ans[XY]);

/* Text display library	*/

extern void *slLocate(uint16_t, uint16_t);
extern void slCurScrn(uint16_t);
extern void slCurColor(uint16_t);
extern void slDispHex(uint32_t, void *);
extern void slPrintHex(uint32_t, void *);
extern void slPrintMatrix(MATRIX, void *);
extern void slPrintFX(FIXED, void *);
extern uint32_t slHex2Dec(uint32_t);
extern uint32_t slDec2Hex(uint32_t);
extern uint16_t slAng2Hex(ANGLE);
extern uint16_t slAng2Dec(ANGLE);
extern FIXED slAng2FX(ANGLE);
extern void slPrint(char *, void *);
extern void set_vbar(uint16_t);
extern void reset_vbar(uint16_t);
extern void slExtendScroll(uint8_t *, void *);
extern void *slLocateNbg(uint16_t, uint16_t, uint16_t);
#define slLocateNbg0(x, y) slLocateNbg(x, y, scnNBG0)
#define slLocateNbg1(x, y) slLocateNbg(x, y, scnNBG1)
#define slLocateNbg2(x, y) slLocateNbg(x, y, scnNBG2)
#define slLocateNbg3(x, y) slLocateNbg(x, y, scnNBG3)
extern void *slLocatePage(uint16_t, uint16_t, uint16_t);

/* Sprite library	*/

extern bool slPutPolygon(PDATA *);
extern bool slPutPolygonS(PDATA *);
extern bool slDispPolygon(PDATA *, uint16_t);
extern bool slPutSprite(FIXED *, SPR_ATTR *attr, ANGLE);
extern bool slDispSprite(FIXED *, SPR_ATTR *attr, ANGLE);
extern bool slDispSpriteHV(FIXED *, SPR_ATTR *attr, ANGLE);
extern bool slDispSpriteSZ(FIXED *, SPR_ATTR *attr, ANGLE);
extern bool slDispSprite4P(FIXED *, FIXED, SPR_ATTR *attr);
extern void slPerspective(ANGLE);
extern void slSetScreenDist(FIXED);
extern bool slWindow(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int16_t);
extern void slLight(VECTOR);
extern bool slSetSprite(SPRITE *, FIXED);

/** @brief This function specifies how far in front of the projection surface to actually project (the front boundary surface)
 * @image html slZdspLevel.png
 */
extern void slZdspLevel(uint16_t);

extern bool slPutObject(OBJECT *);
extern void slCurWindow(uint8_t);
enum
{
    /** @brief Far side Window 
     */
    winFar,

    /** @brief Near side Window
     */
    winNear
};
extern uint8_t slSprWinNum();
extern void slFrameClipSize(int16_t, int16_t);
extern void slSetScreenDist(FIXED);
extern void slGetFrameData(void *, uint16_t, uint16_t);

/** \addtogroup scroll_control
 *  @{
 */

extern void slTVOn(void);
extern void slTVOff(void);
extern uint16_t slGetHCount(void);
extern uint16_t slGetVCount(void);

/** \addtogroup slExtSignal
 *  @{
 */
extern void slExtSignal(uint16_t, uint16_t);
/** @}*/

/** \addtogroup slColRAMMode
 *  @{
 */
/** @brief This function determines the color RAM mode.
 *  @note Always be sure to set the color RAM mode before storing color color data in RAM.
 *  @details For the parameter, substitute the value corresponding to the desired color RAM mode. For details on each color RAM mode, refer to the table below.
 * | Color RAM mode | Color bits                                  | Data size | Number of colors                        |
 * | -------------- | ------------------------------------------- | --------- | --------------------------------------- |
 * | Mode 0         | 5 bits for each of R, G and B; total 15bits | 1 word    | 1024 colors out of 32,768 colors        |
 * | Mode 1         | 5 bits for each of R, G and B; total 15bits | 1 word    | 2048 colors out of 32,768 colors        |
 * | Mode 2         | 8 bits for each of R, G and B; total 24bits | 2 words   | 1024 colors out of 16,77 million colors |
 * @note In color mode 0, color RAM is divided into two partitions, each storing the same color data.
 */
extern void slColRAMMode(uint16_t);
/** @}*/

extern void slVRAMMode(uint16_t);
extern void slScrCycleSet(uint32_t, uint32_t, uint32_t, uint32_t);
extern void slScrDisp(uint32_t);
extern bool slScrAutoDisp(uint32_t);
extern void slScrTransparent(uint16_t);
extern void slZrotR(ANGLE);
extern void slScrMatSet(void);
extern void slBackColTable(void *);
extern void slBack1ColSet(void *, uint16_t);
extern void slLineColTable(void *);
extern void slLine1ColSet(void *, uint16_t);
extern void slLineColDisp(uint16_t);
extern void slColorCalc(uint16_t);
extern void slColorCalcMode(uint16_t);
extern void slColorCalcOn(uint16_t);
extern void slColRate(int16_t, uint16_t);
#define slColRateNbg0(rate) slColRate(scnNBG0, rate)
#define slColRateNbg1(rate) slColRate(scnNBG1, rate)
#define slColRateNbg2(rate) slColRate(scnNBG2, rate)
#define slColRateNbg3(rate) slColRate(scnNBG3, rate)
#define slColRateRbg0(rate) slColRate(scnRBG0, rate)
#define slColRateLNCL(rate) slColRate(scnLNCL, rate)
#define slColRateBACK(rate) slColRate(scnBACK, rate)
#define slColRateSpr0(rate) slColRate(scnSPR0, rate)
#define slColRateSpr1(rate) slColRate(scnSPR1, rate)
#define slColRateSpr2(rate) slColRate(scnSPR2, rate)
#define slColRateSpr3(rate) slColRate(scnSPR3, rate)
#define slColRateSpr4(rate) slColRate(scnSPR4, rate)
#define slColRateSpr5(rate) slColRate(scnSPR5, rate)
#define slColRateSpr6(rate) slColRate(scnSPR6, rate)
#define slColRateSpr7(rate) slColRate(scnSPR7, rate)
extern void slColOffsetOn(uint16_t);
extern void slColOffsetOff(uint16_t);
extern void slColOffsetAUse(uint16_t);
extern void slColOffsetBUse(uint16_t);
extern void slColOffsetScrn(uint16_t, uint16_t);
extern void slColOffsetA(int16_t r, int16_t g, int16_t b);
extern void slColOffsetB(int16_t r, int16_t g, int16_t b);
extern void slCharNbg0(uint16_t type, uint16_t size);
extern void slCharNbg1(uint16_t type, uint16_t size);
extern void slCharNbg2(uint16_t type, uint16_t size);
extern void slCharNbg3(uint16_t type, uint16_t size);
extern void slCharRbg0(uint16_t type, uint16_t size);
extern void slPageNbg0(void *, void *, uint16_t);
extern void slPageNbg1(void *, void *, uint16_t);
extern void slPageNbg2(void *, void *, uint16_t);
extern void slPageNbg3(void *, void *, uint16_t);
extern void slPageRbg0(void *, void *, uint16_t);
extern void slPlaneNbg0(uint16_t);
extern void slPlaneNbg1(uint16_t);
extern void slPlaneNbg2(uint16_t);
extern void slPlaneNbg3(uint16_t);
extern void slPlaneRA(uint16_t);
extern void slPlaneRB(uint16_t);
extern void slOverRA(uint16_t);
extern void slOverRB(uint16_t);
extern void slMapNbg0(void *, void *, void *, void *);
extern void slMapNbg1(void *, void *, void *, void *);
extern void slMapNbg2(void *, void *, void *, void *);
extern void slMapNbg3(void *, void *, void *, void *);
extern void sl1MapRA(void *);
extern void sl1MapRB(void *);
extern void sl16MapRA(uint8_t *);
extern void sl16MapRB(uint8_t *);
extern void slRparaInitSet(ROTSCROLL *);
extern void slCurRpara(uint16_t);
extern void slRparaMode(uint16_t);
extern void slMakeKtable(void *);
extern void slKtableRA(void *, uint16_t);
extern void slKtableRB(void *, uint16_t);
extern void slPriority(int16_t, uint16_t);
#define slPriorityNbg0(num) slPriority(scnNBG0, num)
#define slPriorityNbg1(num) slPriority(scnNBG1, num)
#define slPriorityNbg2(num) slPriority(scnNBG2, num)
#define slPriorityNbg3(num) slPriority(scnNBG3, num)
#define slPriorityRbg0(num) slPriority(scnRBG0, num)
#define slPrioritySpr0(num) slPriority(scnSPR0, num)
#define slPrioritySpr1(num) slPriority(scnSPR1, num)
#define slPrioritySpr2(num) slPriority(scnSPR2, num)
#define slPrioritySpr3(num) slPriority(scnSPR3, num)
#define slPrioritySpr4(num) slPriority(scnSPR4, num)
#define slPrioritySpr5(num) slPriority(scnSPR5, num)
#define slPrioritySpr6(num) slPriority(scnSPR6, num)
#define slPrioritySpr7(num) slPriority(scnSPR7, num)
extern void slScrPosNbg0(FIXED x, FIXED y);
extern void slScrPosNbg1(FIXED x, FIXED y);
extern void slScrPosNbg2(FIXED x, FIXED y);
extern void slScrPosNbg3(FIXED x, FIXED y);
extern void slScrMoveNbg0(FIXED x, FIXED y);
extern void slScrMoveNbg1(FIXED x, FIXED y);
extern void slScrMoveNbg2(FIXED x, FIXED y);
extern void slScrMoveNbg3(FIXED x, FIXED y);
extern void slLookR(FIXED x, FIXED y);
extern void slDispCenterR(FIXED x, FIXED y);
extern void slZoomNbg0(FIXED x, FIXED y);
extern void slZoomNbg1(FIXED x, FIXED y);
extern void slZoomR(FIXED x, FIXED y);
extern void slScrScaleNbg0(FIXED x, FIXED y);
extern void slScrScaleNbg1(FIXED x, FIXED y);
extern void slScrScaleR(FIXED x, FIXED y);
extern void slZoomMode(uint16_t, uint16_t);
#define slZoomModeNbg0(mode) slZoomMode(scnNBG0, mode)
#define slZoomModeNbg1(mode) slZoomMode(scnNBG1, mode)
extern void slScrAscSet(void *);
extern void slScrMatConv(void);
extern void slSpriteCCalcCond(uint16_t);
extern void slSpriteCCalcNum(uint16_t);
extern void slSpriteColMode(uint16_t);
extern void slSpriteWinMode(uint16_t);
extern void slSpriteType(uint16_t);
extern void slScrMosaicOn(uint16_t);
extern void slScrMosSize(uint16_t, uint16_t);
extern void slScrWindow0(uint16_t, uint16_t, uint16_t, uint16_t);
extern void slScrWindow1(uint16_t, uint16_t, uint16_t, uint16_t);
extern void slScrLineWindow0(void *);
extern void slScrLineWindow1(void *);
extern void slScrWindowMode(uint16_t, uint16_t);
#define slScrWindowModeNbg0(mode) slScrWindowMode(scnNBG0, mode)
#define slScrWindowModeNbg1(mode) slScrWindowMode(scnNBG1, mode)
#define slScrWindowModeNbg2(mode) slScrWindowMode(scnNBG2, mode)
#define slScrWindowModeNbg3(mode) slScrWindowMode(scnNBG3, mode)
#define slScrWindowModeRbg0(mode) slScrWindowMode(scnRBG0, mode)
#define slScrWindowModeSPR(mode) slScrWindowMode(scnSPR, mode)
#define slScrWindowModeROT(mode) slScrWindowMode(scnROT, mode)
#define slScrWindowModeCCAL(mode) slScrWindowMode(scnCCAL, mode)
extern void slLineScrollMode(uint16_t, uint16_t);
#define slLineScrollModeNbg0(mode) slLineScrollMode(scnNBG0, mode)
#define slLineScrollModeNbg1(mode) slLineScrollMode(scnNBG1, mode)
extern void slLineScrollTable0(void *);
extern void slLineScrollTable1(void *);
extern void slVCellTable(void *);
extern void slGradationOn(uint16_t);
#define slGradationOff() slGradationOn(OFF)
extern void slShadowOn(uint16_t);
extern void slTpShadowMode(uint16_t);
extern void slOverPatRA(uint16_t pat);
extern void slOverPatRB(uint16_t pat);
extern void slColRAMOffsetNbg0(uint16_t offset);
extern void slColRAMOffsetNbg1(uint16_t offset);
extern void slColRAMOffsetNbg2(uint16_t offset);
extern void slColRAMOffsetNbg3(uint16_t offset);
extern void slColRAMOffsetRbg0(uint16_t offset);
extern void slColRAMOffsetSpr(uint16_t offset);
extern void slBitMapNbg0(uint16_t, uint16_t, void *);
extern void slBitMapNbg1(uint16_t, uint16_t, void *);
extern void slBitMapRbg0(uint16_t, uint16_t, void *);
extern void slBMPalette(uint16_t, uint16_t);
#define slBMPaletteNbg0(pal) slBMPalette(bmNBG0, pal)
#define slBMPaletteNbg1(pal) slBMPalette(bmNBG1, pal)
#define slBMPaletteRbg0(pal) slBMPalette(bmRBG0, pal)
extern void slSpecialPrioModeNbg0(uint16_t);
extern void slSpecialPrioModeNbg1(uint16_t);
extern void slSpecialPrioModeNbg2(uint16_t);
extern void slSpecialPrioModeNbg3(uint16_t);
extern void slSpecialPrioModeRbg0(uint16_t);
extern void slSpecialCCalcModeNbg0(uint16_t);
extern void slSpecialCCalcModeNbg1(uint16_t);
extern void slSpecialCCalcModeNbg2(uint16_t);
extern void slSpecialCCalcModeNbg3(uint16_t);
extern void slSpecialCCalcModeRbg0(uint16_t);
extern void slSpecialFuncCodeA(uint16_t);
extern void slSpecialFuncCodeB(uint16_t);
extern void slSpecialFuncSelectB(uint16_t);
extern void slRparaReadCtrlRA(uint16_t);
extern void slRparaReadCtrlRB(uint16_t);
extern void slSpecialPrioBitN01(uint16_t, uint16_t);
extern void slSpecialPrioBitScr(uint16_t, uint16_t);
extern void slSpecialPrioBitRbg0(uint16_t);
#define slSpecialPrioBitNbg0(flag) slSpecialPrioBitN01(scnNBG0, flag)
#define slSpecialPrioBitNbg1(flag) slSpecialPrioBitN01(scnNBG1, flag)
#define slSpecialPrioBitNbg2(flag) slSpecialPrioBitScr(scnNBG2, flag)
#define slSpecialPrioBitNbg3(flag) slSpecialPrioBitScr(scnNBG3, flag)
#define slSpecialPrioBitBMN0(flag) slSpecialPrioBitN01(scnNBG0, flag)
#define slSpecialPrioBitBMN1(flag) slSpecialPrioBitN01(scnNBG1, flag)
#define slSpecialPrioBitBMR0(flag) slSpecialPrioBitRbg0(flag)
extern void slSpecialCCalcBitN01(uint16_t, uint16_t);
extern void slSpecialCCalcBitScr(uint16_t, uint16_t);
extern void slSpecialCCalcBitRbg0(uint16_t);
#define slSpecialCCalcBitNbg0(flag) slSpecialCCalcBitN01(scnNBG0, flag)
#define slSpecialCCalcBitNbg1(flag) slSpecialCCalcBitN01(scnNBG1, flag)
#define slSpecialCCalcBitNbg2(flag) slSpecialCCalcBitScr(scnNBG2, flag)
#define slSpecialCCalcBitNbg3(flag) slSpecialCCalcBitScr(scnNBG3, flag)
#define slSpecialCCalcBitBMN0(flag) slSpecialCCalcBitN01(scnNBG0, flag)
#define slSpecialCCalcBitBMN1(flag) slSpecialCCalcBitN01(scnNBG1, flag)
#define slSpecialCCalcBitBMR0(flag) slSpecialCCalcBitRbg0(flag)
extern void setASC_1to8(uint8_t *, void *, uint32_t, uint8_t);
extern void setASC_4to8(uint8_t *, void *, uint32_t, uint8_t);
extern void setASC_1to4(uint8_t *, void *, uint32_t, uint8_t, int32_t);
extern uint8_t SGL_ASCII_CG[];

/** @}*/

/* BitMap graphics library */

extern bool slInitBitMap(uint16_t, uint16_t, void *);
#define slInitBitMapNbg0(sz, adr) slInitBitMap(bmNBG0, sz, adr)
#define slInitBitMapNbg1(sz, adr) slInitBitMap(bmNBG1, sz, adr)
#define slInitBitMapRbg0(sz, adr) slInitBitMap(bmRBG0, sz, adr)
extern void slBitMapBase(int16_t, int16_t);
extern int8_t *slLocateBitMap(int16_t, int16_t);
extern void slClearBitMap(int8_t);
extern bool slBMPset(int16_t, int16_t, int8_t);
extern int8_t slBMPoint(int16_t, int16_t);
extern void slBMCircle(int16_t, int16_t, int16_t, int8_t);
extern bool slBMLine(int16_t, int16_t, int16_t, int16_t, int8_t);
extern bool slBMBox(int16_t, int16_t, int16_t, int16_t, int8_t);
extern bool slBMBoxFill(int16_t, int16_t, int16_t, int16_t, int8_t);
extern bool slBMGet(int16_t, int16_t, int16_t, int16_t, int8_t *);
extern bool slBMPut(int16_t, int16_t, int16_t, int16_t, int8_t *);
extern bool slBMSprPut(int16_t, int16_t, int16_t, int16_t, int8_t *);

#ifdef ST_VE
/* Graphics enhancer control */

extern bool gxInitEnhancer(uint16_t, uint16_t, uint16_t);
#define EGX_IMMPAL 0    /* Pallet direct */
#define EGX_PAL15 1     /* Pallet indirect 15-bit color */
#define EGX_PAL24 2     /* Pallet indirect 24-bit color */
#define EGX_PAL_RGB15 3 /* Palette, RGB mixed 15-bit color */
#define EGX_PAL_RGB24 4 /* Palette, RGB mixed 24-bit color */

#define USE_EXVDP1 0
#define USE_DSPDMA 1

extern void gxFocus(FIXED, FIXED);
extern void gxCalcMode(uint16_t);
#define CALCNVECTOR 1 /* Normal recalculation */
#define USENVECTOR 0  /* Normal use */

#define NOSHADE 2 /* No shading */
#define FLAT 4    /* Flat shading */
#define GOURAUD 8 /* Gouraud shading */

extern void gxSetMaterial(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t);

#define EFUNC_LoadTrans 0x0c0c0c0c
#define EFUNC_FlashMatrix 0x10101010
#define EFUNC_BaseMatrix 0x11111111
#define EFUNC_PushMatrix 0x12121212
#define EFUNC_PopMatrix 0x13131313
#define EFUNE_Translate 0x14141414
#define EFUNE_Scale 0x15151515
#define EFUNE_RotX 0x16161616
#define EFUNE_RotY 0x17171717
#define EFUNE_RotZ 0x18181818
#define EFUNE_MultiMatrix 0x19191919
#define EFUNC_RotLight 0x1a1a1a1a

extern void gxFuncOut(uint32_t);
extern void gxFuncOut2(uint32_t, uint32_t);
extern void gxFuncOut3(uint32_t, uint32_t, uint32_t, uint32_t);
#define gxFlashMatrix() gxFuncOut(EFUNC_FlashMatrix)
#define gxBaseMatrix() gxFuncOut(EFUNC_BaseMatrix)
#define gxPushMatrix() gxFuncOut(EFUNC_PushMatrix)
#define gxPopMatrix() gxFuncOut(EFUNC_PopMatrix)
#define gxRotLight() gxFuncOut(EFUNC_RotLight)

#define gxLoadTrans(x, y, z) gxFuncOut3(EFUNC_LoadTrans, x, y, z)
#define gxTranslate(x, y, z) gxFuncOut3(EFUNC_Translate, x, y, z)
#define gxScale(x, y, z) gxFuncOut3(EFUNC_Scale, x, y, z)
#define gxRotX(ax) gxFuncOut2(EFUNC_RotX, ax)
#define gxRotY(ay) gxFuncOut2(EFUNC_RotY, ay)
#define gxRotZ(az) gxFuncOut2(EFUNC_RotZ, az)

extern void gxMatrixOut(uint32_t, MATRIX);
#define gxLoadMatrix(mtrx) gxMatrixOut(EFUNC_LoadMatrix, mtrx)
#define gxMultiMatrix(mtrx) gxMatrixOut(EFUNC_MultiMatrix, mtrx)

extern void gxPutPolygon(gxPDATA *);
#endif

/* Peripheral control */

#define SMPC_MASK_STE 0x80      /* SETTIME executed mask */
#define SMPC_MASK_RESD 0x40     /* Reset enable mask */
#define SMPC_MASK_WEEK 0xf0     /* RTC day mask */
#define SMPC_MASK_MONTH 0x0f    /* RTC moon mask */
#define SMPC_MASK_CTG1 0x02     /* Cartridge code 1 */
#define SMPC_MASK_CTG0 0x01     /* Cartridge code 0 */
#define SMPC_MASK_DOTSEL 0x4000 /* DOTSEL signal */
#define SMPC_MASK_MSHNMI 0x0800 /* MSCHNMI signal */
#define SMPC_MASK_SYSRES 0x0200 /* SSREES signal */
#define SMPC_MASK_SNDRES 0x0100 /* Sndres signal */
#define SMPC_MASK_CDRES 0x0040  /* CDRES signal */

#define SMPC_SUN (0 << 4) /* RTC day number (Sunday) */
#define SMPC_MON (1 << 4) /* RTC day number (Monday) */
#define SMPC_TUE (2 << 4) /* RTC day number (Tuesday) */
#define SMPC_WED (3 << 4) /* RTC day number (Wednesday) */
#define SMPC_THU (4 << 4) /* RTC day number (Thursday) */
#define SMPC_FRI (5 << 4) /* RTC day number (Friday) */
#define SMPC_SAT (6 << 4) /* RTC day number (Saturday) */

#define PER_RESULT_MAX 30 /* Maximum number of peripherals */

#define PER_ID_StnPad 0x02       /* Saturn standard pad */
#define PER_ID_StnLacing 0x13    /* Saturn Racing Controller */
#define PER_ID_StnAnalog 0x15    /* Saturn analog joystick */
#define PER_ID_StnMouse 0x23     /* Saturn mouse */
#define PER_ID_StnShooting 0x25  /* Saturn shooting controller */
#define PER_ID_StnKeyBoard 0x34  /* Saturn keyboard */
#define PER_ID_MD3BtnPad 0xe1    /* Megadrive 3 button pad */
#define PER_ID_MD6BtnPad 0xe2    /* Megadrive 6 button pad */
#define PER_ID_ShuttleMouse 0xe3 /* Shuttle mouse */
#define PER_ID_ExtDigital 0x00   /* Extended size digital device */
#define PER_ID_ExtAnalog 0x10    /* Extended size analog device */
#define PER_ID_ExtPointing 0x20  /* Extended size pointing device */
#define PER_ID_ExtKeyBoard 0x30  /* Extended size keyboard device */
#define PER_ID_ExtMegaDrive 0xe0 /* Extended size mega drive device */
#define PER_ID_NotConnect 0xff   /* disconnected */

#define MEGA_ID_StnShooting 0x0a /* Mega drive light gun ID */

/* Digital devices */
#define PER_DGT_KR (1 << 15) /* Right Key */
#define PER_DGT_KL (1 << 14) /* Left Key */
#define PER_DGT_KD (1 << 13) /* Down Key */
#define PER_DGT_KU (1 << 12) /* Up Key */
#define PER_DGT_ST (1 << 11) /* Start */
#define PER_DGT_TA (1 << 10) /* A Trigger */
#define PER_DGT_TC (1 << 9)  /* C Trigger */
#define PER_DGT_TB (1 << 8)  /* B Trigger */
#define PER_DGT_TR (1 << 7)  /* R Trigger */
#define PER_DGT_TX (1 << 6)  /* X Trigger */
#define PER_DGT_TY (1 << 5)  /* Y Trigger */
#define PER_DGT_TZ (1 << 4)  /* Z Trigger */
#define PER_DGT_TL (1 << 3)  /* L Trigger */

/* Keyboard device */
#define PER_KBD_CL (1 << 6) /* Caps Lock */
#define PER_KBD_NL (1 << 5) /* Num Lock */
#define PER_KBD_SL (1 << 4) /* Scroll Lock */
#define PER_KBD_MK (1 << 3) /* Make 	*/
#define PER_KBD_BR (1 << 0) /* Break */

enum SmpcSelect
{                  /* SMPC I / O select mode */
  SMPC_SH2_DIRECT, /* (00) SH2 Direct */
  SMPC_CONTROL     /* (01) SMPC control */
};

enum SmpcExtLatch
{               /* SMPC external latch input mode */
  SMPC_EXL_ENA, /* (00) External latch input enable */
  SMPC_EXL_DIS  /* (01) External latch input prohibited */
};

enum SmpcResetButton
{              /* SMPC reset button status */
  SMPC_RES_ON, /* (00) Reset button ON */
  SMPC_RES_OFF /* (01) Reset button OFF */
};

enum SmpcAreaCode
{                    /* Area code */
  SMPC_AREA_RSV_0,   /* (00) Use prohibited */
  SMPC_AREA_JP,      /* (01) Japan region */
  SMPC_AREA_AJ_NTSC, /* (02) Asia NTSC region */
  SMPC_AREA_RSV_3,   /* (03) Use prohibited */
  SMPC_AREA_NA,      /* (04) Northern region */
  SMPC_AREA_SA_NTSC, /* (05) Central South NTSC region */
  SMPC_AREA_KO,      /* (06) South Korea */
  SMPC_AREA_RSV_7,   /* (07) Use prohibited */
  SMPC_AREA_RSV_8,   /* (08) Use prohibited */
  SMPC_AREA_RSV_9,   /* (09) Use prohibited */
  SMPC_AREA_AJ_PAL,  /* (0a) Asia PAL region */
  SMPC_AREA_RSV_B,   /* (0b) Use prohibited */
  SMPC_AREA_EU_PAL,  /* (0c) Europe PAL region */
  SMPC_AREA_SA_PAL,  /* (0d) Central South Rice PAL area */
  SMPC_AREA_RSV_E,   /* (0e) Use prohibited */
  SMPC_AREA_RSV_F    /* (0f) Use prohibited */
};

enum SmpcOptimize
{               /* Acquisition time optimization mode */
  SMPC_OPT_DIS, /* (00) Prohibition of optimization of acquisition time */
  SMPC_OPT_ENA  /* (01) Obtain permission for time optimization */
};

enum SmpcPortMode
{                /*SMPC port mode*/
  SMPC_PORT_15,  /*(00) 15 byte mode */
  SMPC_PORT_255, /*(01) 255 byte mode */
  SMPC_PORT_RSV, /*(02) Setting prohibited */
  SMPC_PORT_ZERO /*(03) 0 byte mode */
};

enum SmpcLanguage
{                /*SMPC memory language number */
  SMPC_ENGLISH,  /*(00) English */
  SMPC_DEUTSCH,  /*(01) German */
  SMPC_FRANCAIS, /*(02) French */
  SMPC_ESPANOL,  /*(03) Spanish */
  SMPC_ITALIANO, /*(04) Italian */
  SMPC_JAPAN     /*(05) Japanese */
};

enum SmpcSoundEffect
{                  /* SMPC memory sound effect mode */
  SMPC_EFFECT_OFF, /* (00) Effect sound OFF */
  SMPC_EFFECT_ON   /* (01) Effect sound ON */
};

enum SmpcSoundOutput
{                   /* SMPC memory audio output mode */
  SMPC_SOUND_MONO,  /* (00) Mono */
  SMPC_SOUND_STEREO /* (01) Stereo */
};

enum SmpcHelpWindow
{                /* SMPC memory help window mode */
  SMPC_HELP_DIS, /* (00) Hide */
  SMPC_HELP_ENA  /* (01) Display */
};

enum SmpcMonth
{           /* RTC month number */
  SMPC_DMY, /* (00) Dummy */
  SMPC_JAN, /* (01) January */
  SMPC_FEB, /* (02) February */
  SMPC_MAR, /* (03) March */
  SMPC_APR, /* (04) April */
  SMPC_MAY, /* (05) May */
  SMPC_JUN, /* (06) June */
  SMPC_JUL, /* (07) July */
  SMPC_AUG, /* (08) August */
  SMPC_SEP, /* (09) 月 September */
  SMPC_OCT, /* (0a) October */
  SMPC_NOV, /* (0b) November */
  SMPC_DEC  /* (0c) December */
};

enum SmpcCommand
{              /* SMPC command number */
  SMPC_MSHON,  /* (00) Master SH2 ON */
  SMPC_SSHON,  /* (01) Slave SH2 ON */
  SMPC_SSHOFF, /* (02) Slave SH2 OFF */
  SMPC_SNDON,  /* (03) Sound ON */
  SMPC_SNDOFF, /* (04) Sound OFF */
  SMPC_CDON,   /* (05) ＣＤ　ＯＮ			*/
  SMPC_CDOFF,  /* (06) ＣＤ　ＯＦＦ			*/
  SMPC_SYSRES, /* (07) Reset entire system */
  SMPC_CKC352, /* (08) Clock change 352 mode */
  SMPC_CKC320, /* (09) Clock Change 320 mode */
  SMPC_NMIREQ, /* (0a) NMI request */
  SMPC_RESENA, /* (0b) Reset enable */
  SMPC_RESDIS, /* (0c) Reset disabled */
  SMPC_GETSTS, /* (0d) Get status */
  SMPC_GETPER, /* (0e) Get peripheral */
  SMPC_SETMEM, /* (0f) SMPC memory setting */
  SMPC_SETTIM, /* (10) Time setting */
  SMPC_CMDMAX  /* (11) Maximum command number */
};

enum SmpcWaitMode
{               /* SMPC command execution mode */
  SMPC_NO_WAIT, /* (00) No waiting for command execution */
  SMPC_WAIT     /* (01) Waiting for command execution to end */
};

typedef struct
{                 /* RTC moment */
    uint16_t year;  /* West Day Year */
    uint8_t month;  /* Day of the week and month */
    uint8_t date;   /* Day */
    uint8_t hour;   /* Time 	*/
    uint8_t minute; /* Minute 	*/
    uint8_t second; /* second 	*/
    uint8_t dummy;  /* Dummy */
} SmpcDateTime;

typedef struct
{                     /* SMPC status */
    uint8_t cond;       /* State status */
    uint8_t dummy1;     /* Dummy 1 */
    uint16_t dummy2;    /* Dummy 2 */
    SmpcDateTime rtc; /* RTC moment */
    uint8_t ctg;        /* Cartridge code */
    uint8_t area;       /* Area code */
    uint16_t system;    /* System status */
    uint32_t smem;      /* SMPC memory holding data */
} SmpcStatus;

typedef struct
{                     /* Digital device */
    uint8_t id;         /* Peripheral ID */
    uint8_t ext;        /* Extended data size */
    uint16_t data;      /* Button current data */
    uint16_t push;      /* Button press data */
    uint16_t pull;      /* Button pull-up data */
    uint32_t dummy2[4]; /* Dummy 2 */
} PerDigital;

typedef struct
{                     /* Analog device */
    uint8_t id;         /* Peripheral ID */
    uint8_t ext;        /* Extended data size */
    uint16_t data;      /* Button current data */
    uint16_t push;      /* Button press data */
    uint16_t pull;      /* Button pull-up data */
    uint8_t x;          /* X-axis data absolute value */
    uint8_t y;          /* Y-axis data absolute value */
    uint8_t z;          /* Z-axis data absolute value */
    uint8_t dummy1;     /* Dummy 1 */
    uint32_t dummy2[3]; /* Dummy 2 */
} PerAnalog;

typedef struct
{                     /* pointing device		*/
    uint8_t id;         /* Peripheral ID */
    uint8_t ext;        /* Extended data size */
    uint16_t data;      /* Button current data */
    uint16_t push;      /* Button press data */
    uint16_t pull;      /* Button pull-up data */
    uint16_t x;         /* X coordinate */
    uint16_t y;         /* Ｙ Coordinates */
    uint32_t dummy2[3]; /* Dummy 2 */
} PerPoint;

typedef struct
{                     /* Keyboard device */
    uint8_t id;         /* Peripheral ID */
    uint8_t ext;        /* Extended data size */
    uint16_t data;      /* Button current data */
    uint16_t push;      /* Button press data */
    uint16_t pull;      /* Button pull-up data */
    uint8_t cond;       /* Status data */
    uint8_t code;       /* Key code */
    uint16_t dummy1;    /* Dummy 1 */
    uint32_t dummy2[3]; /* Dummy 2 */
} PerKeyBoard;

extern uint8_t slGetPortDir1(void);
extern uint8_t slGetPortDir2(void);
extern bool slSetPortDir1(uint8_t);
extern bool slSetPortDir2(uint8_t);

extern uint8_t slGetPortData1(void);
extern uint8_t slGetPortData2(void);
extern void slSetPortData1(uint8_t);
extern void slSetPortData2(uint8_t);

extern uint8_t slGetPortSelect1(void);
extern uint8_t slGetPortSelect2(void);
extern bool slSetPortSelect1(uint8_t);
extern bool slSetPortSelect2(uint8_t);

extern uint8_t slGetPortExt1(void);
extern uint8_t slGetPortExt2(void);
extern bool slSetPortExt1(uint8_t);
extern bool slSetPortExt2(uint8_t);

extern uint8_t slGetLanguage(void);
extern void slSetLanguage(uint8_t);

extern uint8_t slGetSoundEffect(void);
extern void slSetSoundEffect(uint8_t);

extern uint8_t slGetSoundOutput(void);
extern void slSetSoundOutput(uint8_t);

extern uint8_t slGetHelpWindow(void);
extern void slSetHelpWindow(uint8_t);

extern void slInitPeripheral(void);
extern bool slSetResultBuffer(uint8_t *, PerDigital *, uint16_t);

/** @brief Missing in normal release
 */
extern bool slCheckIntBackSet(void);

extern bool slIntBackCancel(void);
extern uint8_t slCheckReset(void);
extern bool slClearReset(void);

extern uint8_t slGetOptimize(void);
extern bool slSetOptimize(uint8_t);

extern uint8_t slGetPortMode1(void);
extern uint8_t slGetPortMode2(void);
extern bool slSetPortMode1(uint8_t);
extern bool slSetPortMode2(uint8_t);

extern bool slRequestCommand(uint8_t, uint8_t);

extern SmpcStatus *Smpc_Status;
extern PerDigital *Smpc_Peripheral;
extern uint8_t Per_Connect1, Per_Connect2;

#ifdef TITAN
typedef struct
{ /* System Switch */
    uint8_t dummy;
    uint8_t data;
    uint8_t push;
    uint8_t pull;
} SysSwitch;

extern PerDigital PerDigitalBuf[4];
extern SysSwitch SystemSwitchBuf;

#define _ServiceSw 0x80 /* ORed switch on board and JAMMA input */
#define _TestSw 0x40
#define _ServiceSwBD 0x20 /* Tactile switch on board */
#define _TestSwBD 0x10
#define _ServiceSwJM 0x08 /* Switch input from JAMMA connector */
#define _TestSwJM 0x04
#define _Coin2 0x02
#define _Coin1 0x01
#endif

#ifdef ST_V
extern PerDigital PerDigitalBuf[4];
#endif
#ifdef ST_VE
extern PerDigital PerDigitalBuf[4];
#endif

#define slMasterOn() slRequestCommand(SMPC_MSHON, SMPC_NO_WAIT)
#define slSlaveOn() slRequestCommand(SMPC_SSHON, SMPC_NO_WAIT)
#define slSlaveOff() slRequestCommand(SMPC_SSHOFF, SMPC_NO_WAIT)

#ifdef TITAN
extern void slSoundOn();
extern void slSoundOff();
#else
#define slSoundOn() slRequestCommand(SMPC_SNDON, SMPC_NO_WAIT)
#define slSoundOff() slRequestCommand(SMPC_SNDOFF, SMPC_NO_WAIT)
#endif

#define slCDOn() slRequestCommand(SMPC_CDON, SMPC_NO_WAIT)
#define slCDOff() slRequestCommand(SMPC_CDOFF, SMPC_NO_WAIT)
#define slSystemReset() slRequestCommand(SMPC_SYSRES, SMPC_NO_WAIT)
#define slClockChange352() slRequestCommand(SMPC_CKC352, SMPC_NO_WAIT)
#define slClockChange320() slRequestCommand(SMPC_CKC320, SMPC_NO_WAIT)
#define slNMIRequest() slRequestCommand(SMPC_NMIREQ, SMPC_NO_WAIT)
#define slResetEnable() slRequestCommand(SMPC_RESENA, SMPC_NO_WAIT)
#define slResetDisable() slRequestCommand(SMPC_RESDIS, SMPC_NO_WAIT)
#define slGetStatus() slRequestCommand(SMPC_GETSTS, SMPC_NO_WAIT)
#define slGetPeripheral() slRequestCommand(SMPC_GETPER, SMPC_NO_WAIT)
#define slSetSmpcMemory() slRequestCommand(SMPC_SETMEM, SMPC_NO_WAIT)
#define slSetDateTime() slRequestCommand(SMPC_SETTIM, SMPC_NO_WAIT)

#define slMasterOnWait() slRequestCommand(SMPC_MSHON, SMPC_WAIT)
#define slSlaveOnWait() slRequestCommand(SMPC_SSHON, SMPC_WAIT)
#define slSlaveOffWait() slRequestCommand(SMPC_SSHOFF, SMPC_WAIT)
#define slSoundOnWait() slRequestCommand(SMPC_SNDON, SMPC_WAIT)
#define slSoundOffWait() slRequestCommand(SMPC_SNDOFF, SMPC_WAIT)
#define slCDOnWait() slRequestCommand(SMPC_CDON, SMPC_WAIT)
#define slCDOffWait() slRequestCommand(SMPC_CDOFF, SMPC_WAIT)
#define slNMIRequestWait() slRequestCommand(SMPC_NMIREQ, SMPC_WAIT)
#define slResetEnableWait() slRequestCommand(SMPC_RESENA, SMPC_WAIT)
#define slResetDisableWait() slRequestCommand(SMPC_RESDIS, SMPC_WAIT)
#define slSetSmpcMemoryWait() slRequestCommand(SMPC_SETMEM, SMPC_WAIT)
#define slSetDateTimeWait() slRequestCommand(SMPC_SETTIM, SMPC_WAIT)

/* Event library	*/

extern void slInitEvent(void);
extern EVENT *slGetEvent(void);
extern void slReturnEvent(EVENT *);
extern EVENT *slSetEvent(void (*func)());
extern void slCloseEvent(EVENT *);
extern EVENT *slSetEventNext(EVENT *, void (*func)());
extern void slExecuteEvent(void);
extern WORK *slGetWork(void);
extern void slReturnWork(WORK *);

/* DMA control */

extern void slDMACopy(void *, void *, uint32_t);
extern void slDMAXCopy(void *, void *, uint32_t, uint16_t);
extern void slDMAWait();
extern bool slDMAStatus();

/* DMA control with SBL */

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */

/* Constant (CPU-DMA) */
/**** DMA register bit position **************************************************/
/******** Channel control register ***************************************/
#define DMA_CHCR_DE 0  /* DMA enable bit */
#define DMA_CHCR_TE 1  /* Transfer end flag bit */
#define DMA_CHCR_IE 2  /* Interrupt enable bit */
#define DMA_CHCR_TA 3  /* Transfer address mode bit */
#define DMA_CHCR_TB 4  /* Transfer bus mode bit */
#define DMA_CHCR_DL 5  /* DREQ level bit */
#define DMA_CHCR_DS 6  /* DREQ select bit */
#define DMA_CHCR_AL 7  /* Acknowledge level bit */
#define DMA_CHCR_AM 8  /* Acknowledge / transfer mode bit */
#define DMA_CHCR_AR 9  /* Auto request bit */
#define DMA_CHCR_TS 10 /* Transfer size bit */
#define DMA_CHCR_SM 12 /* Source address mode bit */
#define DMA_CHCR_DM 14 /* Destination address mode bit */
/******** DMA operation register ******************************************/
#define DMA_DMAOR_DME 0  /* DMA master enable bit */
#define DMA_DMAOR_NMIF 1 /* NMI flag bit */
#define DMA_DMAOR_AE 2   /* Address error flag bit */
#define DMA_DMAOR_PR 3   /* Priority mode bit */

/*****************************************************************************/
/**** DMA channel ************************************************************/
/*****************************************************************************/
#define DMA_CPU_CH0 0 /* Channel 0 */
#define DMA_CPU_CH1 1 /* Channel 1 */

/**** (DmaCpuComPrm) Common transfer parameters ***************************************/
/******** (pr) Priority mode ******************************************/
#define DMA_CPU_FIX (0 << DMA_DMAOR_PR) /* Priority is fixed */
#define DMA_CPU_ROR (1 << DMA_DMAOR_PR) /* Priority depends on round robin */
/******** (dme) DMA master enable *******************************************/
#define DMA_CPU_DIS (0 << DMA_DMAOR_DME) /* Disable DMA transfer for all channels */
#define DMA_CPU_ENA (1 << DMA_DMAOR_DME) /* Permit DMA transfer for all channels */
/******** (msk)マスクビット **************************************************/
#define DMA_CPU_M_PR (1 << 0)   /* Priority mode */
#define DMA_CPU_M_AE (1 << 1)   /* Address error flag */
#define DMA_CPU_M_NMIF (1 << 2) /* NMI flag */
#define DMA_CPU_M_DME (1 << 3)  /* DMA master enable */

/****(DmaCpuPrm) Transfer parameters **********************************************/
/******** (dm) Destination address mode bits ***********************/
/******** (sm) Source address mode bits *************************************/
#define DMA_CPU_AM_NOM 0x0 /* fixed */
#define DMA_CPU_AM_ADD 0x1 /* increase */
#define DMA_CPU_AM_SUB 0x2 /* cut back */
/******** (ts) Transfer size *********************************************/
#define DMA_CPU_1 (0 << DMA_CHCR_TS)  /* In bytes */
#define DMA_CPU_2 (1 << DMA_CHCR_TS)  /* Word (2 bytes) unit */
#define DMA_CPU_4 (2 << DMA_CHCR_TS)  /* Longword (4 bytes) unit */
#define DMA_CPU_16 (3 << DMA_CHCR_TS) /* 16 byte transfer */
/******** (ar)オートリクエストモード *****************************************/
#define DMA_CPU_MOD (0 << DMA_CHCR_AR)  /* Module request */
#define DMA_CPU_AUTO (1 << DMA_CHCR_AR) /* Auto request */
/******** (ie) Interrupt enable *****************************************/
#define DMA_CPU_INT_ENA (1 << DMA_CHCR_IE) /* Enable interrupt request */
#define DMA_CPU_INT_DIS (0 << DMA_CHCR_IE) /* Disable interrupt requests (initial value) */
/******** (drcr) DMA request / response selection control ****************************************/
#if 0
#define DMA_CPU_DREQ 0x0 /* DREQ (external request) */
/*
 * Specifying external requests for DMA requests is prohibited ('95 -11/14 H.O)
 */
#endif
#define DMA_CPU_RXI 0x1 /* RXI (Internal SCI receive data-Full interrupt transfer request) */
#define DMA_CPU_TXI 0x2 /* TXI (Built-in SCI transmit data-Empty interrupt transfer request) */
/******** (msk) mask bit **************************************************/
#define DMA_CPU_M_SAR (1 << 0)  /* DMA source address */
#define DMA_CPU_M_DAR (1 << 1)  /* DMA destination address */
#define DMA_CPU_M_TCR (1 << 2)  /* DMA transfer count */
#define DMA_CPU_M_DM (1 << 3)   /* DMA destination address mode bit */
#define DMA_CPU_M_SM (1 << 4)   /* DMA source address mode bit */
#define DMA_CPU_M_TS (1 << 5)   /* Transfer size */
#define DMA_CPU_M_AR (1 << 6)   /* Auto request mode */
#define DMA_CPU_M_IE (1 << 7)   /* Interrupt enable */
#define DMA_CPU_M_DRCR (1 << 8) /* DMA request / response selection control */
#define DMA_CPU_M_TE (1 << 9)   /* Transfer end flag */

/**** (DmaCpuComStatus) common status ****************************************/
/******** (ae) Address error flag*******************************************/
#define DMA_CPU_ADR_ERR (1 << DMA_DMAOR_AE)    /* Address error by DMAC- occurred */
#define DMA_CPU_ADR_NO_ERR (0 << DMA_DMAOR_AE) /* Address error by DMAC- None */

/******** (nmif) NMI flag ****************************************************/
#define DMA_CPU_NMI_ON (1 << DMA_DMAOR_NMIF)  /* NMI interrupt occurred */
#define DMA_CPU_NMI_OFF (0 << DMA_DMAOR_NMIF) /* No NMI interrupt */

/****(DmaCpuComStatus) common status****************************************/
#define DMA_CPU_TE_MV (0 << DMA_CHCR_TE) /* DMA transfer or DMA transfer interrupted */
#define DMA_CPU_TE_SP (1 << DMA_CHCR_TE) /* DMA transfer completed successfully */

typedef struct
{
    uint32_t pr;  /* Priority mode */
    uint32_t dme; /* DMA master enable */
    uint32_t msk; /* Mask bit */
} DmaCpuComPrm; /* Common transfer parameter */

typedef struct
{
    uint32_t sar;  /* DMA source address */
    uint32_t dar;  /* DMA destination-address */
    uint32_t tcr;  /* DMA transfer count */
    uint32_t dm;   /* Destination address mode bit */
    uint32_t sm;   /*Source address mode bit */
    uint32_t ts;   /*Transfer size */
    uint32_t ar;   /*Auto request mode */
    uint32_t ie;   /*Interrupt enable */
    uint32_t drcr; /*DMA request / response selection control */
    uint32_t msk;  /*Mask bit */
} DmaCpuPrm;     /*Transfer parameter */

typedef struct
{
    uint32_t ae;     /* Address error flag */
    uint32_t nmif;   /* NMI flag */
} DmaCpuComStatus; /* Common status */

typedef uint32_t DmaCpuStatus; /* status */

/* Constant (SCU-DMA) */
/*****************************************************************************/
/**** DMA channel ************************************************************/
/*****************************************************************************/
#define DMA_SCU_CH0 0x0 /* Channel 0 */
#define DMA_SCU_CH1 0x1 /* Channel 1 */
#define DMA_SCU_CH2 0x2 /* Channel 2 */

/**** DMA register bit position **************************************************/
/******** Addition value register *****************************************************/
#define DMA_DXAD_DXRAD 8 /* Read address addition value */
#define DMA_DXAD_DXWAD 0 /* Write address addition value */
/******** DMA enable register ****************************************************/
#define DMA_DXEN_DXGO 0 /* DMA start bit */
#define DMA_DXEN_DXEN 8 /* DMA enable bit */
/******** DMA mode, address update, activation factor register **************************/
#define DMA_DXMD_DXMOD 24 /* Mode bit */
#define DMA_DXMD_DXRUP 16 /* Read address update bit */
#define DMA_DXMD_DXWUP 8  /* Write address update bit */
#define DMA_DXMD_DXFT 0   /* Activation factor selection bit */

/*****************************************************************************/
/**** (DmaScuPrm) transfer parameters **********************************************/
/*****************************************************************************/
/******** (dxad_r) Read address addition value *************************************/
#define DMA_SCU_R0 (0x0 << DMA_DXAD_DXRAD) /* Do not add */
#define DMA_SCU_R4 (0x1 << DMA_DXAD_DXRAD) /* Add 4 bytes */
/******** (dxad_w) Write address addition value ************************************/
#define DMA_SCU_W0 (0x0 << DMA_DXAD_DXWAD)   /* Do not add */
#define DMA_SCU_W2 (0x1 << DMA_DXAD_DXWAD)   /* Add 2 bytes */
#define DMA_SCU_W4 (0x2 << DMA_DXAD_DXWAD)   /* Add 4 bytes */
#define DMA_SCU_W8 (0x3 << DMA_DXAD_DXWAD)   /* Add 8 bytes */
#define DMA_SCU_W16 (0x4 << DMA_DXAD_DXWAD)  /* Add 16 bytes */
#define DMA_SCU_W32 (0x5 << DMA_DXAD_DXWAD)  /* Add 32 bytes */
#define DMA_SCU_W64 (0x6 << DMA_DXAD_DXWAD)  /* Add 64 bytes */
#define DMA_SCU_W128 (0x7 << DMA_DXAD_DXWAD) /* Add 128 bytes */
/******** (dxmod) mode bit ************************************************/
#define DMA_SCU_DIR (0x0 << DMA_DXMD_DXMOD)    /* Direct mode */
#define DMA_SCU_IN_DIR (0x1 << DMA_DXMD_DXMOD) /* Indirect mode */
/******** (dxrup) Read address update bit **********************************/
/******** (dxwup) Write address update bit **********************************/
#define DMA_SCU_KEEP 0x0 /* Hold */
#define DMA_SCU_REN 0x1  /* Update */

/******** (dxft) Activation factor selection bit *******************************************/
#define DMA_SCU_F_VBLK_IN (0x0 << DMA_DXMD_DXFT)  /* V-blank-IN */
#define DMA_SCU_F_VBLK_OUT (0x1 << DMA_DXMD_DXFT) /* V-blank-OUT */
#define DMA_SCU_F_HBLK_IN (0x2 << DMA_DXMD_DXFT)  /* H-blank-IN */
#define DMA_SCU_F_TIM0 (0x3 << DMA_DXMD_DXFT)     /* Timer 0 */
#define DMA_SCU_F_TIM1 (0x4 << DMA_DXMD_DXFT)     /* Timer 1 */
#define DMA_SCU_F_SND (0x5 << DMA_DXMD_DXFT)      /* Sound-Req */
#define DMA_SCU_F_SPR (0x6 << DMA_DXMD_DXFT)      /* Sprite drawing */
#define DMA_SCU_F_DMA (0x7 << DMA_DXMD_DXFT)      /* Set DMA activation factor bit */
/******** (msk) mask bit *************************************************/
#define DMA_SCU_M_DXR (1 << 0) /* Read address */
#define DMA_SCU_M_DXW (1 << 1) /* Write address */

/*****************************************************************************/
/**** (DmaScuStatus) status ***********************************************/
/*****************************************************************************/
/******** (dxbkr) DMA operation flag *********************************************/
/******** (ddmv) The DSP is in DMA operation. *****************************************/
#define DMA_SCU_MV 0x1    /* In action */
#define DMA_SCU_NO_MV 0x0 /* Not working */

typedef struct
{
    uint32_t dxr;    /* Read address */
    uint32_t dxw;    /* Export address */
    uint32_t dxc;    /* Bytes transferred */
    uint32_t dxad_r; /* Read address addition value */
    uint32_t dxad_w; /* Write address addition value */
    uint32_t dxmod;  /* Mode bit */
    uint32_t dxrup;  /* Read address update bit */
    uint32_t dxwup;  /* Write address update bit */
    uint32_t dxft;   /* Activation factor selection bit */
    uint32_t msk;    /* Mask bit */
} DmaScuPrm;       /* Transfer parameter */

typedef struct
{
    uint32_t dxmv; /* DMA operating flag */
} DmaScuStatus;  /* status */

extern void DMA_CpuSetComPrm(DmaCpuComPrm *);
extern void DMA_CpuSetPrm(DmaCpuPrm *, uint32_t);
extern void DMA_CpuStart(uint32_t);
extern void DMA_CpuStop(uint32_t);
extern void DMA_CpuAllStop();
extern void DMA_CpuGetComStatus(DmaCpuComStatus *);
extern DmaCpuStatus DMA_CpuGetStatus(uint32_t);
extern void DMA_ScuSetPrm(DmaScuPrm *, uint32_t);
extern void DMA_ScuStart(uint32_t);
extern void DMA_ScuGetStatus(DmaScuStatus *, uint32_t);

extern void DMA_ScuMemCopy(void *, void *, uint32_t);
extern uint32_t DMA_ScuResult();

extern void DMA_CpuMemCopy1(void *, void *, uint32_t);
extern void DMA_CpuMemCopy2(void *, void *, uint32_t);
extern void DMA_CpuMemCopy4(void *, void *, uint32_t);
extern void DMA_CpuMemCopy16(void *, void *, uint32_t);
extern uint32_t DMA_CpuResult();

/* Slave CPU control */
extern void slSlaveFunc(void (*func)(void*), void *);

/* System library */
extern void slInitSystem(uint16_t, TEXTURE *, int8_t);
extern void slSetTVMode(uint16_t);
extern void slSetScrTVMode(uint16_t);
extern void slSetSprTVMode(uint16_t);
extern bool slDynamicFrame(uint16_t);
extern void slSynch(void);
extern void slInitSynch(void);
extern bool slTransferEntry(void *, void *, uint16_t);
extern void slIntFunction(void (*func)());
extern void slSynchFunction(void (*func)());
extern void SetCDFunc(void (*func)());
extern void slCashPurge();

/* Sound library */
extern void slInitSound(uint8_t *, uint32_t, uint8_t *, uint32_t);
extern bool slBGMOn(uint16_t, uint8_t, uint8_t, uint8_t);
extern bool slBGMPause();
extern bool slBGMCont();
extern bool slBGMOff();
extern bool slBGMFade(uint8_t, uint8_t);
extern bool slBGMTempo(int16_t);
#define slBGMStat() slSequenceStat(0)
extern bool slSoundAllOff();
extern bool slSoundAllPause();
extern bool slSoundAllCont();
extern bool slDSPOff();
extern bool slSndVolume(uint8_t);
extern uint8_t slSequenceOn(uint16_t, uint8_t, uint8_t, int8_t);
extern bool slSequenceOff(uint8_t);
extern bool slSequenceFade(uint8_t, uint8_t, uint8_t);
extern bool slSequenceTempo(uint8_t, int16_t);
extern bool slSequencePause(uint8_t);
extern bool slSequenceCont(uint8_t);
extern bool slSequencePan(uint8_t, int8_t);
extern bool slSequenceStat(uint8_t);
extern bool slSequenceReset(uint8_t);
extern void *slSndMapChange(uint8_t);
extern void slWaitSound(void *);
extern bool slCDDAOn(uint8_t, uint8_t, int8_t, int8_t);
extern bool slCDDAOff();
extern int8_t slPCMOn(PCM *, void *, uint32_t);
extern bool slPCMOff(PCM *);
extern bool slPCMParmChange(PCM *);
extern bool slPCMStat(PCM *);
extern int8_t slSndPCMNum(uint8_t);
extern uint8_t slSndSeqNum();
extern bool slSndEffect(uint8_t);
extern bool slSndMixChange(uint8_t, uint8_t);
extern bool slSndMixParmChange(uint8_t, uint8_t, int8_t);
extern int8_t slSoundRequest(const char *, ...);
extern void slSndFlush();

/* Gouraud library */

typedef uint16_t GOURAUDTBL[4];

extern void slInitGouraud(GOURAUDTBL *, uint32_t, uint32_t, uint8_t *);
extern void slPutPolygonX(XPDATA *, FIXED *);
extern void slGouraudTblCopy(void);
extern void slSetGouraudTbl(uint16_t *);
extern void slSetGouraudColor(uint16_t);
extern void slSetAmbient(uint16_t);

extern void slSetFlatColor(uint16_t);

extern void slSetDepthLimit(uint32_t, uint16_t, uint16_t);
extern void slSetDepthTbl(uint16_t *, uint16_t, uint16_t);

extern void slNearClipFlag(uint32_t);
extern void slDispPlaneLevel(int32_t);
extern void slWindowClipLevel(int32_t);

/*------------------------------------------------------------------------*/

#endif

/** @}*/
