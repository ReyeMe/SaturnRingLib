#pragma once

#include "srl_tilemap.hpp"
#include "srl_cd.hpp"
#include "srl_bitmap.hpp"

namespace SRL::Tilemap::Interfaces
{

    /** @brief Interface to Convert Bitmap Image into Tilemap
     * @note Maximum Size of bitmap to convert is 0x20000 bytes (512x512 @ 4bpp, 512x256 @ 8bpp, or 512x128 @ 16bppp).
     * @note empty tiles in the source image are detected and removed from the tileset, but duplicate and mirrored tiles are not.
     */
    struct Bmp2Tile : public ITilemap
    {
    private:
        /** @brief Pointer to Cell data table
         */
        uint8_t* cellData;

        /** @brief Pointer to Map data table
         */
        uint16_t* mapData;

        /** @brief Pointer to Palette data table
         */
        uint16_t* palData;

        /** @brief Tilemap configuration
         */
        TilemapInfo info;

        /** @brief Number of tiles extracted into the tileset.
        */
        uint16_t numCells;

        /** @brief Number of pages of map data that will be allocated for this tilemap.
        */
        uint8_t numPages;

        /** @brief Tracks if nonzero data was written to characters.
         */
        uint8_t dataAccumulator = 0;

        /** @brief Extracts 8x8 pixel region of bitmap image and converts to a cell (8x8 tile)
         * @param start Top left pixel of the tile in bitmap.
         * @param Cell start of the cell to write data to in the tileset.
         * @param imgWidth number of bytes in a horizontal line of the source image.
         * @param dataWidth number of bytes in 8 pixel line of a cel.
         * @return The start of the next cel in the tileset
         */
        uint8_t* Bitmap2Cell(uint8_t* start, uint8_t* cell, uint32_t imgWidth, uint8_t dataWidth)
        {
            uint8_t* bitmapCell = cell;

            for (int i = 0; i < 8; ++i)
            {
                for (int j = 0; j < dataWidth; ++j)
                {
                    this->dataAccumulator |= *start;
                    *bitmapCell++ = *start++;
                }

                start += imgWidth - dataWidth;
            }

            return bitmapCell;
        }

        /** @brief Extracts 16x16 pixel region of bitmap image and converts to 2x2 cel character (16x16 tile)
         * @param start Top left pixel of the tile in bitmap.
         * @param Cell start of the cel to write data to in the tileset.
         * @param imgWidth number of bytes in a horizontal line of the source image.
         * @param dataWidth number of bytes in 8 pixel line of a cel.
         * @return The start of the next character in the tileset
         */
        uint8_t* Bitmap2Char2x2(uint8_t* start, uint8_t* cell, uint32_t imgWidth, uint8_t dataWidth)
        {
            uint8_t* bitmapCell = cell;
            bitmapCell = this->Bitmap2Cell(start, bitmapCell, imgWidth, dataWidth);
            bitmapCell = this->Bitmap2Cell(start + dataWidth, bitmapCell, imgWidth, dataWidth);
            bitmapCell = this->Bitmap2Cell(start + (imgWidth << 3), bitmapCell, imgWidth, dataWidth);
            bitmapCell = this->Bitmap2Cell(start + dataWidth + (imgWidth << 3), bitmapCell, imgWidth, dataWidth);
            return bitmapCell;
        }

        /** @brief Splits source bitmap into 8x8 or 16x16 pixel tiles while ignoring empty tiles, and builds a default
         * @page of map data that reproduces the layout of the original image.
         * @param config Desired data configuration of the resulting tilemap
         * @param bmp the Soure bitmap image
         * @param startingPage Which page in the tilemap to write the default map layout to
         */
        void ConvertBitmap(TilemapInfo& config, SRL::Bitmap::IBitmap& bmp, int startingPage)
        {
            uint16_t sz = (config.CharSize) ? 0x7 : 0xf; // Mask to detect valid multiples of 8 or 16
            uint8_t mapDataWidth = (config.MapMode) ? 1 : 2; // 1 word or 2 word map data
            int32_t byteWidth; // Number of bytes in 1 line of pixels from source image (Width* (0.5, 1, or 2))
            uint8_t byteCell; // Number of bytes in 1 line of a cel (4,8, or 16)
            SRL::Bitmap::BitmapInfo bitmapInfo = bmp.GetInfo();
            uint16_t yCells = bitmapInfo.Height >> 3;
            uint16_t xCells = bitmapInfo.Width >> 3;
            uint8_t* currentCell = this->cellData;
            uint8_t* currentData = bmp.GetData();
            uint16_t* currentMap = this->mapData;

            // Set data width of a line of Cel data base on the color mode of the image data
            switch (config.ColorMode)
            {
            case CRAM::TextureColorMode::Paletted16:
                byteWidth = bitmapInfo.Width >> 1;
                byteCell = 4;
                break;

            case CRAM::TextureColorMode::Paletted256:
                byteWidth = bitmapInfo.Width;
                byteCell = 8;
                break;

            default:
                byteWidth = bitmapInfo.Width << 1;
                byteCell = 16;
                break;
            }

            // Check to make sure image dimensions are compatible:
            if ((byteWidth & (uint16_t)sz) ||
                (bitmapInfo.Height & (uint16_t)sz) ||
                bitmapInfo.Height > 512 ||
                bitmapInfo.Width > 512)
            {
                this->cellData = NULL;
                this->mapData = NULL;
                this->palData = NULL;
                this->info = TilemapInfo();
                SRL::Debug::Assert("Tileset conversion failed- Image Dimensions Not supported");
                return;
            }

            int32_t pix = ((8 * (1 + config.CharSize)) * (byteCell * (1 + config.CharSize)));

            for (int32_t i = 0; i < pix; ++i)
            {
                *currentCell++ = 0;
            }

            this->numCells = (byteCell >> 2);

            if (config.CharSize == CHAR_SIZE_1x1) // Convert to 8x8 characters
            {
                for (int32_t i = 0;i < yCells; ++i)
                {
                    for (int32_t j = 0; j < xCells; ++j)
                    {
                        this->dataAccumulator = 0;
                        currentCell = this->Bitmap2Cell(currentData, currentCell, byteWidth, byteCell);
                        currentData += byteCell;

                        if (!config.MapMode) ++currentMap; // Add extra word of spacing to character pattern data

                        if (this->dataAccumulator) // If this tile was not blank keep it
                        {
                            *currentMap++ = this->numCells;
                            this->numCells += (byteCell >> 2);
                        }
                        else // Set map to index 0 and do not retain the tile
                        {
                            *currentMap++ = 0;
                            currentCell -= (byteCell << 3);
                        }
                    }

                    currentData += (byteWidth * 7);
                    currentMap += (64 - xCells);
                }
            }
            else // Convert to 16x16 characters
            {
                for (int32_t i = 0;i < (yCells >> 1); ++i)
                {
                    for (int32_t j = 0; j < (xCells >> 1); ++j)
                    {
                        this->dataAccumulator = 0;
                        currentCell = this->Bitmap2Char2x2(currentData, currentCell, byteWidth, byteCell);
                        currentData += (byteCell << 1);

                        // Add extra word of spacing to character pattern data
                        if (!config.MapMode) ++currentMap;

                        if (this->dataAccumulator) // If this tile was not blank
                        {
                            *currentMap++ = this->numCells;
                            this->numCells += (byteCell >> 2);
                        }
                        else // Set to index 0 and do not retain this tile
                        {
                            *currentMap++ = 0;
                            currentCell -= (byteCell << 5);
                        }
                    }

                    currentData += (byteWidth * 15); // Increment to next line of Characters in image
                    currentMap += (32 - (xCells >> 1));// Increment to next line of the page
                }
            }
        }
    public:

        /** @brief Create a Tilemap out of an existing Bitmap
        * @param bmp The Bitmap Image to convert
        * @param pages the number of pages avaiable to the resulting tilemap
        * @note A page is a unit of map data representing the tiling af a 512x512 pixel region
        * (32x32 or 64x64 tiles depending on the base tile size). The resulting Tilemap always
        * Contains at least 1 page with the default mapping of all tiles extracted from the bitmap,
        * but more pages can be reserved here for further use
        */
        Bmp2Tile(SRL::Bitmap::IBitmap& bmp, uint8_t pages = 1) :
            cellData(nullptr), mapData(nullptr), palData(nullptr), info(TilemapInfo()), numCells(0), numPages(0), dataAccumulator(0)
        {
            if (pages < 1) pages = 1;

            this->numPages = pages;
            this->numCells = 0;
            this->info.CharSize = CHAR_SIZE_2x2;
            this->info.ColorMode = bmp.GetInfo().ColorMode;
            this->info.MapMode = PNB_1WORD | CN_12BIT;
            this->info.PlaneSize = PL_SIZE_1x1;
            this->info.MapHeight = (this->info.CharSize) ? (32 * pages) : (64 * pages);
            this->info.MapWidth = (this->info.CharSize) ? 32 : 64;
            uint8_t bitDepth = 1;
            this->info.CellByteSize = bmp.GetInfo().Height * bmp.GetInfo().Width;

            // Add enough space for empty tile at start of set
            this->info.CellByteSize += (this->info.CharSize) ? 256 : 64;

            if (this->info.ColorMode == CRAM::TextureColorMode::Paletted16)
            {
                this->info.CellByteSize >>= 1;
            }
            else if (this->info.ColorMode == CRAM::TextureColorMode::RGB555)
            {
                this->info.CellByteSize <<= 1;
            }

            if (this->info.CellByteSize > 0x20000 || (numPages << 10) > 0x20000)
            {
                SRL::Debug::Assert("Size Unsupported- Maximum Tileset/Map data Size is 0x20000 bytes each");
                return;
            }

            this->cellData = new uint8_t[this->info.CellByteSize];
            this->mapData = new uint16_t[this->info.MapWidth * this->info.MapHeight];

            if (bmp.GetInfo().Palette != nullptr)
            {
                uint16_t sz = (this->info.ColorMode == CRAM::TextureColorMode::Paletted16) ? 16 : 256;
                this->palData = new uint16_t[sz];
                uint16_t* src = (uint16_t*)bmp.GetInfo().Palette->Colors;

                for (int i = 0; i < sz; ++i) this->palData[i] = *src++;
            }
            else
            {
                this->palData = nullptr;
            }

            for (int i = 0; i < numPages; ++i) this->ClearPage(i);

            this->ConvertBitmap(this->info, bmp, 0);
        }

        /** @brief Free allocated resources and destroy Bmp2Tile object
         */
        ~Bmp2Tile()
        {
            this->FreeCellData();
            this->FreeMapData();
            this->FreePalData();
        }

        /** @brief Get Cell data(Tileset)
         * @return Pointer to Cel data
         */
        void* GetCellData() override
        {
            return this->cellData;
        }

        /** @brief Get Map data(Tilemap)
         * @return Pointer to Map data
         */
        void* GetMapData() override
        {
            return this->mapData;
        }

        /** @brief Get Palette data
         * @return Pointer to palette data
         */
        void* GetPalData() override
        {
            return this->palData;
        }

        /** @brief Get Tilemap Info
         * @return Tilemap Info
         */
        TilemapInfo GetInfo() override
        {
            return this->info;
        }

        /** @brief Copies a rectangular selection of Tilemap data between  2 pages in the tilemap
        * @param sourcePage Index of the Tilemap Page to copy from
        * @param topLeft Page Coordinates of Top left of region to Copy
        * @param botRight Page Coordinates of Bottom Right of region Copy
        * @param destPage Index of the Tilemap page being copied to
        * @param DestTile Starting Coordinate to copy to in the destination page (the new Top left)
        */
        void CopyMap(uint8_t sourcePage, Tilemap::Coord topLeft, Tilemap::Coord botRight, uint8_t destPage, Tilemap::Coord destTile)
        {

            // Check that requested page indicies exist in this map:
            if (!this->mapData || sourcePage >= this->numPages || destPage >= numPages)
            {
                return;
            }

            uint32_t pageOffset = (this->info.CharSize) ? 1024 : 4096;
            uint16_t pageDim = (this->info.CharSize) ? 31 : 63;

            // Check that starting coordinates are in page bounds:
            if (topLeft.X > pageDim || topLeft.Y > pageDim || destTile.X > pageDim || destTile.Y > pageDim)
            {
                return;
            }

            // Crop copied region to fit page bounds
            if (botRight.X > pageDim) botRight.X = pageDim;
            if (botRight.Y > pageDim) botRight.Y = pageDim;

            if (destTile.X + (botRight.X - topLeft.X) > pageDim)
            {
                botRight.X -= (destTile.X + (botRight.X - topLeft.X) - pageDim);
            }

            if (destTile.Y + (botRight.Y - topLeft.Y) > pageDim)
            {
                botRight.Y -= (destTile.Y + (botRight.Y - topLeft.Y) - pageDim);
            }

            // Copy page data
            uint16_t* src = this->mapData + (pageOffset * sourcePage) + topLeft.X + ((pageDim + 1) * topLeft.Y);
            uint16_t* dst = this->mapData + (pageOffset * destPage) + destTile.X + ((pageDim + 1) * destTile.Y);

            for (uint8_t i = topLeft.Y; i <= botRight.Y; ++i)
            {
                for (uint8_t j = topLeft.X; j <= botRight.X; ++j)
                {
                    *dst++ = *src++;
                }

                dst += pageDim - (botRight.X - topLeft.X);
                src += pageDim - (botRight.X - topLeft.X);
            }
        }

        /** @brief Get address of tilemap page
         * @param coord Page coordinate
         * @return Page address
         */
        uint16_t* GetPageAddress(uint8_t page, Tilemap::Coord& coord)
        {
            uint32_t pageDim = (this->info.CharSize) ? 32 : 64;
            uint32_t offset = (this->info.CharSize) ? 1024 : 4096;
            offset *= (uint32_t)page;
            offset += (uint32_t)coord.X + (pageDim * (uint32_t)coord.Y);
            return this->mapData + offset;
        }

        /** @brief clears all data of a page in the tilemap to zero
         * @param pageIndex pageIndex Index of the page to clear in this Tilemap's Page Table
         */
        void ClearPage(int pageIndex)
        {
            if (pageIndex >= this->numPages) return;
            int pageOffset = (this->info.CharSize) ? 1024 : 4096;
            if (this->info.MapMode == PNB_2WORD) pageOffset << 1;
            uint16_t* PageStart = this->mapData + (pageOffset * pageIndex);
            for (int i = 0; i < pageOffset; ++i) *(PageStart++) = 0;
        }

        /** @brief Frees The Work Ram associated with Cell Data in cases where it is no longer required.
         */
        void FreeCellData()
        {
            if (this->cellData) delete this->cellData;
            this->cellData = nullptr;
        }

        /** @brief Frees The Work Ram associated with Map Data in cases where it is no longer required.
         */
        void FreeMapData()
        {
            if (this->mapData) delete this->mapData;
            this->mapData = nullptr;
        }

        /** @brief Frees The Work Ram associated with Pal Data in cases where it is no longer required.
         */
        void FreePalData()
        {
            if (this->palData) delete this->palData;
            this->palData = nullptr;
        }

        /** @brief manually apply a cel offset and Palette offset to all map data for cases when bypassing
        * built in VDP2 tilemap loading functions (such as when performing DMA transfers to VDP2 VRAM)
        * @note Do not use in conjuction with VDP2::ScrollScreen::LoadTilemap, as these offsets are applied automatically.
        * @param celOffset Tile address offset to apply (obtain with VDP2::ScrollScreen::GetCelOffset())
        * @param palOffset CRAM palette index to apply (obtain with VDP2::ScrollScreen::GetPalOffset())
        */
        void ApplyVdp2Offsets(uint32_t celOffset, uint32_t palOffset = 0)
        {
            uint16_t* mapStart = this->mapData;

            for (int i = 0; i < this->info.MapHeight * this->info.MapWidth; ++i)
            {
                mapStart[i] += celOffset;
                mapStart[i] |= palOffset;
            }
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
    struct CubeTile : public ITilemap
    {
    private:

        /** @brief Pointer to Cel data table
         */
        uint8_t* cellData;

        /** @brief Pointer to Map data table
         */
        uint8_t* mapData;

        /** @brief Pointer to Palette data table
         */
        uint8_t* palData;

        /** @brief Tilemap configuration
         */
        TilemapInfo info;

        /** @brief loads SGLb file from CD and Initializes Tilemap Info based on the file header
         * @param file The CubeTile format file to load from CD
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
                this->info.CellByteSize = headerData[1];
                uint32_t celSize = this->info.CellByteSize;
                uint32_t mapSize = headerData[2];

                //decode color mode
                switch (headerData[4])
                {
                case 0x0:
                    this->info.ColorMode = SRL::CRAM::TextureColorMode::Paletted16;
                    break;

                case 0x10:
                    this->info.ColorMode = SRL::CRAM::TextureColorMode::Paletted256;
                    break;

                case 0x30:
                    this->info.ColorMode = SRL::CRAM::TextureColorMode::RGB555;
                    break;
                }

                this->info.CharSize = (uint16_t)headerData[3];
                this->info.PlaneSize = (uint16_t)headerData[5];
                this->info.MapMode = (uint16_t)headerData[6];
                if (headerData[7] > 4) // If header formatted to allow differing height and width
                {
                    this->info.MapWidth = (uint16_t)((headerData[7] & 0xffff) * 32);

                    this->info.MapHeight = (uint16_t)((headerData[7] >> 16) * 32);
                }
                else this->info.MapHeight = this->info.MapWidth = (uint16_t)(headerData[7] * 32); // Old format for compatibility

                if (this->info.CharSize == CHAR_SIZE_1x1)
                {
                    this->info.MapHeight <<= 1;
                    this->info.MapWidth <<= 1;
                }

                if (this->info.PlaneSize == PL_SIZE_2x2)
                {
                    this->info.MapHeight <<= 1;
                    this->info.MapWidth <<= 1;
                }
                else if (this->info.PlaneSize == PL_SIZE_2x1)
                {
                    this->info.MapWidth <<= 1;
                }

                this->cellData = new uint8_t[celSize];
                this->mapData = new uint8_t[mapSize];

                // Load Palette Colors if they exist:
                if (this->info.ColorMode == SRL::CRAM::TextureColorMode::Paletted16)
                {
                    this->palData = new uint8_t[32];

                    for (int i = 0; i < 32; ++i) this->palData[i] = *imageData++;
                }
                else if (this->info.ColorMode == SRL::CRAM::TextureColorMode::Paletted256)
                {
                    this->palData = new uint8_t[512];

                    for (int i = 0; i < 512; ++i) this->palData[i] = *(imageData++);
                }

                // Load map data:
                for (uint32_t j = 0; j < mapSize; ++j)
                {
                    this->mapData[j] = *(imageData++);

                }

                // Load cel data
                for (uint32_t k = 0; k < celSize; ++k)
                {
                    this->cellData[k] = *(imageData++);

                }
            }

            delete stream;
        }

    public:

        /** @brief Initialize with CubeTile filename to load
         * @details Allocates and loads specified CubeTile file to work ram.
         * @param filename Name of the CubeTile file to load.
         */
        CubeTile(const char* filename) :
            cellData(nullptr), mapData(nullptr), palData(nullptr), info(TilemapInfo())
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

        /** @brief Free all allocated resources and destroy CubeTile object
         */
        ~CubeTile()
        {
            if (this->cellData) delete this->cellData;
            if (this->mapData) delete this->mapData;
            if (this->palData) delete this->palData;
        }

        /** @brief Get Cell data(Tileset)
         * @return Pointer to Cel data
         */
        void* GetCellData() override
        {
            return this->cellData;
        }

        /** @brief Get Map data(Tilemap)
         * @return Pointer to Map data
         */
        void* GetMapData() override
        {
            return this->mapData;
        }

        /** @brief Get Palette data
         * @return Pointer to palette data
         */
        void* GetPalData() override
        {
            return this->palData;
        }

        /** @brief Get Tilemap Info
         * @return Tilemap Info
         */
        TilemapInfo GetInfo() override
        {
            return this->info;
        }
    };

    /** @brief SGL tilemap interface using the (cell, map, palette) arrays in a .C file
     */
    struct SGLTile : public ITilemap
    {
    private:
        /** @brief Pointer to Cell data table
         */
        void* cellData;

        /** @brief Pointer to Map data table
         */
        void* mapData;

        /** @brief Pointer to Palette data table
         */
        void* palData;

        /** @brief Tilemap configuration
         */
        TilemapInfo info;

    public:

        /** @brief Initialize with SGLTile data tables and Tilemap config
         * @param cellData SGLTile cell data table
         * @param mapData SGLTile map data table
         * @param palData SGLTile palette data table
         * @param myInfo TilemapInfo Describing configuration of the SGLTile data
         */
        SGLTile(void* cellData, void* mapData, void* palData, const TilemapInfo& info) :
            cellData(cellData), mapData(mapData), palData(palData), info(info)
        {
        }

        /** @brief Get Cell data(Tileset)
         * @return Pointer to Cel data
         */
        void* GetCellData() override
        {
            return this->cellData;
        }

        /** @brief Get Map data(Tilemap)
         * @return Pointer to Map data
         */
        void* GetMapData() override
        {
            return this->mapData;
        }

        /** @brief Get Palette data
         * @return Pointer to palette data
         */
        void* GetPalData() override
        {
            return this->palData;
        }

        /** @brief Get Tilemap Info
         * @return Tilemap Info
         */
        TilemapInfo GetInfo() override
        {
            return this->info;
        }
    };
}
