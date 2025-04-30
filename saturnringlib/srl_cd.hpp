#pragma once

#include "srl_base.hpp"
#include "srl_debug.hpp"

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
        inline static uint32_t GfsWork[GFS_WORK_SIZE(SRL_MAX_CD_BACKGROUND_JOBS) / sizeof(uint32_t)];

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
            Absolute = GFS_SEEK_SET,

            /** @brief Seek relative to current location
             */
            Relative = GFS_SEEK_CUR,

            /** @brief Seek to the end
             */
            EndOfFile = GFS_SEEK_END
        };

        /** @brief access errors
         */
        enum ErrorCode : int32_t
        {
            /** @brief No error occurred, operation successful
             */
            ErrorOk = GFS_ERR_OK,

            /** @brief Error reading from the CD
             */
            ErrorCDRD = GFS_ERR_CDRD,

            /** @brief No disc inserted in the CD drive
             */
            ErrorCDNoDisc = GFS_ERR_CDNODISC,

            /** @brief Error related to CD-ROM hardware or media
             */
            ErrorCDRom = GFS_ERR_CDROM,

            /** @brief Error related to the directory table
             */
            ErrorDirTLD = GFS_ERR_DIRTBL,

            /** @brief Maximum number of open files exceeded
             */
            ErrorOenMax = GFS_ERR_OPENMAX,

            /** @brief Error related to directory operations
             */
            ErrorDir = GFS_ERR_DIR,

            /** @brief Error related to CD block file system
             */
            ErrorCDBFS = GFS_ERR_CDBFS,

            /** @brief Error indicating that the specified file or directory name does not exist
             */
            ErrorNoName = GFS_ERR_NONAME,

            /** @brief File or directory does not exist
             */
            ErrorNExit = GFS_ERR_NEXIST,

            /** @brief Error related to file identifier
             */
            ErrorFID = GFS_ERR_FID,

            /** @brief Error related to file handle operations
             */
            ErrorHandle = GFS_ERR_HNDL,

            /** @brief Error occurred during seek operation
             */
            ErrorSeek = GFS_ERR_SEEK,

            /** @brief Error related to file organization
             */
            ErrorOrg = GFS_ERR_ORG,

            /** @brief Error related to the number of files or directories
             */
            ErrorNum = GFS_ERR_NUM,

            /** @brief Error related to file offset operations
             */
            ErrorOFS = GFS_ERR_OFS,

            /** @brief File is busy, cannot perform the requested operation
             */
            ErrorFBusy = GFS_ERR_FBUSY,

            /** @brief Error related to invalid parameters
             */
            ErrorPara = GFS_ERR_PARA,

            /** @brief The CD drive is currently busy with another operation
             */
            ErrorBusy = GFS_ERR_BUSY,

            /** @brief No handler available for the requested operation
             */
            ErrorNoHandler = GFS_ERR_NOHNDL,

            /** @brief Error indicating that the peripheral unit is in use
             */
            ErrorPUINUSE = GFS_ERR_PUINUSE,

            /** @brief Error related to data alignment issues
             */
            ErrorAlign = GFS_ERR_ALIGN,

            /** @brief Timeout error, operation took too long to complete
             */
            ErrorTimeout = GFS_ERR_TMOUT,

            /** @brief Error indicating that the CD drive is open
             */
            ErrorCDOpen = GFS_ERR_CDOPEN,

            /** @brief Buffer is full, cannot write more data
             */
            ErrorBufferFull = GFS_ERR_BFUL,

            /** @brief Fatal error, unrecoverable condition
             */
            ErrorFatal = GFS_ERR_FATAL,

            /** @brief End of file reached
             */
            ErrorEOF = std::numeric_limits<int32_t>::min(),
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
            /** @brief Maximal number of sectors to be read in a single pass
             */
            inline static const uint16_t SectorsToReadAtOnce = 5;

            /** @brief File identifier
             */
            int32_t identifier;

            /** @brief Number of bytes read in total
             */
            int32_t readBytes;

            /** @brief File read buffer
             */
            uint8_t *workBuffer;

            /** @brief Get the byte offset inside current work buffer
             * @param absolute Absolute position
             * @param workBufferSize Size of a work buffer
             * @return size_t Position within work buffer
             */
            int32_t GetOffsetInWorkBuffer(int32_t absolute, int32_t workBufferSize)
            {
                if (absolute >= workBufferSize)
                {
                    return absolute % workBufferSize;
                }

                return absolute;
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
                                                                   workBuffer(nullptr),
                                                                   readBytes(0)
            {
                #if defined(SRL_MAX_CD_FILES) && (SRL_MAX_CD_FILES < 1)
                    static_assert(false, "SRL_MAX_CD_FILES is not set properly to instantiate this class");
                #endif
            }

            /** @brief Open a file on CD
             * @param name File name
             */
            File(const char *name) : Handle(nullptr),
                                     Size(0),
                                     identifier(-1),
                                     workBuffer(nullptr),
                                     readBytes(0)
            {
                #if defined(SRL_MAX_CD_FILES) && (SRL_MAX_CD_FILES < 1)
                    static_assert(false, "SRL_MAX_CD_FILES is not set properly to instantiate this class");
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

                if (this->workBuffer != nullptr)
                {
                    delete this->workBuffer;
                    this->workBuffer = nullptr;
                }
            }

            /** @brief Open file
             * @return True on success
             */
            bool Open()
            {
                if (this->IsOpen())
                {
                    return true;
                }
                else if (this->identifier >= 0)
                {
                    this->readBytes = 0;
                    this->Handle = GFS_Open(this->identifier);

                    if (this->Handle != nullptr)
                    {
                        GFS_NwCdRead(this->Handle, this->Size.Sectors);
                        GFS_SetTransPara(this->Handle, 10);
                        return true;
                    }
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

            /** @brief Gets the current position in the file
             * @return Position in the file as bytes
             */
            constexpr size_t GetCurrentPosition()
            {
                return this->readBytes;
            }

            /** @brief File is open
             * @return True if file is open
             */
            constexpr bool IsOpen()
            {
                return this->Exists() && this->Handle != nullptr;
            }

            /** @brief EOF has been reached
             * @return true if EOF, false otherwise
             */
            constexpr bool IsEOF()
            {
                return (GFS_IsEof(Handle) == TRUE);
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
                int32_t workBufferSize = this->Size.SectorSize * File::SectorsToReadAtOnce;

                if (this->IsOpen() && size > 0 && this->Size.Bytes > 0)
                {
                    int32_t currentlyRead = 0;

                    // We have not read any data yet
                    if (this->workBuffer == nullptr)
                    {
                        this->workBuffer = autonew uint8_t[workBufferSize];

                        // Refresh data for new sector
                        int32_t error = GFS_Fread(this->Handle, File::SectorsToReadAtOnce, this->workBuffer, workBufferSize);

                        if (error < 0)
                        {
                            delete[] this->workBuffer;
                            this->workBuffer = nullptr;
                            return -1;
                        }
                    }

                    // Read data sector by sector
                    while (currentlyRead < size && this->readBytes < this->Size.Bytes)
                    {
                        // Current location within the sector
                        const int32_t sectorStartOffset = this->GetOffsetInWorkBuffer(this->readBytes, workBufferSize);

                        int32_t toRead = size - currentlyRead;

                        // We can't read more than one sector at a time
                        if (toRead >= workBufferSize - sectorStartOffset)
                        {
                            // We have read whole sector
                            toRead = workBufferSize - sectorStartOffset;

                            // Copy to target buffer
                            for (int32_t byte = 0; byte < toRead; byte++)
                            {
                                reinterpret_cast<uint8_t*>(destination)[currentlyRead + byte] = this->workBuffer[byte + sectorStartOffset];
                            }

                            // Refresh data for new sector
                            int32_t error = GFS_Fread(this->Handle, File::SectorsToReadAtOnce, this->workBuffer, workBufferSize);

                            if (error < 0)
                            {
                                delete[] this->workBuffer;
                                this->workBuffer = nullptr;
                                return -1;
                            }
                        }
                        else
                        {
                            // We have not reached sector bounds, we can just copy bytes over
                            for (int32_t byte = 0; byte < toRead; byte++)
                            {
                                reinterpret_cast<uint8_t*>(destination)[currentlyRead + byte] = this->workBuffer[byte + sectorStartOffset];
                            }
                        }

                        // Set state
                        this->readBytes += toRead;
                        currentlyRead += toRead;
                    }

                    return currentlyRead;
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
                int32_t workBufferSize = this->Size.SectorSize * File::SectorsToReadAtOnce;

                if (this->IsOpen() && offset >= 0 && offset < this->Size.Bytes)
                {
                    // Initialize read buffer if does not exist yet
                    if (this->workBuffer == nullptr)
                    {
                        this->workBuffer = autonew uint8_t[workBufferSize];
                    }
                    
                    this->readBytes = offset;
                    int32_t sector = this->GetSectorCount(offset);

                    if (sector >= 0)
                    {
                        // Seek to predefined location
                        result = GFS_Seek(this->Handle, sector, Cd::SeekMode::Absolute);

                        // Refresh buffer
                        if (result >= 0 && GFS_Fread(this->Handle, File::SectorsToReadAtOnce, this->workBuffer, workBufferSize) >= 0)
                        {
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
            if (!Cd::isInitialized)
            {
                GFS_DIRTBL_TYPE(&Cd::GfsDirectories) = GFS_DIR_NAME;
                GFS_DIRTBL_DIRNAME(&Cd::GfsDirectories) = Cd::GfsDirectoryNames;
                GFS_DIRTBL_NDIR(&Cd::GfsDirectories) = SRL_MAX_CD_FILES;
                Cd::isInitialized = (GFS_Init(SRL_MAX_CD_BACKGROUND_JOBS, Cd::GfsWork, &Cd::GfsDirectories) <= 2);
            }
            return Cd::isInitialized;
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
