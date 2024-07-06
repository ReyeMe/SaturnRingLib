#pragma once

#include "srl_base.hpp"
#include "srl_memory.hpp"
#include "srl_cd.hpp"
#include "srl_math.hpp"

extern "C" { 
    #include <sega_snd.h>
    #include <sega_pcm.h>
}

/** @brief Sound handling
 */
namespace SRL::Sound
{
    /** @brief Sound hardware handling 
     */
    class Hardware
    {
    public:
        /** @brief Initialize sound hardware with SGL sound driver
         */
        static void Initialize()
        {
            // Turn sound off
            slSoundOffWait();

            Cd::File program("SDDRVS.TSK");
            Cd::File areaMap("BOOTSND.MAP");

            if (program.Exists() && areaMap.Exists())
            {
                // Prepare temporary buffers
                uint8_t* programBuffer = new uint8_t[program.Size.Bytes];
                uint8_t* areaMapBuffer = new uint8_t[areaMap.Size.Bytes];

                // Load driver
                program.LoadBytes(0, program.Size.Bytes, programBuffer);
                areaMap.LoadBytes(0, areaMap.Size.Bytes, areaMapBuffer);

                // Initialize sound
                SndIniDt init;
                SND_INI_PRG_ADR(init) = (uint16_t*)programBuffer;
                SND_INI_PRG_SZ(init) = (uint16_t)program.Size.Bytes;
                SND_INI_ARA_ADR(init) = (uint16_t*)areaMapBuffer;
                SND_INI_ARA_SZ(init) = (uint16_t)areaMap.Size.Bytes;
                SND_Init(&init);
	            SND_ChgMap(0);

                slInitSound(programBuffer, program.Size.Bytes, areaMapBuffer, areaMap.Size.Bytes);
                *(volatile unsigned char *)(0x25a004e1) = 0x0;
                CDC_CdInit(0x00, 0x00, 0x05, 0x0f);
                SND_SetCdDaLev(7, 7);
                
#if SRL_ENABLE_FREQ_ANALYSIS == 1
                Cd::File dsp("3BANDANA.EXB");

                if (dsp.Exists())
                {
                    uint8_t* dspProgram = new uint8_t[dsp.Size.Bytes];
                    dsp.LoadBytes(0, dsp.Size.Bytes, dspProgram);
                    SND_MoveData((uint16_t*)dspProgram, dsp.Size.Bytes, SND_KD_DSP_PRG, 1);
                    delete dspProgram;
                }
#endif

                // Clear buffers
                delete programBuffer;
                delete areaMapBuffer;
            }

            // Turn sound back on
            slSoundOnWait();
        }
    };

    /** @brief CD audio playback
     */
    class Cdda
    {
    private:

        /** @brief Last frame playback was at
         */
        inline static int32_t LastLocation = 0x00;

        /** @brief Track to play to
         */
        inline static uint16_t TargetTrack = 0xff;

        /** @brief Loop playback
         */
        inline static bool LoopPlayback = false;

    public:
    
		/** @brief Set CD playback volume
		 *  @param left Left channel audio volume (7 is max)
		 *  @param right Right channle audio volume (7 is max)
		 */
		constexpr static void SetVolume(const uint8_t left, const uint8_t right)
        {
            SND_SetCdDaLev(left, right);
        }

		/** @brief Set CD playback volume
		 *  @param volume Audio volume (7 is max)
		 */
		constexpr static void SetVolume(const uint8_t volume)
        {
            Cdda::SetVolume(volume, volume);
        }

		/** @brief Set CD playback stereo pan
		 *  @param left Left channel volume (7 is max)
		 *  @param right Right channel volume (7 is max)
		 */
		constexpr static void SetPan(const uint8_t left, const uint8_t right)
        {
            SND_SetCdDaPan(left, right);
        }

		/** @brief Play range of tracks
		 *  @param fromTrack Starting track
		 *  @param toTrack Ending track
		 *  @param loop Whether to play the range of track again after it ends
		 */
		static void Play(const uint16_t fromTrack, const uint16_t toTrack, const bool loop)
        {
            Cdda::TargetTrack = toTrack;
            Cdda::LoopPlayback = loop;

            // Prepare playback
            CdcPly ply;

            // Start track
            CDC_PLY_STYPE(&ply) = CDC_PTYPE_TNO;
            CDC_PLY_STNO(&ply) = fromTrack;
            CDC_PLY_SIDX(&ply) = 1;

            // End track
            CDC_PLY_ETYPE(&ply) = CDC_PTYPE_TNO;
            CDC_PLY_ETNO(&ply) = toTrack;
            CDC_PLY_EIDX(&ply) = 1;

            // Set loop mode
            CDC_PLY_PMODE(&ply) = CDC_PM_DFL | (loop ? 0xf : 0); // 0xf = infinite repetitions

            CDC_CdPlay(&ply);
        }

		/** @brief Play single CD track
		 *  @param track Track number (in jo starts at 3)
		 *  @param loop Whether to loop track after it ends
		 */
		constexpr static void PlaySingle(const uint16_t track, const bool loop)
		{
            Cdda::Play(track, track, loop);
		}

        /** @brief Resume audio playback from where it left off
         */
        static void Resume()
        {
            // Check if we have already played something
            if (Cdda::LastLocation == 0 || Cdda::TargetTrack == 0xff)
            {
                return;
            }

            // Get TOC
            Cd::TableOfContents toc = Cd::TableOfContents::GetTable();

            // Prepare playback struct
            CdcPly ply;

            // Start of the playback address
            CDC_PLY_STYPE(&ply) = CDC_PTYPE_FAD; // track number
            CDC_PLY_SFAD(&ply) = Cdda::LastLocation;

            if (Cdda::TargetTrack + 1 < Cd::MaxTrackCount &&
                toc.Tracks[Cdda::TargetTrack + 1].GetType() != Cd::TableOfContents::TrackType::Unknown)
            {
                // End of the playback address is start of next track
                CDC_PLY_ETYPE(&ply) = CDC_PTYPE_FAD;
                CDC_PLY_EFAS(&ply) = toc.Tracks[Cdda::TargetTrack + 1].FrameAddress - toc.Tracks[Cdda::TargetTrack].FrameAddress;
            }
            else
            {
                // End of the playback is end of the disk
                CDC_PLY_ETYPE(&ply) = CDC_PTYPE_DFL;
            }

            // Playback mode
            CDC_PLY_PMODE(&ply) = CDC_PM_DFL | (Cdda::LoopPlayback ? 0xf : 0); // 0xf = infinite repetitions

            // Start playback
            CDC_CdPlay(&ply);
        }

		/** @brief Stop or pause CD music playback
		 */
		static void StopPause()
        {
            // Get current address
            CdcStat stat;
            CDC_GetCurStat(&stat);

            // Restore address
            CdcPos poswk;
            CDC_POS_PTYPE(&poswk) = CDC_PTYPE_DFL;
            CDC_CdSeek(&poswk);

            // Last playback address
            Cdda::LastLocation = stat.report.fad;
        }

        /** @brief CD audio analysis
         */
        class Analysis
        {
        public:

            /** @brief Total volume analysis
             */
            struct TotalVolume
            {
                /** @brief Total volume of left channel
                 */
                uint16_t LeftChannel;

                /** @brief Total volume of right channel
                 */
                uint16_t RightChannel;
            };
            
#if SRL_ENABLE_FREQ_ANALYSIS == 1 || DOXYGEN
            /** @brief Frequency volume analysis
             * @note This functionality can be enabled by setting 'SRL_ENABLE_FREQ_ANALYSIS' to 1 in makefile.
             */
            struct FrequencyVolume
            {
                /** @brief Volume of highs
                 */
                uint16_t Highs;

                /** @brief Volume of mids
                 */
                uint16_t Mids;

                /** @brief Volume of lows
                 */
                uint16_t Lows;
            };
#endif

            /** @brief Start volume analysis
             */
            constexpr static void Start()
            {
#if SRL_ENABLE_FREQ_ANALYSIS == 1 || DOXYGEN
                SND_ChgEfct(1);
#endif
                SND_StartVlAnl();
            }
        
            /** @brief End volume analysis
             */
            constexpr static void End()
            {
                SND_StopVlAnl();
            }

#if SRL_ENABLE_FREQ_ANALYSIS == 1 || DOXYGEN
            /** @brief Get the Frequency volume analysis
             * @note This functionality can be enabled by setting 'SRL_ENABLE_FREQ_ANALYSIS' to 1 in makefile.
             * @note Special DSP effect program must be loaded for this to work, in SRL it will be loaded into effect slot 1.
             * @note Analysis is updated every 16ms
             * @return Frequency volume analysis
             */
            static FrequencyVolume GetFrequencyVolume()
            {
                FrequencyVolume frequency;
                SND_GetAnlHzVl((SndCdHzSrVl*)&frequency);
                return frequency;
            }
#endif

            /** @brief Get the total volume analysis
             * @note Analysis is updated every 16ms
             * @return Volume analysis for left and right channel
             */
            static TotalVolume GetTotalVolume()
            {
                TotalVolume total;
                SND_GetAnlTlVl(&total.LeftChannel, &total.RightChannel);
                return total;
            }
        };
    };
    
    /** @brief PCM playback
     */
    class Pcm
    {
    public:
        /** @brief PCM audio channels
         */
        enum PcmChannels
        {
            /** @brief Single channel
             */
            Mono = _Mono,

            /** @brief Dual channel
             */
            Stereo = _Stereo
        };

        /** @brief PCM audio bit depth
         */
        enum PcmBitDepth
        {
            /** @brief 8-bit data
             * @details sound format is pcm_s8
             */
            Pcm8Bit = _PCM8Bit,

            /** @brief 16-bit data
             * @details sound format is pcm_s16be
             */
            Pcm16Bit = _PCM16Bit
        };

        /** @brief Malloc types to use for loading PCM
         */
        enum PcmMalloc : uint16_t
        {
            /** @brief Will use the same malloc as what was used for object creation
             */
            Default = 0,

            /** @brief High Work RAM malloc, (main system RAM)
             */
            HwRam = 1,

            /** @brief Low work RAM malloc
             */
            LwRam = 2,

            /** @brief Cart RAM malloc
             */
            CartRam = 3
        };

    private:
        
        /** @brief This malloc will be used to allocate memory for parsing a file
         */
        static inline Pcm::PcmMalloc WorkMalloc = Pcm::PcmMalloc::Default;

        /** @brief This malloc will be used to allocated memory for a parsed file
         */
        static inline Pcm::PcmMalloc DataMalloc = Pcm::PcmMalloc::Default;
        
        /** @brief Logarithmic table
         */
        static inline const int8_t LogTable[] = {
        /* 0 */		0, 
        /* 1 */		1, 
        /* 2 */		2, 2, 
        /* 4 */		3, 3, 3, 3, 
        /* 8 */		4, 4, 4, 4, 4, 4, 4, 4, 
        /* 16 */	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
        /* 32 */	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
                    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
        /* 64 */	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
                    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
                    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
                    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
        /* 128 */	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
                    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
                    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
                    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
                    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
                    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
                    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
                    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
        };

        /* 1,3,4,5,10 bit mask */
        #define PCM_MSK1(a)	((a)&0x0001)
        #define PCM_MSK3(a)	((a)&0x0007)
        #define PCM_MSK4(a)	((a)&0x000F)
        #define PCM_MSK5(a)	((a)&0x001F)
        #define PCM_MSK10(a) ((a)&0x03FF)

        /** @brief SCSP basic frequency 44.1[kHz]
         */
        #define PCM_SCSP_FREQUENCY (44100L)

        /** @brief Calculating the octave value
         */
        #define PCM_CALC_OCT(samplingRate) 											\
            ((int32_t)Pcm::LogTable[PCM_SCSP_FREQUENCY / ((samplingRate) + 1)])

        /** @brief Calculating shift reference frequency
         */
        #define PCM_CALC_SHIFT_FREQ(oct)											\
            (PCM_SCSP_FREQUENCY >> (oct))

        /** @brief FNS calculation
         */
        #define PCM_CALC_FNS(samplingRate, shiftFreq)								\
            ((((samplingRate) - (shiftFreq)) << 10) / (shiftFreq))

        /** @brief Pitch value
         */
        #define PCM_SET_PITCH_WORD(oct, fns)										\
		    ((uint16_t)((PCM_MSK4(-(oct)) << 11) | PCM_MSK10(fns)))
        
        /** @brief Available PCM channels
         */
        static inline PCM Channels[4] =
        {
            { _Stereo | _PCM16Bit , 0, 127, 0, 0, 0, 0, 0, 0 },
            { _Stereo | _PCM16Bit , 2, 127, 0, 0, 0, 0, 0, 0 },
            { _Stereo | _PCM16Bit , 4, 127, 0, 0, 0, 0, 0, 0 },
            { _Stereo | _PCM16Bit , 6, 127, 0, 0, 0, 0, 0, 0 }
        };

        /** @brief Look for unused PCM channel
         * @return int8_t Channel number if any found, or -1 if not
         */
        static inline int8_t FindChannel()
        {
            for (int channel = 0; channel < 4; channel++)
            {
                if (!slPCMStat(&Pcm::Channels[channel]))
                {
                    return channel;
                }
            }
            
            return -1;
        }

    public:

        /** @brief Set the Mem Allocation Behaviour for loading PCM files
         * @param work This malloc will be used to allocate memory for parsing a file
         * @param data This malloc will be used to allocate memory for a parsed file
         */
        static inline void SetMemAllocationBehaviour(const Pcm::PcmMalloc work, const Pcm::PcmMalloc data)
        {
            Pcm::WorkMalloc = work;
            Pcm::DataMalloc = data;
        }

        /** @brief Base PCM file interface
         */
        class IPcmFile
        {
        protected:
        
            /** @brief Sound data
             */
            int8_t* data = nullptr;

            /** @brief Number of bytes to play
             */
            uint32_t dataSize = 0;

            /** @brief Sound mode (_Mono or _Stereo)
             */
            uint8_t mode = _Mono;

            /** @brief Bit depth (_PCM8Bit or _PCM16Bit)
             */
            uint8_t depth = _PCM8Bit;

            /** @brief Sound sample rate
             */
            uint16_t sampleRate = 11020;

            /** @brief Loads new sound from a file
             * @param file Sound file (.wav)
             */
            IPcmFile() { }

            /** @brief Free PCM sound data
             */
            ~IPcmFile() { }

            /** @brief Use specified malloc to allocate memory block
             * @param malloc Malloc to use
             * @param size Size in bytes to allocate
             * @return Pointer to allocated block
             */
            void* AllocateWithBehaviour(const Pcm::PcmMalloc malloc, size_t size)
            {
                // User set malloc
                switch (malloc)
                {
                case Pcm::PcmMalloc::HwRam:
                    return SRL::Memory::HighWorkRam::Malloc(size);
                
                case Pcm::PcmMalloc::LwRam:
                    return SRL::Memory::LowWorkRam::Malloc(size);

                case Pcm::PcmMalloc::CartRam:
                    return SRL::Memory::CartRam::Malloc(size);

                default:
                    break;
                }

                // Figure out what malloc we have to use
                uint32_t address = reinterpret_cast<uint32_t>(this);

                if (address >= 0x06000000 && address <= 0x07FFFFFF)
                {
                    return SRL::Memory::HighWorkRam::Malloc(size);
                }
                else if (address >= 0x00200000 && address <= 0x002FFFFF)
                {
                    return SRL::Memory::LowWorkRam::Malloc(size);
                }
                else if (SRL::Memory::CartRam::InRange(this))
                {
                    return SRL::Memory::CartRam::Malloc(size);
                }

                SRL::Debug::Assert("Could not find correct allocator for memory at 0x%x!", reinterpret_cast<uint32_t>(this));
                return nullptr;
            }

        public:

            /** @brief Try to play a sound on a specific channel
             * @param channel Sound channel to play sound on (0-3)
             * @param volume Playback volume (0-127)
             * @param pan Audio channel panning (-127 to 127)
             * @return true if sound was played
             */
            bool PlayOnChannel(uint8_t channel, uint8_t volume = 127, int8_t pan = 0)
            {
                if (!slPCMStat(&Pcm::Channels[channel]))
                {
                    uint16_t octave = PCM_CALC_OCT(this->sampleRate);
                    uint16_t shift = PCM_CALC_SHIFT_FREQ(octave);
                    uint16_t fns = PCM_CALC_FNS(this->sampleRate, shift);

                    Pcm::Channels[channel].mode = this->mode | this->depth;
                    Pcm::Channels[channel].pitch = PCM_SET_PITCH_WORD(octave, fns);
                    Pcm::Channels[channel].level = volume;
                    Pcm::Channels[channel].pan = pan;

                    slPCMOn(&Pcm::Channels[channel], this->data, this->dataSize);
                    return true;
                }

                return false;
            }

            /** @brief Try to play sound on the first free channel
             * @param volume Playback volume (0-127)
             * @param pan Audio channel panning (-127 to 127)
             * @return Channel number if sound was played or -1 if it was not
             */
            int8_t Play(uint8_t volume = 127, int8_t pan = 0)
            {
                int8_t channel = Pcm::FindChannel();
                
                if (channel >= 0 && this->PlayOnChannel(channel, volume, pan))
                {
                    return channel;
                }

                return -1;
            }
        };

        /** @brief Raw PCM sound
         * @details Data format should be either pcm_s16be or pcm_s8. To do stereo, right channel must occupy first half of the file and left channel second half.
         */
        class RawPcm : public Pcm::IPcmFile
        {
        public:

            /** @brief Initializes a new PCM audio handle
             * @param file PCM file
             * @param channels Number of channels (Mono or Stereo)
             * @param depth Bit depth (8 or 16 bit)
             * @param sampleRate Sample rate (in Hz)
             */
            RawPcm(Cd::File* file, const Pcm::PcmChannels channels, const Pcm::PcmBitDepth depth, const uint16_t sampleRate)
            {
                if (file == nullptr)
                {
                    Debug::Assert("FIle cannot be NULL!");
                    return;
                }

                // slPCMOn won't play samples shorter than 0x900
                size_t clampedLength = SRL::Math::Max<uint32_t>(file->Size.Bytes, 0x900);
                
                this->data = (int8_t*)this->AllocateWithBehaviour(Pcm::DataMalloc, clampedLength);
                this->dataSize = clampedLength;
                this->mode = (uint8_t)channels;
                this->depth = (uint8_t)depth;
                this->sampleRate = sampleRate;

                for (size_t byte = file->Size.Bytes; byte < clampedLength; byte++) this->data[byte] = 0x00;
                int32_t loaded = file->LoadBytes(0, file->Size.Bytes, this->data);

                if (loaded != file->Size.Bytes)
                {
                    Debug::Assert("Reached end of a file! Got %dbytes instead of %dbytes", loaded, file->Size.Bytes);
                }
            }

            /** @brief Destroy the Raw Pcm object
             */
            ~RawPcm()
            {
                if (this->data != nullptr) delete this->data;
                this->dataSize = 0;
            }
        };
                
        /** @brief Wave sound effect
         */
        class WaveSound : public Pcm::IPcmFile
        {
        private:

            /** @brief Deserialize number
             * @param buf Value buffer
             * @return Deserialized value
             */
            constexpr inline static uint16_t SwapEndianess16(uint16_t value)
            {
                return ((value & 0xff) << 8) | ((value & 0xff00) >> 8);
            }

            /** @brief Deserialize number
             * @param buf Value buffer
             * @return Deserialized value
             */
            constexpr inline static uint32_t SwapEndianess32(uint32_t value)
            {
                return ((value>>24)&0xff) |
                    ((value<<8)&0xff0000) |
                    ((value>>8)&0xff00) |
                    ((value<<24)&0xff000000);
            }

            /** @brief RIFF data chunk
             */
            struct RiffChunkHeader
            {
                uint8_t Id[4];
                uint32_t Size;	
            };

            /** @brief LIST data chunk
             */
            struct ListChunkHeader
            {
                uint8_t Id[4];
                uint32_t Size;
                uint8_t Type[4];
            };

            /** @brief File data types
             */
            enum WaveTypes : uint16_t
            {
                FormatPCM = 0x0001,
                FormatFloat = 0x0003,
                FormatALAW  = 0x0006,
                FormatMULAW = 0x0007,
                FormatExtensible = 0xFFFE
            };

            /** @brief Wave file header
             */
            struct WaveHeader
            {
                /** @brief Marks the file as a riff file.
                 */
                char Riff[4];

                /** @brief Size of the overall file.
                 */
                uint32_t FileSize;

                /** @brief File Type Header. For our purposes, it always equals “WAVE”.
                 */
                char Wave[4];

                /** @brief Format chunk.
                 */
                RiffChunkHeader FormatChunkHeader;

                /** @brief Type of format (1 is PCM) - 2 byte integer
                 */
                uint16_t Type;

                /** @brief Number of Channels - 2 byte integer
                 */
                uint16_t Channels;

                /** @brief Sample Rate - 32 byte integer. Common values are 44100 (CD), 48000 (DAT).
                 * Sample Rate = Number of Samples per second, or Hertz.
                 */
                uint32_t SampleRate;

                /** @brief (Sample Rate * BitsPerSample * Channels) / 8.
                 */
                uint32_t BitRate;

                /** @brief (BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo
                 */
                uint16_t BitPerChannel;

                /** @brief Bits per sample
                 */
                uint16_t BitPerSample;

                /** @brief Data chunks
                 */
                ListChunkHeader Info;
            };
            
            /** @brief Load PCM data
             * @param waveData Wave pcm data
             * @param size Length of the data block
             * @param is8bit Is audio 8 bits per sample?
             * @param stereo Is audio interleaved left and right channel?
             */
            void LoadPcmData(uint8_t* waveData, uint32_t size, bool is8bit, bool stereo)
            {
                // slPCMOn won't play samples shorter than 0x900
                size_t clampedLength = SRL::Math::Max<uint32_t>(size, 0x900);
                this->data = (int8_t*)this->AllocateWithBehaviour(Pcm::DataMalloc, clampedLength);
                this->dataSize = clampedLength;

                if (this->data == nullptr)
                {
                    Debug::Assert("Out of memory! Sound too large.");
                    return;
                }

                if (is8bit)
                {
                    if (stereo)
                    {
                        for (uint32_t sample = 0; sample < this->dataSize >> 1; sample++)
                        {
                            int8_t left = ((uint8_t)*(waveData++)) - 128;
                            int8_t right = ((uint8_t)*(waveData++)) - 128;
                            this->data[sample] = left;
                            this->data[this->dataSize + sample] = 0;
                        }
                    }
                    else
                    {
                        for (uint32_t sample = 0; sample < size; sample++)
                        {
                            this->data[sample] = ((uint8_t)*(waveData++)) - 128;
                        }
                    }
                }
                else
                {
                    if (stereo)
                    {
                        size_t half = size >> 2;
                        
                        for (size_t sample = 0; sample < half; sample++)
                        {
                            uint8_t* samples = (uint8_t*)(waveData);

                            int16_t left = (int16_t)(samples[0] | samples[1] << 8);
                            int16_t right = (int16_t)(samples[2] | samples[3] << 8);

                            ((int16_t*)this->data)[sample] = right;
                            ((int16_t*)this->data)[half + sample] = left;

                            waveData = waveData + 4;
                        }
                    }
                    else
                    {
                        for (size_t sample = 0; sample < size >> 1; sample++)
                        {
                            ((int16_t*)this->data)[sample] = (((uint8_t)*(waveData++)) | ((uint8_t)*(waveData++) << 8));
                        }
                    }
                }

                // Fill
                for (uint32_t byte = size; byte < clampedLength; byte++)
                {
                    this->data[byte] = 0x00;
                }
            }

        public:

            /** @brief Initializes a new wave sound
             * @param file Sound file
             */
            WaveSound(Cd::File* file)
            {
                if (file == nullptr)
                {
                    Debug::Assert("FIle cannot be NULL!");
                    return;
                }

                uint8_t* waveData = (uint8_t*)this->AllocateWithBehaviour(Pcm::WorkMalloc, file->Size.Bytes);
                int32_t loaded = file->LoadBytes(0, file->Size.Bytes, waveData);
                WaveHeader* header = (WaveHeader*)waveData;

                if (loaded != file->Size.Bytes)
                {
                    Debug::Assert("Reached end of a file! Got %dbytes instead of %dbytes", loaded, file->Size.Bytes);
                    delete waveData;
                    return;
                }

                // Fix endianess
                header->Type = WaveSound::SwapEndianess16(header->Type);
                header->Channels = WaveSound::SwapEndianess16(header->Channels);
                header->BitPerSample = WaveSound::SwapEndianess16(header->BitPerSample);
                header->SampleRate = WaveSound::SwapEndianess32(header->SampleRate);

                // Validate format
                if (header->Riff[0] != 'R' || header->Riff[1] != 'I' || header->Riff[2] != 'F' || header->Riff[3] != 'F' ||
                    header->Wave[0] != 'W' || header->Wave[1] != 'A' || header->Wave[2] != 'V' || header->Wave[3] != 'E')
                {
                    Debug::Assert("Not a valid wave file!");
                    delete waveData;
                    return;
                }
                else if (header->Type != 1 || 
                    header->Channels > 2 || header->Channels < 1 ||
                    (header->BitPerSample != 8 && header->BitPerSample != 16))
                {
                    Debug::Assert("Not supported format!");
                    delete waveData;
                    return;
                }

                if (header != nullptr)
                {
                    this->sampleRate = header->SampleRate;
                    this->mode = header->Channels == 1 ? _Mono : _Stereo;
                    this->depth = header->BitPerSample == 8 ? _PCM8Bit : _PCM16Bit;
                }

                // Skip info chunks
                uint8_t* dataChunk = (uint8_t*)((uint8_t*)&header->Info);

                if (dataChunk[0] != 'd' || dataChunk[1] != 'a' || dataChunk[2] != 't' || dataChunk[3] != 'a')
                {
                    header->Info.Size = WaveSound::SwapEndianess32(header->Info.Size);
                    dataChunk = (uint8_t*)(((uint8_t*)&header->Info) + header->Info.Size + 8);
                }
                
                if (dataChunk[0] == 'd' && dataChunk[1] == 'a' && dataChunk[2] == 't' && dataChunk[3] == 'a')
                {
                    uint32_t size = dataChunk[4] | (dataChunk[5] << 8) | (dataChunk[6] << 16) | (dataChunk[7] << 24);
                    
                    switch ((WaveTypes)header->Type)
                    {
                    case 1:
                        this->LoadPcmData(dataChunk + 8, size, header->BitPerSample == 8, header->Channels != 1);
                        break;
                    
                    default:
                        Debug::Assert("Not supported format!");
                        break;
                    }
                }
                else
                {
                    Debug::Assert("Data not found!");
                }
                
                delete waveData;
            }

            /** @brief Destroy loaded sound
             */
            ~WaveSound()
            {
                if (this->data != nullptr) delete this->data;
                this->data = nullptr;
                this->dataSize = 0;
            }
        };
    
        
        /** @brief Try to play a sound on a specific channel
         * @param pcm Sound effect to play
         * @param channel Sound channel to play sound on (0-3)
         * @param volume Playback volume (0-127)
         * @param pan Audio channel panning (-127 to 127)
         * @return true if sound was played
         */
        static bool PlayOnChannel(Pcm::IPcmFile& pcm, uint8_t channel, uint8_t volume = 127, int8_t pan = 0)
        {
            return pcm.PlayOnChannel(channel, volume, pan);
        }

        /** @brief Try to play sound on the first free channel
         * @param pcm Sound effect to play
         * @param volume Playback volume (0-127)
         * @param pan Audio channel panning (-127 to 127)
         * @return Channel number if sound was played or -1 if it was not
         */
        static int8_t Play(Pcm::IPcmFile& pcm, uint8_t volume = 127, int8_t pan = 0)
        {
            return pcm.Play(volume, pan);
        }

        /** @brief Check whether sound channel is currently free
         * @param channel Sound channel to check (0-3)
         * @return true if channel is not playing anything
         */
        static bool IsChannelFree(uint8_t channel)
        {
            return channel < 4 && !slPCMStat(&Pcm::Channels[channel]);
        }

        /** @brief Stop currently playing sound
         * @param channel Sound channel to stop (0-3)
         * @return true if successfully stopped
         */
        static bool StopSound(uint8_t channel)
        {
            return channel < 4 && slPCMOff(&Pcm::Channels[channel]);
        }

        /** @brief Set the Volume & Panning
         * @param channel Sound channel to modify (0-3)
         * @param volume Playback volume (0-127)
         * @param pan Audio channel panning (-127 to 127)
         */
        static void SetVolumePan(uint8_t channel, uint8_t volume, int8_t pan = 0)
        {
            if (channel < 4)
            {
                Pcm::Channels[channel].pan = pan;
                Pcm::Channels[channel].level = volume;
                slPCMParmChange(&Pcm::Channels[channel]);
            }
        }
    };
}