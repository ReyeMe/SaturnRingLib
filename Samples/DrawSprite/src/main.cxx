#include <srl.hpp>

using namespace SRL::Types;

int main()
{
	SRL::Core::Initialize(SaturnColor(20,10,50));
    SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");   // Loads TGA file into main RAM
    Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);       // Loads TGA into VDP1
    delete tga;                                                 // Freems main RAM

	while(1)
	{
        SRL::Render::DrawSprite(textureIndex, Vector3D(0.0, 0.0, 500.0));
        SRL::Core::Synchronize();
	}

	return 0;
}
