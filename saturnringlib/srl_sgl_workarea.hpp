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
 *  `SRL::Memory::Initialize()`, before global constructors and SGL init.
 */

#include "srl_memory.hpp"

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
     *  @tparam MaxPolygons    SGL polygon limit (make: SGL_MAX_POLYGONS)
     *  @tparam MaxVertices    SGL vertex limit (make: SGL_MAX_VERTICES)
     *  @tparam SlaveBufferSize Slave command buffer size (make: SGL_SLAVE_BUF_SIZE)
     */
    template <uint32_t MaxPolygons = SGL_MAX_POLYGONS,
              uint32_t MaxVertices = SGL_MAX_VERTICES,
              uint32_t SlaveBufferSize = SGL_SLAVE_BUF_SIZE>
    class SglWorkArea
    {
    public:
        /** @brief Smallest power of two >= v
         */
        static constexpr uint32_t Pow2Ceil(uint32_t v)
        {
            v -= 1;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            return v + 1;
        }

        /** @brief (_LongWord_ * 3) * (MaxPolygons + 6) */
        static constexpr uint32_t SortListBytes  = (4 * 3)  * (MaxPolygons + 6);
        /** @brief _LongWord_ * 512 */
        static constexpr uint32_t ZbufferBytes   = 4 * 512;
        /** @brief _Sprite_ * ((MaxPolygons + 6) * 2), _Sprite_ = 18 * uint16_t */
        static constexpr uint32_t SpriteBufBytes = (2 * 18) * ((MaxPolygons + 6) * 2);
        /** @brief (_LongWord_ * 4) * MaxVertices */
        static constexpr uint32_t PbufferBytes   = (4 * 4)  * MaxVertices;
        /** @brief (_Byte_ * 32 * 3) * 32 */
        static constexpr uint32_t CLOfstBufBytes = (32 * 3) * 32;
        /** @brief SGL_SLAVE_BUF_SIZE */
        static constexpr uint32_t CommandBufBytes = SlaveBufferSize;

        /** @brief SortList base alignment: power of two >= SortListBytes,
         *         at least 0x1000 (SCU indirect-table rule).
         */
        static constexpr uint32_t SortListAlign =
            Pow2Ceil(SortListBytes) < 0x1000 ? 0x1000 : Pow2Ceil(SortListBytes);

        static_assert(SortListBytes != 0 && ZbufferBytes != 0 &&
                      SpriteBufBytes != 0 && PbufferBytes != 0 &&
                      CLOfstBufBytes != 0 && CommandBufBytes != 0,
            "SGL work-area buffers must not be zero-sized");
        static_assert(SortListAlign >= SortListBytes,
            "SortList base must be aligned to a power of two >= its size (SCU indirect DMA)");
        static_assert((SlaveBufferSize & 0xF) == 0,
            "SGL_SLAVE_BUF_SIZE must be 0x10-aligned (see shared.mk)");
        static_assert((SortListAlign & (SortListAlign - 1)) == 0,
            "SortListAlign must be a power of two");

        /** @brief Total bytes carved out of the heap (informational)
         */
        static constexpr uint32_t TotalBytes =
            SortListBytes + ZbufferBytes + SpriteBufBytes +
            PbufferBytes + CLOfstBufBytes + CommandBufBytes;

        /** @brief Zero a buffer (all work-area sizes are multiples of 4;
         *         memset is not linked in this freestanding build).
         */
        static void Zero(void* ptr, uint32_t bytes)
        {
            uint32_t* w = static_cast<uint32_t*>(ptr);
            for (uint32_t i = 0; i < bytes / 4; i++)
            {
                w[i] = 0;
            }
        }

        /** @brief Allocate the work area from HWRAM and install the SGL
         *         pointer globals. Call once, before any SGL function.
         */
        static void Install()
        {
            void* sort = Memory::HighWorkRam::Memalign(SortListAlign, SortListBytes);
            void* zbuf = Memory::HighWorkRam::Memalign(0x10, ZbufferBytes);
            void* spr  = Memory::HighWorkRam::Memalign(0x10, SpriteBufBytes);
            void* pbf  = Memory::HighWorkRam::Memalign(0x10, PbufferBytes);
            void* clof = Memory::HighWorkRam::Memalign(0x10, CLOfstBufBytes);
            void* cbuf = Memory::HighWorkRam::Memalign(0x20, CommandBufBytes);

            // Trap on allocation failure — SGL must not run with null
            // work-area pointers.
            if (sort == nullptr || zbuf == nullptr || spr == nullptr ||
                pbf == nullptr || clof == nullptr || cbuf == nullptr)
            {
                while (1);
            }

            // SGL expects the work area zeroed at boot.
            Zero(sort, SortListBytes);
            Zero(zbuf, ZbufferBytes);
            Zero(spr,  SpriteBufBytes);
            Zero(pbf,  PbufferBytes);
            Zero(clof, CLOfstBufBytes);
            Zero(cbuf, CommandBufBytes);

            SortList   = sort;
            Zbuffer    = zbuf;
            SpriteBuf  = spr;
            Pbuffer    = pbf;
            CLOfstBuf  = clof;
            CommandBuf = cbuf;
        }
    };
}
