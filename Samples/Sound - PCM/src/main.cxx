#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "Sound PCM sample");
    
    // Load soundsJINGLE.PCM
    SRL::Debug::Print(1, 7, "Loading... GUN.PCM      ");
    SRL::Cd::File file("GUN.PCM");
    SRL::Sound::Pcm::RawPcm gun(&file, SRL::Sound::Pcm::PcmChannels::Mono, SRL::Sound::Pcm::Pcm8Bit, 15360);

    // Copter.wav is a large file, takes 600k loaded, and another 600k while loading
    // We can use allocation behavior of the PCM class to make it possible to load large files
    // First parameter specifies malloc used for parsing a file from CD, second malloc specifies where parsed file will be loaded
    SRL::Sound::Pcm::SetMemAllocationBehaviour(
        SRL::Sound::Pcm::PcmMalloc::HwRam,
        SRL::Sound::Pcm::PcmMalloc::LwRam);

    SRL::Debug::Print(1, 7, "Loading... COPTER.WAV    ");
    SRL::Cd::File copterFile("COPTER.WAV"); // Stereo, 16bit
    SRL::Sound::Pcm::WaveSound copter(&copterFile);

    // Now we can reset the allocation behaviour back to default
    SRL::Sound::Pcm::SetMemAllocationBehaviour(
        SRL::Sound::Pcm::PcmMalloc::HwRam,
        SRL::Sound::Pcm::PcmMalloc::HwRam);

    SRL::Debug::Print(1, 7, "Loading... TUDU8.WAV     ");
    SRL::Cd::File tudu8File("TUDU8.WAV"); // Mono, 8 bit
    SRL::Sound::Pcm::WaveSound tudu8(&tudu8File);

    // sound playlist
    uint8_t current = 0;

    // Main program loop
    uint32_t loopCount = 0;

	while(1)
	{
        SRL::Debug::Print(2,4, "%d   ", loopCount++);

        if (SRL::Sound::Pcm::IPcmFile::IsChannelFree(0))
        {
            loopCount = 0;

            switch (current++)
            {
            case 0:
                SRL::Debug::Print(1, 7, "Playing TUDU8.WAV, Mono, 8bit     ");
                tudu8.PlayOnChannel(0);
                break;
            case 1:
                SRL::Debug::Print(1, 7, "Playing COPTER.WAV, Stereo, 16bit ");
                copter.PlayOnChannel(0);
                break;
            case 2:
                SRL::Debug::Print(1, 7, "Playing GUN.PCM, Mono, 8bit       ");
                gun.PlayOnChannel(0);
                break;
            
            default:
                current = 0;
                break;
            }
        }


        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}
