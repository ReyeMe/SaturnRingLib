#pragma once

#include "srl_memory.hpp"
#include "srl_cram.hpp"
#include "srl_cd.hpp"
#include "srl_bitmap.hpp"
/*TODO think of a better way to convey the options available for tilemap data loading
in terms of what the they actually provide:
-Character Size
-Map Data Size
-Number Of pages per plane
-Number of planes
*/
namespace SRL::Tilemap
{
   
    /** @brief  All the info necessary to properly configure VDP2 tilemap data in VRAM
    */
    struct TilemapInfo
    {
        /** @brief Color Mode of the Tilemap (Paletted16, Paletted256, or RGB555)
         *  @note Paletted64 and Paletted128 color types will be converted to Paletted256 internally  
         */ 
        SRL::CRAM::TextureColorMode ColorMode = SRL::CRAM::TextureColorMode::RGB555;
        /** @brief Size and bit configuration of the map data (pattern name data) 
         */
        uint16_t MapMode = 0;
        /** @brief Size of the tiles that comprise the tilemap (charater pattern data)  
         */
        uint16_t CharSize = 0;
        /** @brief Size of the Planes that comprise a Scroll Screen 
         */
        uint16_t PlaneSize = 0;
        /** @brief Height of the Tilemap (in tile units)
         */ 
        uint16_t MapHeight = 0;
        /** @brief Width of the tilemap (in tile Units) 
         */
        uint16_t MapWidth = 0;
        /** @brief Number of bytes of image data comprising the Tile set 
         */
        int CelByteSize = 0;
        /** @brief Number of bytes of map data comprising the Tile map
         */
        int MapByteSize = 0;

        TilemapInfo() = default;

        /** @brief Initialize by manually entering all parameters 
         *  
         *  @param color Color Mode of the Tilemap (Paletted16, Paletted256, or RGB555).
         *  @param map Size and bit configuration of the map data (pattern name data).
         *  @param charSz Size of the tiles that comprise the tilemap (charater pattern data).  
         *  @param plane Size of the Planes that comprise a Scroll Screen. 
         *  @param height Height of tilemap (in tile units).
         *  @param width Width of the tilemap (in tile Units). 
         *  @param size Number of bytes of image data comprising the Tile set. 
         */
        TilemapInfo(SRL::CRAM::TextureColorMode color, uint16_t map, uint16_t charSz, uint16_t plane, uint16_t height, uint16_t width, int size)
        {
            this->ColorMode = color;
            this->MapMode = map;
            this->CharSize = charSz;
            this->PlaneSize = plane;
            this->MapHeight = height;
            this->MapWidth = width;
            this->CelByteSize = size;
        }
        /** @brief Gets the SGL macro corresponding to ColorMode
            @return SGL VDP2 color macro for the tile map 
        */
        uint16_t SGLColorMode()
        {
            static const uint8_t SglColor[] = { 0x30,0x30,0x00,0x10,0x10,0x10,0x10 };
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
        /** @brief Get Pal data(Palette)
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
        /** @brief Pointer to Cel data table
         */
        void* CelData;
        /** @brief Pointer to Map data table 
         */
        void* MapData;
        /** @brief Pointer to Palette data table
         */
        void* PalData;
        /** @brief Tilemap configuration
         */
        TilemapInfo Info;

        SGLc()
        {
            this->CelData = NULL;
            this->MapData = NULL;
            this->PalData = NULL;
            this->Info = TilemapInfo();
        }
        /** @brief Initialize with SGL data tables and Tilemap config
         *  @param myCelData SGL cel data table
         *  @param myMapData SGL map data table
         *  @param myPalData SGL palette data table
         *  @parm myInfo TilemapInfo Describing configuration of the SGL data
         */
        SGLc(void* myCelData, void* myMapData, void* myPalData, TilemapInfo myInfo)
        {
            this->CelData = myCelData;
            this->MapData = myMapData;
            this->PalData = myPalData;
            this->Info = myInfo;
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
    /** @brief Test implementation of ITilemap interface for Loading Cubecat binary format for cel/map/pal data.
     *
     *  @details This type assumes the raw data is pre-formatted to all VDP2 specifications other than
     *  VRAM and Palette offsets. Furthermore, the stored format is specified in the file header to build TilemapInfo.
     *  The file layout is as follows (all data is stored as big endian:
     *      32 byte header read as 8 uint32_t values [TypeID, Sizeof(CelData), Sizeof(Mapdata),CharSize,ColorMode,PlaneSize,MapMode,MapSize]
     *      32 or 512 bytes palette data(if a palette type is specified in ColorMode)
     *      Cel Data (size specified in header)
     *      Map Data (size specified in header)
     */
    struct SGLb : public ITilemap
    {
    private:
        uint8_t* CelData;
        uint8_t* MapData;
        uint8_t* PalData;
        TilemapInfo Info;

        /** @brief loads SGLb file from CD and Initializes Tilemap Info based on the file header
         *  
         *  @param file The SGLb format file to load from CD   
         */
        void LoadData(SRL::Cd::File* file)
        {
            uint8_t* stream = new uint8_t[file->Size.Bytes + 1];
            int32_t read = file->LoadBytes(0, file->Size.Bytes, stream);

            if (read == file->Size.Bytes)
            {
                // Load file
                uint8_t* imageData = (uint8_t*)(stream + 32);
                uint32_t* headerData = (uint32_t*)stream;
                uint32_t celSize = Info.CelByteSize = headerData[1];
                uint32_t mapSize = headerData[2];

                //decode color mode
                switch (headerData[4])
                {
                case 0x0: Info.ColorMode = SRL::CRAM::TextureColorMode::Paletted16;
                    break;
                case 0x10: Info.ColorMode = SRL::CRAM::TextureColorMode::Paletted256;
                    break;
                case 0x30: Info.ColorMode = SRL::CRAM::TextureColorMode::RGB555;
                    break;
                }

                this->Info.CharSize = (uint16_t)headerData[3];
                this->Info.PlaneSize = (uint16_t)headerData[5];
                this->Info.MapMode = (uint16_t)headerData[6];
                if (headerData[7] > 4)//if header formatted to allow differing height and width
                {
                    this->Info.MapWidth = (uint16_t)((headerData[7] & 0xffff) * 32);
                    
                    this->Info.MapHeight = (uint16_t)((headerData[7] >> 16) * 32);
                }
                else this->Info.MapHeight = this->Info.MapWidth = (uint16_t)(headerData[7] * 32);//old format for compatibility
              
                if (this->Info.CharSize == CHAR_SIZE_1x1)
                {
                    this->Info.MapHeight <<= 1;
                    this->Info.MapWidth <<= 1;
                }

                if (this->Info.PlaneSize == PL_SIZE_2x2)
                {
                    this->Info.MapHeight <<= 1;
                    this->Info.MapWidth <<= 1;
                }
                else if (this->Info.PlaneSize == PL_SIZE_2x1)
                {
                    this->Info.MapWidth <<= 1;
                }

                this->CelData = new uint8_t[celSize];
                this->MapData = new uint8_t[mapSize];

                //Load Palette Colors if they exist:
                if (this->Info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16)
                {
                    this->PalData = new uint8_t[32];
                    //loaded += 1;
                    for (int i = 0; i < 32; ++i) this->PalData[i] = *imageData++;
                }
                else if (this->Info.ColorMode == SRL::CRAM::TextureColorMode::Paletted256)
                {
                    this->PalData = new uint8_t[512];
                    // ++loaded;
                    for (int i = 0; i < 512; ++i) this->PalData[i] = *(imageData++);
                }

                //Load map data:
                for (uint32_t j = 0; j < mapSize; ++j)
                {
                    this->MapData[j] = *(imageData++);

                }

                //Load cel data            
                for (uint32_t k = 0; k < celSize; ++k)
                {
                    this->CelData[k] = *(imageData++);

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
        /** @brief Initialize with SGLb filename to load
         *    
         *   @details Allocates and loads specified SGLb file to work ram.
         *   @param filename Name of the SGLb file to load.
         */
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
    /** @brief Unsigned 8 bit coordinates for tiles withing a page of a tilemap
     *  @note Coordinates [0,0] represent top left corner of a page (same as Print coordinates)
     *  @note Coordinates are in tile units, which can be either 8x8 or 16x16 pixels.
     *  Max allowed coordinates in a page vary by Tile Size (64 for 8x8 tiles, 32 for 16x16)  
     */
    struct Co
    {
        /*@brief The tiles X coordinate in its page
         */

        uint8_t X;
        /*@brief The tiles Y coordintae in its page
         */

        uint8_t Y;
        Co()
        {
            X = Y = 0;
        }
        
        /*@brief Initialize with desired X and Y coordinates
         * xCoordinate X coordinate of tile in page
         * yCoordinate Y coordinate of tile in page
         */
        Co(uint8_t xCoordinate, uint8_t yCoordinate)
        {
            X = xCoordinate;
            Y = yCoordinate;
        }
    };

    /** @brief Interface to Convert Bitmap Image into Tilemap
     *  @note Maximum Size of bitmap to convert is 0x20000 bytes 
     *  (512x512 @ 4bpp, 512x256 @ 8bpp, or 512x128 @ 16bppp).  
     *  @note empty tiles in the source image are detected and removed from the tileset,
     *  but duplicate and mirrored tiles are not. 
     */
    struct Bmp2Tile : public ITilemap
    {
    private:
        uint8_t* CelData;
        uint16_t* MapData;
        uint16_t* PalData;
        TilemapInfo Info;
        /** @brief Number of tiles extracted into the tileset.
        */
        uint16_t numCels;
        /** @brief Number of pages of map data that will be alllocated for this tilemap.
        */
        uint8_t numPages;
        /** @brief Tracks if nonzero data was written to characters. 
         */
        uint8_t dataAccumulator = 0;

        /** @brief Extracts 8x8 pixel region of bitmap image and converts to a cell (8x8 tile)
         *  @param start Top left pixel of the tile in bitmap.
         *  @param Cel start of the cel to write data to in the tileset.
         *  @param imgWidth number of bytes in a horizontal line of the source image.
         *  @param dataWidth number of bytes in 8 pixel line of a cel.
         *  @return The start of the next cel in the tileset  
         */
        uint8_t* Bitmap2Cel(uint8_t* start,uint8_t* cel, uint32_t imgWidth, uint8_t dataWidth)
        {
            
            for(int i = 0;i<8; ++i)
            {
                for (int j = 0; j < dataWidth; ++j)
                {
                    this->dataAccumulator |= *start;
                    *cel++ = *start++;
                }
                start+=imgWidth-dataWidth;
            }

            return cel;
        }

        /** @brief Extracts 16x16 pixel region of bitmap image and converts to 2x2 cel character (16x16 tile)
         *  @param start Top left pixel of the tile in bitmap.
         *  @param Cel start of the cel to write data to in the tileset.
         *  @param imgWidth number of bytes in a horizontal line of the source image.
         *  @param dataWidth number of bytes in 8 pixel line of a cel.
         *  @return The start of the next character in the tileset  
         */
        uint8_t* Bitmap2Char2x2(uint8_t* start,uint8_t* cel, uint32_t imgWidth, uint8_t dataWidth)
        { 
            cel = Bitmap2Cel(start, cel, imgWidth, dataWidth);
            cel = Bitmap2Cel(start+  dataWidth, cel, imgWidth, dataWidth);
            cel = Bitmap2Cel(start+(imgWidth<<3), cel, imgWidth, dataWidth);
            cel = Bitmap2Cel(start+dataWidth+(imgWidth<<3), cel, imgWidth, dataWidth);

            return cel;
        }
        /** @brief Splits source bitmap into 8x8 or 16x16 pixel tiles while ignoring empty tiles, and builds a default
         *  @page of map data that reproduces the layout of the original image.
         *  @param config Desired data configuration of the resulting tilemap
         *  @param bmp the Soure bitmap image
         *  @param startingPage Which page in the tilemap to write the default map layout to
         */
        void ConvertBitmap(TilemapInfo config, SRL::Bitmap::IBitmap& bmp, int startingPage)
        {
             
            uint16_t sz = (config.CharSize) ? 0x7 : 0xf;//mask to detect valid multiples of 8 or 16
            uint8_t mapDataWidth = (config.MapMode) ? 1 : 2;// 1 word or 2 word map data
            int byteWidth;//number of bytes in 1 line of pixels from source image (Width* (0.5, 1, or 2))
            uint8_t byteCel;//number of bytes in 1 line of a cel (4,8, or 16)
            SRL::Bitmap::BitmapInfo info = bmp.GetInfo();
            uint16_t yCels = info.Height>>3;
            uint16_t xCels = info.Width >> 3;
            uint8_t* currentCel = this->CelData;
            uint8_t* currentData = bmp.GetData();
            uint16_t* currentmap = this->MapData;
            //set data width of a line of Cel data base on the color mode of the image data
            switch(config.ColorMode)
            {
               case CRAM::TextureColorMode::Paletted16:
                  
                   byteWidth = info.Width>>1;
                   byteCel = 4;
                   break;
               case CRAM::TextureColorMode::Paletted256:
                  
                   byteWidth = info.Width;
                   byteCel = 8;
                   break;
               default:
             
                   byteWidth = info.Width<<1;
                   byteCel = 16;
                   break;
            }
            //check to make sure image dimensions are compatible:
            if((byteWidth&(uint16_t)sz)||(info.Height&(uint16_t)sz)||info.Height>512||info.Width>512)
            {
                this->CelData = NULL;
                this->MapData = NULL;
                this->PalData = NULL;
                this->Info = TilemapInfo();
                SRL::Debug::Assert("Tileset conversion failed- Image Dimensions Not supported");
                return;
            }
 
            int pix = ((8 * (1 + config.CharSize)) * (byteCel * (1 + config.CharSize)));
            
            for(int i = 0; i <pix;++i) *currentCel++ = 0;

            this->numCels =  (byteCel >> 2);
           
            if(config.CharSize==CHAR_SIZE_1x1)// convert to 8x8 characters
            {
                for(int i = 0;i<yCels;++i)
                {
                    for(int j =0; j<xCels;++j)
                    {
                        this->dataAccumulator = 0;
                        currentCel = Bitmap2Cel(currentData,currentCel,byteWidth,byteCel);
                        currentData += byteCel;
                       
                        if(!config.MapMode)++currentmap;//add extra word of spacing to character pattern data
                        
                        if (this->dataAccumulator)//if this tile was not blank keep it 
                        {
                            *currentmap++ = this->numCels;
                            this->numCels += (byteCel >> 2);
                        }
                        else//set map to index 0 and do not retain the tile 
                        {
                            *currentmap++ = 0;
                            currentCel -= (byteCel << 3);
                        }
                    }
                    currentData += (byteWidth * 7);
                    currentmap += (64 - xCels);
                }
            }
            else//convert to 16x16 characters
            {
                for(int i = 0;i<(yCels>>1);++i)
                {
                    for(int j =0; j<(xCels>>1);++j)
                    {
                        this->dataAccumulator = 0;
                        currentCel = Bitmap2Char2x2(currentData,currentCel,byteWidth,byteCel);
                        currentData+=(byteCel<<1);
                        if(!config.MapMode)++currentmap;//add extra word of spacing to character pattern data
                        
                        if (this->dataAccumulator)//if this tile was not blank
                        {
                            *currentmap++ = this->numCels;
                            this->numCels += (byteCel >> 2);
                        }
                        else//set to index 0 and do not retain this tile 
                        {
                            *currentmap++ = 0;
                            currentCel -= (byteCel << 5);
                        }
                    }
                    currentData += (byteWidth * 15);//increment to next line of Characters in image
                    currentmap+=(32-(xCels>>1));//increment to next line of the page
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
            this->numCels = 0;
        }
        /** @brief Create a Tilemap out of an existing Bitmap
        * @param bmp The Bitmap Image to convert
        * @param pages the number of pages avaiable to the resulting tilemap
        * @note A page is a unit of map data representing the tiling af a 512x512 pixel region
        * (32x32 or 64x64 tiles depending on the base tile size). The resulting Tilemap always
        * Contains at least 1 page with the default mapping of all tiles extracted from the bitmap,
        * but more pages can be reserved here for further use
        */
        Bmp2Tile(SRL::Bitmap::IBitmap & bmp, uint8_t pages = 1)
        {
            if (pages < 1)pages = 1;
            this->numPages = pages;
            this->numCels = 0;
            this->Info.CharSize = CHAR_SIZE_2x2;
            this->Info.ColorMode = bmp.GetInfo().ColorMode;
            this->Info.MapMode = PNB_1WORD|CN_12BIT;
            this->Info.PlaneSize = PL_SIZE_1x1;
            this->Info.MapHeight = (this->Info.CharSize) ? (32*pages) : (64*pages);
            this->Info.MapWidth = (this->Info.CharSize) ? 32 : 64;
            uint8_t bitdepth = 1;
            this->Info.CelByteSize = bmp.GetInfo().Height * bmp.GetInfo().Width;
                
            Info.CelByteSize += (Info.CharSize) ? 256 : 64;//add enough space for empty tile at start of set
            if(this->Info.ColorMode == CRAM::TextureColorMode::Paletted16)this->Info.CelByteSize>>=1;
            else if(this->Info.ColorMode ==CRAM::TextureColorMode::RGB555)
            {
                this->Info.CelByteSize<<=1;
            }
            
            if(this->Info.CelByteSize>0x20000||(numPages<<10)>0x20000)
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
            
            for (int i = 0; i < numPages; ++i) ClearPage(i);
            
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
        *   @param sourcePage Index of the Tilemap Page to copy from
        *   @param topLeft Page Coordinates of Top left of region to Copy
        *   @param botRight Page Coordinates of Bottom Right of region Copy
        *   @param destPage Index of the Tilemap page being copied to
        *   @param DestTile Starting Coordinate to copy to in the destination page (the new Top left) 
        */
        void CopyMap(uint8_t sourcePage, Tilemap::Co topLeft, Tilemap::Co botRight, uint8_t DestPage, Co DestTile)
        {
            
            //check that requested page indecies exist in this map:
            if (!this->MapData|| sourcePage >= this->numPages || DestPage >= numPages) return;
            uint32_t pageOffset = (this->Info.CharSize) ? 1024 : 4096;
            uint16_t pageDim = (this->Info.CharSize) ? 31 : 63;
            //check that starting coordinates are in page bounds:
            if (topLeft.X > pageDim || topLeft.Y > pageDim || DestTile.X > pageDim || DestTile.Y > pageDim)
            {
                return;
            }
            //crop copied region to fit page bounds
            if (botRight.X > pageDim)botRight.X = pageDim;
            if (botRight.Y > pageDim)botRight.Y = pageDim;
            if (DestTile.X + (botRight.X - topLeft.X) > pageDim)
            {
                botRight.X -= (DestTile.X + (botRight.X - topLeft.X) - pageDim);
            }
            if (DestTile.Y + (botRight.Y - topLeft.Y) > pageDim)
            {
                botRight.Y -= (DestTile.Y + (botRight.Y - topLeft.Y) - pageDim);
            }
        
            //copy page data
            uint16_t* Src = this->MapData + (pageOffset * sourcePage) + topLeft.X + ((pageDim+1) * topLeft.Y);
            uint16_t* Dst = this->MapData + (pageOffset * DestPage) + DestTile.X +((pageDim + 1) * DestTile.Y);
            for (uint8_t i = topLeft.Y; i <= botRight.Y; ++i)
            {
                for (uint8_t j = topLeft.X; j <= botRight.X; ++j) *Dst++ = *Src++;
                Dst += (pageDim ) - (botRight.X-topLeft.X);
                Src += (pageDim ) - (botRight.X-topLeft.X);
            }

        }

        uint16_t* GetPageAddress(uint8_t page, uint8_t x, uint8_t y)
        {
            uint32_t pageDim = (this->Info.CharSize) ? 32 : 64;
            uint32_t offset = (this->Info.CharSize) ? 1024 : 4096;
            offset *= (uint32_t)page;
            offset += (uint32_t)x+ (pageDim * (uint32_t)y);
            return this->MapData + offset;
        }
        
        /** @brief clears all data of a page in the tilemap to zero
         *  @brief pageIndex Index of the page to clear in this Tilemap's Page Table   
         */
        void ClearPage(int pageIndex)
        {
            if (pageIndex >= this->numPages) return;
            int pageOffset = (this->Info.CharSize) ? 1024 : 4096;
            if (this->Info.MapMode == PNB_2WORD) pageOffset << 1;
            uint16_t* PageStart = this->MapData +(pageOffset * pageIndex);
            for (int i = 0; i < pageOffset; ++i) *(PageStart++) = 0;
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
        /** @brief manually apply a cel offset and Palette offset to all map data for cases when bypassing 
        *   built in VDP2 tilemap loading functions (such as when performing DMA transfers to VDP2 VRAM)
        *   @note Do not use in conjuction with VDP2::ScrollScreen::LoadTilemap, as these offsets are applied automatically.
        *   @param celOffset Tile address offset to apply (obtain with VDP2::ScrollScreen::GetCelOffset())
        *   @param palOffset CRAM palette index to apply (obtain with VDP2::ScrollScreen::GetPalOffset())
        */ 
        void ApplyVdp2Offsets(uint32_t celOffset,uint32_t palOffset = 0)
        {
            uint16_t* mapStart = this->MapData;
            for (int i = 0; i < this->Info.MapHeight * this->Info.MapWidth; ++i)
            {
                mapStart[i] += celOffset;
                mapStart[i] |= palOffset;
            }

        }
        
    };
    
}

