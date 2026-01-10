#include <srl.hpp>

using namespace SRL::Types;

/** @brief Movie sprite index store
 */
int32_t movieSprite;

/** @brief Called each time a new frame is decoded
 * @param player Movie player instance
 */
void FrameDecoded(SRL::CinepakPlayer& player)
{
    // Get total size of the frame data
    // Size is multiplied by 2 for RGB555 pixel format and by 4 for RGB24
    const auto size = player.GetResolution();
    const auto length = (size.Width * size.Height) << ((int)player.GetDepth() + 1);

    // Copy frame data to VDP1 texture
    DMA_ScuMemCopy(SRL::VDP1::Textures[movieSprite].GetData(), player.GetFrameData(), length);
}

/** @brief Called after whole movie plays
 * @param player Movie player instance
 */
void PlaybackCompleted(SRL::CinepakPlayer& player)
{
    // Repeat movie
    player.Play();
}

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "VDP1 Cinepak");

    // Initialize player
    SRL::CinepakPlayer player;
    player.OnFrame += FrameDecoded;
    player.OnCompleted += PlaybackCompleted;

    // Load movie
    // It is also possible to specify where ring and decode buffer are in RAM, whether they are in LW/HW or Cart ram,
    // as well as custom ring buffer size, by using second parameter of this function, see documentation for this function, and its second parameter
    player.LoadMovie("SKYBL.CPK");

    // Reserve video surface
    const auto resolution = player.GetResolution();
    movieSprite = SRL::VDP1::TryAllocateTexture(resolution.Width, resolution.Height, SRL::CRAM::TextureColorMode::RGB555, 0);

    // Clear the movie surface
    // Get total size of the frame data, in this case, the total length value contains number of uint8_t values
    const size_t length = (resolution.Width * resolution.Height) << ((int)player.GetDepth() + 1);

    // Initialize the texture in VDP1 RAM,
    // starting movie takes a bit and this will prevent us from seeing garbage on the screen
    for (size_t data = 0; data < length; data++)
    {
        ((uint8_t*)SRL::VDP1::Textures[movieSprite].GetData())[data] = 0;
    }

    // Play movie
    player.Play();

    // Main program loop
	while(1)
	{
        SRL::Debug::Print(1,28, "Time: %f seconds    ", player.GetTime());

        // Draw the frame
        SRL::Scene2D::DrawSprite(
            movieSprite,
            SRL::Math::Vector3D(0.0, 0.0, 500.0),
            SRL::Math::Vector2D(1.0, 1.0),
            SRL::Scene2D::ZoomPoint::Center);

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}
