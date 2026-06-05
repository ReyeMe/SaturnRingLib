#include <srl_memory.hpp>

/** @brief Application initialization
 */
extern "C" {

    /** @brief Start of a .bss section
     */
    extern uint32_t _bstart;

    /** @brief End of a .bss section
     */
    extern uint32_t _bend;

    /** @brief Start of a workarea area section
     */
    extern uint32_t _work_area_start;

    /** @brief End of a command buffer section
     */
    extern uint32_t _command_buffer_end;

    /** @brief Start address of constructor array
     */
    extern void(*__ctors)();

    /** @brief End address of constructor array
     */
    extern void(*__ctors_end)();

    /** @brief Start of SGL program
     * @note Defined within linker script, called before main()
     */
    extern void __Start();

    /** @brief Define PreLoader() in the "PRELOADER" section defined within linker script
     */
    void PreLoader()  __attribute__ ((section ("PRELOADER")));
    
    /** @brief Function run before main()
     */
    void PreLoader()
    {
        // Zero stuff inside .bss section
        for (uint32_t* bssBlock = &_bstart; bssBlock < &_bend; bssBlock++)
        {
            *bssBlock = 0;
        }

        // Zero stuff inside workarea and command buffer section
        for (uint32_t* workareaBlock = &_work_area_start; workareaBlock < &_command_buffer_end; workareaBlock++)
        {
            *workareaBlock = 0;
        }

        // Initialize memory management (malloc stuff)
        SRL::Memory::Initialize();

        // Call all constructors
        void(**constructor)() = &__ctors;
        while (constructor != &__ctors_end) {
            (*constructor)();
            constructor++;
        }

        // Start program
        __Start();
    }
}
