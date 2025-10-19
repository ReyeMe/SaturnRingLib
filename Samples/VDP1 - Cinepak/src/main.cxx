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
    // It is also possible to specify custom ring buffer size, 
    // if the default is not enough or too much, see documentation for this function, and its second parameter
    player.LoadMovie("SKYBL.CPK");

    // Reserve video surface
    const auto resolution = player.GetResolution();
    movieSprite = SRL::VDP1::TryAllocateTexture(resolution.Width, resolution.Height, SRL::CRAM::TextureColorMode::RGB555, 0);

    // Clear the movie surface
    // Get total size of the frame data
    const auto size = player.GetResolution();
    const auto is15bit = player.GetDepth() == SRL::CinepakPlayer::ColorDepth::RGB15;
    const size_t length = size.Width * size.Height;

    for (size_t pixel = 0; pixel < length; pixel++)
    {
        ((SRL::Types::HighColor*)SRL::VDP1::Textures[movieSprite].GetData())[pixel] = SRL::Types::HighColor::Colors::Black;
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
