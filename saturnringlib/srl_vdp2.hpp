#pragma once

#include "srl_tv.hpp"
#include "srl_cram.hpp"
#include "srl_math.hpp"
#include "srl_vector.hpp"
#include "srl_ascii.hpp"
#include "srl_debug.hpp"
#include "srl_cd.hpp"
#include "srl_tilemap.hpp"

extern FIXED MsScreenDist;
extern uint16_t VDP2_RAMCTL;// SGL variable needed for dumb reasons
/* Concept for H-Blank interrupt manager:
//HBlank interval is ~10.9 Microseconds in NTSC -These events must be small and quick
//(probably < 250 SH2 cycles given the overhead of traversing the linked list).
//Could place strict limit on this by allowing only a fixed size set of register-value pairs to modify at each 
//HBlank instead of arbitrary function execution- but that narrows possible uses    
struct HBlank{
Public:    
    uint8_t Scanline;
    void* (*Event)(void* args); 
    HBlank* NextEvent;
    HBlank()
}
static struct HBlankManager
{
private:
    HBlank* Head;// Head actually fires at VBlank and must reset the OG state of all parameters modified by other HBlanks(or else face flicker)
    HBlank* Current;
    HBlank* Pending;
    void HBlankUpdate()// Wrapper called at each subsequent HBlank
    {
        //execute event of current interrupt
        Current->Event(Current->args);
        //Register Scanline and function for next Hblank interrupt
        Current = Current->NextEvent;
        TIM_T0_SET_CMP(Current->Scanline); 
    }
    void VBlankUpdate()//Wrapper Called At Vblank to safely add pending Hblank requests to list
    {
        //Pull Hblank Requests Off Pending and add to active Hblank interrupt list using insertion sort:
        if(Head) Head->Event(Head->args);
    }
public:
    void Init()
    {
        //Registering timer 0 handler for Hblanks.
        INT_SetScuFunc(INT_SCU_TIM0, HblankUpdate); // Register the Hblank interrupt handler to Timer 0
        TIM_T0_SET_CMP(0); // Start Hblank Interrupts at 
        TIM_T1_SET_MODE(TIM_TENB_ON); // Despite the macro's name, this is required to enable timer 0.
        TIM_T0_ENABLE(); // Enables the timer 0 interrupt, not the timer itself.
     }
     void AddHBlank(HBlank* H)
     {
        Pcur = Pending;
        if(Pending==nulptr) Pending  = H;
        else
        {       
            while(Pcur->NextEvent)Pcur = Pcur->NextEvent;
            Pcur->NextEvent = H;
            H->NextEvent = nulptr;
        }
     }

        

}
*/

namespace SRL
{    
    /** @brief VDP2 control
    */
    class VDP2
    {
    private:

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
        public:
            inline static uint8_t* BankBot[4] = { (uint8_t*)VDP2_VRAM_A0,(uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1 };
            inline static uint8_t* BankTop[4] = { (uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1,(uint8_t*)(VDP2_VRAM_B1 + 0x18000) };
            inline static uint8_t* CurrentBot[4] = { (uint8_t*)VDP2_VRAM_A0,(uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1 };;
            inline static uint8_t* CurrentTop[4] = { (uint8_t*)VDP2_VRAM_A1,(uint8_t*)VDP2_VRAM_B0,(uint8_t*)VDP2_VRAM_B1,(uint8_t*)(VDP2_VRAM_B1 + 0x18000) };
            inline static int8_t BankCycles[4] = {-1,-1,-1,-1};//why will init to 0 not work?
            
            /** @brief Gets current amount of free VRAM in a bank
            *   @param Bank the VRAM bank to get free space in
            *   @return number of available bytes in bank
            */
            inline static uint32_t GetAvailable(VDP2::VramBank Bank)
            {
                return (int)CurrentTop[(uint16_t)Bank] - (int)CurrentBot[(uint16_t)Bank];
            }
            /** @brief Linearly Allocates Vram in a bank
            *   @param size Number of bytes to allocate
            *   @param boundary Byte Boundary that the allocation should be alligned to (must be multiple of 32 for all VDP2 Data types) 
            *   @param Bank The VRAM bank to allocate in
            *   @param Cycles (Optional) Number of Bank Cycles this data will require to access (0-8). 
            *   @return void* start of the Allocated region in VRAM (nullptr if allocation failed)
            *   @note  Any VRAM skipped to maintain allignment to a boundary is rendered inaccessible to futher allocations until reset 
            */
            inline static void* Allocate(uint32_t size, uint32_t boundary, VDP2::VramBank Bank, uint8_t cycles = 0)
            {
                
                void* MyAddress = nullptr;
                //ensure allocation is alligned to requested VRAM boundary:
                uint32_t AddrOffset = 0;
                if ((uint32_t)CurrentBot[(uint16_t)Bank] & (boundary - 1))
                {
                    AddrOffset = boundary - ((uint32_t)CurrentBot[(uint16_t)Bank] & (boundary - 1));
                }
                if (GetAvailable(Bank) > size + AddrOffset)
                {
                    if ((BankCycles[(uint16_t)Bank] + cycles) < 8)
                    {
                        MyAddress = CurrentBot[(uint16_t)Bank] + AddrOffset;
                        CurrentBot[(uint16_t)Bank] += size + AddrOffset;
                        BankCycles[(uint16_t)Bank] += cycles;
                    }   
                }             
                return MyAddress;
            }

            inline static void* AutoAllocateCel(Tilemap::TilemapInfo& info, uint16_t ID)
            {
                void* alloc;
                if (ID == scnRBG0)//reserve all 8 cycles of a bank
                {
                   alloc=Allocate(info.CelByteSize, 32,VramBank::A0, 8);
                   if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::A1, 8);
                   if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::B0, 8);
                   if (alloc==nullptr) alloc = Allocate(info.CelByteSize,32, VramBank::B1, 8);
                   if (alloc == nullptr) SRL::Debug::Assert("RBG Cel Allocation failed: insufficient VRAM");
                }
                else// base cycle requirement on color type 
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
            inline static void* AutoAllocateMap(Tilemap::TilemapInfo& info, int16_t ID)
            {
                void* alloc;
                uint32_t page_sz = 0x800;
                uint32_t sz = (info.MapHeight * info.MapWidth)<<1;
                if (info.CharSize == CHAR_SIZE_1x1) page_sz <<= 2;
                if (info.MapMode == PNB_2WORD)
                {
                    sz <<= 1;
                    page_sz <<= 1;
                }
                if (info.PlaneSize == PL_SIZE_2x2) page_sz <<= 2;
                else if (info.PlaneSize == PL_SIZE_2x1) page_sz <<= 1;
                if (ID == scnRBG0)//reserve all 8 cycles of a bank
                {
                    alloc = Allocate(sz, page_sz, VramBank::A0, 8);
                    if (alloc== nullptr) alloc = Allocate(sz,page_sz, VramBank::A1, 8);
                    if (alloc == nullptr) alloc = Allocate(sz,page_sz, VramBank::B0, 8);
                    if (alloc == nullptr) alloc = Allocate(sz, page_sz, VramBank::B1, 8);
                    if (alloc == nullptr) Debug::Assert("RBG Map Allocation failed: insufficient VRAM");
                }
                else// reserve 1 cycle
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

        inline static uint16_t ActiveScrolls = NBG0ON|NBG3ON;
        inline static uint16_t ColorCalcScrolls = NBG0ON|NBG3ON|SPRON;
        
        /** @brief  Functionality available to all Scroll Screen interfaces
        */
        template<class ScreenType>
        class ScrollScreen {
        private:
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
                if (Info.MapWidth>mapX)
                {
                    if (Info.MapHeight> mapY)SetMapLayout(0, 1, 2, 3);
                    else SetMapLayout(0, 1, 0, 1);
                    
                }
                else
                {
                    if (Info.MapHeight>mapY)SetMapLayout(0, 0, 1, 1);
                    else SetMapLayout(0, 0, 0, 0);
                }

            }
        public:
            /** @brief Loads Tilemap data to VRAM using Itilemap Interface and configures the Scroll Screen to use it
            *   
            *   If VRAM for this ScrollScreen's data has aready been allocated by the user, SRL will attempt to load
            *   to the allocated VRAM and raise assert if the Tilemap Data does not fit within the existing allocation.
            *   If VRAM was not allocated SRL will attempt to auto allocate the Tilemap data and raise assert
            *   if there is not enough VRAM available to allocate. 
            *   
            *   @param Tilemap The Tilemap to load                        
            *   @note Manual VRAM allocation is for advanced use cases as it is NOT verified for proper bank allignment   
            *   @note Does not turn Scroll Display on- once loaded use ScrollEnable() to display a Scroll Screen
            *   @note It is recomended to perform loading/allocation for RBG0 before NBG screens if using it
            *   
            */ 
            inline static void LoadTilemap(SRL::Tilemap::ITilemap& Tilemap)
            {
                SRL::Tilemap::TilemapInfo MyInfo = Tilemap.GetInfo();
                ScreenType::Info = Tilemap.GetInfo();
                if ((uint32_t)ScreenType::CelAddress < VDP2_VRAM_A0)
                {
                    ScreenType::CelAddress = VRAM::AutoAllocateCel(MyInfo, ScreenType::ScreenID);
                    if ((uint32_t)ScreenType::CelAddress < VDP2_VRAM_A0) return;
                    
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
                else if (ScreenType::CelAllocSize < (ScreenType::Info.MapWidth * ScreenType::Info.MapHeight)<<(2-ScreenType::Info.MapMode))
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
                        SRL::Debug::Assert("Tilemap Palette Load Failed- no CRAM Pallets available");
                        return;
                    }
                    SRL::CRAM::SetBankUsedState(colorID, ScreenType::Info.ColorMode, true);
                    ScreenType::TilePalette = SRL::CRAM::Palette(ScreenType::Info.ColorMode, colorID);
                    ScreenType::TilePalette.Load((Types::HighColor*)Tilemap.GetPalData(), 256);
                }
                if (ScreenType::ScreenID != scnRBG0) SetPlanesDefault(ScreenType::Info);
                Cel2VRAM((uint8_t*)Tilemap.GetCelData(), ScreenType::CelAddress, ScreenType::Info.CelByteSize);
                Map2VRAM(ScreenType::Info, (uint16_t*)Tilemap.GetMapData(), ScreenType::MapAddress, colorID, GetCelOffset(ScreenType::Info, ScreenType::CelAddress));
            }
            /** @brief Manually Sets VRAM area for Cel Data (Advanced Use Cases)
            *   
            *   @details This function manually sets an area in VRAM for a scrolls Cel Data to be loaded to. Unless the 
            *   Address is obtained from VDP2::VRAM::Allocate(), the VRAM allocator will be bypassed entirely.
            *   No Checks are performed for proper data allignment or cycle conflicts. For advanced use cases only.
            *   @code {.cpp}
            *   //Manually Set NBG0 to store 16bpp Cel Data in an 0x8000 byte region allocated in VRAM bank A1:    
            *   SRL::VDP2::NBG0::SetCelAddress(SRL::VDP2::VRAM::Allocate(0x8000,32,SRL::VDP2::VramBank::A1, 3),0x8000);
            *  
            *   @param Address the VRAM address of the allocation
            *   @param Size the size of the allocation
            *   @return Echoes Address  
            */
            void* SetCelAddress(void* Address, int Size)
            {
                ScreenType::CelAddress = Address;
                ScreenType::CelAllocSize = Size;
                return Address;
            }
            /** @brief Manually Sets VRAM area for Map Data (Advanced Use Cases)
            *
            *   @details This function manually sets an area in VRAM for a scroll's Map Data to be loaded to. Unless the
            *   Address is obtained from VDP2::VRAM::Allocate() the VRAM allocator will be bypassed entirely.
            *   No Checks are performed for proper data allignment or cycle conflicts. For advanced use cases only.
            *   @code {.cpp}
            *   //Manually Set NBG0 to store Map Data in an 0x8000 byte region allocated in VRAM bank A1:    
            *   SRL::VDP2::NBG0::SetMapAddress(0x8000,SRL::VDP2::VRAM::Allocate(SRL::VDP2::VramBank::A1, 1024, 1),0x8000);
            *   
            *   @param Address the VRAM address of the allocation
            *   @param Size the size of the allocation
            *   @return Echoes Address
            */
            void* SetMapAddress(void* Address, int Size)
            {
                ScreenType::MapAddress = Address;
                ScreenType::MapAllocSize = Size;
                return Address;
            }
            /** @brief Registers Scroll in VDP2 cycle pattern to enable display of this Scroll Screen
            *   
            *   Asserts when registration of a scroll fails due to cycle pattern conflicts.            
            *   Possible causes:
            *       -Too many Scroll Layers with higher bit depth are storing data in the same VRAM bank      
            *       -NBG0 or NBG1 have their minimum zoom set too small(eg 1/2x or 1/4x scale)
            *       -NBG Data was stored in a bank reserved by RBG0
            *   Potential conflicts are minimized when using Automatic Allocation and setting the 
            *   desired scaling values of NBG0/NBG1 beforehand.
            *  
            */
            inline static void ScrollEnable()
            {
                VDP2::ActiveScrolls |= ScreenType::ScreenON;
                int check = slScrAutoDisp(VDP2::ActiveScrolls);
                if (check < 0) SRL::Debug::Assert("Scroll Registration Failed- Invalid cycle pattern");
            }
            /** @brief Removes this Scroll Screen from VDP2 cycle pattern register to disable display 
            *
            *   Asserts when registration of a scroll fails due to cycle pattern conflicts.
            *   Possible causes:             
            *   Assert should never occur here unless the user independently invoked SGL's slScrAutoDisp() with 
            *   specifications differing from those tracked by SRL
            */
            inline static void ScrollDisable()
            {
                VDP2::ActiveScrolls &= ~(ScreenType::ScreenON);
                int check = slScrAutoDisp(VDP2::ActiveScrolls);
                if (check < 0) SRL::Debug::Assert("Scroll Registration Failed- Invalid cycle pattern");
            }
            /** @brief Gets the starting address in VRAM of Map data allocated to this scroll
             *  @return Address of Map data
             */
            inline static  void* GetMapAddress() { return ScreenType::MapAddress; }
            /** @brief Gets the starting address in VRAM of Cel data allocated to this scroll
             *  @return Address of Cel data
             *  @note Returns starting address of bitmap when displaying Bitmap Scroll 
             */
            inline static  void* GetCelAddress() { return ScreenType::CelAddress; }
            /** @brief returns the VRAM Address of the specified page number in a scroll's Page Table
            *   or nulptr if the requested page is outside of allocated Map Bounds
            *   @param index The index of desired page in the page table 
            *   @return VRAM Address of the page index 
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
             *  @param index The index of desired plane in page table 
             *  @return VRAM Address of the plane index 
             *  @note PlaneAddress and PageAddress are equal when Plane Size is set to 1x1
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
            *   This function manually sets the 4 planes comprising a NBG scroll screen
            *   in cases when the defualt plane tiling pattern is not desired.
            *   @param a,b,c,d the plane indecies of the 4 planes that will display in the normal scroll
            *   @note RBG0 does not currently support multi plane patterns, so only plane [a] will be used
            */
            inline static void SetMapLayout(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
            {
                ScreenType::Planes(GetPlaneAddress(a), GetPlaneAddress(b), GetPlaneAddress(c), GetPlaneAddress(d));
            }

            /** @brief Set the opacity of a scroll screen
            * 
            *   This Function takes the opacity specified as a fixed point value and converts it to 
            *   one of the 32 color calculation ratios that a scroll screen can use (value is floored to the nearest ratio).
            *   Color Calculation is turned on if Opacity < 1.0, or off if Opacity>= 1.0. Color calculation is unchanged if value is negative.   
            *   @note Color ratios only apply to highest priority pixels in frame
            *   @note When ColorCalcMode is set to UseColorAddition, all scrolls with opacity < 1.0 will use color addition
            *   in place of their specified ratios.
            *   @param opacity Fxp decimal value between 0.0 and 1.0 representing pixel opacity for the scroll screen (Default 1.0)
            */
            inline static void SetOpacity(Types::Fxp Opacity = Types::Fxp(1.0))
            {
                if (Opacity < Types::Fxp(0.0)) return;
                else if (Opacity >= Types::Fxp(1.0))
                {
                    VDP2::ColorCalcScrolls &= ~(ScreenType::ScreenON);
                    slColorCalcOn(VDP2::ColorCalcScrolls);
                }
                else
                {
                    slColRate(ScreenType::ScreenID, 31 - (uint16_t)(Opacity.Value() >> 11));
                    VDP2::ColorCalcScrolls |= ScreenType::ScreenON;
                    slColorCalcOn(VDP2::ColorCalcScrolls);
                }
            }
            /** @brief Set the Dispay Priority of a Scroll Screen
            * @note Higher value layers display on top of lower layers
            * @note When 2 or more scroll screens are assigned the same layer, their priority resolves as
            * SPRITE>RBG0>NBG0>NBG1>NBG2>NBG3
            * @param pr The Priority Layer for the Scroll Screen        
            */
            inline static void SetPriority(SRL::VDP2::Priority  pr) { slPriority(ScreenType::ScreenID, (uint16_t)pr); }
            //static void SetPosition(Types::Vector2D pos) { ScreenType::ScrPosition(pos); }
        private:
            inline static void Cel2VRAM(uint8_t* Cel_Data, void* Cell_Adr, uint32_t size)
            {
                uint8_t* VRAM;
                VRAM = (uint8_t*)Cell_Adr;
                for (uint32_t i = 0; i < size; i++) *(VRAM++) = *(Cel_Data++);
            }
            inline static void Map2VRAM(SRL::Tilemap::TilemapInfo& Info, uint16_t* Map_Data, void* Map_Adr, uint8_t paloff, uint32_t mapoff)
            {
                uint16_t* VRAM = (uint16_t*)Map_Adr;
                uint32_t* VRAM32 = (uint32_t*)Map_Adr;
                uint32_t* Data32 = (uint32_t*)Map_Data;

                for (uint16_t i = 0; i < Info.MapHeight; i++)
                {
                    for (uint16_t j = 0; j < Info.MapWidth; j++)
                    {
                        if (Info.MapMode) *VRAM++ = ((*Map_Data++) + mapoff) | (paloff << 12);//1WORD data
                        else *VRAM32++ = ((*Data32++) + mapoff) | (paloff << 20);//2WORD data
                    }
                    //if(Info.CharSize) VRAM += (32 - Info.MapWidth);
                    //else VRAM+=(64-Info.MapWidth);
                }
            }
        public:
            /** @brief Compute the offset that must be added to map data When Corresponding Cel Data does not start on a VRAM bank boundary
            * @param tile The data configuration of the tilemap
            * @param CelAddress Address of corresponding Cel Data in VRAM (must be a 32 byte boundary)
            */
            inline static uint32_t GetCelOffset(SRL::Tilemap::TilemapInfo& tile, void* CelAddress)
            {
                uint32_t CelOffset;
                if (!tile.MapMode) CelOffset = ((uint32_t)CelAddress - VDP2_VRAM_A0) >> 5;//2WORD
                else if (tile.MapMode == 0x8000)//1WORD MODE 0
                {
                    if (tile.CharSize) CelOffset = (((uint32_t)CelAddress - VDP2_VRAM_A0) & 0x1FFFF) >> 7;// 2x2
                    else CelOffset = (((uint32_t)CelAddress - VDP2_VRAM_A0) & 0x7FFF) >> 5;//1x1
                }
                else//1WORD MODE 1
                {
                    if (tile.CharSize) CelOffset = ((uint32_t)CelAddress - VDP2_VRAM_A0) >> 7;//2x2
                    else CelOffset = (((uint32_t)CelAddress - VDP2_VRAM_A0) & 0x1FFFF) >> 5;//1x1
                }
                return CelOffset;
            }
        };

        /** @brief  NBG0 interface
        */
        class NBG0 : public ScrollScreen<NBG0> {
        public:
            inline static void* CelAddress = (void*)(VDP2_VRAM_A0-1);
            inline static void* MapAddress = (void*)(VDP2_VRAM_A0-1);
            inline static CRAM::Palette TilePalette = CRAM::Palette();
            inline static void* LineAddress = (void*)(VDP2_VRAM_A0-1);
            static inline Tilemap::TilemapInfo Info = Tilemap::TilemapInfo();
            static const int16_t ScreenID = scnNBG0;
            static const uint16_t ScreenON = NBG0ON;
            inline static int CelAllocSize = -1;
            inline static int MapAllocSize = -1;
            /* Specific Implentations Required for this Scroll Screen */
            static void Init(SRL::Tilemap::TilemapInfo & Info)
            {
                //int = Hplanes = Info.MapH eight;
                slCharNbg0(Info.SGLColorMode(), Info.CharSize);
                slPageNbg0(NBG0::CelAddress, 0, Info.MapMode);
                slPlaneNbg0(Info.PlaneSize);
                slMapNbg0(MapAddress, MapAddress, MapAddress, MapAddress);
            }
            static void Planes(void* a, void* b, void* c, void* d) { slMapNbg0(a, b, c, d); }
            static void SetPosition(Types::Vector2D pos) { slScrPosNbg0(pos.X.Value(), pos.Y.Value()); }
            static void SetScale(Types::Vector2D scl) { slScrScaleNbg0(scl.X.Value(), scl.Y.Value()); }

        };
        /** @brief  NBG1 interface
         */
        class NBG1 : public ScrollScreen<NBG1> {
        public:
            inline static void* CelAddress = (void*)(VDP2_VRAM_A0-1);
            inline static void* MapAddress = (void*)(VDP2_VRAM_A0-1);
            inline static CRAM::Palette TilePalette = CRAM::Palette();
            inline static void* LineAddress = (void*)(VDP2_VRAM_A0-1);
            static inline Tilemap::TilemapInfo Info = Tilemap::TilemapInfo();
            static const int16_t ScreenID = scnNBG1;
            static const uint16_t ScreenON = NBG1ON;
            inline static int CelAllocSize = -1;
            inline static int MapAllocSize = -1;
            /* Specific Implentations Required for this Scroll Screen */
            static void Init(SRL::Tilemap::TilemapInfo& Info)
            {
                slCharNbg1(Info.SGLColorMode(), Info.CharSize);
                slPageNbg1(NBG1::CelAddress, 0, Info.MapMode);
                slPlaneNbg1(Info.PlaneSize);
                slMapNbg1(MapAddress, MapAddress, MapAddress, MapAddress);
            }
            static void Planes(void* a, void* b, void* c, void* d) { slMapNbg1(a, b, c, d); }
            static void SetPosition(Types::Vector2D pos) { slScrPosNbg1(pos.X.Value(), pos.Y.Value()); }
            static void SetScale(Types::Vector2D scl) { slScrScaleNbg1(scl.X.Value(), scl.Y.Value()); }

        };
        /** @brief  NBG2 interface
        */
        class NBG2 : public ScrollScreen<NBG2> {
        public:
            inline static void* CelAddress = (void*)(VDP2_VRAM_A0-1);
            inline static void* MapAddress = (void*)(VDP2_VRAM_A0-1);
            inline static CRAM::Palette TilePalette = CRAM::Palette();
            static inline Tilemap::TilemapInfo Info = Tilemap::TilemapInfo();
            static const int16_t ScreenID = scnNBG2;
            static const uint16_t ScreenON = NBG2ON;
            inline static int CelAllocSize = -1;
            inline static int MapAllocSize = -1;
            /* Specific SGL Implementations Required for this Scroll Screen */
            static void Init(SRL::Tilemap::TilemapInfo& Info)
            {
                slCharNbg2(Info.SGLColorMode(), Info.CharSize);
                slPageNbg2(NBG2::CelAddress, 0, Info.MapMode);
                slPlaneNbg2(Info.PlaneSize);
                slMapNbg2(MapAddress, MapAddress, MapAddress, MapAddress);
            }
            static void Planes(void* a, void* b, void* c, void* d) { slMapNbg2(a, b, c, d); }
            static void SetPosition(Types::Vector2D pos) { slScrPosNbg2(pos.X.Value(), pos.Y.Value()); }
        };
        /** @brief  NBG3 interface
         */
        class NBG3 : public ScrollScreen<NBG3>
        {
        public:
            inline static void* CelAddress = (void*)(VDP2_VRAM_A0-1);
            inline static void* MapAddress = (void*)(VDP2_VRAM_A0-1);
            inline static CRAM::Palette TilePalette = CRAM::Palette();
            static inline Tilemap::TilemapInfo Info = Tilemap::TilemapInfo();
            static const int16_t ScreenID = scnNBG3;
            static const uint16_t ScreenON = NBG3ON;
            inline static int CelAllocSize = -1;
            inline static int MapAllocSize = -1;
            /* Specific SGL Implementations Required for this Scroll Screen */
            static void Init(SRL::Tilemap::TilemapInfo& Info)
            {
                slCharNbg3(Info.SGLColorMode(), Info.CharSize);
                slPageNbg3(NBG3::CelAddress, 0, Info.MapMode);
                slPlaneNbg3(Info.PlaneSize);
                slMapNbg3(MapAddress, MapAddress, MapAddress, MapAddress);
            }
            static void Planes(void* a, void* b, void* c, void* d) { slMapNbg3(a, b, c, d); }
            static void SetPosition(Types::Vector2D pos) { slScrPosNbg3(pos.X.Value(), pos.Y.Value()); }
        };
        /** @brief setting for RBG0,1 rotation constraints
        *   @note more axis require more VRAM resources
        */
        enum class RotationMode
        {
            /** @brief 2d rotation with only roll and zoom 
            *   @note No additional VRAM requirements
            */
            OneAxis,
            /* @brief 3d rotation with pitch and yaw, but no roll (modified per line)
            *  @note Requires 0x2000-0x18000 bytes in arbitrary VRAM Bank (No cylces)
            */
            TwoAxis,
            /** @brief Full 3d rotation with pitch, yaw and roll (modified per pixel) 
            *  @note Requires 0x2000- 0x18000 bytes in Reserved VRAM bank (8 cycles)
            */
            ThreeAxis,
        };
        /** @brief  RBG0 interface
        */
        class RBG0 : public ScrollScreen<RBG0>
        {
        public:
            inline static void* CelAddress = (void*)(VDP2_VRAM_A0 - 1);
            inline static void* MapAddress = (void*)(VDP2_VRAM_A0 - 1);
            inline static void* KtableAddress = (void*)(VDP2_VRAM_A0 - 1);
            /*info of the currently registered map*/
            inline static CRAM::Palette TilePalette = CRAM::Palette();
            static inline Tilemap::TilemapInfo Info = Tilemap::TilemapInfo();
            static inline SRL::CRAM::Palette CurrentPalette = SRL::CRAM::Palette();
            static const int16_t ScreenID = scnRBG0;
            static const uint16_t ScreenON = RBG0ON;
            inline static int CelAllocSize = -1;
            inline static int MapAllocSize = -1;
            /* Specific SGL Implementations Required for this Scroll Screen */
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
                return;
            }

            /** @brief Select what type of rotation to use for the rotating scroll (Call before Loading RBG0)
            *   @param Mode The RotationMode to use for this scroll
            *   @param Vblank chose to update VRAM at VBLANK to reduce amount of coefficient
            *   data required for rotation of a plane (default = true)
            *   @note when 2 or 3 axis rotation is Selected, VRAM will be allocated to store
            *   necessary coefficient data. If Vblank is set false, All coefficients will be
            *   statically stored in VRAM as a 0x18000 byte table. If Vblank is set true, only
            *   the coefficients necessary for the current frame will be dynamically written
            *   to VRAM at Vblank, reducing required VRAM footprint to 0x2000 bytes per
            *   rotation parameter.
            */
            inline static void SetRotationMode(VDP2::RotationMode Mode, bool Vblank = true)
            {
                slRparaInitSet((ROTSCROLL*)(VDP2_VRAM_B1 + 0x1ff00));
                switch (Mode)
                {
                case RotationMode::OneAxis:
                    slKtableRA(nullptr, K_OFF);
                    VDP2_RAMCTL &= 0xffcf;//Bypasses problem in slKtableRA: this never resets if K_DOT was previously specified   
                    break;
                case RotationMode::TwoAxis:
                    if (!Vblank)
                    {
                        KtableAddress = VDP2::VRAM::Allocate(0x18000, 0x20000, VDP2::VramBank::B0, 0);
                        slMakeKtable((void*)KtableAddress);
                        slKtableRA((void*)KtableAddress, K_FIX | K_LINE | K_2WORD | K_ON);

                    }
                    else
                    {
                        KtableAddress = VDP2::VRAM::Allocate(0x2000, 0x20000, VDP2::VramBank::B0, 0);
                        slKtableRA((void*)KtableAddress, K_LINE | K_2WORD | K_ON);

                    }
                    VDP2_RAMCTL &= 0xffcf;//Bypasses problem in slKtableRA: this never resets if K_DOT was previously specified 
                    break;
                case RotationMode::ThreeAxis:
                    if (!Vblank)
                    {
                        KtableAddress = VDP2::VRAM::Allocate(0x18000, 0x20000, VDP2::VramBank::B0, 8);
                        slMakeKtable((void*)KtableAddress);
                        slKtableRA((void*)KtableAddress, K_FIX | K_DOT | K_2WORD | K_ON);
                    }
                    else
                    {
                        KtableAddress = VDP2::VRAM::Allocate(0x2000, 0x20000, VDP2::VramBank::B0, 8);
                        slKtableRA((void*)KtableAddress, K_DOT | K_2WORD | K_ON);

                    }
                    break;
                }

            }

            /** @brief Write the current matrix transform to RBG0RA Rotation parameters
               to update its perspective on screen
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
            static void Planes(void* a, void* b, void* c, void* d) { sl1MapRA(a); }//does not work for multi plane maps yet

        };
        /**@brief Sprite Color Calculaiton Conditions (See SpriteLayer::SetColorCondition() for details)
        */
        enum class ColorCondition : uint16_t
        {
            /**@brief Perform color calculation only if priority is less than TestValue
        */
            PriorityLessThan = CC_pr_CN,
            /**@brief Perform color calculation only if priority is equal to  TestValue
        */
            PriorityEquals = CC_PR_CN,
            /**@brief Perform color calculation only if priority is greater than TestValue
        */
            PriorityGreaterThan = CC_PR_cn,
        };

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
       
        class SpriteLayer
        {
        public:
            inline static void ColorCalcON()
            {
                VDP2::ColorCalcScrolls |= SPRON;
                slColorCalcOn(VDP2::ColorCalcScrolls);
            }
            inline static void ColorCalcOFF()
            {
                VDP2::ColorCalcScrolls &= (~SPRON);
                slColorCalcOn(VDP2::ColorCalcScrolls);
            }
           /** @brief Set the opacities that sprites can select from in CC regesters
           *
           *   This Function takes the opacity specified as a fixed point value and converts it to
           *   one of the 32 color calculation ratios that the system can use (value is floored to the nearest ratio).
           *   It then sets the ratio in the specified sprite cc register (cc register 0 if not specified)
           *   @note Color ratios only apply to highest priority pixels in frame
           *   @note When Color Calc is ON, max opacity is ~(0.97). Fully opaque sprites must select with color condition 
           *   @note When ColorCalcMode is set to UseColorAddition, The Opacity levels are ignored and Color addition is applied
           *   to all sprites whose priority meets the color condition (see SpriteLayer::SetColorCondition() for details)
           *   @note RGB sprites always use the opacity set in CC register[bank0]
           *   @note Does NOT turn color calc ON or OFF for the Sprite Layer(use SpriteLayer::ColorCalcON,OFF())  
           *   @note available cc registers vary by Pallet code config- default allows all 8 banks
           * @param opacity Fxp decimal value between 0.0 and 1.0 representing pixel opacity of the cc register
           * @param bank (optional) which of the 8 CC registers to Set the opacity in (defaults to 0)
           */
            inline static void SetOpacity(Types::Fxp Opacity, VDP2::SpriteBank bank = VDP2::SpriteBank::Bank0)
            {
                if (Opacity < Types::Fxp(0.0)||Opacity>Types::Fxp(1.0)) return;
                else 
                {
                    slColRate((int16_t) bank, 31 - (uint16_t)(Opacity.Value() >> 11));
                }
            }
            /**@brief Set the priority Layers That sprites can select from in PR registers
            *   
            *   This function sets one of the 8 priority registers that a Pallet sprite can reference (default bank0)
            *   @note available registers vary by Pallet code config- default can only pull from bank0 and bank1 
            *   @note RGB sprites always use the priority from bank0
            *   @note During VDP2 init, priority bank0 and bank1 are initialized to Layer3 and Layer4 respectively
            *   @note Changing these priorities will result in differing behavior for sprite color calculation 
            *   (See SpriteLayer::SetColorCondition() for more details
            *   @param pr enum VDP2::Priority Layer 
            *   @param bank (optional) enum VDP2::SpriteBank designating which priority bank to write to 
            */
            inline static void SetPriority(SRL::VDP2::Priority pr, VDP2::SpriteBank bank = VDP2::SpriteBank::Bank0)
            {
                slPriority((int16_t)bank,(uint16_t) pr);
            }

            /**@brief Set conditions under which VDP2 color calculation is performed on sprites based on their priority.
           
            *  Sets up a condition that allows only select sprites to recieve Half Transparent color calculation with VDP2 layers.
            *  To make a sprite fully opaque, selectively turn color calculation off for it by
            *  assigning it to use a Priority Bank containing a priority layer that does not satisfy the Color Condition.
            *  The default VDP2 initialization uses ColorCondition::PriorityEquals Priority::Layer4,
            *  with SpriteBank0 set to Layer3 and SpriteBank1 set to Layer4. With this config RGB sprites recieve no
            *  color calculation, while Pallet sprites only recieve color calculation when select priority from bank1
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
        *   as all CRAM allocations associated with VDP2 Scroll Screens
        *  @note, When Loading a new set Data and Configurations for Scroll Screens with auto allocation, Call this first 
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
            for (int i = 0; i < 4; ++i)// clear VRAM banks
            {
                VDP2::VRAM::CurrentTop[i] = VDP2::VRAM::BankTop[i];
                VDP2::VRAM::CurrentBot[i] = VDP2::VRAM::BankBot[i];
                VDP2::VRAM::BankCycles[i] = -1;
            }
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
        inline static  void SetPrintPaletteColor(const uint16_t index, const Types::HighColor& color)
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
            SRL::VDP2::NBG0::ScrollDisable();//we don't want this on by default anymore
            //Fix param table at top of VRAM outside range of allocator - now user calls to slPerspective will always update here:
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
        * 
        */
        inline static void SetColorCalcMode(VDP2::ColorCalcMode Mode = VDP2::ColorCalcMode::UseColorRatiosTop)
        {
            slColorCalc((uint16_t)Mode);
        }    
    };
}
