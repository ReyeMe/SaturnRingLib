#pragma once

#include "srl_memory.hpp"
#include "srl_cram.hpp"
#include "srl_cd.hpp"
#include "srl_bitmap.hpp"
/*TODO think of a better way to convey the options available for tilemap data loading
in terms of what the options actually provide:
-Character Size
-Map Data Size
-Number Of pages per plane
-Number of planes
*/
namespace SRL::Tilemap
{
   
    /** @brief  All the info necessary to properly configure the tilemap data in VRAM
    */
    struct TilemapInfo
    {
        SRL::CRAM::TextureColorMode ColorMode = SRL::CRAM::TextureColorMode::RGB555;
        uint16_t MapMode = 0;
        uint16_t CharSize = 0;
        uint16_t PlaneSize = 0;
        uint16_t MapHeight = 0;
        uint16_t MapWidth = 0;
        int CelByteSize = 0;
        int MapByteSize = 0;
        TilemapInfo() = default;

        TilemapInfo(SRL::CRAM::TextureColorMode Color, uint16_t Map, uint16_t CharSz, uint16_t Plane, uint16_t Height, uint16_t Width, int size)
        {
            this->ColorMode = Color;
            this->MapMode = Map;
            this->CharSize = CharSz;
            this->PlaneSize = Plane;
            this->MapHeight = Height;
            this->MapWidth = Width;
            this->CelByteSize = size;
        }
        /** @brief Gets the SGL macro corresponding to ColorMode
            @return SGL VDP2 color macro 
        */
        uint16_t SGLColorMode()
        {
            static const uint8_t SglColor[] = { 0x30,0x30,0x00,0x10,0x10,0x10,0x10 };//bad dum use a switch statement instead  
            return (uint16_t)SglColor[(uint16_t)this->ColorMode];
        }
    };
    /* Virtual interface for tilemaps*/
    struct ITilemap {
        /** @brief Get Cel data(Tileset)
         * @return Pointer to Cel data
         */
        virtual void* GetCelData()
        {
            return nullptr;
        }
        /** @brief Get Map data(Tilemap)
        * @return Pointer to Map data
        */
        virtual void* GetMapData()
        {
            return nullptr;
        }
        /** @brief Get Pal data(Pallet)
        * @return Pointer to Pal data
        */
        virtual void* GetPalData()
        {
            return nullptr;
        }
        /** @brief Get Tilemap Info
        * @return Tilemap Info
        */
        virtual TilemapInfo GetInfo()
        {
            return TilemapInfo();
        }
    };
    /** @brief Test implementation of ITilemap interface for Hardcoded SGL cel/map/pal data
    */
    struct SGLc : public ITilemap
    {
        //private:
        void* CelData;
        void* MapData;
        void* PalData;
        TilemapInfo Info;

        //public:
        SGLc()
        {
            this->CelData = NULL;
            this->MapData = NULL;
            this->PalData = NULL;
            this->Info = TilemapInfo();
        }
        SGLc(void* MyCelData, void* MyMapData, void* MyPalData, TilemapInfo MyInfo)
        {
            this->CelData = MyCelData;
            this->MapData = MyMapData;
            this->PalData = MyPalData;
            this->Info = MyInfo;
        }
        void* GetCelData() override
        {
            return this->CelData;
        }
        void* GetMapData() override
        {
            return this->MapData;
        }
        void* GetPalData() override
        {
            return this->PalData;
        }
        TilemapInfo GetInfo() override
        {
            return this->Info;
        }
    };
    /* @brief Test implementation of ITilemap interface for Loading Cubecat binary format for cel/map/pal data.
    *
    *   This type assumes the raw data is pre-formatted to all VDP2 specifications other than
    *   VRAM and Pallet offsets. Furthermore, the stored format is specified in the header.
    *   The file layout is:
    *   32 byte header- read as 8 uint32_t values [TypeID, Sizeof(CelData), Sizeof(Mapdata),CharSize,ColorMode,PlaneSize,MapMode,MapSize]
    *   32 or 512 bytes palette data(if pallet type is specified in ColorMode)
    *   Cel Data (size specified in header)
    *   Map Data (size specified in header)
    */
    struct SGLb : public ITilemap
    {
    private:
        uint8_t* CelData;
        uint8_t* MapData;
        uint8_t* PalData;
        TilemapInfo Info;
        void LoadData(SRL::Cd::File* file)
        {
            uint8_t* stream = new uint8_t[file->Size.Bytes + 1];
            int32_t read = file->LoadBytes(0, file->Size.Bytes, stream);

            if (read == file->Size.Bytes)
            {
                // Load file
                uint8_t* img_data = (uint8_t*)(stream + 32);
                uint32_t* header_data = (uint32_t*)stream;
                uint32_t CelSize = Info.CelByteSize = header_data[1];
                uint32_t MapSize = header_data[2];
                switch (header_data[4])//decode color mode
                {
                case 0x0: Info.ColorMode = SRL::CRAM::TextureColorMode::Paletted16;
                    break;
                case 0x10: Info.ColorMode = SRL::CRAM::TextureColorMode::Paletted256;
                    break;
                case 0x30: Info.ColorMode = SRL::CRAM::TextureColorMode::RGB555;
                    break;
                }
                Info.CharSize = (uint16_t)header_data[3];
                Info.PlaneSize = (uint16_t)header_data[5];
                Info.MapMode = (uint16_t)header_data[6];
                if (header_data[7] > 4)//if header formatted to allow different height and width
                {
                    Info.MapWidth = (uint16_t)((header_data[7] & 0xffff) * 32);
                    
                    Info.MapHeight = (uint16_t)((header_data[7] >> 16) * 32);
                }
                else Info.MapHeight = Info.MapWidth = (uint16_t)(header_data[7] * 32);//old format
              
                if (Info.CharSize == CHAR_SIZE_1x1)
                {
                    Info.MapHeight <<= 1;
                    Info.MapWidth <<= 1;
                }
                if (Info.PlaneSize == PL_SIZE_2x2)
                {
                    Info.MapHeight <<= 1;
                    Info.MapWidth <<= 1;
                }
                else if (Info.PlaneSize == PL_SIZE_2x1)
                {
                    Info.MapWidth <<= 1;
                }
                this->CelData = new uint8_t[CelSize];
                this->MapData = new uint8_t[MapSize];

                //Load Pallet Colors if they exist:
                if (Info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16)
                {
                    this->PalData = new uint8_t[32];
                    //loaded += 1;
                    for (int i = 0; i < 32; ++i) this->PalData[i] = *img_data++;
                }
                else if (Info.ColorMode == SRL::CRAM::TextureColorMode::Paletted256)
                {
                    this->PalData = new uint8_t[512];
                    // ++loaded;
                    for (int i = 0; i < 512; ++i) this->PalData[i] = *(img_data++);
                }
                //Load map data:
                for (uint32_t j = 0; j < MapSize; ++j)
                {
                    this->MapData[j] = *(img_data++);

                }
                //Load cel data            
                for (uint32_t k = 0; k < CelSize; ++k)
                {
                    this->CelData[k] = *(img_data++);

                }
            }
            delete stream;
        }
    public:
        SGLb()
        {
            this->CelData = NULL;
            this->MapData = NULL;
            this->PalData = NULL;
            this->Info = TilemapInfo();
        }
        SGLb(const char* filename)
        {
            SRL::Cd::File file = SRL::Cd::File(filename);
            if (file.Exists())
            {
                this->LoadData(&file);
            }
            else
            {
                SRL::Debug::Assert("File '%s' is missing!", filename);
            }
        }
        ~SGLb()
        {
            if (this->CelData) delete this->CelData;
            if (this->MapData) delete this->MapData;
            if (this->PalData) delete this->PalData;
        }
        void* GetCelData() override
        {
            return this->CelData;
        }
        void* GetMapData() override
        {
            return this->MapData;
        }
        void* GetPalData() override
        {
            return this->PalData;
        }
        TilemapInfo GetInfo() override
        {
            return this->Info;
        }
    };
    /** @brief  unsigned 8 bit coordinates for page data within a tilemap
     *  @note Coordinates [0,0] represent top left corner of a page (same as Print coordinates)
     *  @note Max allowed coordinates vary by Tile Size (64 for 8x8 tiles, 32 for 16x16)  
     */
    struct Co
    {
        uint8_t X;
        uint8_t Y;
        Co()
        {
            X = Y = 0;
        }
        Co(uint8_t Xcoordinate, uint8_t Ycoordinate)
        {
            X = Xcoordinate;
            Y = Ycoordinate;
        }
    };
    /** @brief Interface to Convert Bitmap Image into Tilemap
    *  @note Maximum Size of bitmap to convert is 0x20000 bytes (512x512 @ 4bpp)
    *  
    */
    
    struct Bmp2Tile : public ITilemap
    {
    private:
        uint8_t* CelData;
        uint16_t* MapData;
        uint16_t* PalData;
        TilemapInfo Info;
        uint16_t numcels;
        uint8_t numpages;
        uint8_t DataAccumulator = 0;//tracks if nonzero data was written to characters
        uint8_t* Bitmap2Cel(uint8_t* start,uint8_t* Cel, uint32_t ImgWidth, uint8_t DataWidth)
        {
            
            for(int i = 0;i<8; ++i)
            {
                for (int j = 0; j < DataWidth; ++j)
                {
                    DataAccumulator |= *start;
                    *Cel++ = *start++;
                }
                start+=ImgWidth-DataWidth;
            }
            //if(!accumulator)Cel = 0;
            return Cel;
        }
        uint8_t* Bitmap2Char2x2(uint8_t* start,uint8_t* Cel, uint32_t ImgWidth, uint8_t DataWidth)
        { 
            uint8_t* StartCel = Cel;
            Cel = Bitmap2Cel(start, Cel, ImgWidth, DataWidth);
            Cel = Bitmap2Cel(start+  DataWidth, Cel, ImgWidth, DataWidth);
            Cel = Bitmap2Cel(start+(ImgWidth<<3), Cel, ImgWidth, DataWidth);
            Cel = Bitmap2Cel(start+DataWidth+(ImgWidth<<3), Cel, ImgWidth, DataWidth);

            return Cel;
        }
        void ConvertBitmap(TilemapInfo config, SRL::Bitmap::IBitmap& bmp, int StartingPage)
        {
            uint16_t Xcels;
            uint16_t Ycels;
            uint16_t sz = (config.CharSize) ? 0x7 : 0xf;//mask to detect valid multiples of 8 or 16
            //uint8_t celsize = 8*(1+config.CelSize);
            uint8_t MapDataWidth = (config.MapMode) ? 1 : 2;// 1 word or 2 word
           
            int ByteWidth;//number of bytes in 1 line of pixels from source image (Width* (0.5, 1, or 2))
            SRL::Bitmap::BitmapInfo info = bmp.GetInfo();
            Ycels = info.Height>>3;
            Xcels = info.Width >> 3;
            uint8_t ByteCel;//number of bytes in 1 line of a cel (4,8, or 16)
            uint8_t* CurrentCel = this->CelData;
            uint8_t* CurrentData = bmp.GetData();
            uint16_t* CurrentMap = this->MapData;

            switch(config.ColorMode)
            {
               case CRAM::TextureColorMode::Paletted16:
                  
                   ByteWidth = info.Width>>1;
                   ByteCel = 4;
                   break;
               case CRAM::TextureColorMode::Paletted256:
                  
                   ByteWidth = info.Width;
                   ByteCel = 8;
                   break;
               default:
             
                   ByteWidth = info.Width<<1;
                   ByteCel = 16;
                   break;
            }
             //check to make sure image dimensions are compatible:
            if((ByteWidth&(uint16_t)sz)||(info.Height&(uint16_t)sz)||info.Height>512||info.Width>512)
            {
                this->CelData = NULL;
                this->MapData = NULL;
                this->PalData = NULL;
                this->Info = TilemapInfo();
                SRL::Debug::Assert("Tileset conversion failed- Image Dimensions Not supported");
                return;
            }
 
            int pix = ((8 * (1 + config.CharSize)) * (ByteCel * (1 + config.CharSize)));
          
            for(int i = 0; i <pix;++i) *CurrentCel++ = 0;
            numcels =  (ByteCel >> 2);
           
            if(config.CharSize==CHAR_SIZE_1x1)// convert to 8x8 characters
            {
                for(int i = 0;i<Ycels;++i)
                {
                    for(int j =0; j<Xcels;++j)
                    {
                        DataAccumulator = 0;
                        CurrentCel = Bitmap2Cel(CurrentData,CurrentCel,ByteWidth,ByteCel);
                        CurrentData += ByteCel;
                       
                        //if(!config.MapMode)++CurrentMap;//add extra word of spacing to character pattern data
                        if (DataAccumulator)//if this tile was not blank 
                        {
                            *CurrentMap++ = numcels;
                            numcels += (ByteCel >> 2);
                        }
                        else//set to index 0 and do not retain the tile 
                        {
                            *CurrentMap++ = 0;
                            CurrentCel -= (ByteCel << 3);
                        }
                    }
                    CurrentData += (ByteWidth * 7);// -ByteCel;
                    CurrentMap += (64 - Xcels);
                }
            }
            else//convert to 16x16 characters
            {
                for(int i = 0;i<(Ycels>>1);++i)
                {
                    for(int j =0; j<(Xcels>>1);++j)
                    {
                        DataAccumulator = 0;
                        CurrentCel = Bitmap2Char2x2(CurrentData,CurrentCel,ByteWidth,ByteCel);
                        CurrentData+=(ByteCel<<1);
                        //if(!config.MapMode)++CurrentMap;//add extra word of spacing to character pattern data
                        if (DataAccumulator)//if this tile was not blank
                        {
                            *CurrentMap++ = numcels;
                            numcels += (ByteCel >> 2);
                        }
                        else//set to index 0 and do not retain this tile 
                        {
                            *CurrentMap++ = 0;
                            CurrentCel -= (ByteCel << 4);
                        }
                    }
                    CurrentData += (ByteWidth * 15);//increment to next line of Characters in image
                    CurrentMap+=(32-(Xcels>>1));//increment to next line of the page
                }
            }
        }
    public:
        Bmp2Tile()
        {
            this->CelData = NULL;
            this->MapData = NULL;
            this->PalData = NULL;
            this->Info = TilemapInfo();
            this->numcels = 0;
        }
        /** @brief Create a Tilemap out of an existing Bitmap
        * @param bmp The Bitmap Image to convert
        * @param Pages the number of pages avaiable to the resulting tilemap
        * @note A page is a unit of map data representing the tiling af a 512x512 pixel region
        * (32x32 or 64x64 tiles depending on the base tile size) The resulting Tilemap always
        * Contains at least 1 page with the default mapping of all tiles extracted from the bitmap,
        * but more pages can be reserved here for further use
        */
        Bmp2Tile(SRL::Bitmap::IBitmap & bmp, uint8_t NumPages)
        {
            if (NumPages < 1)NumPages = 1;
            this->numpages = NumPages;
            this->numcels = 0;
            this->Info.CharSize = CHAR_SIZE_2x2;
            this->Info.ColorMode = bmp.GetInfo().ColorMode;
            this->Info.MapMode = PNB_1WORD|CN_12BIT;
            this->Info.PlaneSize = PL_SIZE_1x1;
            this->Info.MapHeight = (this->Info.CharSize) ? (32*NumPages) : (64*NumPages);
            this->Info.MapWidth = (this->Info.CharSize) ? 32 : 64;
            uint8_t bitdepth = 1;
            this->Info.CelByteSize = bmp.GetInfo().Height * bmp.GetInfo().Width;
                
            Info.CelByteSize += (Info.CharSize) ? 256 : 64;//add enough space for empty tile at start of set
            if(this->Info.ColorMode == CRAM::TextureColorMode::Paletted16)this->Info.CelByteSize>>=1;
            else if(this->Info.ColorMode ==CRAM::TextureColorMode::RGB555)
            {
                this->Info.CelByteSize<<=1;
            }
            if(this->Info.CelByteSize>0x20000||(NumPages<<10)>0x20000)
            {
                SRL::Debug::Assert("Size Unsupported- Maximum Tileset/Map data Size is 0x20000 bytes each");
                Bmp2Tile();
                return;
            }
            
            this->CelData = new uint8_t[this->Info.CelByteSize];
            this->MapData = new uint16_t[this->Info.MapWidth * this->Info.MapHeight];
            
            if (bmp.GetInfo().Palette != nullptr)
            {
                uint16_t sz = (this->Info.ColorMode == CRAM::TextureColorMode::Paletted16) ? 16 : 256;
                this->PalData = new uint16_t[sz];
                uint16_t* srcpal = (uint16_t*)bmp.GetInfo().Palette->Colors;
                for (int i = 0; i < sz; ++i)this->PalData[i] = *srcpal++;
            }
            else {
                this->PalData = nullptr;
               
            }
            for (int i = 0; i < NumPages; ++i) ClearPage(i);
            ConvertBitmap(this->Info, bmp, 0);
                       
        }

        ~Bmp2Tile()
        {
            if (this->CelData) delete this->CelData;
            if (this->MapData) delete this->MapData;
            if (this->PalData) delete this->PalData;
        }
        void* GetCelData() override
        {
            return (void*)this->CelData;
        }
        void* GetMapData() override
        {
            return (void*)this->MapData;
        }
        void* GetPalData() override
        {
            return (void*)this->PalData;
        }
        TilemapInfo GetInfo() override
        {
            return this->Info;
        }

        /** @brief copies a rectangular selection of Tilemap data between  2 pages in the tilemap
        *   @param SourcePage Index of the Tilemap Page to copy from
        *   @param TopLeft Page Coordinates of Top left of region to Copy
        *   @param BotRight Page Coordinates of Bottom Right of region Copy
        *   @param DestPage Index of the Tilemap page being copied to
        *   @param DestTile Starting Coordinate to copy to in the destination page (the new Top left) 
        */
        void CopyMap(uint8_t SourcePage, Co TopLeft, Co BotRight, uint8_t DestPage, Co DestTile)
        {
            
            //check that requested page indecies exist in this map:
            if (!this->MapData|| SourcePage >= this->numpages || DestPage >= numpages) return;
            uint32_t PageOffset = (this->Info.CharSize) ? 1024 : 4096;
            uint16_t PageDim = (this->Info.CharSize) ? 31 : 63;
            //check that starting coordinates are in page bounds:
            if (TopLeft.X > PageDim || TopLeft.Y > PageDim || DestTile.X > PageDim || DestTile.Y > PageDim)
            {
                return;
            }
            //crop copied region to fit page bounds
            if (BotRight.X > PageDim)BotRight.X = PageDim;
            if (BotRight.Y > PageDim)BotRight.Y = PageDim;
            if (DestTile.X + (BotRight.X - TopLeft.X) > PageDim)
            {
                BotRight.X -= (DestTile.X + (BotRight.X - TopLeft.X) - PageDim);
            }
            if (DestTile.Y + (BotRight.Y - TopLeft.Y) > PageDim)
            {
                BotRight.Y -= (DestTile.Y + (BotRight.Y - TopLeft.Y) - PageDim);
            }
        
            //copy page data
            uint16_t* Src = this->MapData + (PageOffset * SourcePage) + TopLeft.X + ((PageDim+1) * TopLeft.Y);
            uint16_t* Dst = this->MapData + (PageOffset * DestPage) + DestTile.X +((PageDim + 1) * DestTile.Y);
            for (uint8_t i = TopLeft.Y; i <= BotRight.Y; ++i)
            {
                for (uint8_t j = TopLeft.X; j <= BotRight.X; ++j) *Dst++ = *Src++;
                Dst += (PageDim ) - (BotRight.X-TopLeft.X);
                Src += (PageDim ) - (BotRight.X-TopLeft.X);
            }

        }

        uint16_t* GetPageAddress(uint8_t page, uint8_t x, uint8_t y)
        {
            uint32_t pagedim = (this->Info.CharSize) ? 32 : 64;
            uint32_t offset = (this->Info.CharSize) ? 1024 : 4096;
            offset *= (uint32_t)page;
            offset += (uint32_t)x+ (pagedim * (uint32_t)y);
            return this->MapData + offset;
        }
        
        void ClearPage(int PageIndex)
        {
            if (PageIndex >= this->numpages) return;
            int PageOffset = (this->Info.CharSize) ? 1024 : 4096;
            if (this->Info.MapMode == PNB_2WORD) PageOffset << 1;
            uint16_t* PageStart = this->MapData +(PageOffset * PageIndex);
            for (int i = 0; i < PageOffset; ++i) *(PageStart++) = 0;
        }
        /** @brief Frees The Work Ram associated with Cel Data in cases where it is no longer required.
        */
        void FreeCelData()
        {
            if (this->CelData) delete this->CelData;
            this->CelData = NULL;
        }
        /** @brief Frees The Work Ram associated with Map Data in cases where it is no longer required.
        */
        void FreeMapData()
        {
            if (this->MapData) delete this->MapData;
            this->MapData = NULL;
        }
        /** @brief Frees The Work Ram associated with Pal Data in cases where it is no longer required.
        */
        void FreePalData()
        {
            if (this->PalData) delete this->PalData;
            this->PalData = NULL;
        }
        /** @brief manually apply a cel offset to all map data for cases when bypassing 
        *   normal VDP2 tilemap loading functions (such as using DMA)
        *   @param Offset the offset that must be applied to map data (ScrollLayer::GetCelOffset())
        */ 
        void ApplyCelOffset(uint32_t Offset)
        {
            uint16_t* MapStart = this->MapData;
            for (int i = 0; i < this->Info.MapHeight * this->Info.MapWidth; ++i)
            {
                MapStart[i] += Offset;
            }

        }
        
    };
    
}

