#pragma once

#include "srl_base.hpp"
#include "srl_cd.hpp"

extern "C" { 
    #include <sega_snd.h>
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


    /** @brief TODO: Implement!
     */
    class PCM
    {
    private:

    public:

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
}