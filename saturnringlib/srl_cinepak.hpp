#pragma once

#include <srl_core.hpp>

// SGL functions
extern "C" {
    #include <sgl_cpk.h>
}

namespace SRL
{
    /** @brief Cinepak movie player
     * @note Available only if SGL sound driver is enabled (set SRL_USE_SGL_SOUND_DRIVER to 1 in makefile)
     */
    class CinepakPlayer final
    {
    public:
    
        /** @brief Playback color depth
         */
        enum class ColorDepth
        {
            /** @brief 16-bit RGB555
             */
            RGB15 = 0,

            /** @brief 24-bit RGB888
             */
            RGB24 = 1
        };

        /** @brief Playback status
         */
        enum class PlaybackStateEnum
        {
            /** @brief Playback is stopped with error
             */
            Error = -1,

            /** @brief Playback is stopped
             */
            Stop = 0,
            
            /** @brief Playback is paused
             */
            Paused = 1,

            /** @brief Playback has started
             */
            Started = 2,

            /** @brief Movie header is being processed
             */
            HeaderProcessing = 3,

            /** @brief Playing movie, timer has started
             */
            Timer = 4,

            /** @brief Playback of the movie has completed
             */
            Completed = 5
        };

        /** @brief Parameters to specify movie playback decoding
         */
        struct MovieDecodeParams
        {
            /** @brief Construct default decode parameters
             */
            MovieDecodeParams() : 
                RingBufferSize(1024L*200),
                RingBufferLocation(SRL::Memory::Zone::LWRam),
                DecodeBufferLocation(SRL::Memory::Zone::Default),
                PCMAddress((uint16_t*)0x25a20000),
                PCMSize(4096 * 16),
                ColorDepth(CinepakPlayer::ColorDepth::RGB15) {}

            /** @brief Size of a ring buffer
             * @note To configure where RingBuffer is allocated use MovieDecodeParams::RingBufferLocation field
             */
            uint32_t RingBufferSize;

            /** @brief Location of the ring buffer in the memory, by default it is placed in LWRAM
             */
            SRL::Memory::Zone RingBufferLocation;

            /** @brief Location of the decode buffer in the memory, by default uses autonew for allocation, contains decoded frame
             * @note Size of the decode buffer is automatically selected based on color depth and video resolution
             * @warning Placing decode buffer anywhere else than HWRAM might introduce stutters with a fullscreen playback
             */
            SRL::Memory::Zone DecodeBufferLocation;

            /** @brief Location of PCM buffer
             * @note Location must be somewhere in sound RAM
             */
            uint16_t* PCMAddress;

            /** @brief Size of the PCM buffer
             */
            uint32_t PCMSize;

            /** @brief Playback color depth
             */
            CinepakPlayer::ColorDepth ColorDepth;
        };

        /** @brief Global error handler event
         */
        inline static SRL::Types::Event<int32_t> OnError;

        /** @brief Movie playback was completed
         * @details Called when playback is completed. Event parameter contains pointer to the player instance.
         * @note Always happens before SRL::Core::Synchronize();
         */
        SRL::Types::Event<CinepakPlayer&> OnCompleted;

        /** @brief Movie frame was decoded
         * @details Called each time new frame is decoded. Event parameter contains pointer to the player instance.
         * @note Always happens before SRL::Core::Synchronize();
         */
        SRL::Types::Event<CinepakPlayer&> OnFrame;

        /** @brief Construct a new Cinepak Player instance
         */
        CinepakPlayer() :
            handle(nullptr),
            ringBuffer(nullptr),
            workBuffer(nullptr),
            decodeBuffer(nullptr),
            movieFileHandle(nullptr),
            decodeParams(MovieDecodeParams()),
            completedTriggered(false),
            timeScale(SRL::Math::Fxp()),
            size(0, 0)
        {
            CinepakPlayer::Initialize();
            SRL::Core::OnBeforeSync += &this->cinepakTask;
        }

        /** @brief Destroy the Cinepak Player instance
         */
        ~CinepakPlayer()
        {
            SRL::Core::OnBeforeSync -= &this->cinepakTask;
            this->UnloadMovie();
        }

        /** @brief Load movie to play
         * @param file Movie file name
         * @param decodeParams Decode parameters
         * @return true if movie was loaded successfully
         */
        bool LoadMovie(const char* file, const MovieDecodeParams decodeParams = MovieDecodeParams())
        {
            this->timeScale = 0.0;
            this->completedTriggered = false;
            this->decodeParams = decodeParams;
            const auto is15bit = decodeParams.ColorDepth == ColorDepth::RGB15;
            const auto identifier = GFS_NameToId((int8_t*)file);

            if (identifier >= 0)
            {
                // Open file
                this->movieFileHandle = GFS_Open(identifier);

                if (this->movieFileHandle == nullptr)
                {
                    SRL::Debug::Assert("Could not open movie '%s'!", file);
                    this->UnloadMovie();
                    return false;
                }

                // Initialize buffers
                this->workBuffer = new uint32_t[is15bit ? CPK_15WORK_DSIZE : CPK_24WORK_DSIZE];
                
                if (decodeParams.RingBufferLocation == SRL::Memory::Zone::Default)
                {
                    // Allocate in the same zone as the current object
                    this->ringBuffer = autonew uint32_t[decodeParams.RingBufferSize >> 2];
                }
                else
                {
                    // Allocate based on user setting
                    this->ringBuffer = new(decodeParams.RingBufferLocation) uint32_t[decodeParams.RingBufferSize >> 2];
                }

                if (this->ringBuffer == nullptr || this->workBuffer == nullptr)
                {
                    SRL::Debug::Assert("Not enough space for ring and work buffers!");
                    this->UnloadMovie();
                    return false;
                }
                
                // Load header
                CpkCreatePara para;
                CPK_PARA_WORK_ADDR(&para) = this->workBuffer;
                CPK_PARA_WORK_SIZE(&para) = is15bit ? CPK_15WORK_BSIZE : CPK_24WORK_BSIZE;
                CPK_PARA_BUF_ADDR(&para) = this->ringBuffer;
                CPK_PARA_BUF_SIZE(&para) = decodeParams.RingBufferSize;
                CPK_PARA_PCM_ADDR(&para) = decodeParams.PCMAddress;
                CPK_PARA_PCM_SIZE(&para) = decodeParams.PCMSize;
                this->handle = CPK_CreateGfsMovie(&para, this->movieFileHandle);

                if (this->handle == nullptr)
                {
                    SRL::Debug::Assert("Could not create a movie! %d");
                    this->UnloadMovie();
                    return false;
                }
                
                // Set color depth
                CPK_SetColor(this->handle, is15bit ? CPK_COLOR_15BIT : CPK_COLOR_24BIT);

                // Load file header
                CPK_PreloadHeader(this->handle);
                const auto header = CPK_GetHeader(this->handle);
                this->timeScale = SRL::Math::Fxp::BuildRaw(header->time_scale_film);

                if (header == nullptr)
                {
                    SRL::Debug::Assert("Could not load movie header!");
                    this->UnloadMovie();
                    return false;
                }

                this->size = SRL::Types::Resolution(header->width, header->height);

                // Assign decode buffer
                if (decodeParams.DecodeBufferLocation == SRL::Memory::Zone::Default)
                {
                    // Allocate in the same zone as the current object
                    this->decodeBuffer = autonew uint32_t[(header->width * header->height) >> (is15bit ? 1 : 0)];
                }
                else
                {
                    // Allocate based on user setting
                    this->decodeBuffer = new(decodeParams.DecodeBufferLocation) uint32_t[(header->width * header->height) >> (is15bit ? 1 : 0)];
                }

                if (this->decodeBuffer == nullptr)
                {
                    SRL::Debug::Assert("Not enough space for decode buffer!");
                    this->UnloadMovie();
                    return false;
                }

                CPK_SetDecodeAddr(this->handle, this->decodeBuffer, header->width << ((int)decodeParams.ColorDepth + 1));

                return true;
            }

            return false;
        }

        /** @brief Unload movie file
         */
        void UnloadMovie()
        {
            this->Stop();

            if (this->handle)
            {
                CPK_DestroyGfsMovie(this->handle);
                this->handle = nullptr;
            }

            if (this->workBuffer)
            {
                delete[] this->workBuffer;
                this->workBuffer = nullptr;
            }

            if (this->ringBuffer)
            {
                delete[] this->ringBuffer;
                this->ringBuffer = nullptr;
            }

            if (this->decodeBuffer)
            {
                delete[] this->decodeBuffer;
                this->decodeBuffer = nullptr;
            }

            if (this->movieFileHandle != nullptr)
            {
                GFS_Close(this->movieFileHandle);
                this->movieFileHandle = nullptr;
            }
        }

        /** @brief Get movie color depth
         * @return Movie color depth 
         */
        ColorDepth GetDepth() const
        {
            return this->decodeParams.ColorDepth;
        }

        /** @brief Get decoded frame data
         * @return Frame data
         */
        void* GetFrameData() const
        {
            return this->decodeBuffer;
        }

        /** @brief Get movie resolution
         * @return Movie resolution
         */
        SRL::Types::Resolution GetResolution() const
        {
            return this->size;
        }

        /** @brief Get movie playback status
         * @return Status code of the playback
         */
        PlaybackStateEnum GetStatus() const
        {
            return this->handle == nullptr ? PlaybackStateEnum::Error : (PlaybackStateEnum)CPK_GetPlayStatus(this->handle);
        }

        /** @brief Get current playback time
         * @return Current playback time
         */
        SRL::Math::Fxp GetTime() const
        {
            if (this->handle != nullptr && this->timeScale > 0.0)
            {
                return SRL::Math::Fxp::BuildRaw(CPK_GetTime(this->handle)) / this->timeScale;
            }

            return 0.0;
        }

        /** @brief Set left/right audio panning
         * @param pan Audio panning
         * Value | Meaning
         * ------|---------
         * 0     | Both channels balanced
         * 1-15  | Lower volume of left channel until off (15), right channel stays at max volume
         * 16    | Both channels balanced
         * 17-31 | Lower volume of right channel until off (31), left channel stays at max volume
         */
        void SetAudioPan(const uint8_t pan) const
        {
            if (this->handle != nullptr)
            {
                CPK_SetPan(this->handle, pan);
            }
        }

        /** @brief Set movie playback speed
         * @param rate Speed at which movie should play
         * Value | Meaning
         * ------|---------
         * 0     | Play at normal speed
         * 1-1024| Speed ratio x1024<br/>(1024 = normal speed, 512 = half speed, 256 = quarter speed)
         * @param outputAudio If true, allows audio output while playing at different rate, setting to false is same as SetVolume(0)
         */
        void SetSpeed(const int32_t rate, const bool outputAudio = true) const
        {
            if (this->handle != nullptr)
            {
                CPK_SetSpeed(this->handle, rate, outputAudio);
            }
        }

        /** @brief Set Movie playback volume
         * @param volume Movie volume in range 0-7
         */
        void SetVolume(const uint32_t volume) const
        {
            if (this->handle != nullptr)
            {
                CPK_SetVolume(this->handle, volume);
            }
        }

        /** @brief Stop movie playback
         */
        void Stop()
        {
            if (this->GetStatus() == PlaybackStateEnum::Timer)
            {
                CPK_Stop(this->handle);
                this->completedTriggered = true;
            }
        }

        /** @brief Pause movie playback
         */
        void Pause() const
        {
            if (this->GetStatus() == PlaybackStateEnum::Timer)
            {
                CPK_Pause(this->handle, CPK_PAUSE_ON_KEYFRAME);
            }
        }

        /** @brief Start movie playback
         */
        void Play()
        {
            if (this->completedTriggered)
            {
                const auto is15bit = this->GetDepth() == ColorDepth::RGB15;

                // Restart the movie
                CPK_DestroyGfsMovie(this->handle);

                // Reset handle
                int32_t fid;
                int32_t fn;
                int32_t fsize;
                int32_t fattr;
                GFS_GetFileInfo(this->movieFileHandle, &fid, &fn, &fsize, &fattr);
                GFS_Close(this->movieFileHandle);
                this->movieFileHandle = GFS_Open(fid);

                // Load header
                CpkCreatePara para;
                CPK_PARA_WORK_ADDR(&para) = this->workBuffer;
                CPK_PARA_WORK_SIZE(&para) = is15bit ? CPK_15WORK_BSIZE : CPK_24WORK_BSIZE;
                CPK_PARA_BUF_ADDR(&para) = this->ringBuffer;
                CPK_PARA_BUF_SIZE(&para) = this->decodeParams.RingBufferSize;
                CPK_PARA_PCM_ADDR(&para) = this->decodeParams.PCMAddress;
                CPK_PARA_PCM_SIZE(&para) = this->decodeParams.PCMSize;
                this->handle = CPK_CreateGfsMovie(&para, this->movieFileHandle);
                
                // Set color depth
                CPK_SetColor(this->handle, is15bit ? CPK_COLOR_15BIT : CPK_COLOR_24BIT);

                // Load file header
                CPK_PreloadHeader(this->handle);
                const auto header = CPK_GetHeader(this->handle);
                CPK_SetDecodeAddr(this->handle, this->decodeBuffer, header->width << ((int)this->GetDepth() + 1));

                // Start again
                CPK_Start(this->handle);
                this->completedTriggered = false;
            }
            else
            {
                const auto state = this->GetStatus();

                if (state == PlaybackStateEnum::Stop)
                {
                    CPK_Start(this->handle);
                }
                else if (state == PlaybackStateEnum::Paused)
                {
                    CPK_Pause(this->handle, CPK_PAUSE_OFF);
                }
            }
        }

    private:
    
        /** @brief Library is initialized
         */
        inline static bool Initialized = false;

        /** @brief Initialize library
         */
        static void Initialize()
        {
            if (!CinepakPlayer::Initialized)
            {
                CinepakPlayer::Initialized = true;
                CPK_Init();
                SRL::Core::OnVblank += CPK_VblIn;
            }
        }

        /** @brief Completed event has been triggered
         */
        bool completedTriggered;

        /** @brief Each frame is decoded into this buffer
         */
        uint32_t* decodeBuffer;

        /** @brief Remember decode parameters
         */
        MovieDecodeParams decodeParams;

        /** @brief Cinepak playback handle
         */
        CpkHn handle;

        /** @brief Handle to the movie file
         */
        GfsHn movieFileHandle;

        /** @brief Location of a ring buffer
         * @note Ring buffer will always be placed into LWRAM
         */
        uint32_t* ringBuffer;

        /** @brief Movie resolution
         */
        SRL::Types::Resolution size;

        /** @brief Movie time scale
         */
        SRL::Math::Fxp timeScale;

        /** @brief Location of the work buffer
         */
        uint32_t* workBuffer;

        /** @brief Cinepak task execution
         */
        SRL::Types::MemberProxy<> cinepakTask = SRL::Types::MemberProxy([this]() 
        {
            if (this->handle != nullptr && !this->completedTriggered)
            {
                const auto state = this->GetStatus();

                if (state > PlaybackStateEnum::Stop &&
                    state < PlaybackStateEnum::Completed)
                {
                    CPK_Task(this->handle);

                    if (CPK_IsDispTime(this->handle))
                    {
                        this->OnFrame.Invoke(*this);
                        CPK_CompleteDisp(this->handle);
                    }
                }
                else if (!this->completedTriggered && state == PlaybackStateEnum::Completed)
                {
                    this->completedTriggered = true;
                    this->OnCompleted.Invoke(*this);
                }
            }
        });
    };
}