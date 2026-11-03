#pragma GCC optimize("O0")
#include "sl_def.h"

/* Pointer globals consumed by LIBSGL.A. The buffers behind them are
 * allocated at boot from the HWRAM heap by SRL::SglWorkArea::Install()
 * (srl_sgl_workarea.hpp), which also enforces the alignment contract:
 * SortList on a power-of-two boundary >= its size (SCU indirect-DMA
 * table requirement), everything else 0x10, CommandBuf 0x20.
 */
const void* SortList;
const void* Zbuffer;
const void* SpriteBuf;
const void* Pbuffer;
const void* CLOfstBuf;
const void* CommandBuf;

const void* PCM_Work = (void*)SoundRAM + 0x78000; /* PCM Stream Address      */
const uint32_t PCM_WkSize = 0x8000;               /* PCM Stream Size         */
const void* SlaveStack = (void*)0x06001e00;       /* SlaveSH2  StackPointer  */
const void* TransList = (void*)0x060fb800;        /* DMA Transfer Table      */
const void* MasterStack =  (void*)0x060ffc00;     /* MasterSH2 StackPointer  */

const uint16_t MaxVertices = SGL_MAX_VERTICES;
const uint16_t MaxPolygons = SGL_MAX_POLYGONS;

/* Buffer sizes consumed by SGL — must match SRL::SglWorkArea. */
const uint32_t SortListSize  = (sizeof(uint32_t) * 3) * (SGL_MAX_POLYGONS + 6);
const uint32_t SpriteBufSize = (sizeof(uint16_t) * 18) * ((SGL_MAX_POLYGONS + 6) * 2);

const uint16_t EventSize = sizeof(EVENT);
const uint16_t MaxEvents = SGL_MAX_EVENTS;
EVENT EventBuf[SGL_MAX_EVENTS];
EVENT* RemainEvent[SGL_MAX_EVENTS];

const uint16_t WorkSize = sizeof(WORK);
const uint16_t MaxWorks = SGL_MAX_WORKS;
WORK WorkBuf[SGL_MAX_WORKS];
WORK* RemainWork[SGL_MAX_WORKS];
#pragma GCC reset_options
