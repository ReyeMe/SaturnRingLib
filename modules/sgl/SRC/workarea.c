#pragma GCC optimize("O0")
#include "sl_def.h"

#define _Byte_ sizeof(uint8_t)
#define _LongWord_ sizeof(uint32_t)
#define _Sprite_ (sizeof(uint16_t) * 18)

struct WorkArea_
{
    char __attribute__((aligned(0x10))) SortList[(_LongWord_ * 3) * (SGL_MAX_POLYGONS + 6)];
    char __attribute__((aligned(0x10))) Zbuffer[_LongWord_ * 512];
    char __attribute__((aligned(0x10))) SpriteBuf[_Sprite_ * ((SGL_MAX_POLYGONS + 6) * 2)];
    char __attribute__((aligned(0x10))) Pbuffer[(_LongWord_ * 4) * SGL_MAX_VERTICES];
    char __attribute__((aligned(0x10))) CLOfstBuf[(_Byte_ * 32 * 3) * 32];
};

struct CommandBufArea_
{
    char CommandBuf[SGL_SLAVE_BUF_SIZE];
};

// Start must be aligned to 0x1000
struct WorkArea_ __attribute__((section("WORK_AREA_DUMMY"))) WORK_AREA_DUMMY;
struct WorkArea_ __attribute__((aligned(0x1000), used, section("WORK_AREA"))) WorkArea;

// Contains commands for slave CPU
struct CommandBufArea_ __attribute__((section("COMMAND_BUF_DUMMY"))) COMMAND_BUF_DUMMY;
struct CommandBufArea_ __attribute__((aligned(0x20), used, section("COMMAND_BUF"))) CommandBufArea;

const void* PCM_Work = (void*)SoundRAM + 0x78000; /* PCM Stream Address      */
const uint32_t PCM_WkSize = 0x8000;                 /* PCM Stream Size         */
const void* SlaveStack = (void*)0x06001e00;       /* SlaveSH2  StackPointer  */
const void* TransList = (void*)0x060fb800;        /* DMA Transfer Table      */
const void* MasterStack =  (void*)0x060ffc00;     /* MasterSH2 StackPointer  */

const uint16_t MaxVertices = SGL_MAX_VERTICES;
const uint16_t MaxPolygons = SGL_MAX_POLYGONS;
const void* SortList = WorkArea.SortList;
const uint32_t SortListSize = sizeof(WorkArea.SortList);
const void* Zbuffer = WorkArea.Zbuffer;
const void* SpriteBuf = WorkArea.SpriteBuf;
const uint32_t SpriteBufSize = sizeof(WorkArea.SpriteBuf);
const void* Pbuffer = WorkArea.Pbuffer;
const void* CLOfstBuf = WorkArea.CLOfstBuf;
const void* CommandBuf = CommandBufArea.CommandBuf;

// #define SGL_MAX_EVENTS 64 /* number of events that can be used   */
const uint16_t EventSize = sizeof(EVENT);
const uint16_t MaxEvents = SGL_MAX_EVENTS;
EVENT EventBuf[SGL_MAX_EVENTS];
EVENT* RemainEvent[SGL_MAX_EVENTS];

// #define SGL_MAX_WORKS 64 /* number of works that can be used    */
const uint16_t WorkSize = sizeof(WORK);
const uint16_t MaxWorks = SGL_MAX_WORKS;
WORK WorkBuf[SGL_MAX_WORKS];
WORK* RemainWork[SGL_MAX_WORKS];
#pragma GCC reset_options
