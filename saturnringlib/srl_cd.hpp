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
			Sint32 Size;

			/** @brief Open a file on CD
			 * @param handle File handle
			 */
			File(char* name) : Handle(nullptr), Size(0), identifier(-1)
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
							GFS_GetFileSize(this->Handle, &sectorSize, &sectorCount, &lastSectorSize);
							this->Size = (sectorSize * (sectorCount - 1)) + lastSectorSize;
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

			/** @brief Read file
			 * @param startOffset Number of bytes to skip at the start
			 * @param size Number of bytes to read
			 * @param stream File stream
			 * @return Sint32 Number of bytes read
			 */
			Sint32 Read(Sint32 startOffset, Sint32 size, void* stream)
			{
				if (this->Handle != nullptr)
				{
					return GFS_Load(this->identifier, startOffset, stream, this->Size);
				}

				return 0;
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
