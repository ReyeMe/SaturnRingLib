#include <srl.hpp>

#define MEMORY_BLOCK_SIZE (16 * 1024)

// Using to shorten names for HighColor and Fxp
using namespace SRL::Types;
using namespace SRL::Math::Types;

// Main program entry 
int main()
{
    // Test values
    int8_t  CharNumber = -1;
    uint8_t UnsignedCharNumber = 2;
    int16_t  ShortNumber = -1000;
    uint16_t UnsignedShortNumber = 2000;
    int32_t  IntNumber = -1000000;
    uint32_t UnsignedIntNumber = 2000000;
    Fxp FixedNumber = Fxp(-500.321);
    int8_t Character = 65; // ASCII character 'A'
    const char* String = "ABCD1234";
    
    // Initialize library
    SRL::Core::Initialize(HighColor::Colors::Black);
 
    uint32_t* MemoryPointer = static_cast<uint32_t*>(SRL::Memory::HighWorkRam::Malloc(MEMORY_BLOCK_SIZE));
       
    SRL::Debug::Print(1,1, "Debug Print sample");    
    
    SRL::Debug::Print(1,3, "\%d (decimal int):");
    SRL::Debug::Print(4,4, " int8_t  \"%d\" expected: \"-1\"", CharNumber);
    SRL::Debug::Print(4,5, "uint8_t   \"%d\" expected:  \"2\"", UnsignedCharNumber);
    SRL::Debug::Print(4,6, " int16_t \"%d\" expected: \"-1000\"", ShortNumber);
    SRL::Debug::Print(4,7, "uint16_t  \"%d\" expected:  \"2000\"", UnsignedShortNumber);
       
    SRL::Debug::Print(1,8, "\%i (int - same as \%d):");
    SRL::Debug::Print(4,9, " int32_t \"%i\" expected: \"-1000000\"", IntNumber);
    SRL::Debug::Print(4,10, "uint32_t  \"%i\" expected:  \"2000000\"", UnsignedIntNumber);
       
    SRL::Debug::Print(1,11, "\%u (unsigned int - similar to %d):");
    SRL::Debug::Print(4,12, "uint32_t  \"%u\" expected:  \"2000000\"", UnsignedIntNumber);
    
    SRL::Debug::Print(1,13, "\%f (fixed 16.16):");
    SRL::Debug::Print(4,14, "Fxp \"%f\" expected: \"-500.32100\"", FixedNumber);
    
    SRL::Debug::Print(1,15, "\%3d (space padding):");
    SRL::Debug::Print(4,16, " int8_t \"%3d\" expected: \" -1\"", CharNumber);
    SRL::Debug::Print(4,17, "uint8_t \"%3d\" expected: \"  2\"", UnsignedCharNumber);
    
    SRL::Debug::Print(1,18, "\%03d (0 padding):");
    SRL::Debug::Print(4,19, " int8_t \"%03d\" expected: \"-01\"", CharNumber);
    SRL::Debug::Print(4,20, "uint8_t \"%03d\" expected: \"002\"", UnsignedCharNumber);
    
    SRL::Debug::Print(1,21, "\%08X (similar to \%x):");
    SRL::Debug::Print(4,22, "Hex: \"0x%08X\" expected \"0x06018214\"", MemoryPointer);
    
    SRL::Debug::Print(1,23, "\%s (string):");
    SRL::Debug::Print(4,24, "String: \"%s\" expected \"ABCD1234\"", String);

    SRL::Debug::Print(1,25, "\%c (ASCII index):");
    SRL::Debug::Print(4,26, "Char: \"%c\" expected \"A\"", Character);

    SRL::Core::Synchronize();

    return 0;
}
