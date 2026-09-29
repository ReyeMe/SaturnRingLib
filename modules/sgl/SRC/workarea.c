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

// SGL hands SortList to SCU DMA level 1 as an indirect-mode transfer table (one 12-byte entry per sprite
// command). The SCU only increments the low bits of the table address, so the table must start on a
// power-of-two boundary >= its size, otherwise the transfer wraps inside its block and locks the machine
// (real hardware only, emulators do not model it). SortList is the first member of WorkArea, so the whole
// work area gets that alignment (at least 0x1000); sgl.linker places it using ALIGNOF(WORK_AREA).
#define SORT_LIST_SIZE ((_LongWord_ * 3) * (SGL_MAX_POLYGONS + 6))
#define SMEAR1_(x) ((x) | ((x) >> 1))
#define SMEAR2_(x) (SMEAR1_(x) | (SMEAR1_(x) >> 2))
#define SMEAR4_(x) (SMEAR2_(x) | (SMEAR2_(x) >> 4))
#define SMEAR8_(x) (SMEAR4_(x) | (SMEAR4_(x) >> 8))
#define SMEAR16_(x) (SMEAR8_(x) | (SMEAR8_(x) >> 16))
#define POW2_CEIL(x) (SMEAR16_((x) - 1) + 1)
#define WORK_AREA_ALIGN (POW2_CEIL(SORT_LIST_SIZE) > 0x1000 ? POW2_CEIL(SORT_LIST_SIZE) : 0x1000)

_Static_assert(__builtin_offsetof(struct WorkArea_, SortList) == 0, "SortList must be the first member of WorkArea_");

struct WorkArea_ __attribute__((section("WORK_AREA_DUMMY"))) WORK_AREA_DUMMY;
struct WorkArea_ __attribute__((aligned(WORK_AREA_ALIGN), used, section("WORK_AREA"))) WorkArea;

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
