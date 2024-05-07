#include "sl_def.h"



#define _Byte_ sizeof(Uint8)
#define _LongWord_ sizeof(Uint32)
#define _Sprite_ (sizeof(Uint16) * 18)

// #define SGL_MAX_VERTICES 2500 /* number of vertices that can be used */
// #define SGL_MAX_POLYGONS 1700 /* number of polygons that can be used */

struct WorkArea_
{
    char SortList[(_LongWord_ * 3) * (SGL_MAX_POLYGONS + 6)];
    char Zbuffer[_LongWord_ * 512];
    char SpriteBuf[_Sprite_ * ((SGL_MAX_POLYGONS + 6) * 2)];
    char Pbuffer[(_LongWord_ * 4) * SGL_MAX_VERTICES];
    char CLOfstBuf[(_Byte_ * 32 * 3) * 32];
    char CommandBuf[(_LongWord_ * 8) * SGL_MAX_POLYGONS];
    char TransList[0x4400];
    char MasterStack[0x400];
};

struct WorkArea_ __attribute__((section("WORK_AREA_DUMMY"))) WORK_AREA_DUMMY;
struct WorkArea_ __attribute__((aligned(0x10), used, section("WORK_AREA"))) WorkArea;

const void* PCM_Work = (void*)SoundRAM + 0x78000; /* PCM Stream Address      */
const Uint32 PCM_WkSize = 0x8000;                 /* PCM Stream Size         */
const void* SlaveStack = (void*)0x06001e00;       /* SlaveSH2  StackPointer  */
const void* TransList = (void*)0x060fb800;        /* DMA Transfer Table      */
const void* MasterStack =  (void*)0x060ffc00;     /* MasterSH2 StackPointer  */

const Uint16 MaxVertices = SGL_MAX_VERTICES;
const Uint16 MaxPolygons = SGL_MAX_POLYGONS;
const void* SortList = WorkArea.SortList;
const Uint32 SortListSize = sizeof(WorkArea.SortList);
const void* Zbuffer = WorkArea.Zbuffer;
const void* SpriteBuf = WorkArea.SpriteBuf;
const Uint32 SpriteBufSize = sizeof(WorkArea.SpriteBuf);
const void* Pbuffer = WorkArea.Pbuffer;
const void* CLOfstBuf = WorkArea.CLOfstBuf;
const void* CommandBuf = WorkArea.CommandBuf;

// #define SGL_MAX_EVENTS 64 /* number of events that can be used   */
const Uint16 EventSize = sizeof(EVENT);
const Uint16 MaxEvents = SGL_MAX_EVENTS;
EVENT EventBuf[SGL_MAX_EVENTS];
EVENT* RemainEvent[SGL_MAX_EVENTS];

// #define SGL_MAX_WORKS 64 /* number of works that can be used    */
const Uint16 WorkSize = sizeof(WORK);
const Uint16 MaxWorks = SGL_MAX_WORKS;
WORK WorkBuf[SGL_MAX_WORKS];
WORK* RemainWork[SGL_MAX_WORKS];
