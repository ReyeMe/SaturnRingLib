#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;

/** @brief Simple canvas
 */
class Canvas : SRL::Bitmap::IBitmap
{
private:

    /** @brief Canvas width
     */
    uint16_t width;

    /** @brief Canvas height
     */
    uint16_t height;

    /** @brief Image data
     */
    HighColor* imageData;

public:

    /** @brief Construct RGB555 canvas
     * @param filename TGA file name
     * @param settings TGA loader settings
     */
    Canvas(uint16_t width, uint16_t height) : width(width), height(height), imageData(new HighColor[width * height]) { }

    /** @brief Destroy the canvas
     */
    ~Canvas()
    {
        if (this->imageData != nullptr)
        {
            delete this->imageData;
        }
    }

    /** @brief Get image data
     * @return Pointer to image data
     */
    uint8_t* GetData() override
    {
        return (uint8_t*)this->imageData;
    }

    /** @brief Get image info
     * @return image info
     */
    SRL::Bitmap::BitmapInfo GetInfo() override
    {
        return SRL::Bitmap::BitmapInfo(this->width, this->height);
    }
};

/** @brief Canvas instance
 */
Canvas* canvas = nullptr;

/** @brief Id of the canvas texture in VDP1 sprite RAM heap
 */
int32_t canvasTextureId;

/** @brief Canvas width
 */
constexpr uint16_t width = 240;

/** @brief Canvas height
 */
constexpr uint16_t height = 180;

/** @brief Copy the texture during vblank
 */
void VBlankCopy()
{
    if (canvas != nullptr)
    {
        slDMACopy(canvas->GetData(), SRL::VDP1::Textures[canvasTextureId].GetData(), width * height * sizeof(uint16_t));
        slDMAWait();
    }
}

// Main program entry
int main()
{
	SRL::Core::Initialize(HighColor(20,10,50));
    SRL::Debug::Print(1,1, "Random image generator sample");

    // Initialize canvas in VDP1 sprite ram
    canvas = new Canvas(width, height);
    canvasTextureId = SRL::VDP1::TryLoadTexture((SRL::Bitmap::IBitmap*)canvas);

    // Initialize random number generator with a specific seed
    SRL::Math::Random rnd = SRL::Math::Random(15);

    // Setup event to copy the canvas to VDP1 sprite RAM on every vblank
    SRL::Core::OnVblank += VBlankCopy;

    // Main program loop
	while(1)
	{
        // Render noise on canvas
        for (int pixel = 0; pixel < width * height; pixel++)
        {
            uint8_t color = rnd.GetNumber(0, 255);
            ((HighColor*)canvas->GetData())[pixel] = HighColor(color, color, color);
        }

        // Draw the canvas on screen as VDP1 sprite
        SRL::Scene2D::DrawSprite(canvasTextureId, Vector3D(0.0, 0.0, 500.0));

        SRL::Core::Synchronize();
	}

	return 0;
}
