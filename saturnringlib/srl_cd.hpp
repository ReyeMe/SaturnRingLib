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

        /** @brief initialization status
         */
        inline static bool isInitialized = false;

    public:
        /** @brief Number of tracks CD can have
         */
        static const int MaxTrackCount = 99;

        /** @brief disable default constructor
         */
        Cd() = delete;

        /** @brief CD seek mode
         */
        enum SeekMode : uint32_t
        {
            /** @brief Seek to absolute location
             */
            Absolute = 0,

            /** @brief Seek relative to current location
             */
            Relative,

            /** @brief Seek to the end
             */
            EndOfFile
        };

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
            FileSize() : Bytes(0), Sectors(0), SectorSize(0), LastSectorSize(0) {}

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

            /** @brief File read buffer
             */
            uint8_t *readBuffer;

            /** @brief Number of bytes already read from the sector
             */
            int32_t readSectorBytes;

            /** @brief Number of bytes read in total
             */
            int32_t readBytes;

            /** @brief Copy specified number of bytes from source buffer to target buffer
             * @param target Target buffer
             * @param source Source buffer
             * @param count Number of bytes to copy
             * @param targetOffset Offset in bytes in target buffer
             */
            static void CopyBuffers(uint8_t *target, uint8_t *source, size_t count, size_t targetOffset)
            {
                for (size_t byte = 0; byte < count; byte++)
                {
                    *(target + byte + targetOffset) = source[byte];
                }
            }

        public:
            /** @brief File handle
             */
            GfsHn Handle;

            /** @brief Size of the file
             */
            FileSize Size;

            /** @brief disable default constructor
             */
            File() = delete;

            /** @brief Construct a new File object from Gfs handle and identifier
             * @param handle Gfs handle
             * @param fid File identifier
             * @param getSize Fetch file size (will cause file to be open for a bit)
             */
            File(GfsHn handle, int32_t fid, bool getSize = true) : Handle(handle),
                                                                   Size(getSize ? FileSize(handle) : FileSize()),
                                                                   identifier(fid),
                                                                   readBuffer(nullptr),
                                                                   readSectorBytes(0),
                                                                   readBytes(0)
            {
                #if defined(SRL_MAX_CD_FILES) && (SRL_MAX_CD_FILES < 1)
                    static_assert(false, "SRL_MAX_CD_FILES is not set properly to instanciate this class");
                #endif
            }

            /** @brief Open a file on CD
             * @param name File name
             */
            File(const char *name) : Handle(nullptr),
                                     Size(0),
                                     identifier(-1),
                                     readBuffer(nullptr),
                                     readSectorBytes(0),
                                     readBytes(0)
            {
                #if defined(SRL_MAX_CD_FILES) && (SRL_MAX_CD_FILES < 1)
                    static_assert(false, "SRL_MAX_CD_FILES is not set properly to instanciate this class");
                #endif

                if (name != nullptr)
                {
                    int32_t sectorSize;
                    int32_t sectorCount;
                    int32_t lastSectorSize;
                    int32_t id = GFS_NameToId((int8_t *)name);

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

            /**
             * @name File handling and status
             * @{
             */

            /** @brief Close file
             */
            void Close()
            {
                if (this->Handle != nullptr)
                {
                    GFS_Close(this->Handle);
                    this->Handle = nullptr;
                }

                if (this->readBuffer != nullptr)
                {
                    delete this->readBuffer;
                    this->readBuffer = nullptr;
                }
            }

            /** @brief Open file
             * @return True on success
             */
            bool Open()
            {
                if (this->identifier >= 0)
                {
                    this->readBytes = 0;
                    this->readSectorBytes = 0;
                    this->Handle = GFS_Open(this->identifier);
                    return true;
                }

                return false;
            }

            /** @brief File exists
             * @return True if exists
             */
            constexpr bool Exists()
            {
                return this->identifier >= 0;
            }

            /** @brief File is open
             * @return True if file is open
             */
            constexpr bool IsOpen()
            {
                return this->Exists() && this->Handle != nullptr;
            }

            /**
             * @}
             */

            /**
             * @name File reading
             * @{
             */

            /** @brief Loads specified amount of bytes from a file
             * @note This function does not need file to be open
             * @param sectorOffset Number of sectors to skip at the start
             * @param size Number of bytes to read (length of the batch)
             * @param destination Buffer to read batch into
             * @return Number of bytes read (if lower than 0, error was encountered)
             */
            int32_t LoadBytes(size_t sectorOffset, int32_t size, void *destination)
            {
                bool wasOpen = false;
                int32_t result = 0;

                if (this->identifier >= 0)
                {
                    if (this->IsOpen())
                    {
                        this->Close();
                        wasOpen = true;
                    }

                    // This function reads whole file, file does not need to be opened
                    result = GFS_Load(this->identifier, sectorOffset, destination, size);

                    // Open again
                    if (wasOpen)
                    {
                        this->Open();
                    }
                }

                return result;
            }

            /** @brief Read specified number of bytes from the file and advances file access pointer
             * @param size Number of bytes to read
             * @param destination Buffer to read bytes into
             * @return Number of bytes read (if lower than 0, error was encountered)
             */
            int32_t Read(int32_t size, void *destination)
            {
                if (this->IsOpen() && size > 0 && this->Size.Bytes > 0)
                {
                    int32_t chunkBytes = 0;
                    bool firstRead = this->readBuffer == nullptr;

                    if (firstRead)
                    {
                        this->readBuffer = new uint8_t[this->Size.SectorSize + 1];
                    }

                    while (chunkBytes < size && this->readBytes < this->Size.Bytes)
                    {
                        int32_t read;

                        // We have first read or reached end of a sector
                        if (firstRead || this->readSectorBytes == this->Size.SectorSize)
                        {
                            this->readSectorBytes = 0;
                            read = GFS_Fread(this->Handle, 1, this->readBuffer, this->Size.SectorSize);
                            firstRead = false;
                        }
                        else
                        {
                            read = this->Size.SectorSize - this->readSectorBytes;
                        }

                        // We got an error
                        if (read < 0)
                        {
                            return read;
                        }

                        // Clamp to target end of file
                        int32_t remaining = this->Size.Bytes - this->readBytes;

                        if (read > remaining)
                        {
                            read = remaining;
                        }

                        // Clamp to target buffer size
                        if (read > size)
                        {
                            read = size;
                        }

                        // Copy to target buffer
                        File::CopyBuffers((uint8_t *)destination, this->readBuffer + this->readSectorBytes, read, chunkBytes);

                        // Set state
                        this->readSectorBytes += read;
                        this->readBytes += read;
                        chunkBytes += read;
                    }

                    return chunkBytes;
                }

                return -1;
            }

            /** @brief Seek file access pointer to specific byte
             * @param offset offset from start of the file
             * @return New position of the access pointer otherwise negative on error
             */
            int32_t Seek(int32_t offset)
            {
                int32_t result = -1;

                if (this->IsOpen() && offset >= 0 && offset < this->Size.Bytes)
                {
                    // Get sector count for offset
                    bool firstRead = this->readBuffer == nullptr;

                    // Initialize read buffer
                    if (firstRead)
                    {
                        this->readBuffer = new uint8_t[this->Size.SectorSize + 1];
                    }
                    else if ((this->readBytes - this->readSectorBytes) < offset &&
                             ((this->readBytes - this->readSectorBytes) + this->Size.SectorSize) > offset)
                    {
                        this->readSectorBytes = offset - (this->readBytes - this->readSectorBytes);
                        this->readBytes = offset;
                        return this->readBytes;
                    }

                    int32_t sector = this->GetSectorCount(offset);

                    if (sector >= 0)
                    {
                        // Seek to predefined location
                        result = GFS_Seek(this->Handle, sector, Cd::SeekMode::Absolute);

                        // Refresh buffer
                        if (result >= 0 &&
                            GFS_Fread(this->Handle, 1, this->readBuffer, this->Size.SectorSize) >= 0)
                        {
                            this->readBytes = offset;
                            this->readSectorBytes = offset % this->Size.SectorSize;
                            return offset;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                }

                return result;
            }

            /**
             * @}
             */

            /**
             * @name Other
             * @{
             */

            /** @brief
             * @return Current state of file access pointer
             */
            int32_t GetCurrentAccessPointer()
            {
                return this->IsOpen() ? this->readBytes : -1;
            }

            /** @brief Get file identifier
             * @return File identifier
             */
            constexpr int32_t GetIdentifier()
            {
                return this->identifier;
            }

            /** @brief Get number sectors take by specified byte count for this file
             * @param bytes Number of bytes
             * @return Number of sectors, -1 on error
             */
            int32_t GetSectorCount(size_t bytes)
            {
                if (this->IsOpen())
                {
                    return GFS_ByteToSct(this->Handle, bytes);
                }

                return -1;
            }

            /**
             * @}
             */
        };

        /** @brief Initialize file handling stuff
         * @return True if initialized without error
         */
        inline static bool Initialize()
        {
            if (!isInitialized)
            {
                GFS_DIRTBL_TYPE(&GfsDirectories) = GFS_DIR_NAME;
                GFS_DIRTBL_DIRNAME(&GfsDirectories) = Cd::GfsDirectoryNames;
                GFS_DIRTBL_NDIR(&GfsDirectories) = SRL_MAX_CD_FILES;
                isInitialized = (GFS_Init(SRL_MAX_CD_BACKGROUND_JOBS, Cd::GfsWork, &GfsDirectories) <= 2);
            }
            return isInitialized;
        }

        /** @brief Change current directory
         * @param name Directory name (NULL for root directory)
         */
        inline static void ChangeDir(const char *name)
        {
            ChangeDir(const_cast<char *>(name));
        }

        /** @brief Change current directory
         * @param name Directory name (NULL for root directory)
         */
        inline static void ChangeDir(char *name)
        {
            if (name == nullptr)
            {
                name = (char *)0;
            }

            int32_t fid = GFS_NameToId((int8_t *)name);
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
            TableOfContents() {}

            /** @brief Track interface
             */
            struct ITrack
            {
                /** @brief Track type and flags
                 */
                unsigned int Control : 4;

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
                unsigned int Number : 4;

                /** @brief Frame address
                 */
                unsigned int FrameAddress : 24;
            };

            /** @brief Track information data
             */
            struct TrackInformation : public ITrack
            {
                /** @brief Track address
                 */
                unsigned char Address : 4;

                /** @brief Track number
                 */
                unsigned char Number;

                /** @brief Track location
                 */
                union
                {
                    /** @brief Track location address
                     */
                    short Location;
                    struct
                    {
                        /** @brief Sector number
                         */
                        char Sector;

                        /** @brief Frame number
                         */
                        char Frame;
                    } LocationData;
                } LocationBody;
            };

            /** @brief Session data
             */
            struct Session
            {

                /** @brief Session control flags
                 */
                unsigned int Control : 4;

                /** @brief Track address
                 */
                unsigned int Address : 4;

                /** @brief Frame address
                 */
                unsigned int fad : 24;
            };

            /** @brief Get table of contents of the current CD
             * @return Table of contents
             */
            static TableOfContents GetTable()
            {
                TableOfContents toc;
                CDC_TgetToc((uint32_t *)&toc);
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
