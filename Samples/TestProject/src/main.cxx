#include <srl.hpp>

using namespace SRL::Types;

int main()
{
	SRL::Core::Initialize(SRL::Types::SaturnColor(200,100,50));
	SRL::Debug::Print(1,1, "Sprite demo");

	SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");
	Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);
	delete tga;
	
	Uint32 frame = 0;

	// Game loop
	while(1)
	{
		SRL::Render::DrawSprite(textureIndex, Vector3D(0.0, 0.0, 500.0));
		SRL::Debug::PrintWithWrap(1, 2, 1, 39, "Frame %d\n addr %x", frame++, SRL::VDP1::Metadata->GetData());
		slSynch();
	}

	return 0;
}
