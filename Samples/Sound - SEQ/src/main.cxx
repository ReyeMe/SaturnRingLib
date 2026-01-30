#define SRL_ENABLE_AUDIO_SEQ_SUPPORT 1
#define SoundMem 0x25a0b000 // The playback data is stored starting from address 0xB000 in the MC68000 memory area (0x25A0B000 in the program) https://antime.kapsi.fi/sega/files/ST-237-R1-051795.pdf
#include <srl.hpp>

// SEQ sample (Sega MIDI-style music playback) for SRL
//   Heavely inspired by the Jo Engine sample made by jfsantos (João Felipe Santos) => https://github.com/jfsantos/mid2seq
//   Special thanks to Andreas Scholl for helping me understand the MAP structure

// MAP CMD IDs
#define MAP_CMD_SEQ 0x11 // for SEQ bank N°1
#define MAP_CMD_TON 0x01 // for TON bank N°1

uint32_t GetOffsetFromMAP(const uint16_t* pMap, size_t wordCount, uint8_t targetCommand)
{
    const uint16_t* end = pMap + wordCount;
    while (pMap + 4 <= end) // One entry = 4 words (8 bytes)
    {
        if (pMap[0] == 0xFFFF) // Detect End
            break;

        uint8_t cmd = (uint8_t)(pMap[0] >> 8);
        if (cmd == targetCommand)
        {
            // Big-Endian address: low 8 bits of pMap[0] hold the high byte (bits 23-16), pMap[1] holds the middle and low bytes (bits 15-0)
            uint32_t addr_high = (uint32_t)(pMap[0] & 0x00FF) << 16;
            uint32_t addr_low  = (uint32_t)pMap[1];
            uint32_t addr = addr_high | addr_low;

            return (addr) - 0xB000; // Relative offset (subtract MC68000 base 0xB000)
        }

        pMap += 4;
    }

    return 0xFFFFFFFF; // Security if not found
}

void LoadSEQandTON(const char* seqName, const char* tonName)
{
    slBGMOff(); // Security: Always stop music before touching sound RAM

    // Open the MAP/SEQ/TON files
    SRL::Cd::File mapFILE("CUSTOM.MAP");
    SRL::Cd::File seqFILE(seqName); // SONG_07P.SEQ from Funky Fantasy (J) patched to only use Bank 1 | BGM01.SEQ from https://github.com/jfsantos/mid2seq | SONIC.SEQ and AFTERB.SEQ (converted MIDI by Frogbull)
    SRL::Cd::File tonFILE(tonName); // SONG_07.TON from Funky Fantasy (J) | MUSIC.TON and SE.TON from Shining Wisdom (US) | BGM01.TON from https://github.com/jfsantos/mid2seq

    // Load MAP in RAM
    size_t mapBytes = mapFILE.Size.Bytes;
    if (mapBytes % 2 != 0)
    {
        // Invalid MAP
        seqFILE.Close(); tonFILE.Close(); mapFILE.Close();
        return;
    }

    size_t mapWordsCount = mapBytes/2;
    uint16_t* mapData = new uint16_t[mapWordsCount];
    mapFILE.LoadBytes(0, mapBytes, (uint8_t*)mapData);

    // Get Offsets from the MAP file
    uint32_t seqOffset = GetOffsetFromMAP(mapData, mapWordsCount, MAP_CMD_SEQ);
    uint32_t tonOffset = GetOffsetFromMAP(mapData, mapWordsCount, MAP_CMD_TON);

    if (seqOffset == 0xFFFFFFFF || tonOffset == 0xFFFFFFFF) // Problem with the offset detections
    {
        delete[] mapData;
        seqFILE.Close();
        tonFILE.Close();
        mapFILE.Close();
        return;
    }

    // Load SEQ/TON in RAM
    uint8_t* seqBuf = new uint8_t[seqFILE.Size.Bytes];
    uint8_t* tonBuf = new uint8_t[tonFILE.Size.Bytes];

    seqFILE.LoadBytes(0, seqFILE.Size.Bytes, seqBuf);
    tonFILE.LoadBytes(0, tonFILE.Size.Bytes, tonBuf);

    // Copy to Sound RAM via DMA
    slDMACopy(seqBuf, (uint8_t*)SoundMem + seqOffset, seqFILE.Size.Bytes); // SEQ data
    slDMACopy(tonBuf, (uint8_t*)SoundMem + tonOffset, tonFILE.Size.Bytes); // TON data

    // Clean/Free RAM
    delete[] seqBuf;
    delete[] tonBuf;
    delete[] mapData;
    seqFILE.Close();
    tonFILE.Close();
    mapFILE.Close();
}

static const char* SEQ_LIST[] =
{
    "BGM01.SEQ",
    "SONIC.SEQ",
    "FF7.SEQ",
    "CRASHB.SEQ",
    "SONG_07P.SEQ",
    "AFTERB.SEQ",
    "DAYTO.SEQ"
};

static const char* TON_LIST[] =
{
    "BGM01.TON",
    "MUSIC.TON",
    "SONG_07.TON",
    "SE.TON"
};

int main()
{
    SRL::Core::Initialize(SRL::Types::HighColor::Colors::Black);

    uint8_t seqIndex = 0;
    uint8_t tonIndex = 0;

    static uint8_t SEQ_COUNT = (int)(sizeof(SEQ_LIST) / sizeof(SEQ_LIST[0]));
    static uint8_t TON_COUNT = (int)(sizeof(TON_LIST) / sizeof(TON_LIST[0]));

    // Load and initialize a Saturn SEQ music track (Sega MIDI-style music playback)
    LoadSEQandTON(SEQ_LIST[seqIndex], TON_LIST[tonIndex]);

    // Initialize gamepad
    SRL::Input::Digital pad(0);

    // Load PCM sound to test if PCM could be played while SEQ music is also played
    SRL::Cd::File file("GUN.PCM");
    SRL::Sound::Pcm::RawPcm gun(&file, SRL::Sound::Pcm::PcmChannels::Mono, SRL::Sound::Pcm::Pcm8Bit, 15360);

    bool seqPaused = false;
    bool seqPlay = false;

    uint8_t seqPriority = 0; // Priority (range only from 0 to 31. The larger the value, the higher the priority)
    uint8_t seqVolume = 127; // Volume (range only from 0 to 127. 0 = 0% | 127 = 100%)
    uint8_t seqRate = 0; // Rate (range from 0 to 255)
    int16_t musicTempo = 0; // Tempo (a value ranging from -32768 to 32767 for the parameter Tempo. 0 = Normal Tempo)
    slBGMTempo(musicTempo);

    bool ReloadSEQorTON = false;

    while (true)
    {
        SRL::Debug::Print(1,1, "Sound SEQ/TON sample");

        SRL::Debug::Print(1,5, "Press Up/Down to change the Volume");
        if (pad.WasPressed(SRL::Input::Digital::Button::Up))
        {
            if (seqVolume < 127)
            {
                seqVolume++;
                slBGMFade(seqVolume, 0);
            }
        }
        else if (pad.WasPressed(SRL::Input::Digital::Button::Down))
        {
            if (seqVolume > 0)
            {
                seqVolume--;
                slBGMFade(seqVolume, 0);
            }
        }
        SRL::Debug::Print(3,6, "Current Volume:    ");
        SRL::Debug::Print(3+16,6, "%d", seqVolume);

        SRL::Debug::Print(1,9, "Press Left/Right to change the Tempo");
        if (pad.WasPressed(SRL::Input::Digital::Button::Left))
        {
            if (musicTempo > INT16_MIN+16)
            {
                musicTempo -= 16;
                slBGMTempo(musicTempo);
            }
        }
        else if (pad.WasPressed(SRL::Input::Digital::Button::Right))
        {
            if (musicTempo < INT16_MAX-16)
            {
                musicTempo += 16;
                slBGMTempo(musicTempo);
            }
        }
        SRL::Debug::Print(3,10, "Current Tempo:        ");
        SRL::Debug::Print(3+15,10, "%d", musicTempo);

        SRL::Debug::Print(1,13, "Press START to Stop/Start the Music");
        if (pad.WasPressed(SRL::Input::Digital::Button::START))
        {
            if (seqPlay)
            {
                seqPlay = false;
                slBGMOff();
            }
            else
            {
                seqPlay = true;
                slBGMOn((1 << 8) + 0, 0, seqVolume, 0);
                slBGMTempo(musicTempo);
            }
        }

        if (seqPlay)
        {
            SRL::Debug::Print(3,14, "SEQ Music Playing...");
        }
        else
        {
            SRL::Debug::Print(3,14, "No SEQ Music...     ");
        }

        if (pad.WasPressed(SRL::Input::Digital::Button::X))
        {
            seqIndex++;
            if (seqIndex >= SEQ_COUNT)
            {
                seqIndex = 0;
            }
            ReloadSEQorTON = true;
        }
        if (pad.WasPressed(SRL::Input::Digital::Button::Y))
        {
            tonIndex++;
            if (tonIndex >= TON_COUNT)
            {
                tonIndex = 0;
            }
            ReloadSEQorTON = true;
        }

        if (ReloadSEQorTON)
        {
            LoadSEQandTON(SEQ_LIST[seqIndex], TON_LIST[tonIndex]);
            ReloadSEQorTON = false;
            seqPlay = false;
        }

        SRL::Debug::Print(1, 17, "Press X/Y to change SEQ/TON");
        SRL::Debug::Print(3, 18, "SEQ:                         ");
        SRL::Debug::Print(3, 18, "SEQ: %s", SEQ_LIST[seqIndex]);
        SRL::Debug::Print(3, 19, "TON:                         ");
        SRL::Debug::Print(3, 19, "TON: %s", TON_LIST[tonIndex]);

        SRL::Debug::Print(1,22, "Press B to Pause/Resume SEQ Music");
        if (pad.WasPressed(SRL::Input::Digital::Button::B))
        {
            if (!seqPaused)
            {
                slBGMPause();
                seqPaused = true;
            }
            else
            {
                slBGMCont();
                seqPaused = false;
            }
        }

        SRL::Debug::Print(1,24, "Press A to Play PCM Gun Shot       ");
        if (pad.WasPressed(SRL::Input::Digital::Button::A))
        {
            SRL::Debug::Print(1,24, "Press A to Play PCM Gun Shot -=<>=-");
            gun.PlayOnChannel(0);
        }

        SRL::Debug::Print(1,26, "Press Z to Reset Volume & Tempo");
        if (pad.WasPressed(SRL::Input::Digital::Button::Z))
        {
            seqVolume = 127;
            slBGMFade(seqVolume, 0);
            musicTempo = 0;
            slBGMTempo(musicTempo);
        }

        SRL::Core::Synchronize();
    }

    return 0;
}
