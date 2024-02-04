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

		/** @brief File size
		 */
		struct FileSize
		{
			/** @brief Number of bytes this file takes
			 */
			Sint32 Bytes;

			/** @brief Number of sectors on disk the file takes
			 */
			Sint32 Sectors;

			/** @brief Size of the sector
			 */
			Sint32 SectorSize;

			/** @brief Size of the last sector
			 */
			Sint32 LastSectorSize;

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
			Sint32 identifier;

		public:
			/** @brief File handle
			 */
			GfsHn Handle;

			/** @brief Size of the file
			 */
			FileSize Size;

			/** @brief Open a file on CD
			 * @param handle File handle
			 */
			File(const char* name) : Handle(nullptr), Size(0), identifier(-1)
			{
				if (name != nullptr)
				{
					Sint32 sectorSize;
					Sint32 sectorCount;
					Sint32 lastSectorSize;
					Sint32 id = GFS_NameToId((Sint8*)name);

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
			constexpr Bool Exists()
			{
				return this->identifier >= 0;
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

			/** @brief Open batch from file (this function does not need file to be open)
			 * @param startOffset Number of bytes to skip at the start
			 * @param size Number of bytes to read (length of the batch)
			 * @param destination Buffer to read batch into
			 * @return Number of bytes read (if lower than 0, error was encountered)
			 */
			Sint32 OpenBatch(Sint32 startOffset, Sint32 size, void* destination)
			{
				Bool wasOpen = false;
				Sint32 result = 0;

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

			Sint32 fid = GFS_NameToId((Sint8*)name);
			GFS_DIRTBL_TYPE(&GfsDirectories) = GFS_DIR_NAME;
			GFS_DIRTBL_DIRNAME(&GfsDirectories) = Cd::GfsDirectoryNames;
			GFS_DIRTBL_NDIR(&GfsDirectories) = SRL_MAX_CD_FILES;
			GFS_LoadDir(fid, &GfsDirectories);
			GFS_SetDir(&GfsDirectories);
        }
    };
}
