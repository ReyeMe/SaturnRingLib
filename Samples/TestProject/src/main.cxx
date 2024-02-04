#include <srl.hpp>

int main()
{
	SRL::Core::Initialize(SRL::Types::SaturnColor(200,100,50));
	SRL::Debug::Print(1,1, "Sprite demo");

	SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");

	// Game loop
	while(1)
	{
		SRL::Bitmap::BitmapInfo info = tga->GetInfo();
		SRL::Debug::PrintWithWrap(1,2, 1, 30, "w=%d\nh=%d", info.Width, info.Height);

		slSynch();
	}

	return 0;
}
