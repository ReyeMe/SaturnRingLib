#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "Sound PCM sample");
    
    // Load sound
    SRL::Cd::File tudu8File("TUDU8.WAV");
    SRL::Sound::Pcm::WaveSound* tudu8 = new SRL::Sound::Pcm::WaveSound(&tudu8File);

    SRL::Cd::File file("GUN.PCM");
    SRL::Sound::Pcm::RawPcm gun(&file, SRL::Sound::Pcm::PcmChannels::Mono, SRL::Sound::Pcm::Pcm8Bit, 15360);

    tudu8->PlayOnChannel(0);


    SRL::Debug::Assert("PCM: 0x%x size: %d\nWAV: 0x%x size: %d", reinterpret_cast<uint32_t>(gun.data), gun.dataSize, reinterpret_cast<uint32_t>(tudu8->data), tudu8->dataSize);

    // sound playlist
    uint8_t current = 0;

    // Main program loop
    uint32_t loopCount = 0;

	while(1)
	{
        SRL::Debug::Print(4,4, "%d", loopCount++);

        /*if (SRL::Sound::Pcm::IPcmFile::IsChannelFree(0))
        {
            switch (current++)
            {
            case 0:
                SRL::Debug::Print(1, 4, "Playing TUDU8.WAV");
                tudu8.PlayOnChannel(0);
                break;
            case 1:
                SRL::Debug::Print(1, 4, "Playing TUDU.WAV");
                //tudu.PlayOnChannel(0);
                break;
            case 2:
                SRL::Debug::Print(1, 4, "Playing GUN.PCM");
                //gun.PlayOnChannel(0);
                break;
            
            default:
                current = 0;
                break;
            }
        }*/


        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}
