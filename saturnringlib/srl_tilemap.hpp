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
    *This type assumes the raw data is pre-formatted to all VDP2 specifications other than
    *VRAM and Pallet offsets. Furthermore, the stored format is specified in the header.
    *The file layout is:
    *    32 byte header- read as 8 uint32_t values [TypeID, Sizeof(CelData), Sizeof(Mapdata),CharSize,ColorMode,PlaneSize,MapMode,MapSize]
    *    32 or 512 bytes palette data(if pallet type is specified in header)
    *    Cel Data (size specified in header)
    *    Map Data (size specified in header)
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

    /** @brief Interface to Convert Bitmap Image into Tilemap
    *  @note Maximum Size of bitmap to convert at once is 512x512 (1 page of tile data)
    *  if desired image is larger, split into 512x512 chunks and append a new page for each chunk 
    */

    /*struct BitmapTile : public ITilemap
    {
    private:
        uint8_t* CelData;
        uint16_t* MapData;
        uint8_t* PalData;
        TilemapInfo Info;
        uint16_t numcels;
        uint8_t* Bitmap2Cel(uint8_t* start,uint8_t* Cel, uint32_t ImgWidth, uint8 DataWidth)
        {
            for(int i = 0;i<8 ++i)
            {
                for(int j = 0; j<DataWidth;++j) Cel++ = start++;
                start+=ImgWidth;
            }
            return Cel;
        }
        uint_t8* Bitmap2Char2x2(uint8_t* start,uint8_t* Cel, uint32_t ImgWidth, uint8 DataWidth)
        {
            Cel = Bitmap2Cel(start, Cel, ImgWidth, DataWidth);
            Cel = Bitmap2Cel(start+DataWidth, Cel, ImgWidth, DataWidth);
            Cel = Bitmap2Cel(start+(ImgWidth<<3), Cel, ImgWidth, DataWidth);
            Cel = Bitmap2Cel(start+DataWidth+(ImgWidth<<3), Cel, ImgWidth, DataWidth);
            return Cel;
        }
        ConvertBitmap(TilemapInfo config, SRL::Bitmap::IBitmap & bmp, int Page)
        {
            uint16_t Xcels;
            uint16_t Ycels;
            uint8_t celsize = 8*(1+config.CelSize);
            uint8_t MapDataWidth = (config.MapMode) ? 1 : 2;
            int ByteWidth;
            BitmapInfo info = bmp.GetInfo();
            Ycels = info.height>>3;
            uint8_t ByteCel;
            uint8_t* CurrentCel = this.CelData;
            uint8_t* CurrentData = bmp.GetData();
            switch(config.ColorMode)
            {
               case CRAM::TextureColorMode::Paletted16:
                   Xcels = info.width>>2;
                   ByteWidth = info.width>>1;
                   ByteCel = 4
               case CRAM::TxtureColorMode::Paletted256:
                   Xcels = info.width>>3;
                   ByteWidth = info.width;
                   ByteCel = 8;
               default:
                   Xcels = info.width>>4;
                   ByteWidth = info.width<<1;
                   ByteCel = 8;
            }
             //check to make sure image dimensions are compatible:
            if((ByteWidth&(uint16_t)sz)||(info.Height&(uint16_t)sz)||info.Height>512||info.Width>512)
            {
                this->CelData = NULL;
                this->MapData = NULL;
                this->PalData = NULL;
                this->Info = TilemapInfo();
                debug::assert("Tileset conversion failed- Image Dimensions Not supported");
                return;
            }
            //(1+!config.MapType)
            //uint16_t PageWidth = (config.CharSize ? 32: 64);
            //add empty tile at beginning of tileset
            if(!numcels)
            {
                for(int i = 0; i < ((8*(1+config.CelSize))*(ByteCel*(1+config.CelSize)));++i) *CelData++ = 0;
                numcels+=1;
            }

            //we want to build map at the same time as cels....
            //what info do we need for map data? colormode...offset 
            uint8_t* char_start = this->CelData+(numcels*((8*(1+config.CelSize))*(ByteCel*(1+config.CelSize))));
            if(sz==Tilesize::Tile8x8) convert to 8x8 characters
            {
                for(int i = 0;i<Ycels;++i)
                {
                    for(int j =0; j<Xcels;++j)
                    {
                        Bitmap2Cel(CurrentData,CurrentCel,ByteWidth,ByteCel);
                        CurrentData+=ByteCel;
                        if(!config.MapMode)++CurrentMap;//add extra word of spacing to character pattern data
                        CurrentMap++ = numcels;//write address offset of character from start of CelData
                        numcels+=ByteCel>>2;
                    }
                    CurrentData+=(ByteWidth<<3)-ByteCel;
                    CurrentMap+=MapDataWidth*(64-Xcels);
                }
            }
            else//convert to 16x16 characters
            {
                for(int i = 0;i<Ycels;++i)
                {
                    for(int j =0; j<Xcels;++j)
                    {
                        Bitmap2Char2x2(CurrentData,CurrentCel,ByteWidth,ByteCel);
                        CurrentData+=(ByteCel<<1);
                        if(!config.MapMode)++CurrentMap;//add extra word of spacing to character pattern data
                        CurrentMap++ = numcels;//write address offset of character from start of CelData
                        numcels+=ByteCel>>2;
                    }
                    CurrentData+=(ByteWidth<<4)-(ByteCel<<1);//increment to next line of Characters in image
                    CurrentMap+=MapDataWidth*(32-Xcels);//increment to next line of the page
                }
            }
        }



    public:
        BitmapTile()
        {
            this->CelData = NULL;
            this->MapData = NULL;
            this->PalData = NULL;
            this->Info = TilemapInfo();
            this->numcels = 0;
        }
        //@brief Create a Tilemap out of an existing Bitmap 
        //@param bmp The Bitmap Image to convert
        //@param Pages the number of pages avaiable to the resulting tilemap
        //@note A page is a unit of map data representing the tiling af a 512x512 pixel region
        //(32x32 or 64x64 tiles depending on the tile size) The resulting Tilemap always
        //Contains at least 1 page with the default mapping of all tiles extracted from the bitmap,
        //but more pages can be reserved here for further use
        BitmapTile(SRL::Bitmap::IBitmap & bmp, uint8_t Pages)
        {
            this->numcels = 0;
            this->Info.CharSize = CHAR_SIZE_2x2;
            this->Info.ColorMode = bmp.GetInfo().ColorMode;
            this->Info.MapMode = PNB_1WORD|CN_12BIT;
            this->Info.PlaneSize = PL_SIZE_1x1;
            this->Info.MapHeight = 32;
            this->Info.MapWidth = 32;
            uint8_t bitdepth = 1;
            this->Info.CelByteSize =  bmp.GetInfo().Height*bmp.GetInfo.Width
            if(NumPages<1)NumPages = 1;            
            if(this->Info.ColorMode == CRAM::TextureColorMode::Paletted16)this->Info.CelByteSize>>=1;
            else if(this->Info.ColorMode ==CRAM::TextureColorMode::RGB555)this->Info.CelByteSize<<=1;
            if(this->Info.CelByteSize>0x20000||(NumPages<<10)>0x20000)
            {
                Debug::Assert("Size Unsupported- Maximum Tileset/Map data Size is 0x20000 bytes each")
                BitmapTile();
                return;
            }
            this->CelData = new uint8_t[this->Info.CelByteSize];
            this->MapData = new uint8_t[NumPages<<9];
            ConvertBitmap(this->Info, bmp
        }

        ~BitmapTile()
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
        //int AppendMap();
        //int CopyMap();
        //int CopyCel();
        //int ClearPage();

    };
    */
}

