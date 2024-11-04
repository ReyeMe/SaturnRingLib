#pragma once

#include "srl_memory.hpp"
#include "srl_cram.hpp"

/** @brief VDP2 tilemap interface and utilities
 * @todo Think of a better way to convey the options available for tilemap data loading
 * in terms of what the they actually provide:
 * -Character Size
 * -Map Data Size
 * -Number Of pages per plane
 * -Number of planes
 */
namespace SRL::Tilemap
{
    /** @brief All the info necessary to properly configure VDP2 tilemap data in VRAM
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
        int32_t CellByteSize = 0;

        /** @brief Number of bytes of map data comprising the Tile map
         */
        int32_t MapByteSize = 0;

        /** @brief Initializes an empty tilemap info
         */
        TilemapInfo() = default;

        /** @brief Initialize by manually entering all parameters
         *  @param color Color Mode of the Tilemap (Paletted16, Paletted256, or RGB555).
         *  @param map Size and bit configuration of the map data (pattern name data).
         *  @param charSz Size of the tiles that comprise the tilemap (charater pattern data).
         *  @param plane Size of the Planes that comprise a Scroll Screen.
         *  @param height Height of tilemap (in tile units).
         *  @param width Width of the tilemap (in tile Units).
         *  @param size Number of bytes of image data comprising the Tile set.
         */
        TilemapInfo(SRL::CRAM::TextureColorMode color, uint16_t map, uint16_t charSz, uint16_t plane, uint16_t height, uint16_t width, int size) :
            ColorMode(color), MapMode(map), CharSize(charSz), PlaneSize(plane), MapHeight(height), MapWidth(width), CellByteSize(size)
        {
        }

        /** @brief Gets the SGL macro corresponding to ColorMode
         * @return SGL VDP2 color macro for the tile map
         */
        uint16_t SGLColorMode()
        {
            static const uint8_t sglColor[] = { 0x30,0x30,0x00,0x10,0x10,0x10,0x10 };
            return (uint16_t)sglColor[(uint16_t)this->ColorMode];
        }
    };

    /** @brief Virtual interface for tilemap
     */
    struct ITilemap
    {
        /** @brief Get Cell data(Tileset)
         * @return Pointer to Cell data
         */
        virtual void* GetCellData()
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

        /** @brief Get Palette data
        * @return Pointer to palette data
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

    /** @brief Unsigned 8 bit coordinates for tiles withing a page of a tilemap
     * @note Coordinates [0,0] represent top left corner of a page (same as Print coordinates)
     * @note Coordinates are in tile units, which can be either 8x8 or 16x16 pixels.
     * Max allowed coordinates in a page vary by Tile Size (64 for 8x8 tiles, 32 for 16x16)
     */
    struct Coord
    {
        /** @brief The tiles X coordinate in its page
         */
        uint16_t X;

        /** @brief The tiles Y coordinate in its page
         */
        uint16_t Y;

        /** @brief Initializes a new tile coordinate
         */
        Coord() : X(0), Y(0) {}

        /** @brief Initialize with desired X and Y coordinates
         * @param x X coordinate of tile in page
         * @param y Y coordinate of tile in page
         */
        Coord(uint16_t x, uint16_t y) : X(x), Y(y) {}
    };
}
