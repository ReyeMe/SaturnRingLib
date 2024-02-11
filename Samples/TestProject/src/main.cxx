#include <srl.hpp>

using namespace SRL::Types;

int main()
{
	SRL::Core::Initialize(SRL::Types::SaturnColor(200,100,50));
	SRL::Debug::Print(1,1, "Sprite demo");

	SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");
	Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);
	delete tga;
	
    SPR_ATTR attr = SPR_ATTRIBUTE(0, No_Palet, No_Gouraud, MESHoff | CL32KRGB | No_Window, sprVflip | FUNC_Sprite);
	attr.colno = 0;
    FIXED sgl_pos[XYZSS];
    sgl_pos[0] = Fxp(0.0).Value();
    sgl_pos[1] = Fxp(0.0).Value();
    sgl_pos[2] = Fxp(500.0).Value();
    sgl_pos[3] = Fxp(10.0).Value();
    sgl_pos[4] = Fxp(10.0).Value();
	
	Uint32 frame = 0;

	
	
	// Game loop
	while(1)
	{
        Bool valid = slPutSprite(sgl_pos, &attr, 0);
		SRL::Debug::PrintWithWrap(1, 2, 1, 39, "Frame %d\n addr %x", frame++, SRL::VDP1::Metadata->GetData());
		slSynch();
	}

	return 0;
}
