#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "Sound PCM sample");
    
    // Load sound in HWRAM
    SRL::Debug::Print(1, 7, "Loading... GUN.PCM      ");
    SRL::Cd::File file("GUN.PCM");
    SRL::Sound::Pcm::RawPcm gun(&file, SRL::Sound::Pcm::PcmChannels::Mono, SRL::Sound::Pcm::Pcm8Bit, 15360);

    // We can also use 'new', 'lwnew', 'cartnew' keywords to decide where the PCM object and its contents would live
    // To free these objects we can just call 'delete', just like if using the classic 'new' keyword

    // Load sound in LWRAM
    SRL::Debug::Print(1, 7, "Loading... COPTER.WAV    ");
    SRL::Sound::Pcm::WaveSound* copter = lwnew SRL::Sound::Pcm::WaveSound("COPTER.WAV"); // Stereo, 16bit

    // Loads sound in HWRAM
    SRL::Debug::Print(1, 7, "Loading... TUDU8.WAV     ");
    SRL::Sound::Pcm::WaveSound* tudu8 = new SRL::Sound::Pcm::WaveSound("TUDU8.WAV"); // Mono, 8 bit

    // sound playlist
    uint8_t current = 0;

    // Main program loop
    uint32_t loopCount = 0;

	while(1)
	{
        SRL::Debug::Print(2,8, "%d   ", loopCount++);

        SRL::Debug::Print(2, 4, "Free HWRAM: %d/%d", SRL::Memory::HighWorkRam::GetFreeSpace(), SRL::Memory::HighWorkRam::GetSize());
        SRL::Debug::Print(2, 5, "Free LWRAM: %d/%d", SRL::Memory::LowWorkRam::GetFreeSpace(), SRL::Memory::LowWorkRam::GetSize());

        if (SRL::Sound::Pcm::IsChannelFree(0))
        {
            loopCount = 0;

            switch (current++)
            {
            case 0:
                SRL::Debug::Print(1, 7, "Playing TUDU8.WAV, Mono, 8bit     ");
                tudu8->PlayOnChannel(0);
                break;
            case 1:
                SRL::Debug::Print(1, 7, "Playing COPTER.WAV, Stereo, 16bit ");
                copter->PlayOnChannel(0);
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
