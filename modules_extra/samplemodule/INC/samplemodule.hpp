#pragma once
#include <srl.hpp>
// include other plugins here
// #include <example.hpp>

using namespace SRL::Types;
using namespace SRL::Input;

namespace SampleModule
{
    class HelloWorld final
    {
    private:
        static constexpr const char* privateMessage = "Good Bye!";
        
        static inline const char* getPrivateMessage()
        {
            return privateMessage;
        }

    public:
        static constexpr const char* publicMessage = "Hello World!";
        
        static void ShowMessage(bool showPrivateMessage = false)
        {
            if (!showPrivateMessage)
            {
                SRL::Debug::Print(2,2, "publicMessage:");
                SRL::Debug::Print(10,4, "%s", publicMessage);
                SRL::Debug::Print(2,7, "Press 'A' to continue");
            }
            else
            {
                SRL::Debug::Print(2,10, "privateMessage:");
                SRL::Debug::Print(10,12, "%s", getPrivateMessage());
            }
        }
    };
}

