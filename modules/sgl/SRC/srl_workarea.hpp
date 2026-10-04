#pragma once

/** @file srl_sgl_workarea.hpp
 *  @brief Runtime allocation of the SGL work-area buffers.
 *
 *  LIBSGL.A consumes the pointer globals `_SortList`, `_Zbuffer`,
 *  `_SpriteBuf`, `_Pbuffer`, `_CLOfstBuf` and `_CommandBuf`; this file
 *  fills them at boot with heap blocks carrying the required alignment.
 *
 *  `SortList` is used by SGL as an SCU indirect-DMA table. The SCU table
 *  pointer only increments the low bits of the table address, so the
 *  table base must sit on a power-of-two boundary >= the table size or
 *  the pointer wraps inside the block and the machine locks.
 *
 *  `SRL::SglWorkArea<>::Install()` runs in PreLoader right after
 */

#ifndef DOXYGEN

#ifndef SGL_MAX_POLYGONS
#define SGL_MAX_POLYGONS 0
#endif

#ifndef SGL_MAX_VERTICES
#define SGL_MAX_VERTICES 0
#endif

#ifndef SGL_SLAVE_BUF_SIZE
#define SGL_SLAVE_BUF_SIZE 0
#endif

#endif

// Need this only because we want the size of sprite, and we also want standard types
#include <sl_def.h>

extern "C"
{
    // Pointer globals consumed by LIBSGL.A — tentative definitions in
    // modules/sgl/SRC/workarea.c. The pointee is const but the pointer
    // itself is mutable.
    extern const void* SortList;
    extern const void* Zbuffer;
    extern const void* SpriteBuf;
    extern const void* Pbuffer;
    extern const void* CLOfstBuf;
    extern const void* CommandBuf;
}

namespace SRL
{
    /** @brief Compile-time sized/validated SGL work area, allocated at boot
     *  @tparam MaxPolygons SGL polygon limit (make: SGL_MAX_POLYGONS)
     *  @tparam MaxVertices SGL vertex limit (make: SGL_MAX_VERTICES)
     *  @tparam SlaveBufferSize Slave command buffer size (make: SGL_SLAVE_BUF_SIZE)
     */
    template <uint32_t MaxPolygons = SGL_MAX_POLYGONS,
              uint32_t MaxVertices = SGL_MAX_VERTICES,
              uint32_t SlaveBufferSize = SGL_SLAVE_BUF_SIZE>
    class SglWorkArea
    {
    private:

        /** @brief Smallest power of two >= v
         */
        static constexpr size_t Pow2Ceil(size_t  v)
        {
            v -= 1;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            return v + 1;
        }

        /** @brief Zero a buffer (all work-area sizes are multiples of 4;
         * memset is not linked in this freestanding build).
         */
        static void Zero(void* ptr, uint32_t bytes)
        {
            for (auto* w = static_cast<uint32_t*>(ptr); bytes >= 4; bytes -= 4)
            {
                *w++ = 0;
            }
        }

        /** @brief (_LongWord_ * 3) * (MaxPolygons + 6)
         */
        static constexpr uint32_t SortListBytes = (sizeof(uint32_t) * 3) * (MaxPolygons + 6);

        /** @brief _LongWord_ * 512
         */
        static constexpr uint32_t ZSortBufferBytes = sizeof(uint32_t) * 512;

        /** @brief _Sprite_ * ((MaxPolygons + 6) * 2), _Sprite_ = 18 * uint16_t
         */
        static constexpr uint32_t SpriteBufBytes = sizeof(SPRITE) * ((MaxPolygons + 6) * 2);

        /** @brief (_LongWord_ * 4) * MaxVertices
         */
        static constexpr uint32_t PointBufferBytes = (sizeof(uint32_t) * 4) * MaxVertices;

        /** @brief (_Byte_ * 32 * 3) * 32
         */
        static constexpr uint32_t ColorBufferBytes = (32 * 3) * 32;

        /** @brief SGL_SLAVE_BUF_SIZE
         */
        static constexpr uint32_t CommandBufBytes = SlaveBufferSize;

        /** @brief SortList base alignment
         * @details power of two >= SortListBytes, at least 0x1000 (SCU indirect-table rule).
         */
        static constexpr uint32_t SortListAlign =
            Pow2Ceil(SortListBytes) < 0x1000 ? 0x1000 : Pow2Ceil(SortListBytes);

        /** @brief Total bytes carved out of the heap (informational)
         */
        static constexpr uint32_t TotalBytes =
            SortListBytes + ZSortBufferBytes + SpriteBufBytes +
            PointBufferBytes + ColorBufferBytes + CommandBufBytes;

        /** @name Workarea constraints */
        /*@{*/

        static_assert(SortListBytes != 0 && ZSortBufferBytes != 0 &&
                      SpriteBufBytes != 0 && PointBufferBytes != 0 &&
                      ColorBufferBytes != 0 && CommandBufBytes != 0,
            "SGL work-area buffers must not be zero-sized");

        static_assert(SortListAlign >= SortListBytes,
            "SortList base must be aligned to a power of two >= its size (SCU indirect DMA)");

        static_assert((SlaveBufferSize & 0xF) == 0,
            "SGL_SLAVE_BUF_SIZE must be 0x10-aligned (see shared.mk)");

        static_assert((SortListAlign & (SortListAlign - 1)) == 0,
            "SortListAlign must be a power of two");

        /*@}*/

        /** @brief Table buffer for DMA transfer of sprite control data
         */
        alignas(SortListAlign) static inline char SortTransferList[SortListBytes];

        /** @brief Command z-buffer sort list
         */
        alignas(0x10) static inline char ZSortBuffer[ZSortBufferBytes];

        /** @brief sprite control data buffer
         */
        alignas(0x10) static inline char SpriteControlBuffer[SpriteBufBytes];

        /** @brief Vertex position buffer for polygon calculation
         */
        alignas(0x10) static inline char PointBuffer[PointBufferBytes];

        /** @brief Color data table due to the influence of light source
         */
        alignas(0x10) static inline char ColorBuffer[ColorBufferBytes];
        
        /** @brief Command passing buffer from master to slave
         */
        alignas(0x20) static inline char SlaveCommandBuffer[CommandBufBytes];

    public:

        /** @brief Allocate the work area from HWRAM and install the SGL
         * pointer globals. Call once, before any SGL function.
         */
        static void Install()
        {
            // SGL expects the work area zeroed at boot.
            SglWorkArea::Zero(SglWorkArea::SortTransferList, SglWorkArea::SortListBytes);
            SglWorkArea::Zero(SglWorkArea::ZSortBuffer, SglWorkArea::ZSortBufferBytes);
            SglWorkArea::Zero(SglWorkArea::SpriteControlBuffer, SglWorkArea::SpriteBufBytes);
            SglWorkArea::Zero(SglWorkArea::PointBuffer, SglWorkArea::PointBufferBytes);
            SglWorkArea::Zero(SglWorkArea::ColorBuffer, SglWorkArea::ColorBufferBytes);
            SglWorkArea::Zero(SglWorkArea::SlaveCommandBuffer, SglWorkArea::CommandBufBytes);

            // Give SGL access to the buffers
            SortList = SglWorkArea::SortTransferList;
            Zbuffer = SglWorkArea::ZSortBuffer;
            SpriteBuf = SglWorkArea::SpriteControlBuffer;
            Pbuffer = SglWorkArea::PointBuffer;
            CLOfstBuf = SglWorkArea::ColorBuffer;
            CommandBuf = SglWorkArea::SlaveCommandBuffer;
        }
    };
}
