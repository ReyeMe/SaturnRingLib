#pragma once

#include "srl_tv.hpp"
#include "srl_cram.hpp"
#include "srl_math.hpp"
#include "srl_vector.hpp"
#include "srl_ascii.hpp"
#include "srl_debug.hpp"
#include "srl_cd.hpp"
#include "srl_tilemap.hpp"

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

        /** @brief used to specify the 4 VRAM banks that are available for VDP2 allocation
         */
        enum class VramBank : uint16_t
        {
            A0 = 0,
            A1 = 1,
            B0 = 2,
            B1 = 3,
        };

        /** @brief Manages VDP2 VRAM allocation
         */
        class VRAM
        {
        private:

            /** @brief VDP2 class needs access to some of the internal variables
             */
            friend class VDP2;

            /** @brief Bottom RAM bank zones
             */
            inline static uint8_t* bankBot[4] = { (uint8_t*)VDP2_VRAM_A0,(uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1 };
            
            /** @brief Top RAM bank zones
             */
            inline static uint8_t* bankTop[4] = { (uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1,(uint8_t*)(VDP2_VRAM_B1 + 0x18000) };
            
            /** @brief Currently allocated bottom RAM bank zones
             */
            inline static uint8_t* currentBot[4] = { (uint8_t*)VDP2_VRAM_A0,(uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1 };;
            
            /** @brief Currently allocated top RAM bank zones
             */
            inline static uint8_t* currentTop[4] = { (uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1,(uint8_t*)(VDP2_VRAM_B1 + 0x18000) };
            
            /** @brief Number of cycles available for each bank
             * @note Indicates number of cycles that are free in the VRAM bank for per pixel operations
             */
            inline static int8_t bankCycles[4] = {-1,-1,-1,2};//why will init to 0 not work?

        public:
            /** @brief Gets current amount of free VRAM in a bank
            * @param Bank the VRAM bank to get free space in
            * @return number of available bytes in bank
            */
            inline static uint32_t GetAvailable(VDP2::VramBank bank)
            {
                return (int)currentTop[(uint16_t)bank] - (int)currentBot[(uint16_t)bank];
            }

            /** @brief Linearly Allocates Vram in a bank
            * @param size Number of bytes to allocate
            * @param boundary Byte Boundary that the allocation should be alligned to (must be multiple of 32 for all VDP2 Data types)
            * @param Bank The VRAM bank to allocate in
            * @param Cycles (Optional) Number of Bank Cycles this data will require to access (0-8).
            * @return void* start of the Allocated region in VRAM (nullptr if allocation failed)
            * @note  Any VRAM skipped to maintain allignment to a boundary is rendered inaccessible to futher allocations until reset
            */
            inline static void* Allocate(uint32_t size, uint32_t boundary, VDP2::VramBank bank, uint8_t cycles = 0)
            {
                void* myAddress = nullptr;

                // Ensure allocation is aligned to requested VRAM boundary:
                uint32_t addrOffset = 0;

                if ((uint32_t)currentBot[(uint16_t)bank] & (boundary - 1))
                {
                    addrOffset = boundary - ((uint32_t)currentBot[(uint16_t)bank] & (boundary - 1));
                }

                if (VDP2::VRAM::GetAvailable(bank) > size + addrOffset)
                {
                    if ((bankCycles[(uint16_t)bank] + cycles) < 8)
                    {
                        myAddress = currentBot[(uint16_t)bank] + addrOffset;
                        currentBot[(uint16_t)bank] += size + addrOffset;
                        bankCycles[(uint16_t)bank] += cycles;
                    }
                }

                return myAddress;
            }

            /** @brief Automatically allocates cell data for specified screen
             * @param info Tile cell data description
             * @param screen The screen identifier
             * @return Pointer to the allocated memory
             */
            inline static void* AutoAllocateCel(Tilemap::TilemapInfo& info, uint16_t screen)
            {
                void* alloc;

                if (screen == scnRBG0) // Reserve all 8 cycles of a bank
                {
                   alloc=Allocate(info.CelByteSize, 32,VramBank::A0, 8);
                   if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::A1, 8);
                   if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::B0, 8);
                   if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::B1, 8);
                   if (alloc == nullptr) SRL::Debug::Assert("RBG Cel Allocation failed: insufficient VRAM");
                }
                else // Base cycle requirement on color type
                {
                    uint8_t ReqCycles = 0;

                    switch (info.ColorMode)
                    {
                    case CRAM::TextureColorMode::Paletted16:
                        ReqCycles = 1;
                        break;
                    case CRAM::TextureColorMode::Paletted256 :
                        ReqCycles = 2;
                        break;
                    case CRAM::TextureColorMode::RGB555 :
                        ReqCycles = 3;
                        break;
                    }

                    alloc = Allocate(info.CelByteSize,32, VramBank::B1, ReqCycles);
                    if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::B0, ReqCycles);
                    if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::A1, ReqCycles);
                    if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::A0, ReqCycles);
                    if (alloc == nullptr) SRL::Debug::Assert("NBG Cel Allocation failed: insufficient VRAM");
                }

                return alloc;
            }

            /** @brief Automatically allocates map data for specified screen
             * @param info Tile map data description
             * @param screen The screen identifier
             * @return Pointer to the allocated memory
             */
            inline static void* AutoAllocateMap(Tilemap::TilemapInfo& info, int16_t screen)
            {
                void* alloc;
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

                if (screen == scnRBG0) // Reserve all 8 cycles of a bank
                {
                    alloc = Allocate(sz, page_sz, VramBank::A0, 8);
                    if (alloc== nullptr) alloc = Allocate(sz,page_sz, VramBank::A1, 8);
                    if (alloc == nullptr) alloc = Allocate(sz,page_sz, VramBank::B0, 8);
                    if (alloc == nullptr) alloc = Allocate(sz, page_sz, VramBank::B1, 8);
                    if (alloc == nullptr) Debug::Assert("RBG Map Allocation failed: insufficient VRAM");
                }
                else // Reserve 1 cycle
                {
                    alloc = Allocate(sz, page_sz, VramBank::B1, 1);
                    if (!alloc) alloc = Allocate(sz, page_sz, VramBank::B0, 1);
                    if (!alloc) alloc = Allocate(sz, page_sz, VramBank::A1, 1);
                    if (!alloc) alloc = Allocate(sz, page_sz, VramBank::A0, 1);
                    if (alloc == nullptr) SRL::Debug::Assert("NBG Map Allocation failed: insufficient VRAM");
                }

                return alloc;
            }
        };

        /** @brief Bitfield recording all Currently enabled Scroll Screens*/
        inline static uint16_t ActiveScrolls = NBG0ON|NBG3ON;

        /** @Brief Bitfield recording all Scroll Screens with VDP2 Color Calculation enabled*/
        inline static uint16_t ColorCalcScrolls = NBG0ON|NBG3ON|SPRON;

        /** @brief Functionality available to all Scroll Screen interfaces
         */
        template<class ScreenType, int16_t Id, uint16_t On>
        class ScrollScreen {
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
            inline static void SetPlanesDefault(Tilemap::TilemapInfo & Info)
            {
                int mapX, mapY;
                mapX = mapY = 32;

                if (Info.CharSize == CHAR_SIZE_1x1)
                {
                    mapY <<= 1;
                    mapX <<= 1;
                }

                if (Info.PlaneSize == PL_SIZE_2x2)
                {
                   mapY <<= 1;
                   mapX <<= 1;
                }
                else if (Info.PlaneSize == PL_SIZE_2x1)
                {
                    mapX <<= 1;
                }

                if (Info.MapWidth > mapX)
                {
                    if (Info.MapHeight > mapY) VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 1, 2, 3);
                    else VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 1, 0, 1);

                }
                else
                {
                    if (Info.MapHeight>mapY) VDP2::ScrollScreen<ScreenType, Id, On>::SetMapLayout(0, 0, 1, 1);
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

            /** @brief VRAM Address to start of Cel data when displying Tilemap or Bitmap data when using bitmaps
             */
            inline static void* CelAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief  VRAM Address  of ScrollScreen's Map data when displaying Tilemap
             */
            inline static void* MapAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief CRAM Pallet for the ScrollScreen
             */
            inline static CRAM::Palette TilePalette = CRAM::Palette();

            /** @brief Tilemap config info for the ScrollScreen
             */
            static inline Tilemap::TilemapInfo Info = Tilemap::TilemapInfo();

            /** Size of manually allocated VRAM for Cel Data
             */
            inline static int CelAllocSize = -1;

            /** Size of manually allocated VRAM for Map Data
             */
            inline static int MapAllocSize = -1;

            /** @brief Loads Tilemap data to VRAM using ITilemap Interface and configures the Scroll Screen to use it
             *
             * @details If VRAM for this ScrollScreen's data has already been allocated by the user, SRL will attempt to load
             * to the allocated VRAM and raise assert if the Tilemap Data does not fit within the existing allocation.
             * If VRAM was not allocated SRL will attempt to auto allocate the Tilemap data and raise assert
             * if there is not enough VRAM available to allocate.
             *
             * @param tilemap The Tilemap to load
             * @note Manual VRAM allocation is for advanced use cases as is NOT verified for proper bank alignment
             * @note Does not turn Scroll Display on- once loaded use ScrollEnable() to display a Scroll Screen
             * @note It is recommended to perform loading/allocation for RBG0 before NBG0-3 screens if using it
             */
            inline static void LoadTilemap(SRL::Tilemap::ITilemap& tilemap)
            {
                SRL::Tilemap::TilemapInfo MyInfo = tilemap.GetInfo();
                ScreenType::Info = tilemap.GetInfo();

                if ((uint32_t)ScreenType::CelAddress < VDP2_VRAM_A0)
                {
                    ScreenType::CelAddress = VRAM::AutoAllocateCel(MyInfo, ScreenType::ScreenID);

                    if ((uint32_t)ScreenType::CelAddress < VDP2_VRAM_A0)
                    {
                        SRL::Debug::Assert("Tilemap Load Failed- CEL DATA exceeds existing VRAM allocation");
                        return;
                    }
                }
                else if (ScreenType::CelAllocSize < ScreenType::Info.CelByteSize)
                {
                    SRL::Debug::Assert("Tilemap Load Failed- CEL DATA exceeds existing VRAM allocation");
                    return;
                }

                if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0)
                {
                    ScreenType::MapAddress = VRAM::AutoAllocateMap(MyInfo, ScreenType::ScreenID);
                    if ((uint32_t)ScreenType::MapAddress < VDP2_VRAM_A0) return;

                }
                else if (ScreenType::MapAllocSize < (ScreenType::Info.MapWidth * ScreenType::Info.MapHeight)<<(2-ScreenType::Info.MapMode))
                {
                    SRL::Debug::Assert("Tilemap Load Failed- MAP DATA exceeds existing VRAM allocation");
                    return;
                }

                ScreenType::Init(ScreenType::Info);
                int colorID = 0;

                if (ScreenType::Info.ColorMode != SRL::CRAM::TextureColorMode::RGB555)
                {
                    if ((colorID = SRL::CRAM::GetFreeBank(ScreenType::Info.ColorMode)) < 0)
                    {
                        SRL::Debug::Assert("Tilemap Palette Load Failed- no CRAM Palettes available");
                        return;
                    }

                    SRL::CRAM::SetBankUsedState(colorID, ScreenType::Info.ColorMode, true);
                    ScreenType::TilePalette = SRL::CRAM::Palette(ScreenType::Info.ColorMode, colorID);
                    ScreenType::TilePalette.Load((Types::HighColor*)tilemap.GetPalData(), 256);
                }

                if (ScreenType::ScreenID != scnRBG0) VDP2::ScrollScreen<ScreenType, Id, On>::SetPlanesDefault(ScreenType::Info);
                
                VDP2::ScrollScreen<ScreenType, Id, On>::Cel2VRAM((uint8_t*)tilemap.GetCelData(), ScreenType::CelAddress, ScreenType::Info.CelByteSize);
                VDP2::ScrollScreen<ScreenType, Id, On>::Map2VRAM(
                    ScreenType::Info,
                    (uint16_t*)tilemap.GetMapData(),
                    ScreenType::MapAddress,
                    colorID,
                    VDP2::ScrollScreen<ScreenType, Id, On>::GetCelOffset(ScreenType::Info, ScreenType::CelAddress));
            }
            
            /** @brief Manually Sets VRAM area for Cel Data (Advanced Use Cases)
             *
             * @details This function manually sets an area in VRAM for a scrolls Cel Data to be loaded to. Unless the
             * Address is obtained from VDP2::VRAM::Allocate(), the VRAM allocator will be bypassed entirely.
             * No Checks are performed for proper data allignment or cycle conflicts. For advanced use cases only.
             * @code {.cpp}
             * //Manually Set NBG0 to store 16bpp Cel Data in an 0x8000 byte region allocated in VRAM bank A1:
             * SRL::VDP2::NBG0::SetCelAddress(SRL::VDP2::VRAM::Allocate(0x8000,32,SRL::VDP2::VramBank::A1, 3),0x8000);
             * @endcode
             * @param address the VRAM address of the allocation
             * @param size the size of the allocation
             * @return Echoes Address
             */
            inline static void* SetCelAddress(void* address, int size)
            {
                ScreenType::CelAddress = address;
                ScreenType::CelAllocSize = size;
                return address;
            }

            /** @brief Manually Sets VRAM area for Map Data (Advanced Use Cases)
             *
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
            
            /** @brief Registers Scroll in VDP2 cycle pattern to enable display of this Scroll Screen
             *
             * @details Asserts when registration of a scroll fails due to cycle pattern conflicts.
             * Possible causes:
             *     -Too many Scroll Layers with higher bit depth are storing data in the same VRAM bank
             *     -NBG0 or NBG1 have their minimum scale limit set too small(eg 1/2x or 1/4x scale)
             *     -NBG Data was stored in a bank reserved by RBG0
             * Potential conflicts are minimized when using Automatic Allocation and setting the
             * desired scale limits of NBG0/NBG1 beforehand.
             *
             */
            inline static void ScrollEnable()
            {
                VDP2::ActiveScrolls |= ScreenType::ScreenON;
                int check = slScrAutoDisp(VDP2::ActiveScrolls);
                if (check < 0) SRL::Debug::Assert("Scroll Registration Failed- Invalid cycle pattern");
            }

            /** @brief Removes the Scroll Screen from VDP2 cycle pattern register to disable display
             *
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

            /** @brief Gets the starting address in VRAM of Cel data allocated to this scroll
             *  @return Address of Cel data
             *  @note Returns starting address of bitmap when displaying Bitmap Scroll
             */
            inline static  void* GetCelAddress() { return ScreenType::CelAddress; }

            /** @brief returns the VRAM Address of the specified page number in a scroll's Page Table
            * or nullptr if the requested page is outside of allocated Map Bounds
            * @param index The index of desired page in the page table
            * @return VRAM Address of the page index
            */

            inline static  void* GetPageAddress(uint8_t index)
            {
                if ((uint32_t)ScreenType::MapAddress<VDP2_VRAM_A0) return nullptr;
                uint32_t offset = 2048*(uint32_t)index;
                if (ScreenType::Info.CharSize = CHAR_SIZE_1x1) offset <<= 2;
                if (ScreenType::Info.MapMode = PNB_2WORD) offset <<= 1;
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
                if (ScreenType::Info.PlaneSize == PL_SIZE_2x2)offset <<= 2;
                else if (ScreenType::Info.PlaneSize == PL_SIZE_2x1)offset <<= 1;
                return (void*)((uint32_t)ScreenType::MapAddress+offset);
            }

            /** @brief Manually set the Plane layout of a Scroll Screen
            *
            * This function manually sets the 4 planes comprising a NBG scroll screen
            * in cases when the default plane tiling pattern is not desired.
            * @param a,b,c,d the plane indicies of the 4 planes that will display in the normal scroll
            * @note RBG0 does not currently support multi plane patterns, so only plane [a] will be used
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
            *
            * This Function takes the opacity specified as a fixed point value and converts it to
            * one of the 32 color calculation ratios that a scroll screen can use (value is floored to the nearest ratio).
            * Color Calculation is turned on if Opacity < 1.0, or off if Opacity>= 1.0. Color calculation is unchanged if value is negative.
            * @note Color ratios only apply to highest priority pixels in frame
            * @note When ColorCalcMode is set to UseColorAddition, all scrolls with opacity < 1.0 will use color addition
            * in place of their specified ratios.
            * @param opacity Fxp decimal value between 0.0 and 1.0 representing pixel opacity for the scroll screen (Default 1.0)
            */
            inline static void SetOpacity(Types::Fxp opacity = 1.0)
            {
                if (opacity < 0.0) return;

                else if (opacity >= 1.0)
                {
                    VDP2::ColorCalcScrolls &= ~(ScreenType::ScreenON);
                    slColorCalcOn(VDP2::ColorCalcScrolls);
                }
                else
                {
                    slColRate(ScreenType::ScreenID, 31 - (uint16_t)(opacity.Value() >> 11));
                    VDP2::ColorCalcScrolls |= ScreenType::ScreenON;
                    slColorCalcOn(VDP2::ColorCalcScrolls);
                }
            }

            /** @brief Set the Display Priority of a Scroll Screen
            * @note Higher value layers display on top of lower layers
            * @note When 2 or more scroll screens are assigned the same layer, their priority resolves as
            * SPRITE>RBG0>NBG0>NBG1>NBG2>NBG3
            * @param pr The Priority Layer for the Scroll Screen
            */
            inline static void SetPriority(SRL::VDP2::Priority pr) { slPriority(ScreenType::ScreenID, (uint16_t)pr); }

            /** @brief Compute the offset that must be added to map data When Corresponding Cel Data does not start on a VRAM bank boundary
            * @param tile The data configuration of the tilemap
            * @param CelAddress Address of corresponding Cel Data in VRAM (must be a 32 byte boundary)
            * @return The Cel Offset to add to to map data
            */
            inline static uint32_t GetCelOffset(SRL::Tilemap::TilemapInfo& tile, void* celAddress)
            {
                uint32_t CelOffset;

                if (!ScreenType::Info.MapMode) CelOffset = ((uint32_t)celAddress - VDP2_VRAM_A0) >> 5; // 2WORD
                else if (ScreenType::Info.MapMode == 0x8000) // 1WORD MODE 0
                {
                    if (ScreenType::Info.CharSize) CelOffset = (((uint32_t)celAddress - VDP2_VRAM_A0) & 0x1FFFF) >> 7; // 2x2
                    else CelOffset = (((uint32_t)celAddress - VDP2_VRAM_A0) & 0x7FFF) >> 5; // 1x1
                }
                else // 1WORD MODE 1
                {
                    if (ScreenType::Info.CharSize) CelOffset = ((uint32_t)celAddress - VDP2_VRAM_A0) >> 7; // 2x2
                    else CelOffset = (((uint32_t)celAddress - VDP2_VRAM_A0) & 0x1FFFF) >> 5; // 1x1
                }
                
                return CelOffset;
            }

            /** @brief Gets the Pallet Bank That must be included in Map Data to Reference a Palette in CRAM
             * @param PaletteID (optional) specify to reference an arbitrary palette, otherwise uses Id from ScrollScreen::TilePalette
             * @return The Formatted Palette ID to be included in Map Indicies to reference a specified palette
             */
            inline static uint32_t GetPalOffset(int8_t paletteID = -1)
            {
                uint32_t PaletteOffset;

                if (paletteID < 0) paletteID = ScreenType::TilePalette.GetId();

                if (ScreenType::Info.MapMode) PaletteOffset = paletteID << 12;
                else PaletteOffset = paletteID << 24;

                return PaletteOffset;
            }
        private:

            /** @brief Copies Cel data to VRAM (adapted from SGL samples).
            * @param CelData Cel Data to copy.
            * @param CelAdr VRAM address to copy to.
            * @param Size Number of bytes to copy.
            */
            inline static void Cel2VRAM(uint8_t* celData, void* celAdr, uint32_t size)
            {
                uint8_t* VRAM;
                VRAM = (uint8_t*)celAdr;

                // Note: Consider DMA
                for (uint32_t i = 0; i < size; i++) *(VRAM++) = *(celData++);
            }

            /** @brief Copies map data to VRAM and applies necessary offsets (adapted from SGL Samples).
             * @param info Tilemap data config.
             * @param mapData Map data to copy to VRAM.
             * @param mapAdr VRAM address to copy map to .
             * @param mapoff offset added when Cel data does not start at bank boundary .
             * @param paloff Palette index in CRAM.
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

        /** @brief NBG0 interface
         */
        class NBG0 : public ScrollScreen<NBG0,scnNBG0,NBG0ON> {
        public:
            /** @brief VRAM address of line scroll table
             */
            inline static void* LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo & info)
            {
                slCharNbg0(info.SGLColorMode(), info.CharSize);
                slPageNbg0(NBG0::CelAddress, 0, info.MapMode);
                slPlaneNbg0(info.PlaneSize);
                slMapNbg0(MapAddress, MapAddress, MapAddress, MapAddress);
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
            static void SetPosition(Types::Vector2D& pos) { slScrPosNbg0(pos.X.Value(), pos.Y.Value()); }

            /** @brief Sets the Scale of NBG0 Screen display
             *  @param scl Fixed Point X an Y scaling values
             *  @note The minimum scale that can be displayed is determined by The Scale Limit, Scaling values
             *  lower than the minimum will be clamped to the minimum
             */
            static void SetScale(Types::Vector2D& scl) { slScrScaleNbg0(scl.X.Value(), scl.Y.Value()); }
        };

        /** @brief NBG1 interface
         */
        class NBG1 : public ScrollScreen<NBG1,scnNBG1,NBG1ON> {
        public:
            /** @brief VRAM address of line scroll table
             */
            inline static void* LineAddress = (void*)(VDP2_VRAM_A0-1);

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg1(info.SGLColorMode(), info.CharSize);
                slPageNbg1(NBG1::CelAddress, 0, info.MapMode);
                slPlaneNbg1(info.PlaneSize);
                slMapNbg1(MapAddress, MapAddress, MapAddress, MapAddress);
            }

            /** @brief Set the 2x2 grid of planes for the layer
             * @param a First plane
             * @param b Second plane
             * @param c Third plane
             * @param d Fourth plane
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { slMapNbg1(a, b, c, d); }

            /** @brief Sets the Screen Position of NBG Scroll Screen
             * @param pos Fixed Point X and Y Screen Position
             */
            static void SetPosition(Types::Vector2D& pos) { slScrPosNbg1(pos.X.Value(), pos.Y.Value()); }

            /** @brief Sets the Scale of NBG1 Screen display
             * @param scl Fixed Point X an Y scaling values
             * @note The minimum scale that can be displayed is determined by The Scale Limit, Scaling values
             * lower than the minimum will be clamped to the minimum
             */
            static void SetScale(Types::Vector2D& scl) { slScrScaleNbg1(scl.X.Value(), scl.Y.Value()); }
        };

        /** @brief  NBG2 interface
         */
        class NBG2 : public ScrollScreen<NBG2,scnNBG2,NBG2ON> {
        public:

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg2(info.SGLColorMode(), info.CharSize);
                slPageNbg2(NBG2::CelAddress, 0, info.MapMode);
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
             * @param ppPos Fixed Point X and Y  Screen Position
             */
            static void SetPosition(Types::Vector2D& pos) { slScrPosNbg2(pos.X.Value(), pos.Y.Value()); }
        };

        /** @brief NBG3 interface
         */
        class NBG3 : public ScrollScreen<NBG3,scnNBG3,NBG3ON>
        {
        public:

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slCharNbg3(info.SGLColorMode(), info.CharSize);
                slPageNbg3(NBG3::CelAddress, 0, info.MapMode);
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
            static void SetPosition(Types::Vector2D& pos) { slScrPosNbg3(pos.X.Value(), pos.Y.Value()); }
        };

        /** @brief setting for RBG0,1 rotation constraints
         * @note more axis require more VRAM resources
         */
        enum class RotationMode
        {
            /** @brief 2d rotation with only roll and zoom
             * @note No additional VRAM requirements
             */
            OneAxis,

            /** @brief 3d rotation with pitch and yaw, but no roll (modified per line)
             * @note Requires 0x2000-0x18000 bytes in arbitrary VRAM Bank (No cycles)
             */
            TwoAxis,

            /** @brief Full 3d rotation with pitch, yaw and roll (modified per pixel)
             * @note Requires 0x2000-0x18000 bytes in Reserved VRAM bank (8 cycles)
             */
            ThreeAxis,
        };

        /** @brief RBG0 interface
         */
        class RBG0 : public ScrollScreen<RBG0,scnRBG0,RBG0ON>
        {
        public:
            /** @brief VRAM Address of RBG0 Coefficient table
             */
            inline static void* KtableAddress = (void*)(VDP2_VRAM_A0 - 1);

            /** @brief Initializes the ScrollScreen's tilemap specifications
             * @param info Tile map info
             */
            inline static void Init(SRL::Tilemap::TilemapInfo& info)
            {
                slRparaInitSet((ROTSCROLL*)(VDP2_VRAM_B1 + 0x1ff00));
                slRparaMode(RA);
                slOverRA(0);
                slCharRbg0(info.SGLColorMode(), info.CharSize);
                slPageRbg0(CelAddress, 0, info.MapMode);
                slPlaneRA(info.PlaneSize);
                sl1MapRA(MapAddress);
                slPopMatrix();
            }

            /** @brief Select what type of rotation to use for the rotating scroll (Call before Loading RBG0)
             * @param mode The RotationMode to use for this scroll
             * @param vblank Chose to update VRAM at VBLANK to reduce amount of coefficient
             * data required for rotation of a plane (default = true)
             * @note when 2 or 3 axis rotation is Selected, VRAM will be allocated to store
             * necessary coefficient data. If Vblank is set false, all coefficients will be
             * statically stored in VRAM as a 0x18000 byte table. If Vblank is set true, only
             * the coefficients necessary for the current frame will be dynamically written
             * to VRAM at Vblank, reducing required VRAM footprint to 0x2000 bytes per
             * rotation parameter.
             */
            inline static void SetRotationMode(VDP2::RotationMode mode, bool vblank = true)
            {
                slRparaInitSet((ROTSCROLL*)(VDP2_VRAM_B1 + 0x1ff00));

                switch (mode)
                {
                case RotationMode::OneAxis:
                    slKtableRA(nullptr, K_OFF);
                    VDP2_RAMCTL &= 0xffcf; // Bypasses problem in slKtableRA: this never resets if K_DOT was previously specified
                    break;

                case RotationMode::TwoAxis:
                    if (!vblank)
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x18000, 0x20000, VDP2::VramBank::B0, 0);
                        slMakeKtable((void*)VDP2::RBG0::KtableAddress);
                        slKtableRA((void*)VDP2::RBG0::KtableAddress, K_FIX | K_LINE | K_2WORD | K_ON);
                    }
                    else
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x2000, 0x20000, VDP2::VramBank::B0, 0);
                        slKtableRA((void*)VDP2::RBG0::KtableAddress, K_LINE | K_2WORD | K_ON);
                    }

                    VDP2_RAMCTL &= 0xffcf;//Bypasses problem in slKtableRA: this never resets if K_DOT was previously specified
                    break;

                case RotationMode::ThreeAxis:
                    if (!vblank)
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x18000, 0x20000, VDP2::VramBank::B0, 8);
                        slMakeKtable((void*)VDP2::RBG0::KtableAddress);
                        slKtableRA((void*)VDP2::RBG0::KtableAddress, K_FIX | K_DOT | K_2WORD | K_ON);
                    }
                    else
                    {
                        VDP2::RBG0::KtableAddress = VDP2::VRAM::Allocate(0x2000, 0x20000, VDP2::VramBank::B0, 8);
                        slKtableRA((void*)VDP2::RBG0::KtableAddress, K_DOT | K_2WORD | K_ON);
                    }

                    break;
                }
            }

            /** @brief Writes the current matrix transform to RBG0RA Rotation parameters
             * to update its position and perspective
             */
            inline static void SetCurrentTransform()
            {
                slCurRpara(RA);
                slPushMatrix();
                {
                    slScrMatConv();
                    slScrMatSet();
                }
                slPopMatrix();
            }

            /** @brief Sets the plane of Tilemap Data to be displayed
             * @param a,b,c,d Page Table addresses of the planes to display
             * @note Multi Plane Maps are not supported yet for RBG0, only plane a is used
             */
            static void SetPlanes(void* a, void* b, void* c, void* d) { sl1MapRA(a); }
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

        /** @brief Interface to control VDP2 settings for the Sprite Layer (data from VDP1 framebuffer),
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

            /** @brief Set the opacities that sprites can select from in CC regesters
             * @details This Function takes the opacity specified as a fixed point value and converts it to
             * one of the 32 color calculation ratios that the system can use (value is floored to the nearest ratio).
             * It then sets the ratio in the specified sprite cc register (cc register 0 if not specified)
             * @note Color ratios only apply to highest priority pixels in frame
             * @note When Color Calc is ON, max opacity is ~(0.97). Fully opaque sprites must select with color condition
             * @note When ColorCalcMode is set to UseColorAddition, The Opacity levels are ignored and Color addition is applied
             * to all sprites whose priority meets the color condition (see SpriteLayer::SetColorCondition() for details)
             * @note RGB sprites always use the opacity set in CC register[bank0]
             * @note Does NOT turn color calc ON or OFF for the Sprite Layer(use SpriteLayer::ColorCalcON,OFF())
             * @note available cc registers vary by Palette code config- default allows all 8 banks
             * @param opacity Fxp decimal value between 0.0 and 1.0 representing pixel opacity of the cc register
             * @param bank (optional) which of the 8 CC registers to Set the opacity in (defaults to 0)
             */
            inline static void SetOpacity(Types::Fxp Opacity, VDP2::SpriteBank bank = VDP2::SpriteBank::Bank0)
            {
                if (Opacity < Types::Fxp(0.0) || Opacity>Types::Fxp(1.0)) return;
                else
                {
                    slColRate((int16_t) bank, 31 - (uint16_t)(Opacity.Value() >> 11));
                }
            }

            /** @brief Set the priority Layers That sprites can select from in PR registers
             * @details This function sets one of the 8 priority registers that a Palette sprite can reference (default bank0)
             * @note available registers vary by Palette code config- default can only pull from bank0 and bank1
             * @note RGB sprites always use the priority from bank0
             * @note During VDP2 init, priority bank0 and bank1 are initialized to Layer3 and Layer4 respectively
             * @note Changing these priorities will result in differing behavior for sprite color calculation
             * (See SpriteLayer::SetColorCondition() for more details
             * @param pr enum VDP2::Priority Layer
             * @param bank (optional) enum VDP2::SpriteBank designating which priority bank to write to
             */
            inline static void SetPriority(SRL::VDP2::Priority pr, VDP2::SpriteBank bank = VDP2::SpriteBank::Bank0)
            {
                slPriority((int16_t)bank,(uint16_t) pr);
            }

            /** @brief Set conditions under which VDP2 color calculation is performed on sprites based on their priority.
             * @details Sets up the condition that allows only select sprites to recieve Half Transparent
             *  color calculation with VDP2 layers. To make a sprite fully opaque, selectively turn color calculation off for it by
             *  assigning it to use a Priority Bank containing a priority layer that does not satisfy the Color Condition.
             *  The default VDP2 initialization uses ColorCondition::PriorityEquals Priority::Layer4,
             *  with SpriteBank0 set to Layer3 and SpriteBank1 set to Layer4. With this config RGB sprites recieve no
             *  color calculation, while Palette sprites only recieve color calculation when select priority from bank1
             * @param Condition The type of condition that VDP2 Color Calculation will follow
             * @param TestValue The Layer that a sprite's priority will be tested against in the condition
             */
            inline static void SetColorCondition(VDP2::ColorCondition Condition, SRL::VDP2::Priority TestValue)
            {
                slSpriteCCalcCond((uint16_t)Condition);
                slSpriteCCalcNum((uint16_t) TestValue);
            }
        };

        /** @brief Clear all VDP2 VRAM allocations and reset all Scroll Screen VRAM References, as well
         *  as all CRAM allocations associated with VDP2 Scroll Screens
         * @note, When Loading a new set Data and Configurations for Scroll Screens with auto allocation, Call this first
         *  to ensure old data is freed
         */
        inline static void ClearVRAM()
        {
            //reset ScrollScreen VRAM References
            VDP2::NBG0::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG0::CelAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG0::LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::NBG0::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::NBG0::TilePalette.GetId(), VDP2::NBG0::Info.ColorMode, false);
                VDP2::NBG0::TilePalette = SRL::CRAM::Palette();
            }

            VDP2::NBG1::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG1::CelAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG1::LineAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::NBG1::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::NBG1::TilePalette.GetId(), VDP2::NBG1::Info.ColorMode, false);
                VDP2::NBG1::TilePalette = SRL::CRAM::Palette();
            }

            VDP2::NBG2::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG2::CelAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::NBG2::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::NBG2::TilePalette.GetId(), VDP2::NBG2::Info.ColorMode, false);
                VDP2::NBG2::TilePalette = SRL::CRAM::Palette();
            }

            VDP2::NBG3::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::NBG3::CelAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::NBG3::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::NBG3::TilePalette.GetId(), VDP2::NBG3::Info.ColorMode, false);
                VDP2::NBG3::TilePalette = SRL::CRAM::Palette();
            }

            VDP2::RBG0::MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::RBG0::CelAddress = (void*)(VDP2_VRAM_A0 - 1);
            VDP2::RBG0::KtableAddress = (void*)(VDP2_VRAM_A0 - 1);

            if (VDP2::RBG0::TilePalette.GetData())
            {
                SRL::CRAM::SetBankUsedState(VDP2::RBG0::TilePalette.GetId(), VDP2::RBG0::Info.ColorMode, false);
                VDP2::RBG0::TilePalette = SRL::CRAM::Palette();
            }

            // Clear VRAM banks
            for (int i = 0; i < 4; ++i)
            {
                VDP2::VRAM::currentTop[i] = VDP2::VRAM::bankTop[i];
                VDP2::VRAM::currentBot[i] = VDP2::VRAM::bankBot[i];
                VDP2::VRAM::bankCycles[i] = -1;
            }

            VDP2::VRAM::bankCycles[3] = 1;
        }

        /** @brief Set the back color
         * @param color Color to use
         */
        inline static  void SetBackColor(const Types::HighColor& color)
        {
            slBack1ColSet((void*)(VDP2_VRAM_A1 + 0x1fffe), (uint16_t)color);
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

            slCharNbg0(COL_TYPE_256, CHAR_SIZE_1x1);
            VDP2::SetPrintPaletteColor(0, Types::HighColor::Colors::White);
            VDP2::SetPrintPaletteColor(1, Types::HighColor::Colors::Black);
            VDP2::SetPrintPaletteColor(2, Types::HighColor::Colors::Red);
            VDP2::SetPrintPaletteColor(3, Types::HighColor::Colors::Green);
            VDP2::SetPrintPaletteColor(4, Types::HighColor::Colors::Blue);
            VDP2::SetPrintPaletteColor(5, Types::HighColor::Colors::Yellow);
            VDP2::SetPrintPaletteColor(6, Types::HighColor::Colors::Magenta);

            // Set default color to white
            slCurColor(0);

            // Set back color
            VDP2::SetBackColor(backColor);

            // Initialize ascii print to use NBG3
            SRL::ASCII::LoadFontSG((uint8_t*)(VDP2_VRAM_B1 + 0x800), 0);
            SRL::ASCII::SetPalette(0);
            int myID = SRL::CRAM::GetFreeBank(SRL::CRAM::TextureColorMode::Paletted256);
            SRL::CRAM::SetBankUsedState(myID, SRL::CRAM::TextureColorMode::Paletted256, true);
            SRL::ASCII::SetFont(0);
            slCharNbg3(COL_TYPE_16, CHAR_SIZE_1x1);
            slPageNbg3((void*)(VDP2_VRAM_B1 + 0x1D000), 0, PNB_1WORD | CN_10BIT);
            slPlaneNbg3(PL_SIZE_1x1);
            slMapNbg3((void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000), (void*)(VDP2_VRAM_B1 + 0x1E000));
            SRL::VDP2::NBG3::SetPriority(SRL::VDP2::Priority::Layer7);
            SRL::VDP2::NBG3::ScrollEnable();
            SRL::VDP2::NBG0::ScrollDisable(); // We don't want this on by default anymore
            // Fix param table at top of VRAM outside range of allocator - now user calls to slPerspective will always update here:
            slRparaInitSet((ROTSCROLL*)(VDP2_VRAM_B1 + 0x1ff00));
        }

        /** @brief Used to Select behavior of VDP2 Half Transparent Color Calculation
         */
        enum class ColorCalcMode: uint16_t
        {
            /** @brief Blend colors by the ratio specified in Higher priority Pixels
             */
            UseColorRatiosTop = 0x000,

            /** @brief Blend colors by the ratio specified in Lower priority Pixels
             */
            UseColorRatios2nd = 0x200,

            /** @brief Use additive color blending instead of ratios
             */
            UseColorAddition  = 0x100,
        };

        /** @brief Sets VDP2 Half Transparent Color Calculation Mode (only one mode can be used at once)
         * @param Mode The VDP2 color calculation mode to use
         */
        inline static void SetColorCalcMode(VDP2::ColorCalcMode Mode = VDP2::ColorCalcMode::UseColorRatiosTop)
        {
            slColorCalc((uint16_t)Mode);
        }
    };
}
