#include <srl.hpp>

int main()
{
    SRL::Core::Initialize(SRL::Types::HighColor::Colors::Black);

    // Small work buffer
    char content[256] = { '\0' };

    // Open first file and print its content
    SRL::Cd::File file1("FIRST.TXT");
    file1.LoadBytes(0, 256, content);
    SRL::Debug::Print(2, 2, content);

    // Clear buffer
    for (int c = 0; c < 256; c++) content[c] = '\0';

    // Open folder and read second file
    SRL::Cd::ChangeDir("FOLDER");
    SRL::Cd::File file2("SECOND.TXT");
    file2.LoadBytes(0, 256, content);
    SRL::Debug::Print(2, 3, content);
    
    // Clear buffer
    for (int c = 0; c < 256; c++) content[c] = '\0';

    // Open folder and go to a deeper folder
    SRL::Cd::ChangeDir("DEEP");
    SRL::Cd::File file3("DEEPFL.TXT");
    file3.LoadBytes(0, 256, content);
    SRL::Debug::Print(2, 4, content);

    // Clear buffer
    for (int c = 0; c < 256; c++) content[c] = '\0';

    // Go back one level
    SRL::Cd::ChangeDir("..");
    SRL::Cd::File file4("RET.TXT");
    file4.LoadBytes(0, 256, content);
    SRL::Debug::Print(2, 5, content);

    // Clear buffer
    for (int c = 0; c < 256; c++) content[c] = '\0';

    // Go back to root and read last file, (char*)nullptr or empty string can be used
    SRL::Cd::ChangeDir("");
    SRL::Cd::File file5("THIRD.TXT");
    file5.LoadBytes(0, 256, content);
    SRL::Debug::Print(2, 6, content);

    while (true);
    return 0;
}
