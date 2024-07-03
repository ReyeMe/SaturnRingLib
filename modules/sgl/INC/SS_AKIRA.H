#define     toFIXEDa(p)                 ((FIXED)(p * 65536.0 * (-300.0)))
#define     RAGtoANG(r)                 ((int16_t)(r * 65536.0 / ( 2 * 3.1415 )))

typedef struct mo{
    FIXED   HARApos[XYZ] ;
    ANGLE   HARAang[XYZ] ;
    ANGLE   MUNEang[XYZ] ;
    ANGLE   KAOang[XYZ] ;
    ANGLE   R_KATAang[XYZ] ;
    ANGLE   R_UDEang ;
    ANGLE   R_TEang[XYZ] ;
    ANGLE   L_KATAang[XYZ] ;
    ANGLE   L_UDEang ;
    ANGLE   L_TEang[XYZ] ;
    ANGLE   KOSHIang[XYZ] ;
    ANGLE   R_MOMOang[XYZ] ;
    ANGLE   R_HIZAang ;
    ANGLE   R_ASHIang[XYZ] ;
    ANGLE   L_MOMOang[XYZ] ;
    ANGLE   L_HIZAang ;
    ANGLE   L_ASHIang[XYZ] ;
} MOTION ;

#define    WALK_CNT           60
#define    SUSO_CNT           68

#define    INITPOSX           toFIXED(0.0)
#define    INITPOSY           toFIXED(100.0)
#define    INITPOSZ           toFIXED(15000.0)
#define    INITANGX           DEG2ANG(0.0)
#define    INITANGY           DEG2ANG(90.0)
#define    INITANGZ           DEG2ANG(0.0)

extern     void slInitSystem(uint16_t , TEXTURE * , int8_t) ;
extern     void objectdata(ANGLE * , FIXED *) ;
extern     void Debuge() ;

extern     uint16_t TotalPolygon , DispPolygons ;

