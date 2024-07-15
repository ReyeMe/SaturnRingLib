#pragma once

#include "srl_base.hpp"

namespace SRL
{
    /** @brief File and CD access wrapper
     */
    class Cd
    {
    private:

        /** @brief Gfs directory table
         */
        inline static GfsDirTbl GfsDirectories;

        /** @brief Gfs directory listing
         */
        inline static GfsDirName GfsDirectoryNames[SRL_MAX_CD_FILES];

        /** @brief Gfs work area
         */
        inline static char GfsWork[GFS_WORK_SIZE(SRL_MAX_CD_BACKGROUND_JOBS)];

    public:

        /** @brief Number of tracks CD can have
         */
        static const int MaxTrackCount = 99;

        /** @brief File size
         */
        struct FileSize
        {
            /** @brief Number of bytes this file takes
             */
            int32_t Bytes;

            /** @brief Number of sectors on disk the file takes
             */
            int32_t Sectors;

            /** @brief Size of the sector
             */
            int32_t SectorSize;

            /** @brief Size of the last sector
             */
            int32_t LastSectorSize;

            /** @brief Construct a new File Size object
             */
            FileSize() : Bytes(0), Sectors(0), SectorSize(0), LastSectorSize(0) { }

            /** @brief Construct a new File Size object
             * @param handle File handle
             */
            FileSize(GfsHn handle)
            {
                GFS_GetFileSize(handle, &this->SectorSize, &this->Sectors, &this->LastSectorSize);
                this->Bytes = (this->SectorSize * (this->Sectors - 1)) + this->LastSectorSize;
            }
        };
        
        /** @brief Disk file
         */
        struct File
        {
        private:

            /** @brief File identifier
             */
            int32_t identifier;

        public:
            /** @brief File handle
             */
            GfsHn Handle;

            /** @brief Size of the file
             */
            FileSize Size;

            /** @brief Construct a new File object from Gfs handle and identifier
             * @param handle Gfs handle
             * @param fid File identifier
             * @param getSize Fetch file size (will cause file to be open for a bit)
             */
            File(GfsHn handle, int32_t fid, bool getSize = true) : Handle(handle), Size(getSize ? FileSize(handle) : FileSize()), identifier(fid) { }

            /** @brief Open a file on CD
             * @param handle File handle
             */
            File(const char* name) : Handle(nullptr), Size(0), identifier(-1)
            {
                if (name != nullptr)
                {
                    int32_t sectorSize;
                    int32_t sectorCount;
                    int32_t lastSectorSize;
                    int32_t id = GFS_NameToId((int8_t*)name);

                    if (id >= 0)
                    {
                        this->identifier = id;

                        if (this->Open())
                        {
                            this->Size = FileSize(this->Handle);
                            this->Close();
                        }
                        else
                        {
                            this->identifier = -1;
                        }
                    }
                }
            }

            /** @brief Close file
             */
            ~File()
            {
                this->Close();
            }

            /** @brief File exists
             * @return True if exists
             */
            constexpr bool Exists()
            {
                return this->identifier >= 0;
            }

            /** @brief Get file identifier
             * @return File identifier
             */
            constexpr int32_t GetIdentifier()
            {
                return this->identifier;
            }

            /** @brief Close file
             */
            void Close()
            {
                if (this->Handle != nullptr)
                {
                    GFS_Close(this->Handle);
                    this->Handle = nullptr;
                }
            }

            /** @brief Read specified number of bytes from the file
             * @param size Number of bytes to read
             * @param destination Buffer to read bytes into
             * @return Number of bytes read (if lower than 0, error was encountered)
             */
            int32_t Read(int32_t size, void* destination)
            {
                if (this->Handle != nullptr)
                {
                    return GFS_Fread(this->Handle, this->Size.Sectors, destination, size);
                }

                return -1;
            }

            /** @brief Loads specified amount of bytes from a file
             * @note This function does not need file to be open
             * @param startOffset Number of bytes to skip at the start
             * @param size Number of bytes to read (length of the batch)
             * @param destination Buffer to read batch into
             * @return Number of bytes read (if lower than 0, error was encountered)
             */
            int32_t LoadBytes(int32_t startOffset, int32_t size, void* destination)
            {
                bool wasOpen = false;
                int32_t result = 0;

                if (this->identifier >= 0)
                {
                    if (this->Handle != nullptr)
                    {
                        this->Close();
                        wasOpen = true;
                    }

                    // This function reads whole file, file does not need to be opened
                    result = GFS_Load(this->identifier, startOffset, destination, size);

                    // Open again
                    if (wasOpen)
                    {
                        this->Open();
                    }
                }
                
                return result;
            }

            /** @brief Open file
             * @return True on success
             */
            bool Open()
            {
                if (this->identifier >= 0)
                {
                    this->Handle = GFS_Open(this->identifier);
                    return true;
                }

                return false;
            }
        };

        /** @brief Initialize file handling stuff
         * @return True if initialized without error
         */
        inline static bool Initialize()
        {
            GFS_DIRTBL_TYPE(&GfsDirectories) = GFS_DIR_NAME;
            GFS_DIRTBL_DIRNAME(&GfsDirectories) = Cd::GfsDirectoryNames;
            GFS_DIRTBL_NDIR(&GfsDirectories) = SRL_MAX_CD_FILES;
            return GFS_Init(SRL_MAX_CD_BACKGROUND_JOBS, Cd::GfsWork, &GfsDirectories) <= 2;
        }
        
        /** @brief Change current directory
         * @param name Directory name (NULL for root directory)
         */
        inline static void ChangeDir(char* name)
        {
            if (name == nullptr)
            {
                name = (char*)0;
            }

            int32_t fid = GFS_NameToId((int8_t*)name);
            GFS_DIRTBL_TYPE(&GfsDirectories) = GFS_DIR_NAME;
            GFS_DIRTBL_DIRNAME(&GfsDirectories) = Cd::GfsDirectoryNames;
            GFS_DIRTBL_NDIR(&GfsDirectories) = SRL_MAX_CD_FILES;
            GFS_LoadDir(fid, &GfsDirectories);
            GFS_SetDir(&GfsDirectories);
        }

        /** @brief Table of contents of the CD
         */
        struct TableOfContents
        {
        public:

            /** @brief Type of track present
             */
            enum TrackType
            {
                /** @brief Unknown or empty track
                 */
                Unknown = 0x00,

                /** @brief Is audio track
                 */
                Audio = 0x01,

                /** @brief Is 4 channel audio track
                 */
                Audio4Ch = 0x02,

                /** @brief Is data track
                 */
                Data = 0x08
            };

        private:

            /** @brief Construct a new Table Of Contents object
             */
            TableOfContents() { }

            /** @brief Track interface
             */
            struct ITrack
            {
                /** @brief Track type and flags
                 */
                unsigned int Control:4;

                /** @brief Get type of the track
                 * @return Type of the track 
                 */
                constexpr TrackType GetType()
                {
                    if (this->Control == 0x0f)
                    {
                        return TrackType::Unknown;
                    }
                    else if ((this->Control & 0x04) == 0)
                    {
                        return TrackType::Audio;
                    }
                    else if ((this->Control & 0x0C) == 0x08)
                    {
                        return TrackType::Audio4Ch;
                    }
                    else if ((this->Control & 0x0C) == 0x04)
                    {
                        return TrackType::Data;
                    }
                    
                    return TrackType::Unknown;
                }

                /** @brief Check whether digital copy of this track is permitted
                 * @return true if digital copy of this track is permitted
                 */
                constexpr bool IsCopyPermitted()
                {
                    return (this->Control & 0x02) == 0x02;
                }

                /** @brief Check whether track is data, recorded incrementally
                 * @return true if data track is recorded incrementally
                 */
                constexpr bool IsIncremental()
                {
                    return ((this->Control & 0x0D)) == 0x05;
                }
            };

        public:

            /** @brief Track location data
             */
            struct TrackLocation : public ITrack
            {
                /** @brief Track number
                 */
                unsigned int Number:4;

                /** @brief Frame address
                 */
                unsigned int FrameAddress:24;
            };

            /** @brief Track information data
             */
            struct TrackInformation : public ITrack
            {
                /** @brief Track address
                 */
                unsigned char Address:4;

                /** @brief Track number
                 */
                unsigned char Number;

                union {
                    /** @brief Track location
                     */
                    short Location;
                    struct {
                        /** @brief Sector number
                         */
                        char Sector;

                        /** @brief Frame number
                         */
                        char Frame;
                    } LocationData;
                }LocationBody;
            };

            /** @brief Session data
             */
            struct Session
            {

                unsigned int Control:4;

                /** @brief Track address
                 */
                unsigned int Address:4;

                /** @brief Frame address
                 */
                unsigned int fad:24;
            };

            /** @brief Get table of contents of the current CD
             * @return Table of contents
             */
            static TableOfContents GetTable()
            {
                TableOfContents toc;
                CDC_TgetToc((uint32_t*)&toc);
                return toc;
            }

            /** @brief Information about each track present on the CD
             */
            TableOfContents::TrackLocation Tracks[Cd::MaxTrackCount];

            /** @brief Information about first track on the CD
             */
            TableOfContents::TrackInformation FirstTrack;

            /** @brief Information about last track on the CD
             */
            TableOfContents::TrackInformation LastTrack;

            /** @brief Information about current session data
             */
            TableOfContents::Session Session;
        };
    };
}
