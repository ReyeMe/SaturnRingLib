/* Application entry point*/
#define APP_ENTRY (0x6004000)

/* Boot entry point */
int main(void)
{
    // Start application
    return ((int(*)())APP_ENTRY)();
}

/* Boot entry point 2 */
int _main() {
    return main();
}
