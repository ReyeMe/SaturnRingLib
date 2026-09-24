/*VDP2 Scroll Layer Demo:
This Demo provides a sample of using the SRL::ASCII module to load custom fonts
and swap between multiple fonts and color palettes. The system is largely meant
to display debug messages to developers, but can be used for simple dialog 
systems as well. 
However because The characters printed are fixed to a tilemap grid, font
formatting features such as variable width characters and kerning are not
possible with this system. Fonts are limited to 16 color 8x8 characters.
*/
#include <srl.hpp>

using namespace SRL::Types;
using namespace SRL::Input;
using namespace SRL::Math;

//A custom 16 color palette to load for a font:
HighColor CustomPalette[16] = 
{
    HighColor::Colors::Black,//note: index 0 color in a palette will be transparent 
    HighColor::Colors::Red,
    HighColor::Colors::Green,
    HighColor::Colors::Blue,
    HighColor::Colors::White,
    HighColor::Colors::Magenta,
    HighColor::Colors::Yellow,
    HighColor::Colors::Black,
    HighColor::Colors::Black,
    HighColor::Colors::Black,
    HighColor::Colors::Black,
    HighColor::Colors::Black,
    HighColor::Colors::Black,
    HighColor::Colors::Black,
    HighColor::Colors::Black,
    HighColor::Colors::Black,
};

int main()
{
 
    SRL::Core::Initialize(HighColor(20,10,50));
    
    /*  During initialization, SRL loads the SGL font to font index 0.
    By default print statements will display using font index 0, palette index 0:*/
    
    SRL::Debug::Print(1,2,"SRL Ascii Font Demo");
    SRL::Debug::Print(1,7,"This is the Default SGL Font");

    /*The font system can use the first 8 16 color palettes in CRAM (0-7).
    So we can load additional palettes to use with fonts: */

    //reserve CRAM palette 1 for a font palette:
    SRL::CRAM::SetBankUsedState(1, SRL::CRAM::TextureColorMode::Paletted16, true);
    SRL::CRAM::Palette MyPalette(SRL::CRAM::TextureColorMode::Paletted16, 1);

    //load custom colors to CRAM palette 1
    MyPalette.Load(CustomPalette,16);

    //Switch the current font palette:
    SRL::ASCII::SetPalette(1);

    //Now subsequent print statments will use palette 1:
    SRL::Debug::Print(1,10, "This is a custom font palette-");
    SRL::Debug::Print(1,11, "8 palettes can display simultaneously!");
    
    //Switch back to palette 0 
    SRL::ASCII::SetPalette(0);

    /*Custom fonts can also be used. Up to 6 can be loaded and displayed at once (0-5).
    The system loads 8x8 character ASCII tables from a bitmap source.
    -Ensure the source is 4bpp (16 color) and the table begins with 
    one whitespace character followed by the standard Ascii characters. See the 
    included image in the CD::Data folder for an example of the proper table format.
    -Only 7bit ascii tables (first 128 characters) are supported.*/
    
    //load bitmap of the custom font to work ram
    SRL::Bitmap::TGA* CustomFont = new SRL::Bitmap::TGA("FONT1.TGA");

    //load custom font to Ascii index 1:
    SRL::ASCII::LoadFont(CustomFont, 1);

    //Now we can switch to the new font like we switched palettes:
    SRL::ASCII::SetFont(1);
    SRL::Debug::Print(1,14,"This is a custom font-");
    SRL::Debug::Print(1,15,"6 fonts can display simultaneously!");
    
    //Main Game Loop 
    while(1)
    {
        //Print statments will remain on screen until SRL::ASCII::Clear() is Called
        SRL::Core::Synchronize();
    }
    return 0;
}
