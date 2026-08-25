#pragma once

#include "srl_base.hpp"
#include "srl_tv.hpp"
#include "srl_cram.hpp"
#include "srl_ascii.hpp"
#include "srl_debug.hpp"
#include "srl_cd.hpp"
#include "srl_tilemap_interfaces.hpp"

namespace SRL
{
    /** @brief VDP2 control
     */
    class VDP2
    {
    public:

        /** @brief  VDP2 scroll priorities (Higher layers draw on top of lower layers)
         */
        enum class Priority : uint16_t
        {
            Layer0 = 0,
            Layer1 = 1,
            Layer2 = 2,
            Layer3 = 3,
            Layer4 = 4,
            Layer5 = 5,
            Layer6 = 6,
            Layer7 = 7,
        };

        /** @brief Options for using color offsets on Scroll Screens
         *
         *  @details Scroll and sprite screens can apply one of 2 color offsets
         *  (A or B) to all pixels from that screen.
         *  See VDP2::ScrollScreen::UseColorOffset() and VDP2::SetColorOffsetA/B()
         *  for further details
         */
        enum class OffsetChannel
        {
            /* @brief Do not use any color offset
             */
            NoOffset,
            /* @brief Use color offset A
             */
            OffsetA,
            /* @brief Use color offset B
             */
            OffsetB,
        };

        /** @brief used to specify the 4 VRAM banks that are available for VDP2 allocation
         */
        enum class VramBank : uint16_t
        {
            A0 = 0,
            A1 = 1,
            B0 = 2,
            B1 = 3,
        };

        /** @brief Used to set the minimum allowed scale for NBG0 and NBG1 scrolls
        */
        enum class ScaleLimit: uint16_t
        {
            /** @brief Set minimum scale to 1x (default)
             */
            One = ZOOM_1,
            /** @brief Set minimum scale to 1/2x
             */
            Half = ZOOM_HALF,
            /** @brief Set minimum scale to 1/4x
             */
            Quarter = ZOOM_QUARTER,
        };

        /** @brief Used to specify the type of Combining Logic used when applying multiple VDP2 window areas
         *  to one ScrollScreen
         */
        enum class Logic : uint16_t
        {
            /** @brief Display ScrollScreen in the Intersection of the specified window areas
             */
            And = 0,

            /** @brief Display ScrollScreen in the Union of the specified window areas
             */
            Or = 0x80,
        };

        /** @brief Used to specify how a scrollscreen applies a VDP2 Window area to its display
         */
        enum class Area : uint16_t
        {
            /** @brief Do not use the window to affect the ScrollScreen
             */
            Off = 0,

            /** @brief Only Display Scrollscreen outside the window area
             */
            Out = 2,

            /** @brief Only Display Scrollscreen inside the window area
             */
            In = 3,
        };

        /**  @brief converts an SRL TextureColorMode enum to its corresponding SGL macro for VDP2
         *   @param colorMode the colorMode to convert
         *   @return SGL VDP2 color macro corresponding to the colormode
         */
        static uint16_t GetSGLColorMode(SRL::CRAM::TextureColorMode colorMode)
        {
            static const uint8_t sglColor[] = { 0x30,0x30,0x00,0x10,0x10,0x10,0x10 };
            return (uint16_t)sglColor[(uint16_t)colorMode];
        }

        /** @brief Manages VDP2 VRAM allocation
         */
        class VRAM
        {
        private:

            /** @brief VDP2 class needs access to some of the internal variables
             */
            friend class VDP2;

            /** @brief Absolute Bottom Address of RAM bank zones
             */
            inline static uint8_t* bankBot[4] = { (uint8_t*)VDP2_VRAM_A0,(uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1 };

            /** @brief Abolute Top Address of RAM bank zones
             */
            inline static uint8_t* bankTop[4] = { (uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1,(uint8_t*)(VDP2_VRAM_B1 + 0x18000) };

            /** @brief Currently allocated bottom RAM bank zones
             */
            inline static uint8_t* currentBot[4] = { (uint8_t*)VDP2_VRAM_A0,(uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1 };

            /** @brief Currently allocated top RAM bank zones
             */
            inline static uint8_t* currentTop[4] = { (uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1,(uint8_t*)(VDP2_VRAM_B1 + 0x18000) };

            /** @brief Number of cycles available for each bank
             * @note Indicates number of cycles that are free in the VRAM bank for per pixel operations
             */
            inline static int8_t bankCycles[4] = { -1,-1,-1,2 };//why will init to 0 not work?

            /** @brief Indicates if map data has been stored in bank A0 by a previous allocation
             */
            inline static bool MapBankA0 = true;

            /** @brief Indicates if map data has been stored in bank B0 by a previous allocation
             */
            inline static bool MapBankB0 = true;

        public:
            /** @brief Gets current amount of free VRAM in a bank
            * @param bank the VRAM bank to get free space in
            * @return number of available bytes in bank
            */
            inline static uint32_t GetAvailable(VDP2::VramBank bank)
            {
                return (int)currentTop[(uint16_t)bank] - (int)currentBot[(uint16_t)bank];
            }

            /** @brief Linearly Allocates Vram in a bank and returns address to start of allocation. Allocation fails if
            * there is not enough free space in the bank or if access requires too many cycle slots.
            * @param size Number of bytes to allocate
            * @param boundary Byte Boundary that the allocation should be aligned to (must be multiple of 32 for all VDP2 Data types)
            * @param bank The VRAM bank to allocate in
            * @param cycles (Optional)The Number of Access Slots in a bank's Cycle Pattern that the data will require during display.
            * @return void* start of the Allocated region in VRAM (nullptr if allocation failed)
            * @note  Any VRAM padded to maintain alignment to a requested boundary is rendered inaccessible to further
            * allocations until VRAM is reset through VDP2::ClearVram().
            */
            inline static void* Allocate(uint32_t size, uint32_t boundary, VDP2::VramBank bank, uint8_t cycles = 0)
            {
                void* myAddress = nullptr;

                // Ensure allocation is aligned to requested VRAM boundary:
                uint32_t addrOffset = 0;

                if ((uint32_t)VRAM::currentBot[(uint16_t)bank] & (boundary - 1))
                {
                    addrOffset = boundary - ((uint32_t)currentBot[(uint16_t)bank] & (boundary - 1));
                }

                if (VDP2::VRAM::GetAvailable(bank) >= size + addrOffset)
                {
                    if ((VRAM::bankCycles[(uint16_t)bank] + cycles) < 8)
                    {
                        myAddress = VRAM::currentBot[(uint16_t)bank] + addrOffset;
                        VRAM::currentBot[(uint16_t)bank] += size + addrOffset;
                        VRAM::bankCycles[(uint16_t)bank] += cycles;
                    }
                }

                return myAddress;
            }

            /** @brief Automatically allocates general table data with no additional allignment
             * or cycle access requirements
             * @details Data types that fall under this use case include Line Scroll Tables,
             * Line Color Tables, Line Window Tables, and Coefficient Tables for 2 Axis Rotation
             * @param sz the size of the allocation
             * @return Pointer to the allocated memory
             */
            inline static void *AutoAllocateTable(uint32_t sz)
            {
                void *alloc = VRAM::Allocate(sz, 32, VramBank::A0, 0);
                if (alloc == nullptr) alloc = VRAM::Allocate(sz, 32, VramBank::A1, 0);

                if (alloc == nullptr) alloc = VRAM::Allocate(sz, 32, VramBank::B0, 0);

                if (alloc == nullptr) alloc = VRAM::Allocate(sz, 32, VramBank::B1, 0);

                if (alloc == nullptr) SRL::Debug::Assert("VDP2 Data Allocation Failed, insufficient VRAM");
                return alloc;
            }

            /** @brief Automatically allocates cell data for specified screen
             * @param info Tile cell data description
             * @param screen The screen identifier
             * @return Pointer to the allocated memory
             * @note The Maximum supported allocation size is 0x20000 bytes (1 Bank).Single Allocations
             * can not be spit between multiple banks
             */
            inline static void* AutoAllocateCell(Tilemap::TilemapInfo& info, uint16_t screen)
            {
                void* alloc;

                if (screen == scnRBG0) // Reserve all 8 cycles of a bank
                {
                    alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::A1, 8);
                    if (alloc == nullptr) SRL::Debug::Assert("RBG Cell Allocation failed: insufficient VRAM");
                }
                else // Base cycle requirement on color type
                {
                    uint8_t reqCycles = 0;

                    switch (info.ColorMode)
                    {
                    case CRAM::TextureColorMode::Paletted16:
                        reqCycles = 1;
                        break;
                    case CRAM::TextureColorMode::Paletted256:
                        reqCycles = 2;
                        break;
                    case CRAM::TextureColorMode::RGB555:
                        reqCycles = 4;
                        break;
                    }

                    alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::B0, reqCycles);
                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::A1, reqCycles);

                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::A0, reqCycles);

                    if (alloc == nullptr) alloc = VRAM::Allocate(info.CellByteSize, 32, VramBank::B1, reqCycles);

                    if (alloc == nullptr) SRL::Debug::Assert("NBG Cell Allocation failed: insufficient VRAM");
                }

                return alloc;
            }

            /** @brief Automatically allocates map data for specified screen
             * @param info Tile map data description
             * @param screen The screen identifier
             * @param size Optional pointer to pass the resulting allocation size back to
             * @return Pointer to the allocated memory
             * @note The Maximum supported allocation size is 0x20000 bytes (1 Bank). Single Allocations
             * can not be split between multiple banks.
             */
            inline static void* AutoAllocateMap(Tilemap::TilemapInfo& info, int16_t screen,int* size = nullptr )
            {
                void* alloc = nullptr;
                uint32_t page_sz = 0x800;
                uint32_t sz = (info.MapHeight * info.MapWidth) << 1;

                if (info.CharSize == CHAR_SIZE_1x1) page_sz <<= 2;

                if (info.MapMode == PNB_2WORD)
                {
                    sz <<= 1;
                    page_sz <<= 1;
                }

                if (info.PlaneSize == PL_SIZE_2x2) page_sz <<= 2;
                else if (info.PlaneSize == PL_SIZE_2x1) page_sz <<= 1;

                if (screen == scnRBG0) // Reserve all 8 cycles of bank 0
                {
                    alloc = VRAM::Allocate(sz, page_sz, VramBank::A0, 8);
                    if (alloc == nullptr) Debug::Assert("RBG Map Allocation failed: insufficient VRAM");
                    else
                    {
                        if (size!=nullptr)*size = sz;
                        MapBankA0 = true;
                    }
                }
                else // Reserve 1 cycle in bank B1 (or B0 if it doesn't conflict with RBG0 map)
                {
                    alloc =  VRAM::Allocate(sz, page_sz, VramBank::B1, 1);
                    if (alloc == nullptr && MapBankA0==false)
                    {
                        alloc = VRAM::Allocate(sz, page_sz, VramBank::B0, 1);
                        if (alloc)MapBankB0 = true;
                    }

                    if ((alloc == nullptr) && MapBankB0==false)
                    {
                        alloc = VRAM::Allocate(sz, page_sz, VramBank::A0, 1);
                        if (alloc)MapBankA0 = true;
                    }

                    if (alloc == nullptr) SRL::Debug::Assert("NBG Map Allocation failed: insufficient VRAM");
                    else if (size!=nullptr) *size = sz;
                }

                return alloc;
            }

            /** @brief Automatically allocates bitmap for specified screen
            * @param info Bitmap data description
            * @param screen The screen identifier
            * @param size optional pointer to pass the resulting allocation size back from function
            * @return Pointer to the allocated memory
            * @note The maximum allocation size supported is 0x40000 bytes (2 banks of VRAM).
            * 2 bank allocations must split either betweeen BankA0 and BankA1, Or BankA1 and BankB0.
            */
            inline static void* AutoAllocateBmp(Bitmap::BitmapInfo& info, int16_t screen, int* size = nullptr)
            {
                void* alloc = nullptr;
                uint8_t numCycles = 2;
                //chack that image size is compatible
                if ((info.Width>1024) || (info.Height>512) || (screen == scnRBG0 && info.Width > 512))
                {
                    Debug::Assert("Bmp Allocation Failed: Unsupported image size");
                    return nullptr;
                }
                //scale based on bitdepth
                uint32_t sz = info.Height > 256 ? 512 : 256;
                sz *= info.Width > 512 ? 1024 : 512;
                if (info.ColorMode == CRAM::TextureColorMode::Paletted16)
                {
                    sz >= 1;
                    numCycles >>= 1;
                }
                else if (info.ColorMode == CRAM::TextureColorMode::RGB555)
                {
                    sz <<= 1;
                    numCycles <<= 1;
                }

                if (screen == scnRBG0) numCycles = 8;

                if (sz > 262144) //case: bmp is too large for allocator at this color depth
                {
                    Debug::Assert("Bmp Allocation Failed: Size exceeds 2 banks");
                    return nullptr;
                }
                else if (sz > 131072)//case: bmp requires 2 out of the 4 VRAM banks
                {
                    if ((uint32_t)currentBot[0] == VDP2_VRAM_A0 && (uint32_t)currentBot[1] == VDP2_VRAM_A1)
                    {
                        alloc = VRAM::Allocate(131072, 32, VramBank::A0, numCycles);
                        VRAM::Allocate(131072, 32, VramBank::A1, numCycles);
                    }
                    else if ((uint32_t)currentBot[1] == VDP2_VRAM_A1 && (uint32_t)currentBot[2] == VDP2_VRAM_B0)
                    {
                        alloc = VRAM::Allocate(131072, 32, VramBank::A1, numCycles);
                        VRAM::Allocate(131072, 32, VramBank::B0, numCycles);
                    }
                    else
                    {
                        Debug::Assert("Bmp Allocation Failed: Insufficient VRAM Available");
                        return nullptr;
                    }
                }
                else//case: bmp requires 1 or 1/2 VRAM bank
                {
                    alloc = VRAM::Allocate(sz, 32, VramBank::A0, numCycles);
                    if (alloc == nullptr) alloc = VRAM::Allocate(sz, 32, VramBank::B0,numCycles);

                    if (alloc == nullptr) alloc = VRAM::Allocate(sz, 32, VramBank::A1, numCycles);

                    if (alloc == nullptr) alloc = VRAM::Allocate(sz, 32, VramBank::B1, numCycles);

                    if (alloc == nullptr) SRL::Debug::Assert("Bmp Allocation failed: insufficient VRAM Available");
                }

                if (size) *size = sz;

                return alloc;
            }

            /** @brief Overwrites a specified VRAM region with 0 data.
             *  @param address The vram address of the area to blank.
             *  @param size The size of area to overwrite in bytes.
             */
            static void Blank(void* address, uint32_t size)
            {
                if (address < (void*)VDP2_VRAM_A0 || ((uint8_t*)address + size) > bankTop[3]) return;
                for (uint8_t* current = (uint8_t*)address; current < ((uint8_t*)address + size); current++) *current = 0;
            }

        };

        /** @brief Bitfield recording all Currently enabled Scroll Screens
        */
        inline static uint16_t ActiveScrolls = NBG3ON | SPRON;

        /** @brief Bitfield recording all Scroll Screens with VDP2 Color Calculation enabled
         */
        inline static uint16_t ColorCalcScrolls = NBG3ON | SPRON;

        /** @brief Bitfield recording all Scroll Screens using Color Offset A
         */
        inline static uint16_t OffsetAScrolls = NBG3ON;

        /** @brief Bitfield recording all Scroll Screens using Color Offset B
          */
        inline static uint16_t OffsetBScrolls = NBG3ON;

        /** @brief Bitfield recording all Scroll Screens That Disable transparent pixels
          */
        inline static uint16_t TransparentScrolls = SPRON;

        /** @brief Functionality available to all Scroll Screen interfaces
         */
        template<class ScreenType, int16_t Id, uint16_t On>
        class ScrollScreen
        {
        private:
            /** @brief Initializes the default tiling pattern of the 4 Planes of the Scroll Screen Depending on the
            * height and width of the source Tilemap.
            * @details The following Tiling Patterns Can Occur:
            *     When Map Data fills area of 2x2 planes or greater
            *         |0|1|
            *         |2|3|
            *     When Map data fits within area of 2x1 planes
            *         |0|1|
            *         |0|1|
            *     When map Data fits within area of 1x2 planes
            *         |0|0|
            *         |1|1|
            *     When map data fits within area of 1 plane
            *         |0|0|
            *         |0|0|
            * @note underlying page Size of the 4 planes is specified separately in TileMapInfo::PlaneSize
            */
            inline static void SetPlanesDefault(Tilemap::TilemapInfo& info)
            {
                int mapX, mapY;
                mapX = mapY = 32;

                if (info.CharSize == CHAR_SIZE_1x1)
                {
                    mapY <<= 1;
                    mapX <<= 1;
                }

                if (info.PlaneSize == PL_SIZE_2x2)
                {
                    mapY <<= 1;
                    mapX <<= 1;
                }
                else if (info.PlaneSize == PL_SIZE_2x1)
                {
                    mapX <<= 1;
                }

                if (info.MapWidth > mapX)
                {
                    if (info.MapHeight > mapY) VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 1, 2, 3);
                    else VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 1, 0, 1);

                }
                else
                {
                    if (info.MapHeight > mapY) VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 0, 1, 1);
                    else VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 0, 0, 0);
                }
            }

        public:
            /** @brief SGL identifier for the ScrollScreen
             */
            static constexpr int16_t ScreenID = Id;

            /** @brief SGL flag for the ScrollScreen
             */
            static constexpr uint16_t ScreenON = On;

            /** @brief VRAM Address to start of Cell data when displying Tilemap or Bitmap data when using bitmaps
             */
            inline static void* CellAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief  VRAM Address  of ScrollScreen's Map data when displaying Tilemap
             */
            inline static void* MapAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief CRAM Pallet for the ScrollScreen
             */
            inline static CRAM::Palette TilePalette = CRAM::Palette();

            /** @brief Tilemap config info for the ScrollScreen
             */
            static inline Tilemap::TilemapInfo Info = Tilemap::TilemapInfo();

            /** Size of currently allocated VRAM for Cell Data or Bitmap data
             */
            inline static int CellAllocSize = -1;

            /** @brief Size of currently allocated VRAM for Map Data
             */
            inline static int MapAllocSize = -1;

            /** @brief Loads Tilemap data to VRAM using ITilemap Interface and configures the Scroll Screen to use it
             *
             * @details If VRAM for this ScrollScreen's data has already been allocated by the user, SRL will attempt to load
             * to the allocated VRAM and raise assert if the Tilemap Data does not fit within the existing allocation.
             * If VRAM was not allocated SRL will attempt to auto allocate the Tilemap data and raise assert
             * if there is not enough VRAM/cycle slots available to allocate.
             *
             * @param tilemap The Tilemap to load
             * @param loadPalette (optional: default = true) Auto load the Tilemap's palette to CRAM if present.
             * If a CRAM palette is already assigned, loads to that bank, overwriting existing data.
             * If a CRAM palette is not already assigned, attempts to assign palette and asserts if unavailable.
             *
             * @note Manual VRAM allocation is for advanced use cases and is NOT verified for proper bank alignment.
             * @note Does not turn Scroll Display on- once loaded use ScrollEnable() to display a Scroll Screen.
             * @note As RBG0 must reserve dedicated VRAM banks, always perform loading/allocation
             * for RBG0 before NBG0-3 screens if using it.
             */
            inline static void LoadTilemap(SRL::Tilemap::ITilemap& tilemap, bool loadPalette = true)
            {
                SRL::Tilemap::TilemapInfo myInfo = tilemap.GetInfo();
                ScreenType::Info = tilemap.GetInfo();
                int autoMapSize = 0;
                if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0 || (uint32_t)ScreenType::MapAddress >= (VDP2_VRAM_B1 + 0x18000))
                {
                    ScreenType::MapAddress = VRAM::AutoAllocateMap(myInfo, ScreenType::ScreenID, &autoMapSize);
                    if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0) return;
                    else ScreenType::MapAllocSize = autoMapSize;

                }
                else if (ScreenType::MapAllocSize < ((ScreenType::Info.MapWidth * ScreenType::Info.MapHeight) << (1+!ScreenType::Info.MapMode)))
                {
                    SRL::Debug::Assert("Tilemap Load Failed- MAP DATA exceeds existing VRAM allocation");
                    return;
                }

                if ((uint32_t)ScreenType::CellAddress < VDP2_VRAM_A0 || (uint32_t)ScreenType::CellAddress >= (VDP2_VRAM_B1 + 0x18000))
                {
                    ScreenType::CellAddress = VRAM::AutoAllocateCell(myInfo, ScreenType::ScreenID);

                    if ((uint32_t)ScreenType::CellAddress < VDP2_VRAM_A0)
                    {
                        //SRL::Debug::Assert("Tilemap Load Failed- CEL DATA exceeds existing VRAM allocation");
                        return;
                    }
                    else ScreenType::CellAllocSize = myInfo.CellByteSize;
                }
                else if (ScreenType::CellAllocSize < ScreenType::Info.CellByteSize)
                {
                    SRL::Debug::Assert("Tilemap Load Failed- CEL DATA exceeds existing VRAM allocation");
                    return;
                }

                int colorID = 0;
                if (loadPalette && ScreenType::Info.ColorMode != SRL::CRAM::TextureColorMode::RGB555)
                {
                    if (ScreenType::TilePalette.GetData() == nullptr)
                    {
                        if ((colorID = SRL::CRAM::GetFreeBank(ScreenType::Info.ColorMode)) < 0)
                        {
                            SRL::Debug::Assert("Tilemap Palette Load Failed- no CRAM Palettes available");
                            return;
                        }

                        SRL::CRAM::SetBankUsedState(colorID, ScreenType::Info.ColorMode, true);
                        ScreenType::TilePalette = SRL::CRAM::Palette(ScreenType::Info.ColorMode, colorID);
                    }
                    uint16_t len = (ScreenType::Info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16) ? 16 : 256;
                    ScreenType::TilePalette.Load((Types::HighColor*)tilemap.GetPalData(), len);
                }

                if (ScreenType::ScreenID != scnRBG0) VDP2::ScrollScreen<ScreenType, Id, On>::SetPlanesDefault(ScreenType::Info);

                VDP2::ScrollScreen<ScreenType, Id, On>::Cell2VRAM((uint8_t*)tilemap.GetCellData(), ScreenType::CellAddress, ScreenType::Info.CellByteSize);
                VDP2::ScrollScreen<ScreenType, Id, On>::Map2VRAM(
                    ScreenType::Info,
                    (uint16_t*)tilemap.GetMapData(),
                    ScreenType::MapAddress,
                    ScreenType::TilePalette.GetId(),
                    VDP2::ScrollScreen<ScreenType, Id, On>::GetCellOffset(ScreenType::Info, ScreenType::CellAddress));
                ScreenType::Init(ScreenType::Info);
            }

            /** @brief Manually Sets VRAM area for Cell Data (Advanced Use Cases)
             * @details This function manually sets an area in VRAM for a scrolls Cell Data to be loaded to. Unless the
             * Address is obtained from VDP2::VRAM::Allocate(), the VRAM allocator will be bypassed entirely.
             * No Checks are performed for proper data alignment or cycle conflicts. For advanced use cases only.
             * @code {.cpp}
             * //Manually Set NBG0 to store 16bpp Cell Data in an 0x8000 byte region allocated in VRAM bank A1:             *
             * SRL::VDP2::NBG0::SetCellAddress(SRL::VDP2::VRAM::Allocate(0x8000,32,SRL::VDP2::VramBank::A1, 3),0x8000);
             * @endcode
             * @param address the VRAM address of the allocation
             * @param size the size of the allocation
             * @return Echoes Address
             */
            inline static void* SetCellAddress(void* address, int size)
            {
                ScreenType::CellAddress = address;
                ScreenType::CellAllocSize = size;
                return address;
            }

            /** @brief Manually Sets VRAM area for Map Data (Advanced Use Cases)
             * @details This function manually sets an area in VRAM for a scroll's Map Data to be loaded to. Unless the
             * Address is obtained from VDP2::VRAM::Allocate() the VRAM allocator will be bypassed entirely.
             * No Checks are performed for proper data alignment or cycle conflicts. For advanced use cases only.
             * @code {.cpp}
             * //Manually Set NBG0 to store Map Data in an 0x8000 byte region allocated in VRAM bank A1:
             * SRL::VDP2::NBG0::SetMapAddress(SRL::VDP2::VRAM::Allocate(0x8000,1024,SRL::VDP2::VramBank::A1, 1),0x8000);
             * @endcode
             * @param Address the VRAM address of the allocation
             * @param Size the size of the allocation
             * @return Echoes Address
             */
            inline static void* SetMapAddress(void* Address, int Size)
            {
                ScreenType::MapAddress = Address;
                ScreenType::MapAllocSize = Size;
                return Address;
            }

            /** @brief Manually sets the CRAM palette index for this scrollScreen(Advanced Use Cases)
             * @details Shallow copies the specified palette handler to the ScrollScreen's
             * TilePalette, forcing it to register with the specified palette. Use in cases
             * where the user desires more control over palette placement in CRAM, such as palette
             * sharing between tilemaps.
             * @note when set this way, multiple sources may be able to write to the same palette.
             * specify the LoadPalette parameter as false when using ScrollScreen::LoadTilemap()
             * to ensure pre-existing palette is not overwritten during loading.
            */
            inline static void SetPalette(CRAM::Palette palette)
            {
                ScreenType::TilePalette = palette;
            }

            /** @brief Registers the ScrollScreen to Display SRL's Debug Ascii Scroll
            *   @details A scrollscreen that is Registered to the Ascii Scroll will remain
            *   registered even if VDP2::ClearVRAM() is called. It will automatically unregister
            *   and revert to normal loading when it's LoadTilemap()/LoadBitmap() method is next called.
            *   @note  -The Ascii Scroll is not compatible with Rotation Scroll RBG0.
            *   @note  -The Ascii Scroll was designed to have its position fixed on screen, and may display garbage
            *   data in the margins if its position is offset or used in High res mode without proper scaling.
            *   @note  -The Ascii scroll data is stored outside The VRAM Alloacator's valid memory region, so
            *   can not be written to with the standard ScrollScreen loading functions. Manage loading
            *   fonts with the SRL::ASCII module instead.
            */
            inline static void RegisterAsciiScroll()
            {
                SRL::Tilemap::TilemapInfo AsciiInfo(CRAM::TextureColorMode::Paletted16, PNB_1WORD|CN_10BIT,
                    CHAR_SIZE_1x1,PL_SIZE_1x1, 1, 1, 0);
                SetCellAddress((void*)(VDP2_VRAM_B1 + 0x1D000), 0);
                SetMapAddress((void*)(VDP2_VRAM_B1 + 0x1E000), 0);
                ScreenType::Init(AsciiInfo);
            }

            /** @brief Registers Scroll in VDP2 cycle pattern to enable display of this Scroll Screen
             * @details Asserts when registration of a scroll fails due to cycle pattern conflicts.
             * Possible causes:
             *     -Too many Scroll Layers with higher bit depth are storing data in the same VRAM bank
             *     -NBG0 or NBG1 have their minimum scale limit set too small(eg 1/2x or 1/4x scale)
             *     -NBG Data was stored in a bank reserved by RBG0
             * Potential conflicts are minimized when using Automatic Allocation and setting the
             * desired scale limits of NBG0/NBG1 beforehand.
             * @note Even when registration is successful, some scrolls may be unable to  display simultaneously
             * when the color depth of NBG0 or NBG1 is too High:
             *        -When NBG0 > 8bpp, NBG2 will not display
             *        -When NBG1 > 8bpp, NBG3 will not display
             * By default, debug ASCII text is displayed on NBG3, so will not be available if NBG1 is
             * displayed in a high color mode.
             */
            inline static void ScrollEnable()
            {
                VDP2::ActiveScrolls |= ScreenType::ScreenON;
                int check = slScrAutoDisp(VDP2::ActiveScrolls);
                if (check < 0) SRL::Debug::Assert("Scroll Registration Failed- Invalid cycle pattern");
            }

            /** @brief Removes the Scroll Screen from VDP2 cycle pattern register to disable display
             * @details Asserts when registration of a scroll fails due to cycle pattern conflicts.
             * Possible causes:
             * Assert should never occur here unless the user independently invoked SGL's slScrAutoDisp() with
             * specifications differing from those tracked by SRL
             */
            inline static void ScrollDisable()
            {
                VDP2::ActiveScrolls &= ~(ScreenType::ScreenON);
                int check = slScrAutoDisp(VDP2::ActiveScrolls);
                if (check < 0) SRL::Debug::Assert("Scroll Registration Failed- Invalid cycle pattern");
            }

            /** @brief Gets the starting address in VRAM of Map data allocated to this scroll
             * @return Address of Map data
             */
            inline static  void* GetMapAddress() { return ScreenType::MapAddress; }

            /** @brief Gets the starting address in VRAM of Cell data allocated to this scroll
             *  @return Address of Cell data
             *  @note Returns starting address of bitmap when displaying Bitmap Scroll
             */
            inline static  void* GetCellAddress() { return ScreenType::CellAddress; }

            /** @brief returns the VRAM Address of the specified page number in a scroll's Page Table
             * or nullptr if the requested page is outside of allocated Map Bounds
             * @param index The index of desired page in the page table
             * @return VRAM Address of the page index
             */

            inline static  void* GetPageAddress(uint8_t index)
            {
                if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0) return nullptr;
                uint32_t offset = 2048 * (uint32_t)index;
                if (ScreenType::Info.CharSize == CHAR_SIZE_1x1) offset <<= 2;
                if (ScreenType::Info.MapMode == PNB_2WORD) offset <<= 1;
                return ScreenType::MapAddress + (void*)offset;
            }

            /** @brief Gets the VRAM Address of the specified plane in a scroll's Page Table (units of 1x1, 2x1, or 2x2 pages)
             * @param index The index of desired plane in page table
             * @return VRAM Address of the plane index
             * @note PlaneAddress and PageAddress are equal when Plane Size is set to 1x1
             */
            inline static  void* GetPlaneAddress(uint8_t index)
            {
                if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0) return nullptr;

                uint32_t offset = 2048 * (uint32_t)index;

                if (ScreenType::Info.CharSize == CHAR_SIZE_1x1) offset <<= 2;

                if (ScreenType::Info.MapMode == PNB_2WORD) offset <<= 1;

                if (ScreenType::Info.PlaneSize == PL_SIZE_2x2) offset <<= 2;
                else if (ScreenType::Info.PlaneSize == PL_SIZE_2x1)offset <<= 1;

                return (void*)((uint32_t)ScreenType::MapAddress + offset);
            }

            /** @brief Manually set the Plane layout of a Scroll Screen
             * @details This function manually sets the 4 planes comprising a NBG scroll screen
             * in cases when the default plane tiling pattern is not desired.
             *      The Planes will display in a 2x2 grid as:
             *          |a|b|
             *          |c|d|
             * @param a,b,c,d the plane indicies of the 4 planes that will display in the normal scroll
             * @note RBG0 provides a separate function to handle 16 planes, See RBG0 interface for details.
             */
            inline static void SetMapLayout(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
            {
                ScreenType::SetPlanes(
                    SRL::VDP2::ScrollScreen<ScreenType, Id, On>::GetPlaneAddress(a),
                    SRL::VDP2::ScrollScreen<ScreenType, Id, On>::GetPlaneAddress(b),
                    SRL::VDP2::ScrollScreen<ScreenType, Id, On>::GetPlaneAddress(c),
                    SRL::VDP2::ScrollScreen<ScreenType, Id, On>::GetPlaneAddress(d));
            }

            /** @brief Set the opacity of a scroll screen
             *  @details This Function takes the opacity specified as a fixed point value and converts it to
             *  one of the 32 color calculation ratios that a scroll screen can use (value is floored to the nearest ratio).
             *  Color Calculation is turned on if Opacity < 1.0, or off if Opacity>= 1.0. Color calculation is unchanged if value is negative.
             *  @note Color ratios only apply to highest priority pixels in the frame
             *  @note When VDP2 ColorCalcMode is set to UseColorAddition, all scrolls with opacity < 1.0 will use color addition
             *  in place of their specified ratios.
             *  @param opacity Fxp decimal value between 0.0 and 1.0 representing pixel opacity for the scroll screen (Default 1.0)
             */
            inline static void SetOpacity(Math::Fxp opacity = 1.0)
            {
                if (opacity < Math::Fxp(0.0)) return;

                else if (opacity >= Math::Fxp(1.0))
                {
                    VDP2::ColorCalcScrolls &= ~(ScreenType::ScreenON);
                    slColorCalcOn(VDP2::ColorCalcScrolls);
                }
                else
                {
                    slColRate(ScreenType::ScreenID, 31 - (uint16_t)(opacity.RawValue() >> 11));
                    VDP2::ColorCalcScrolls |= ScreenType::ScreenON;
                    slColorCalcOn(VDP2::ColorCalcScrolls);
                }
            }

            /** @brief Set the Display Priority of a Scroll Screen
             * @note -Higher priority values display on top of lower priorities. Setting Priority::Layer0
             * disables display of a scroll screen without unregistering it.
             * @note -When 2 or more scroll screens are assigned the same layer, their priority resolves as
             * SPRITE > RBG0 > NBG0 > NBG1 > NBG2 > NBG3.
             * @param pr The Priority Layer for the Scroll Screen
             */
            inline static void SetPriority(SRL::VDP2::Priority pr) { slPriority(ScreenType::ScreenID, (uint16_t)pr); }

            /** @brief Sets Which Color Offset that a scroll Screen should use
            *  @details Scroll Screens can optionally apply one of 2 registered RGB color offsets
            *  to their pixels at the end of VDP2 processing- either Offset A or Offset B.
            *  The values of the offsets for each RGB channel are set with VDP2::SetOffsetA() and VDP2::SetOffsetB().
            *  Use this function to enable a scroll to use one of these offsets.
            *  @param mode the color offset to use for this scroll
            *  @note Because the color offset is applied at the end of VDP2 pipeline, only top priority pixels are affected.
            */
            inline static void UseColorOffset(VDP2::OffsetChannel mode)
            {
                if (mode == OffsetChannel::OffsetA)
                {
                    VDP2::OffsetAScrolls |= ScreenType::ScreenON;
                    OffsetBScrolls &= ~ ScreenType::ScreenON;
                }
                else if (mode == OffsetChannel::OffsetB)
                {
                    OffsetBScrolls |= ScreenType::ScreenON;
                    OffsetAScrolls &= ~ScreenType::ScreenON;
                }
                else
                {
                    OffsetAScrolls &= ~ScreenType::ScreenON;
                    OffsetBScrolls &= ~ScreenType::ScreenON;
                }

                slColOffsetOn(0);//clear all offsets
                slColOffsetAUse(OffsetAScrolls);//re register offsets for A
                slColOffsetBUse(OffsetBScrolls);//re register offsets for B
            }

            /** @brief Enable transparent pixels for a scroll screen
            *   @details When enabled any pixel data that is 0 (regardless of bit depth)
            *   will be treated as transparent and display the layer behind it.
            *   @note Transparent pixels are enabled for all Scroll Screens by default
            */
            inline static void TransparentEnable()
            {
                VDP2::TransparentScrolls &= ~ScreenType::ScreenON;
                //SGLs naming convention is reversed- flagging ScreenON turns OFF transparency
                slScrTransparent(VDP2::TransparentScrolls);
            }

            /** @brief Disable transparent pixels for a scroll screen
            *   @details When disabled any pixel data that is 0 (regardless of bit depth)
            *   will use the color from index 0 in its CRAM pallet, or black if RGB.
            *   @note Transparent pixels are enabled for all Scroll Screens by default.
            */
            inline static void TransparentDisable()
            {
                VDP2::TransparentScrolls |= ScreenType::ScreenON;
                //SGLs naming convention is reversed- flagging ScreenON turns OFF transparency
                slScrTransparent(VDP2::TransparentScrolls);
            }

            /** @brief Configures the ScrollScreen's use of VDP2 Windows
             * @details Use to register the scrollScreen to apply VDP2 window processing
             * @param combo The type of logic used to apply multiple window areas to the ScrollScreen (AND/OR)
             * @param Window0 The desired setting for use of window 0
             * @param Window1 The desired setting for use of window 1
             * @param SpriteWindow (Optional) The desired setting for use of the Sprite Window. Defaults to Off.
             * @note The Sprite Window can only be used when VDP1 framebuffer is set to palette exclusive mode
             * and EnableSpriteWindow is set with the VDP2::SpriteLayer Interface.
             */
            inline static void UseWindows(VDP2::Logic combo, VDP2::Area Window0, VDP2::Area Window1, VDP2::Area SpriteWindow = VDP2::Area::Off)
            {
                uint16_t WindowArea = (uint16_t)combo | (uint16_t)Window0 | ((uint16_t)Window1 << 2) | ((uint16_t)SpriteWindow << 4);
                slScrWindowMode(ScreenType::ScreenID, WindowArea);
            }

            inline static void SetWindows(uint16_t win)
            {
                slScrWindowMode(ScreenType::ScreenID, win);
            }

            /** @brief Compute the offset that must be added to map data When Corresponding Cell Data does not start on a VRAM bank boundary
            * @param tile The data configuration of the tilemap
            * @param cellAddress Address of corresponding Cell Data in VRAM (must be a 32 byte boundary)
            * @return The Cell Offset to add to to map data
            */
            inline static uint32_t GetCellOffset(SRL::Tilemap::TilemapInfo& tile, void* cellAddress)
            {
                uint32_t cellOffset;

                if (!ScreenType::Info.MapMode) cellOffset = ((uint32_t)cellAddress - VDP2_VRAM_A0) >> 5; // 2WORD
                else if (ScreenType::Info.MapMode == 0x8000) // 1WORD MODE 0
                {
                    if (ScreenType::Info.CharSize) cellOffset = (((uint32_t)cellAddress - VDP2_VRAM_A0) & 0x1FFFF) >> 7; // 2x2
                    else cellOffset = (((uint32_t)cellAddress - VDP2_VRAM_A0) & 0x7FFF) >> 5; // 1x1
                }
                else // 1WORD MODE 1
                {
                    if (ScreenType::Info.CharSize) cellOffset = ((uint32_t)cellAddress - VDP2_VRAM_A0) >> 7; // 2x2
                    else cellOffset = (((uint32_t)cellAddress - VDP2_VRAM_A0) & 0x1FFFF) >> 5; // 1x1
                }

                return cellOffset;
            }

            /** @brief Gets the Palette Bank That must be included in Map Data to Reference a Palette in CRAM
             * @param paletteID (optional) specify to reference an arbitrary palette, otherwise uses Id from ScrollScreen::TilePalette
             * @return The Formatted Palette ID to be included in Map Indicies to reference a specified palette
             */
            inline static uint32_t GetPalOffset(int8_t paletteID = -1)
            {
                uint32_t paletteOffset;

                if (paletteID < 0) paletteID = ScreenType::TilePalette.GetId();

                if (ScreenType::Info.MapMode) paletteOffset = paletteID << 12;
                else paletteOffset = paletteID << 24;

                return paletteOffset;
            }

            /** @brief Copies Cell data to VRAM (adapted from SGL samples).
            * @param CellData Cell Data to copy.
            * @param CellAdr VRAM address to copy to.
            * @param Size Number of bytes to copy.
            */
            inline static void Cell2VRAM(uint8_t* cellData, void* cellAdr, uint32_t size)
            {
                uint8_t* VRAM = (uint8_t*)cellAdr;

                // Note: Consider DMA
                for (uint32_t i = 0; i < size; i++) *(VRAM++) = *(cellData++);
            }

            /** @brief Copies map data to VRAM and applies necessary offsets (adapted from SGL Samples).
             * @param info Tilemap data config.
             * @param mapData Map data to copy to VRAM.
             * @param mapAdr VRAM address to copy map to .
             * @param paloff Palette index in CRAM.
             * @param mapoff offset added when Cell data does not start at bank boundary .
             */
            inline static void Map2VRAM(SRL::Tilemap::TilemapInfo& info, uint16_t* mapData, void* mapAdr, uint8_t paloff, uint32_t mapoff)
            {
                uint16_t* VRAM = (uint16_t*)mapAdr;
                uint32_t* VRAM32 = (uint32_t*)mapAdr;
                uint32_t* Data32 = (uint32_t*)mapData;

                for (uint16_t i = 0; i < info.MapHeight; i++)
                {
                    for (uint16_t j = 0; j < info.MapWidth; j++)
                    {
                        if (info.MapMode) *VRAM++ = ((*mapData++) + mapoff) | (paloff << 12); // 1WORD data
                        else *VRAM32++ = ((*Data32++) + mapoff) | (paloff << 20); // 2WORD data
                    }
                }
            }

        };

        /** @brief extension of ScrollScreen interface to include support for loading bitmap images.
        * Used in NBG0, NBG1, and RBG0 interfaces.
        */
        template<class ScreenType, int16_t Id, uint16_t On>
        class BmpScreen: public ScrollScreen<ScreenType, Id, On>
        {
        public:

            /** @brief Loads a bitmap into VRAM and registers it for display on the ScrollScreen.
            *  @details The maximum supported loading size is 2 VRAM banks (262,144 bytes).
            *  Furthermore, VDP2 Bitmaps can only be stored in VRAM with one of 4 container sizes:
            *  512x256, 512x512, 1024x256, or 1024x512.
            *  These factors place the following bounds on the maximum size and VRAM requirements of a bitmap scroll:
            *
            *  |Bitdepth | Max Image Size     |    VRAM Usage     |
            *  |---------|--------------------|-------------------|
            *  | 4bpp    | 1024x512           | 1/2, 1, or 2 banks|
            *  | 8bpp    | 512x512 or 1024x256| 1 or 2 banks      |
            *  | 16bpp   | 512x256            | Always 2 banks    |
            *
            *  @param bmp Pointer to the bitmap interface to load from.
            *  @param loadPalette (optional) Enable/disable Auto loading a bitmaps's color palette to CRAM (default = true)
            *  @note If source Bitmap size is not equal to one of the container sizes, excess VRAM will be wasted.
            *  To conserve VRAM, consider converting to tilemap format with Bmp2Tile for smaller images when possible.
            *
            *  @note RBG0 only supports the 512x256 and 512x512 containers.
            */
            inline static void LoadBitmap(SRL::Bitmap::IBitmap* bmp, bool loadPalette = true)
            {
                SRL::Bitmap::BitmapInfo myInfo = bmp->GetInfo();
                int autoSize = 0;
                if ((uint32_t)ScreenType::CellAddress < VDP2_VRAM_A0)
                {
                    ScreenType::CellAddress = VRAM::AutoAllocateBmp(myInfo, ScreenType::ScreenID, &autoSize);
                    if (ScreenType::CellAddress) ScreenType::CellAllocSize = autoSize;
                }

                if (ScreenType::CellAddress == nullptr)
                {
                    SRL::Debug::Assert("Bitmap Allocation Failed");
                    return;
                }

                Bmp2VRAM(bmp->GetData(), ScreenType::CellAddress, myInfo);
                int colorID = -1;
                if (loadPalette && myInfo.Palette!=nullptr && myInfo.Palette->Count != 0)
                {
                    if (ScreenType::TilePalette.GetData() == nullptr)
                    {
                        SRL::CRAM::TextureColorMode col = myInfo.ColorMode;

                        //ensure 64 and 128 color palettes are converted to 256
                        if (col == SRL::CRAM::TextureColorMode::Paletted128 || col == SRL::CRAM::TextureColorMode::Paletted64)
                        {
                            col = SRL::CRAM::TextureColorMode::Paletted256;
                        }

                        //handle 16 color palette mapping to the allowed banks
                        if (GetSGLColorMode(col) == 0)
                        {
                            for (int i = 0; i < 8; ++i)
                            {
                                if (!SRL::CRAM::GetBankUsedState(i<<4, col))
                                {
                                    colorID = i<<4;
                                    break;
                                }
                            }
                        }
                        else colorID = SRL::CRAM::GetFreeBank(col);

                        if (colorID < 0)
                        {
                            SRL::Debug::Assert("Palette Load Failed- no CRAM Palettes available");
                            return;
                        }
                        SRL::CRAM::SetBankUsedState(colorID, col, true);
                        ScreenType::TilePalette = SRL::CRAM::Palette(col, colorID);
                        ScreenType::TilePalette.Load(myInfo.Palette->Colors, myInfo.Palette->Count);
                    }
                    ScreenType::TilePalette.Load(myInfo.Palette->Colors, myInfo.Palette->Count);
                }
                ScreenType::Init(myInfo);
            }

            /** @brief Copies Bmp data to VRAM
            * @param bmpData Source address of Data to copy.
            * @param bmpAdr VRAM address to copy to.
            * @param info info about bmp
            * @note When the source image does not completely fill the container the
            * transfer will pad each line of the image to allign to the container width.
            * However the padded VRAM is not cleared when using this function, so may display
            * garbage data. Use VDP2::VRAM::Blank() beforehand to clear VRAM area to zeros if desired.
            */
            inline static void Bmp2VRAM(void* bmpData, void* bmpAdr, SRL::Bitmap::BitmapInfo info)
            {
                uint8_t* data = (uint8_t*)bmpData;
                uint8_t* vram = (uint8_t*)bmpAdr;
                uint16_t dataWidth = info.Width;
                uint16_t containerWidth = dataWidth <= 512 ? 512 : 1024;
                if (info.ColorMode == CRAM::TextureColorMode::RGB555)
                {
                    dataWidth <<= 1;
                    containerWidth <<= 1;
                }
                else if (info.ColorMode == CRAM::TextureColorMode::Paletted16)
                {
                    dataWidth >>= 1;
                    containerWidth >>= 1;
                }

                for (int i = 0; i < info.Height; ++i)
                {
                   slDMACopy(data, vram, dataWidth);
                   vram += containerWidth;
                   data += dataWidth;
                }
            }
        };

        /** @brief NBG0 interface
         *  @details Normal Background Scroll 0:
         *
         *      -Available color depths: Paletted16, Paletted256, RGB555
         *      -Available Modes: Bitmap, Tilemap
         *      -Available features: Vertical/Horizontal Scrolling, Scaling, LineScroll Tables
         */
        class NBG0 : public BmpScreen<NBG0, scnNBG0, NBG0ON>
        {
        public:
            /** @brief VRAM address of line scroll table
             */
            inline static void* LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg0(info.SGLColorMode(), info.CharSize);
                slPageNbg0(NBG0::CellAddress, (void*)NBG0::TilePalette.GetData(), info.MapMode);
                slPlaneNbg0(info.PlaneSize);
                slMapNbg0(MapAddress, MapAddress, MapAddress, MapAddress);
            }

            /** @brief Initializes the ScrollScreen's bitmap specifications
             * @param info bitmap info
             */
            static void Init(SRL::Bitmap::BitmapInfo& info)
            {
                //(building bits of sgl size flag based on bitmap height/width)
                uint16_t sglFlag = info.Height <= 256 ? 0x2 : 0x6;
                sglFlag |= info.Width <= 512 ?  0 : 1<<3;
                slBitMapNbg0(GetSGLColorMode(info.ColorMode), sglFlag, NBG0::CellAddress);
                if (info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16)
                {
                    slBMPaletteNbg0(NBG0::TilePalette.GetId() >> 4);
                }
                else slBMPaletteNbg0(NBG0::TilePalette.GetId());
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg0(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y Screen Position
             */
            static void SetPosition(Math::Vector2D& pos) { slScrPosNbg0(pos.X.RawValue(), pos.Y.RawValue()); }

            /** @brief Sets the Scale of NBG0 Screen display
             *  @param scl Fixed Point X an Y scaling values
             *  @note The minimum scale that can be displayed is determined by the Scale Limit , Scaling values
             *  lower than the minimum will be clamped to the minimum (see SetScaleLimit() for more details)
             */
            static void SetScale(Math::Vector2D& scl) { slScrScaleNbg0(scl.X.RawValue(), scl.Y.RawValue()); }

            /** @brief Sets the minimum scale that the ScrollScreen can display
             *  @details NBG0 can scale its displayed tilemap or bitmap up freely, but has
             *  a minimum default scale of 1x due to cycle pattern limits.
             *  This minimum can be changed to 1/2x or 1/4x for further reduction at the
             *  cost of increasing its required cycle accesses( By 2x for 1/2x scaling or 4x
             *  for 1/4x scaling)
             *  @note -If the number of cycle accesses required for scaling is too large, it can interfere with
             *  registering scrolls for display with ScrollEnable().
             *  @note -Only 8bpp and 4bpp data support 1/2x scaling, and only 4bpp supports 1/4x scaling
             */
            static void SetScaleLimit(VDP2::ScaleLimit limit){slZoomModeNbg0((uint16_t)limit); }
        };

        /** @brief NBG1 interface
         * @details Normal Background Scroll 1:
         *
         *      -Available color depths: Paletted16, Paletted256, RGB555
         *      -Available Modes: Bitmap, Tilemap
         *      -Available features: Vertical/Horizontal Scrolling, Scaling, LineScroll Tables
         */
        class NBG1 : public BmpScreen<NBG1, scnNBG1, NBG1ON>
        {
        public:
            /** @brief VRAM address of line scroll table
             */
            inline static void* LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg1(info.SGLColorMode(), info.CharSize);
                slPageNbg1(NBG1::CellAddress, (void*)NBG1::TilePalette.GetData(), info.MapMode);
                slPlaneNbg1(info.PlaneSize);
                slMapNbg1(MapAddress, MapAddress, MapAddress, MapAddress);
            }
            /** @brief Initializes the ScrollScreen's bitmap specifications
             * @param info bitmap info
             */
            static void Init(SRL::Bitmap::BitmapInfo& info)
            {
                //(building bits of sgl size flag based on bitmap height/width)
                uint16_t sglFlag = info.Height <= 256 ? 0x2 : 0x6;
                sglFlag |= info.Width <= 512 ?  0 : 1<<3;
                slBitMapNbg1(GetSGLColorMode(info.ColorMode), sglFlag, NBG1::CellAddress);
                if (info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16)
                {
                    slBMPaletteNbg1(NBG1::TilePalette.GetId() >> 4);
                }
                else slBMPaletteNbg1(NBG1::TilePalette.GetId());
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg1(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y Offset of Scoll Position relative to screen
             */
            static void SetPosition(Math::Vector2D& pos) { slScrPosNbg1(pos.X.RawValue(), pos.Y.RawValue()); }

            /** @brief Sets the Scale of NBG1 Screen display
             * @param scl Fixed Point X and Y scaling values
             * @note The minimum scale that can be displayed is determined by The Scale Limit, Scaling values
             * lower than the minimum will be clamped to the minimum
             */
            static void SetScale(Math::Vector2D& scl) { slScrScaleNbg1(scl.X.RawValue(), scl.Y.RawValue()); }

            /** @brief Sets the minimum scale that the ScrollScreen can display
             *  @details NBG1 can scale its displayed tilemap or bitmap up freely, but has
             *  a default minimum scale of 1x due to cycle pattern limits.
             *  This minimum can be changed to 1/2x or 1/4x for further reduction at the
             *  cost of increasing its required cycle accesses( By 2x for 1/2x scaling or 4x
             *  for 1/4x scaling)
             *  @note -If the number of cycle access required for scaling is too large, it can interfere with
             *  registering scrolls for display with ScrollEnable().
             *  @note -Only 8bpp and 4bpp data supports 1/2x scaling, and only 4bpp supports 1/4x scaling
             */
            static void SetScaleLimit(VDP2::ScaleLimit limit){slZoomModeNbg1((uint16_t)limit); }
        };

        /** @brief  NBG2 interface
         *  @details Normal Background Scroll 2:
         *
         *      -Available color depths: Paletted16, Paletted256
         *      -Available Modes: Tilemap
         *      -Available features: Vertical/Horizontal Scrolling
         *  @note Unavailable when NBG0 color mode is RGB555
         */
        class NBG2 : public ScrollScreen<NBG2, scnNBG2, NBG2ON>
        {
        public:

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg2(info.SGLColorMode(), info.CharSize);
                slPageNbg2(NBG2::CellAddress, (void*)NBG2::TilePalette.GetData(), info.MapMode);
                slPlaneNbg2(info.PlaneSize);
                slMapNbg2(MapAddress, MapAddress, MapAddress, MapAddress);
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg2(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y Screen Position
             */
            static void SetPosition(Math::Vector2D& pos) { slScrPosNbg2(pos.X.RawValue(), pos.Y.RawValue()); }
        };

        /** @brief NBG3 interface
         *  @details Normal Background Scroll 3:
         *
         *      -Available color depths: Paletted16, Paletted256
         *      -Available Modes: Tilemap
         *      -Available features: Vertical/Horizontal Scrolling
         *  @note Unavailable when NBG0 color mode is RGB555.
         *  @note SRL Uses to dispay Degbug Text by default.
         */
        class NBG3 : public ScrollScreen<NBG3, scnNBG3, NBG3ON>
        {
        public:

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg3(info.SGLColorMode(), info.CharSize);
                slPageNbg3(NBG3::CellAddress, (void*)NBG3::TilePalette.GetData(), info.MapMode);
                slPlaneNbg3(info.PlaneSize);
                slMapNbg3(MapAddress, MapAddress, MapAddress, MapAddress);
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg3(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y  Screen Position
             */
            static void SetPosition(Math::Vector2D& pos) { slScrPosNbg3(pos.X.RawValue(), pos.Y.RawValue()); }
        };

        /** @brief Settings for RBG0 rotation constraints
         * @note More simulated axis of rotation require more VDP2 VRAM resources to display.
         */
        enum class RotationMode
        {
            /** @brief 2d rotation with only roll and zoom
             * @note No additional VRAM requirements
             */
            OneAxis,

            /** @brief Enables psuedo 3d rotation with pitch and yaw, but no roll (scale modified per line)
             * @note Requires 0x2000-0x18000 bytes in arbitrary VRAM Bank (No cycle slots required)
             */
            TwoAxis,

            /** @brief Enables full 3d rotation with pitch, yaw and roll (scale modified per pixel)
             * @note Requires 0x2000-0x18000 bytes in Reserved VRAM bank (all 8 cycle slots required)
             */
            ThreeAxis,
        };

        /** @brief Options for treating display of rotation parameters outside of the
         * main tilemap/bitmap area.
         * @details when 3d rotation and scaling is simulated, some perspectives may display
         * areas on screen that extend beyond the 4x4 plane area defined by the tilemap pattern.
         * Use these options with RBG0::SetOverDisplayA/B to specify what to display in
         * those areas.
        */
        enum class OverPattern
        {

             /** @brief Screen areas outside the map repeat the full tilemap pattern (Default)
             */
            RepeatMap,

            /** @brief Screen areas outside the map repeat a single selectable tile from the map's tileset.
            *   @note Not available in Bitmap Mode
            */
            RepeatTile,

            /** @brief Screen areas outside the map area are treated as transparent pixels
            */
            Transparent,

            /** @brief Areas outside a 512x512 square centered at  coordintate [256,256]
            *  are made transparent regardless of the underlying map area
            */
            Transparent512x512,

        };

        /** @brief Options for switching between the 2 Rotation parameters available for RBG0 display
         */
        enum class SwitchMode
        {
            /** @brief Only display Primary Tilemap and rotation params
             */
            OnlyPrimary = 0,

             /** @brief Only display Secondary Tilemap and rotation params
             */
            OnlySecondary = 1,

            /** @brief Switch RBG0 Tilemap display based on perspective of Tilemap A
             *  @details In regions of the screen beyond the Primary Parameter's simulated 'horizon'*,
             *  the Secondary Parameter will be displayed instead.
             *  @note -*(More technically, any screen coordinate where the scaling coefficient
             *  read for the primary parameter has its high bit set to 1 will display Parameter B data instead).
             *  @note - Only Parameter A can display 3 Axis (Per Dot) Rotation in this mode.
            */
            PerspectiveSwitch,

            /** @brief Switch RBG0 rotation display based on a VDP2 window
             *  @details Rotation A displays in regions inside the selected VDP2 window While
             *  Rotation B displays outside the window
             *  @note - This is the only avaiable mode where Primary and Secondary Parameters can
             *  Simultaneously display 3 Axis (Per Dot) Rotation.
            */
            WindowSwitch,

        };

        /** @brief Used to select between the 2 Rotation Parameters on RBG0: Primary or Secondary.
         * Primary rotation parameter controls the display of the Primary Tilemap, Secondary Parameter controls
         * display of the Secondary Tilemap.
         */
        enum class RotationParameter : uint16_t
        {
            /** @brief Select Primary Rotation Parameter (RA)
            */
            Primary = RA,
            /** @brief Select Secondary Rotation Parameter (RB)
            */
            Secondary = RB,
        };

        /** @brief RBG0 interface
         * @details Rotating Background Scroll 0:
         *
         *      -Available color depths: Paletted16, Paletted256, RBG555
         *      -Available image modes: Tilemap, Bitmap
         *      -Available features: Scrolling, Scaling, Rotation,
         *      -Reading a coefficient table from VRAM allows per-line/per-pixel scaling
         *       to simulate a 3D plane.
         *      -Simulate up to 2 planes using separate rotation parameters and tilemaps
         * @note Tilemap Loading for Secondary rotation parameter through RBG0 interface does not support configuration
         * for display on RBG1. While it is possible to control RBG1 rotation mode and perspective through
         * the functions for the secondary parameter, the required VRAM reservations for its display must be
         * configured manually.
         */
        class RBG0 : public BmpScreen<RBG0, scnRBG0, RBG0ON>
        {

        private:
            /** @brief Initializes the Tilemap settings for Secondary Tilemap on Rotation parameter B
             * @param info Tilemap info to initialize with
             * @note Initialization fails if the tilemap's data format does not match the primary tilemap
             */
            inline static bool InitB(SRL::Tilemap::TilemapInfo info)
            {
                if (info.MapMode!= RBG0::Info.MapMode || info.SGLColorMode() != RBG0::Info.SGLColorMode() || info.CharSize != RBG0::Info.CharSize)
                {

                    if (info.MapMode!= RBG0::Info.MapMode)
                        SRL::Debug::Print(0, 10, "MAP");
                    else if (info.SGLColorMode() != RBG0::Info.SGLColorMode())
                        SRL::Debug::Print(0, 10, "COL");
                    else
                        SRL::Debug::Print(0, 10, "CHAR");
                    // SRL::Debug::Assert("RBG0 Secondary Tilemap Init Failed: Format does not match primary tilemap");
                    return false;
                }
                slPlaneRB(info.PlaneSize);
                sl1MapRB(MapAddressRB);
                return true;
            }

            /** @brief Function to Load tilemap to VRAM for use with Rotation Parameter RB, called in Overloads of Load tilemap.
             * Because RB data can not be loaded without RA data present, This is never explicitly called alone by user. Instead
             * it is called within RBG0's LoadTilemap overloads which allow loading both tilemaps at once.
            */
            inline static void LoadTilemapB(SRL::Tilemap::ITilemap& tilemap, VDP2::SwitchMode mode = VDP2::SwitchMode::PerspectiveSwitch, bool LoadPalette = true)
            {
                SRL::Tilemap::TilemapInfo info = tilemap.GetInfo();

                uint32_t page_sz = 0x800;
                uint32_t sz = (info.MapHeight * info.MapWidth) << 1;
                if (info.CharSize == CHAR_SIZE_1x1) page_sz <<= 2;
                if (info.MapMode == PNB_2WORD)
                {
                    sz <<= 1;
                    page_sz <<= 1;
                }

                if (info.PlaneSize == PL_SIZE_2x2) page_sz <<= 2;
                else if (info.PlaneSize == PL_SIZE_2x1) page_sz <<= 1;

                if (MapAddressRB < (void*)VDP2_VRAM_A0)
                {
                    MapAddressRB = VRAM::Allocate(sz, page_sz, VramBank::A0, 0);
                    MapAllocSizeB = sz;
                }

                if (CellAddressRB < (void*)VDP2_VRAM_A0)
                {
                    CellAddressRB = VRAM::Allocate(info.CellByteSize, 32, VramBank::A1, 0);
                    CellAllocSizeB = info.CellByteSize;
                }

                if (MapAddressRB == nullptr || CellAddressRB == nullptr || MapAllocSizeB < sz || CellAllocSizeB < info.CellByteSize)
                {
                    SRL::Debug::Assert("RBG0 Tilemap B Load Failed: Insufficient VRAM");
                    return;
                }

                if (!InitB(info)) return;
                int colorID = 0;
                if (LoadPalette && info.ColorMode != SRL::CRAM::TextureColorMode::RGB555)
                {
                    if (RBG0::TilePaletteB.GetData() == nullptr)
                    {
                        if ((colorID = SRL::CRAM::GetFreeBank(RBG0::Info.ColorMode)) < 0)
                        {
                            SRL::Debug::Assert("Tilemap Palette Load Failed- no CRAM Palettes available");
                            return;
                        }
                        SRL::CRAM::SetBankUsedState(colorID, RBG0::Info.ColorMode, true);
                        RBG0::TilePaletteB = SRL::CRAM::Palette(RBG0::Info.ColorMode, colorID);
                    }
                    uint16_t len = (info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16) ? 16 : 256;
                    RBG0::TilePaletteB.Load((Types::HighColor*)tilemap.GetPalData(), len);
                }

                RBG0::Cell2VRAM((uint8_t*)tilemap.GetCellData(), RBG0::CellAddressRB, info.CellByteSize);
                RBG0::Map2VRAM(
                    info,
                    (uint16_t*)tilemap.GetMapData(),
                    RBG0::MapAddressRB,
                    RBG0::TilePaletteB.GetId(),
                    RBG0::GetCellOffset(info, RBG0::CellAddressRB));
            }

        public:
            using ScrollScreen<RBG0, scnRBG0, RBG0ON>::LoadTilemap;

            /** @brief VRAM Address of RBG0 Coefficient table
             */
            inline static void* KtableAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief VRAM Address of RBG0 Coefficient table B
             */
            inline static void* KtableAddressB = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief VRAM Address of TilemapB map data
             */
            inline static void* MapAddressRB  = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief VRAM Address of TilemapB cell data
             */
            inline static void* CellAddressRB = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief Size of Map B allocation
            */
            inline static uint32_t MapAllocSizeB;

            /** @brief Size of Cell B allocation
            */
            inline static int32_t CellAllocSizeB;

            /* @brief The Palette handler for LoadTilemapB
            */
            inline static CRAM::Palette TilePaletteB = CRAM::Palette();

            /** @brief Manually Sets VRAM area for Priamary or Secondary RBG0 Cell Data (For Advanced Use Cases)
             * @details This function manually sets an area in VRAM for Cell Data to be loaded to. Unless the
             * Address is obtained using VDP2::VRAM::Allocate(), the VRAM allocator will be bypassed entirely.
             * No Checks are performed for proper data alignment or cycle conflicts. For advanced use cases only.
             * @param address the VRAM address of the allocation
             * @param size the size of the allocation
             * @param param (optional) The Rotation parameter that will use this address (Defaults to Primary)
             * @return Echoes Address
             * @note For valid dual parameter access on RBG0, Secondary data must ALWAYS be allocated in the same
             * bank as Primary data. This is not verified by this funciton.
             */
            void* SetCellAddress(void* address, int size, VDP2::RotationParameter param =VDP2::RotationParameter::Primary)
            {
                if (param == VDP2::RotationParameter::Primary)
                {
                    RBG0::CellAddress = address;
                    RBG0::CellAllocSize = size;
                }
                else
                {
                    RBG0::CellAddressRB = address;
                    RBG0::CellAllocSizeB = size;
                }
                return address;
            }

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @details Initailization is performed automatically when using the built in
             * LoadTilemap functions. This function is only required when performing custom loading
             * routines. Ensure that VRAM for Cell and Map data has already been allocated before calling.
             * @param info Tile map info
             * @note This function initializes both rotation parameters using the same
             * source tilemap. To use a secondary tilemap in RB, use the respective
             * overloads
             */
            inline static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slRparaMode(RA);
                slOverRA(0);
                slOverRB(0);
                slCharRbg0(info.SGLColorMode(), info.CharSize);
                slPageRbg0(RBG0::CellAddress, (void*)RBG0::TilePalette.GetData(), info.MapMode);
                slPlaneRA(info.PlaneSize);
                slPlaneRB(info.PlaneSize);
                sl1MapRA(RBG0::MapAddress);
                sl1MapRB(RBG0::MapAddress);
            }

            /** @brief Initializes the ScrollScreen's bitmap specifications
             *  @details Initailization is performed automatically when using the built in
             * LoadBitmap function. This function is only required when performing custom loading
             * routines. Ensure that VRAM has already been allocated before calling.
             * @param info bitmap info
             */
            static void Init(SRL::Bitmap::BitmapInfo& info)
            {
                slRparaMode(RA);
                slOverRA(0);
                uint16_t sglFlag = info.Height <= 256 ? 0x2 : 0x6;
                sglFlag |= info.Width <= 512 ? 0 : 1<<3;
                slPageRbg0(RBG0::CellAddress, (void*)RBG0::TilePalette.GetData(), RBG0::Info.MapMode);//this is required to reserve the VRAM bank
                slBitMapRbg0(GetSGLColorMode(info.ColorMode), sglFlag, RBG0::CellAddress);
                if (info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16)
                {
                    slBMPaletteRbg0(RBG0::TilePalette.GetId() >> 4);
                }
                else slBMPaletteRbg0(RBG0::TilePalette.GetId());
            }

            /** @brief Overload initializes Primary and Secondary rotation parameters to separate source tilemaps
             *  @details Initailization is performed automatically when using the built in
             *  LoadTilemap functions. This function is only required when performing custom loading
             *  routines. Ensure that VRAM for Cell and Map data has already been allocated before calling.
             *  @note Initialization for Secondary fails if it does not share the same configuration as Primary for
             *  color depth, tile size, and map data type.
             *  @param infoA Tilemap info of primary tilemap to initialize
             *  @param infoB Tilemap info of secondary tilemap to initialize
            */
            static void Init(SRL::Tilemap::TilemapInfo& infoA,SRL::Tilemap::TilemapInfo& infoB)
            {
                RBG0::Init(infoA);
                RBG0::InitB(infoB);
            }

            /** @brief Simultaneously Configures the desired rotation mode and loads primary tilemap
             *  @details Overload to load the primary tilemap and configure its rotation
             *  with one function. Calls SetRotationMode() followed by LoadTilemap().
             *  @param tilemap The tilemap to load
             *  @param mode The rotation mode to configure for the tilemap (See SetRotationMode for details)
             *  @param LoadPalette (optional) enable/disable auto loading of palette data (default = true)
             */
            inline static void LoadTilemap(SRL::Tilemap::ITilemap& tilemap,VDP2::RotationMode mode, bool LoadPalette = true)
            {
                SetRotationMode(mode);
                LoadTilemap(tilemap,LoadPalette);
            }

            /** @brief Load and configure both a Primary and Secondary Tilemap
             *  @details Overload to load and configure both primary and secondary tilemaps on RBG0.
             *  The SwitchMode will be initialized to UsePerspective. Use SetSwitchMode() after loading
             *  to change the setting if desired.
             *  @param tilemapA The tilemap to load to Primary Tilemap (A)
             *  @param tilemapB The tilemap to load to Secondary Tilemap (B)
             *  @param modeA The rotation mode to configure for primary map (RA)
             *  @param modeB The rotation mode to configure for secondary map (RB)
             *  @param LoadPalette (optional) set false to disable auto loading of palette data (default = true)
             *  @note -Due to shared registers, the Secondary Tilemap must have the same underlying data format as the
             *  Primary Tilemap (color depth, tile size, map data type). Furthermore its data must be able to fit within the same
             *  Reserved VRAM banks as the corresponding primary data ((celDataA + celDataB) <= 0x20000 bytes and (mapDataA+mapDataB)<=0x20000 bytes).
             *  @note -See VDP2::Switchmode documentation for limits on rotation modes for A and B under various conditions.
             */
            inline static void LoadTilemap(SRL::Tilemap::ITilemap& tilemapA,SRL::Tilemap::ITilemap& tilemapB, VDP2::RotationMode modeA,VDP2::RotationMode modeB, bool LoadPalette = true)
            {
                SetRotationMode(modeA,modeB,false);
                SetParameterMode(SwitchMode::PerspectiveSwitch);
                LoadTilemap(tilemapA,LoadPalette);
                LoadTilemapB(tilemapB);
            }

            /** @brief Sets How screen regions outside the tilemap area are displayed by the rotation parameters
             *  @param param The Tilemap/Rotation Parameter that is being set(primary or secondary)
             *  @param mode the display mode to select
             *  @param tileIndex (Optional) The tile index to display in exterior regions if RepeatTile is
             *  selected as mode (defaults to tile 0 if not specified)
             *  @note See OverPattern documentatioin for descriptions of available modes
             */
            inline static void SetOverDisplay(VDP2::RotationParameter param, VDP2::OverPattern mode, uint16_t tileIndex = 0)
            {
                if (param == VDP2::RotationParameter::Primary)
                {
                    slOverPatRA(tileIndex);
                    slOverRA((uint16_t)mode);
                }
                else
                {
                    slOverPatRB(tileIndex);
                    slOverRB((uint16_t)mode);
                }
            }

            /** @brief Sets the condition for switching between display of Primary and Secondary parameters/Tilemaps on screen.
             * @param mode the display mode to select
             * @note See VDP2::SwitchMode documentation for description of available modes
             * @note If a mode is selected that includes display of Secondary Parameter (RB) when no
             * secondary tilemap has been loaded, it will use the same
             * source tilemap as the primary parameter.
             */
            inline static void SetParameterMode(SwitchMode mode)
            {
                slRparaMode((uint16_t)mode);
            }

            /** @brief Select what type of rotation to use for the rotating scroll.
             * @details This initializes both Primary and Secondary parameters to the same mode.
             * Use the overload to initialize separate modes for each.
             * @param mode The RotationMode to use for RBG0
             * @param vblank (Optional) Chose to update VRAM at VBLANK to reduce amount of stored coefficient
             * data required (default = true).
             * When 2 or 3 axis rotation is selected, VRAM is allocated to store the
             * required scaling coefficients. If vblank is set false, all possible coefficients will be
             * statically stored in VRAM as a 0x18000 byte table. If vblank is set true,
             * select coefficients for the next frame's perspective will be dynamically written
             * to VRAM during Vblank, reducing the VRAM footprint to 0x2000 bytes per
             * rotation parameter. If this transfer can not complete within the Vblank window
             * screen tearing may be visible in the plane.
             */
            inline static void SetRotationMode(VDP2::RotationMode mode, bool vblank = true)
            {
                //Allocate coeficient tables if necessary for either param:
                uint16_t kflag = 0;
                if (mode != VDP2::RotationMode::OneAxis)
                {
                    if (vblank == true)
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x4000, 0x20000, VDP2::VramBank::B0, 0);
                        VDP2::RBG0::KtableAddressB =(void*)((uint32_t)VDP2::RBG0::KtableAddress + 0x2000);
                    }
                    else
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x18000, 0x20000, VDP2::VramBank::B0, 0);
                        VDP2::RBG0::KtableAddressB = VDP2::RBG0::KtableAddress;
                        slMakeKtable(VDP2::RBG0::KtableAddress);
                        kflag = K_FIX;
                    }
                }

                //set mode of params A and B
                switch (mode)
                {
                case RotationMode::OneAxis:
                    slKtableRA(nullptr, K_OFF);
                    slKtableRB(nullptr, K_OFF);
                    break;

                case RotationMode::TwoAxis:
                    slKtableRA((void*)VDP2::RBG0::KtableAddress, kflag | K_LINE | K_2WORD | K_ON);
                    slKtableRB((void*)VDP2::RBG0::KtableAddressB, kflag| K_LINE | K_2WORD | K_ON);
                    break;

                case RotationMode::ThreeAxis:
                    slKtableRA((void*)VDP2::RBG0::KtableAddress, kflag | K_DOT | K_2WORD | K_ON);
                    slKtableRB((void*)VDP2::RBG0::KtableAddressB, kflag | K_DOT | K_2WORD | K_ON);
                    break;
                }
            }

            /** @brief Overload to select separate rotation modes for both the primary and secondary
             * rotating planes.
             * @param modeA The RotationMode to use for primary tilemap
             * @param modeB The RotationMode to use for secondary tilemap
             * @param vblank (Optional) Chose to update VRAM at VBLANK to reduce amount of stored coefficient
             * data required (default = true).
             * When 2 or 3 axis rotation is selected, VRAM is allocated to store the
             * required scaling coefficients. If vblank is set false, all possible coefficients will be
             * statically stored in VRAM as a 0x18000 byte table. If vblank is set true,
             * select coefficients for the next frame's perspective will be dynamically written
             * to VRAM during Vblank, reducing the VRAM footprint to 0x2000 bytes per
             * rotation parameter. If this transfer can not complete within the Vblank window
             * screen tearing may be visible in the plane.
             */
            inline static void SetRotationMode(VDP2::RotationMode modeA, VDP2::RotationMode modeB, bool vblank = true)
            {
                uint16_t kflag = 0;
                //Allocate coeficient tables if necessary for either param:
                if (modeA != VDP2::RotationMode::OneAxis || modeB != VDP2::RotationMode::OneAxis)
                {
                    if (vblank == true)
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x4000, 0x20000, VDP2::VramBank::B0, 0);
                        VDP2::RBG0::KtableAddressB = (void*)((uint32_t)VDP2::RBG0::KtableAddress+0x2000);
                    }
                    else
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x18000, 0x20000, VDP2::VramBank::B0, 0);
                        VDP2::RBG0::KtableAddressB =  VDP2::RBG0::KtableAddress;
                        slMakeKtable(VDP2::RBG0::KtableAddress);
                        kflag = K_FIX;
                    }
                }

                //set mode of param B
                switch (modeB)
                {
                case RotationMode::OneAxis:
                    slKtableRB(nullptr, K_OFF);
                    break;

                case RotationMode::TwoAxis:
                    slKtableRB((void*)VDP2::RBG0::KtableAddressB, kflag | K_LINE | K_2WORD | K_ON);
                    break;

                case RotationMode::ThreeAxis:
                    slKtableRB((void*)VDP2::RBG0::KtableAddressB, kflag | K_DOT | K_2WORD | K_ON);
                    break;
                }

                //set mode of param A
                switch (modeA)
                {
                case RotationMode::OneAxis:
                    slKtableRA(nullptr, K_OFF);
                    break;

                case RotationMode::TwoAxis:
                    slKtableRA((void*)VDP2::RBG0::KtableAddress, kflag | K_LINE | K_2WORD | K_ON);
                    break;

                case RotationMode::ThreeAxis:
                    slKtableRA((void*)VDP2::RBG0::KtableAddress, kflag | K_DOT | K_2WORD | K_ON);
                    break;
                }
            }

            /** @brief Writes the current matrix transform to RBG0 rotation parameters
             * to update its perspective
             * @param param  selects which rotation parameter to write the matrix to (optional- defaults to Primary)
             */
            inline static void SetCurrentTransform(VDP2::RotationParameter param = VDP2::RotationParameter::Primary)
            {
                slCurRpara((uint16_t)param);
                slPushMatrix();
                {
                    slScrMatConv();
                    slScrMatSet();
                }
                slPopMatrix();
                slCurRpara(RA);
            }

            /** @brief Sets the plane of Tilemap Data to be displayed
             * @param a,b,c,d Page Table addresses of the planes to display
             * @note Multi Plane maps and maps for the Secondary Rotation parameter
             * must be set with the 16 plane overloads of this function
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) {

                sl1MapRA(a);
                sl1MapRB(b);
            }

            /** @brief Sets the layout of Tilemaps comprised of multiple planes from a 4x4 grid of plane indices
             * @details Unlike NBG scrolls RBG0 only loads with the default repetition of 1 plane of tilemap data. Use this
             * function after loading to set the layout planes of larger multi-plane tilemaps within a 4x4 grid:
             *      |a,b,c,d|
             *      |e,f,g,h|
             *      |i,j,k,l|
             *      |m,n,o,p|
             * @param layout 4x4 array of uint8_t indices representing the index of each plane in the map data
             * (index 0 = first plane of the map data, increments by 1 per plane of map data stored)
             * @param param The Rotation parameter whose map is being set: primary or secondary (defaults to primary)
             * @note No check is performed to ensure the indecies entered are within the bounds of
             * the scroll's map data. Specifying indices larger than the number of planes loaded
             * will result in the diplay of garbage data in those portions of the map
             */
            static void SetPlanes(const uint8_t layout[4][4], VDP2::RotationParameter param = VDP2::RotationParameter::Primary)
            {
                uint32_t offset = param == VDP2::RotationParameter::Primary ? ((uint32_t)RBG0::MapAddress & 0x1ffff) >> 11 : ((uint32_t)RBG0::MapAddressRB & 0x1ffff) >> 11;
                uint8_t sz = (VDP2::RBG0::Info.CharSize == CHAR_SIZE_1x1) ? 4:1;
                uint8_t sLayout[4][4] = {};
                if (VDP2::RBG0::Info.MapMode == PNB_2WORD) sz <<= 1;
                if (VDP2::RBG0::Info.PlaneSize == PL_SIZE_2x2) sz <<= 2;
                else if (VDP2::RBG0::Info.PlaneSize == PL_SIZE_2x1) sz <<= 1;
                for (size_t i = 0; i < 4; ++i) {
                    for (size_t j = 0; j < 4; ++j) {
                        sLayout[i][j] = (layout[i][j] * sz) + (uint8_t)offset;
                    }
                }
                if (param == VDP2::RotationParameter::Primary) sl16MapRA((uint8_t*)sLayout);
                else sl16MapRB((uint8_t*)sLayout);
            }

            /** @brief Registers a VDP2 window area to Display Seconday rotation in.
             * @details When the parameter switching mode is set to WindowSwitch, Secondary Tilemap and Rotation
             * will display inside the window area specified here, while Primary Tilemap will display outside the window.
             * This can be set independently from the the window specified with RBG0::UseWindow(), which specifies the area
             * in which to display RBG0 as a whole.
             * @param combo The type of logic used to compine window0 and window1 areas when using both for Secondary Tilemap display (AND/OR)
             * @param Window0 The desired setting for window 0
             * @param Window1 The desired setting for window 1
             * @note The Rotation Window can not be set to use the Sprite Window.
             */
            inline static void SetRotationWindow(VDP2::Logic combo, VDP2::Area Window0, VDP2::Area Window1)
            {
                uint16_t WindowArea = (uint16_t)combo | (uint16_t)Window0 | ((uint16_t)Window1 << 2);
                slScrWindowMode(scnROT, WindowArea);
            }
        };

        /** @brief Sprite Color Calculation Conditions (See SpriteLayer::SetColorCondition() for details)
         */
        enum class ColorCondition : uint16_t
        {
            /** @brief Perform color calculation only if priority is less than TestValue
             */
            PriorityLessThan = CC_pr_CN,

            /** @brief Perform color calculation only if priority is equal to  TestValue
             */
            PriorityEquals = CC_PR_CN,

            /** @brief Perform color calculation only if priority is greater than TestValue
             */
            PriorityGreaterThan = CC_PR_cn,
        };

        /** @brief Used to reference the Sprite color calculation ratio and priority registers
         * @note Can refer to Color Calculation banks or Priority Banks depending on use
         */
        enum class SpriteBank : int16_t
        {
            Bank0 = scnSPR0,
            Bank1 = scnSPR1,
            Bank2 = scnSPR2,
            Bank3 = scnSPR3,
            Bank4 = scnSPR4,
            Bank5 = scnSPR5,
            Bank6 = scnSPR6,
            Bank7 = scnSPR7,
        };

        /** @brief Interface to control VDP2 settings for the Sprite Layer (data from the VDP1 framebuffer),
         * such as Display priority and VDP2 color calculation)
         */
        class SpriteLayer
        {
        public:
            /** @brief Turns VDP2 Color Calculation on for Sprite Layer
             */
            inline static void ColorCalcON()
            {
                VDP2::ColorCalcScrolls |= SPRON;
                slColorCalcOn(VDP2::ColorCalcScrolls);
            }

            /** @brief Turns VDP2 Color Calculation Off for Sprite Layer
             */
            inline static void ColorCalcOFF()
            {
                VDP2::ColorCalcScrolls &= (~SPRON);
                slColorCalcOn(VDP2::ColorCalcScrolls);
            }

            /** @brief Set the opacities that sprites can select from in CC registers
             * @details This Function takes the opacity specified as a fixed point value and converts it to
             * one of the 32 color calculation ratios that the system can use (value is floored to the nearest ratio).
             * It then sets the ratio in the specified sprite cc register (cc register 0 if not specified)
             * @note -Color ratios only apply to highest priority pixels in frame
             * @note -When Color Calc is ON, max opacity is ~(0.97). Fully opaque sprites must be excluded by the color condition.
             * @note -When ColorCalcMode is set to UseColorAddition, The Opacity levels are ignored
             * and Color addition is applied to all sprites whose priority meets the color condition (see SpriteLayer::SetColorCondition() for details)
             * @note -RGB sprites always use the opacity set in CC register[bank0]
             * @note -Does NOT turn color calc ON or OFF for the Sprite Layer(use SpriteLayer::ColorCalcON,OFF())
             * @note -available cc registers vary by Palette code config- default allows selecting all 8 banks
             * @param opacity Fxp decimal value between 0.0 and 1.0 representing the opacity to set in the selected bank
             * @param bank (optional) which of the 8 CC registers to Set the opacity in (defaults to 0)
             */
            inline static void SetOpacity(SRL::Math::Types::Fxp opacity, VDP2::SpriteBank bank = VDP2::SpriteBank::Bank0)
            {
                if (opacity < 0.0 || opacity > 1.0)
                {
                    return;
                }
                else
                {
                    slColRate((int16_t)bank, 31 - (uint16_t)(opacity.RawValue() >> 11));
                }
            }

            /** @brief Set the priority Layers That sprites can select from in PR registers
             * @details This function sets one of the 8 priority registers that a Palette sprite can reference (default bank0)
             * @note -available registers vary by Palette code config- default can only pull from bank0 and bank1
             * @note -RGB555 data always use the priority set in bank0
             * @note -During VDP2 init, priority bank0 and bank1 are initialized to Layer3 and Layer4 respectively
             * @note -Changing these priorities will result in differing behavior for sprite color calculation
             * (See documentation of SpriteLayer::SetColorCondition() for more details)
             * @param pr enum VDP2::Priority Layer
             * @param bank (optional) enum VDP2::SpriteBank designating which priority bank to write to (default = Bank0)
             */
            inline static void SetPriority(SRL::VDP2::Priority pr, VDP2::SpriteBank bank = VDP2::SpriteBank::Bank0)
            {
                slPriority((int16_t)bank, (uint16_t)pr);
            }

            /** @brief Set conditions under which VDP2 color calculation is performed on sprites based on their priority.
             * @details Sets up a condition that allows only select sprites to receive Half Transparent
             *  color calculation with VDP2 layers. To make a sprite fully opaque, selectively turn color calculation off for it by
             *  assigning it to use a Priority Bank containing a layer that does not satisfy the chosen Color Condition.
             *  The default VDP2 initialization uses [ColorCondition::PriorityEquals] [Priority::Layer4],
             *  with Priority Bank0 set to Layer3 and Priority Bank1 set to Layer4. With this config RGB sprites receive no VDP2
             *  color calculation, while Palette sprites only receive color calculation when they select priority from Bank1
             * @param Condition The type of condition that VDP2 Color Calculation will follow.
             * @param TestValue The Layer that a sprite's priority will be tested against in the condition.
             */
            inline static void SetColorCondition(VDP2::ColorCondition Condition, SRL::VDP2::Priority TestValue)
            {
                slSpriteCCalcCond((uint16_t)Condition);
                slSpriteCCalcNum((uint16_t)TestValue);
            }

            /** @brief Causes VDP2 to interpret all data from the VDP1 framebuffer as Palette Data, regardless of the
             * original image type. RGB color can no longer be displayed, and its raw data will be cast to a Palette code.
             * @note Certain effects such as sprite shadow and sprite window can only be used in this mode.
             */
            inline static void PaletteExclusiveMode()
            {
                slSpriteColMode(0);
                slSpriteType(2);
            }

            /** @brief Enable use of the Sprite Window on VDP2 ScrollScreens.
            *  @note Only Available in Palette Exclusive Mode with Palette Code Types 2-7.
            *  Not available in Hi resolution Modes.
            *  VDP2 Sprite Shadow effect becomes unavailable when Sprite Window is enabled.
            */
            inline static void EnableSpriteWindow()
            {
                slSpriteWinMode(1);
            }


        };

        /** @brief Clear all VDP2 VRAM allocations and reset all Scroll Screen VRAM References, as well
         * as all CRAM allocations associated with VDP2 Scroll Screens if desired.
         * @note When Loading a new set of Data and Configurations for Scroll Screens with auto allocation, Call this first
         * to ensure old data is freed
         * @param FreePalettes (optional: default = true) also free all CRAM pallets currently registered to ScrollScreens
         */
        inline static void ClearVRAM(bool FreePalettes = true)
        {
            //reset ScrollScreen VRAM References
            if ((int)NBG0::MapAddress < (VDP2_VRAM_B1 + 0x18000))
            {
                VDP2::NBG0::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
                VDP2::NBG0::CellAddress = (void*)(VDP2_VRAM_A0 - 1);
                VDP2::NBG0::LineAddress = (void*)(VDP2_VRAM_A0 - 1);
                if (FreePalettes && VDP2::NBG0::TilePalette.GetData())
                {
                    SRL::CRAM::SetBankUsedState(VDP2::NBG0::TilePalette.GetId(),  VDP2::RBG0::TilePalette.GetMode(), false);
                    VDP2::NBG0::TilePalette = SRL::CRAM::Palette();
                }
            }

            if ((int)NBG1::MapAddress < (VDP2_VRAM_B1 + 0x18000))
            {
                VDP2::NBG1::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
                VDP2::NBG1::CellAddress = (void*)(VDP2_VRAM_A0 - 1);
                VDP2::NBG1::LineAddress = (void*)(VDP2_VRAM_A0 - 1);
                if (FreePalettes && VDP2::NBG1::TilePalette.GetData())
                {
                    SRL::CRAM::SetBankUsedState(VDP2::NBG1::TilePalette.GetId(),  VDP2::RBG0::TilePalette.GetMode(), false);
                    VDP2::NBG1::TilePalette = SRL::CRAM::Palette();
                }
            }

            if ((int)NBG2::MapAddress < (VDP2_VRAM_B1 + 0x18000))
            {
                VDP2::NBG2::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
                VDP2::NBG2::CellAddress = (void*)(VDP2_VRAM_A0 - 1);
                if (FreePalettes && VDP2::NBG2::TilePalette.GetData())
                {
                    SRL::CRAM::SetBankUsedState(VDP2::NBG2::TilePalette.GetId(),  VDP2::RBG0::TilePalette.GetMode(), false);
                    VDP2::NBG2::TilePalette = SRL::CRAM::Palette();
                }
            }

            if ((int)NBG3::MapAddress < (VDP2_VRAM_B1 + 0x18000))
            {
                VDP2::NBG3::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
                VDP2::NBG3::CellAddress = (void*)(VDP2_VRAM_A0 - 1);
                if (FreePalettes && VDP2::NBG3::TilePalette.GetData())
                {
                    SRL::CRAM::SetBankUsedState(VDP2::NBG3::TilePalette.GetId(),  VDP2::RBG0::TilePalette.GetMode(), false);
                    VDP2::NBG3::TilePalette = SRL::CRAM::Palette();
                }
            }

            VDP2::RBG0::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::RBG0::CellAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::RBG0::KtableAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (FreePalettes && VDP2::RBG0::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::RBG0::TilePalette.GetId(), VDP2::RBG0::TilePalette.GetMode(), false);
                VDP2::RBG0::TilePalette = SRL::CRAM::Palette();
            }

            // Clear VRAM banks
            for (int i = 0; i < 4; ++i)
            {
                VDP2::VRAM::currentTop[i] = VDP2::VRAM::bankTop[i];
                VDP2::VRAM::currentBot[i] = VDP2::VRAM::bankBot[i];
                VDP2::VRAM::bankCycles[i] = -1;
            }
            // Clear Rotation control bits of VDP2_RAMCTL
            VDP2_RAMCTL &= 0xff00;
            //Clear MapBank Flags:
            VDP2::VRAM::MapBankA0 = VDP2::VRAM::MapBankB0 = false;
            //keep 2 cycle slots reserved for ASCII
            VDP2::VRAM::bankCycles[3] = 2;
        }

        /** @brief Set the back color
         * @param color Color to use
         */
        inline static  void SetBackColor(const Types::HighColor& color)
        {
            slBack1ColSet((void*)(VDP2_VRAM_B1 + 0x1fee0), (uint16_t)color);
        }

        /** @brief Set color for print
         * @param index Color index
         * @param color Color to set
         */
        inline static void SetPrintPaletteColor(const uint16_t index, const Types::HighColor& color)
        {
            *(((Types::HighColor*)CRAM::BaseAddress) + (1 + (index << 8))) = (uint16_t)color;
        }

        /** @brief Initialize VDP2
         * @param backColor starting back color
         */
        inline static void Initialize(const Types::HighColor& backColor)
        {
            // Initialize color ram mode
            slColRAMMode(CRM16_2048);

            //slCharNbg0(COL_TYPE_256, CHAR_SIZE_1x1);
            VDP2::SetPrintPaletteColor(0, Types::HighColor::Colors::White);
            VDP2::SetPrintPaletteColor(1, Types::HighColor::Colors::Black);
            VDP2::SetPrintPaletteColor(2, Types::HighColor::Colors::Red);
            VDP2::SetPrintPaletteColor(3, Types::HighColor::Colors::Green);
            VDP2::SetPrintPaletteColor(4, Types::HighColor::Colors::Blue);
            VDP2::SetPrintPaletteColor(5, Types::HighColor::Colors::Yellow);
            VDP2::SetPrintPaletteColor(6, Types::HighColor::Colors::Magenta);

            // Set default color to white
            //slCurColor(0);

            // Set back color
            VDP2::SetBackColor(backColor);

            // Initialize ascii print to use NBG3
            SRL::ASCII::LoadFontSG((uint8_t*)(VDP2_VRAM_B1 + 0x800), 0);
            SRL::ASCII::SetPalette(0);
            int myID = SRL::CRAM::GetFreeBank(SRL::CRAM::TextureColorMode::Paletted16);
            SRL::CRAM::SetBankUsedState(myID, SRL::CRAM::TextureColorMode::Paletted16, true);
            SRL::ASCII::SetFont(0);
            slCharNbg3(COL_TYPE_16, CHAR_SIZE_1x1);
            slPageNbg3((void*)(VDP2_VRAM_B1 + 0x1D000), 0, PNB_1WORD | CN_10BIT);
            slPlaneNbg3(PL_SIZE_1x1);
            slMapNbg3((void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000));
            SRL::VDP2::NBG3::SetPriority(SRL::VDP2::Priority::Layer7);
            SRL::VDP2::NBG3::ScrollEnable();
            SRL::VDP2::NBG0::ScrollDisable(); // We don't want this on by default anymore
            // Fix param table at top of VRAM outside range of allocator - now user calls to slPerspective will always update here:
            SRL::VDP2::SpriteLayer::SetPriority(VDP2::Priority::Layer3,VDP2::SpriteBank::Bank0);
            SRL::VDP2::SpriteLayer::SetPriority(VDP2::Priority::Layer4,VDP2::SpriteBank::Bank0);
            slRparaInitSet((ROTSCROLL*)(VDP2_VRAM_B1 + 0x1ff00));
            VDP2_RAMCTL &= 0xff00;
            VDP2::VRAM::MapBankA0 = false;
            VDP2::VRAM::MapBankB0 = false;

        }

        /** @brief Data structure of a VDP2 color offset to be set in Offset A or Offset B
         *  @details The offset data is a signed 9 bit value per color channel.
         *  The valid range of inputs is -255 to +255. The sign determines whether the color offset
         *  is additive or subtractive on that channel. Values outside the range will be clamped on initialization.
         *  See SetColorOffsetA and SetColorOffsetB for more details on setting up a VDP2 color offset.
         */
        struct ColorOffset
        {
            /** @brief  Red channel offset/
             */
            int16_t Red;

            /** @brief  Green channel offset/
             */
            int16_t Green;

            /** @brief  Blue channel offset/
             */
            int16_t Blue;

            /** @brief Initialize with all channel offsets set to 0 (No Offset)
             */
            ColorOffset()
            {
                Red = 0;
                Green = 0;
                Blue = 0;
            }

            /** @brief initialize red green and blue channel offsets
             *  @param red offset for red channel
             *  @param green offset for green channel
             *  @param blue offset for blue channel
             */
            ColorOffset(int16_t red, int16_t green, int16_t blue)
                : Red(SRL::Math::Clamp<int16_t>(red, -255, 255)),
                  Green(SRL::Math::Clamp<int16_t>(green, -255, 255)),
                  Blue(SRL::Math::Clamp<int16_t>(blue, -255, 255))
            {
            }

            /** @brief initialize from an RGB555 source color
             *  @param col source HighColor to initialize from
             *  @note Can only initialize positive offsets. Use in conjunction with
             *  -= operator to subtract.
             */
            ColorOffset(const SRL::Types::HighColor& col)
            {
                this->Red = col.Red * 8;
                this->Green = col.Green * 8;
                this->Blue = col.Blue * 8;
            }

            /** @brief Set this offset equal to another
             *  @param col offset to set
             */
            constexpr ColorOffset& operator=(const ColorOffset& col)
            {
                Red = col.Red;
                Green = col.Green;
                Blue = col.Blue;
                return *this;
            }

            /** @brief Add to this offset
             *  @param col the offset to add
             */
            constexpr ColorOffset& operator+=(const ColorOffset& col)
            {
                Red = SRL::Math::Clamp<int16_t>(Red+col.Red, -255, 255);
                Green =SRL::Math::Clamp<int16_t>(Green+col.Green,-255,255);
                Blue = SRL::Math::Clamp<int16_t>( Blue+col.Blue,-255, 255);
                return *this;
            }
            /** @brief Subtract from this offset
             *  @param col the offset to subtract
             */
            constexpr ColorOffset& operator-=(const ColorOffset& col)
            {
                Red = SRL::Math::Clamp<int16_t>(Red-col.Red, -255, 255);
                Green =SRL::Math::Clamp<int16_t>(Green-col.Green, -255, 255);
                Blue = SRL::Math::Clamp<int16_t>(Blue-col.Blue, -255, 255);
                return *this;
            }

        };

        /** @brief Sets RGB color channel offset A
         *  @details VDP2 supports 2 RGB color offsets stored in special registers
         *  that any ScrollScreen or SpriteLayer can use. When registered all non-transparent
         *  pixels from the scroll screen will have the offset applied after all other color
         *  calculations are performed.
         *  @param offset The color offset to apply with Offset A
         */
        static void SetColorOffsetA(VDP2::ColorOffset & offset) //int16_t red, int16_t green, int16_t blue)
        {
            slColOffsetA(offset.Red, offset.Green, offset.Blue);
        }

        /** @brief Sets RGB color channel offset B
         *  @details VDP2 supports 2 RGB color offsets stored in special registers
         *  that any ScrollScreen or SpriteLayer can use. When registered all non-transparent
         *  pixels from the scroll screen will have the offset applied after all other color
         *  calculations are performed.
         *  @param offset The color offset to apply with Offset B
         */
        static void SetColorOffsetB(VDP2::ColorOffset & offset)
        {
            slColOffsetB(offset.Red, offset.Green, offset.Blue);
        }

        /** @brief Options for behavior of VDP2 Half Transparent Color Calculation
         */
        enum class ColorCalcMode : uint16_t
        {
            /** @brief Blend colors by the ratio specified in Higher priority Pixels
             */
            UseColorRatiosTop = 0x000,

            /** @brief Blend colors by the ratio specified in Lower priority Pixels
             */
            UseColorRatios2nd = 0x200,

            /** @brief Use additive color blending instead of ratios
             */
            UseColorAddition = 0x100,
        };

        /** @brief Sets VDP2 Half Transparent Color Calculation Mode (only one mode can be used at a time)
         * @param mode The VDP2 color calculation mode to use
         * @param extend Designates whether to extend color calculation to the top 3 Layer Priories instead of just top 2
         * @note Extended color calculation has many restrictions detailed in VDP2 users manual- not all color modes can support
         * extension simultaneously. If supported, 3rd priority pixels will blend with 2nd priority at a 50:50 ratio before the result is
         * blended with top priority pixels. If unsupported the behavior is identical to non-extended color calculation.
         */
        inline static void SetColorCalcMode(VDP2::ColorCalcMode mode = VDP2::ColorCalcMode::UseColorRatiosTop, bool extend = false)
        {
            uint16_t flags = (uint16_t)mode;
            if (extend) flags |= CC_EXT;
            slColorCalc((uint16_t)flags);
        }

        /** @brief Registers a window area where color calculation is turned on.
         * @details VDP2 color calculation (half transparency) will be limited to the specified window area
         * @param combo The type of logic used to compose multiple window areas for color caculation (AND/OR)
         * @param Window0 The desired setting for window 0
         * @param Window1 The desired setting for window 1
         * @param SpriteWindow (Optional) The desired setting for the Sprite Window. Defaults to Off.
         * @note The Sprite Window is only available when VDP1 framebuffer is in exclusive palette mode (No RGB data)
         */
        inline static void SetColorCalcWindow(VDP2::Logic combo, VDP2::Area Window0, VDP2::Area Window1, VDP2::Area SpriteWindow = VDP2::Area::Off)
        {
            uint16_t WindowArea = (uint16_t)combo | (uint16_t)Window0 | ((uint16_t)Window1 << 2) | ((uint16_t)SpriteWindow << 4);
            slScrWindowMode(scnCCAL, WindowArea);
        }

        /** @brief template base class for the 2 configurable vdp2 windows
         *  @note These are different from the VDP1 Clipping windows, which only apply inside VDP1 framebuffer.
         *  These Windows apply on a per Screen basis to the final display of scrollScreens
         *  and VDP1 frambuffer data as a whole. They can be defined with either a retangular region or line table.
         */
        template <class ScrWindow>
        class Window
        {
        public:
            inline static uint16_t *LineAddress = (uint16_t*) VDP2_VRAM_A0 - 1;
            inline static uint16_t *SourceAddress = (uint16_t *)VDP2_VRAM_A0 - 1;
            inline static uint16_t TableSize = 1;
            /** @brief Set rectangular Window Area from Screen Space Fxp Coordinates:
             *
             *  - Screen Center = (0.0,0.0)
             *  - Screen TopLeft = (-160.0, -120.0)
             *  - Screen BotRight = (160.0,120.0)
             *  @param TopLeft X,Y ScreenSpace Fxp Coordinate of Top Left corner of the window
             *  @param BottomRight X,Y ScreenSpace Fxp Coordinate of Top Left corner of the window
             */
            static void ConfigArea(SRL::Math::Vector2D TopLeft, SRL::Math::Vector2D BottomRight)
            {
                ScrWindow::SetWindow((TopLeft.Y.RawValue() >> 16) + 120, (TopLeft.X.RawValue() >> 16) + 160,
                                     (BottomRight.Y.RawValue() >> 16) + 120, (BottomRight.X.RawValue() >> 16) + 160);
            }

            /** @brief Set rectangular Window Area from Screen Pixel Coordinates:
             *
             *  - Screen Top Left = (0, 0)
             *  - Screen Bottom Right = (319,239)
             *  @param TopLeft Pixel Coordinate of Top Left corner of the window
             *  @param BottomRight Pixel Coordinate of Top Left corner of the window
             */
            static void ConfigArea(uint16_t top, uint16_t left, uint16_t bot, uint16_t right)
            {
                ScrWindow::SetWindow(top, left, bot, right);
            }

            /** @brief Set the Window as a Line Window using Window Table data (See VDP2 data types)
             *  @details If VRAM for table has been previously allocated the Source table will be loaded
             *  to the existing allocation. Otherwise, VRAM will first be auto allocated and table will be loaded
             *  to the new allocation.
             *  @param SourceTable address to load line data from
             *
             *  @note Because the table will reside in VRAM, calling VDP2::ClearVRAM will clear
             *  the table references. To avoid referencing an invalid address, the Window will be reset
             *  to rectangular area mode at this time, and must then be reconfigured with this function.
             */
            static void ConfigArea(void *Source, uint16_t size)
            {
                ScrWindow::SetWindow(0, 0, 240, 320); // reset Y window values to full range
                if (ScrWindow::LineAddress < (uint16_t*)VDP2_VRAM_A0) ScrWindow::LineAddress = (uint16_t*)VDP2::VRAM::AutoAllocateTable(240 * 4);

                if (ScrWindow::LineAddress == nullptr) return;

                slDMACopy(Source, (void *)ScrWindow::LineAddress, size);
                ScrWindow::SetWindowL((uint32_t)ScrWindow::LineAddress | 0x80000000);
                ScrWindow::TableSize = size;
                ScrWindow::SourceAddress = (uint16_t*) Source;
            }

            /** @brief Copy Registered Work Ram Table to Window table allocated in VDP2 VRAM.
             *  Can be Registered to execute at VBLANK to update Line Window every frame.
             *  @note The source table and VRAM destination must be registered with ConfigArea()
             *  beforehand, or the transfer will not execute.
             */
            static void CopyLineTable()
            {
                if (ScrWindow::TableSize == 1 || LineAddress < (uint16_t *)VDP2_VRAM_A0) return;
                slDMACopy(ScrWindow::SourceAddress, (void *)ScrWindow::LineAddress, ScrWindow::TableSize);
            }

            /** @brief Set the VRAM address of Line Window Table Manually
            */
            static void SetTableAddress(uint16_t *address)
            {
                ScrWindow::LineAddress = address;
            }

            /** @brief Get the VRAM address Of Line Window Table
             */
            static uint16_t *GetTableAddress() { return ScrWindow::LineAddress; }
        };

        /** @brief interface for VDP2 window 0
         */
        class Window0 : public Window<Window0>
        {
        public:
            static void SetWindow(uint16_t top, uint16_t left, uint16_t bot, uint16_t right) { slScrWindow0(left, top, right, bot); }
            static void SetWindowL(uint32_t address) { slScrLineWindow0((void *)address); }
        };

        /** @brief interface for VDP2 window 1
         */
        class Window1 : public Window<Window1>
        {
        public:
            static void SetWindow(uint16_t top, uint16_t left, uint16_t bot, uint16_t right) { slScrWindow1(left, top, right, bot); }
            static void SetWindowL(uint32_t address) { slScrLineWindow1((void *)address); }
        };
    };
}
