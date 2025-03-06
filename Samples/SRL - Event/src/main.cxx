#include <srl.hpp>

// Using to shorten names for DateTime
using namespace SRL::Types;
using namespace SRL::Math::Types;

/** @brief Test object
 */
class TestObject
{
private:

    /** @brief Text row
     */
    uint8_t row;

    /** @brief Number of calls to our v-blank function
     */
    uint32_t counter = 0;

    /** @brief Define proxy used to attach member function to an event
     */
    MemberProxy<> vblankProxy = MemberProxy(this, &TestObject::Vblank);

    /** @brief This function will get called each v-blank
     */
    void Vblank()
    {
        this->counter++;
    }

public:

    /** @brief Construct a new test object
     * @param y Text row
     */
    TestObject(uint8_t row) : row(row)
    {
        // Register event callback here
        SRL::Core::OnVblank += &vblankProxy;
    }

    /** @brief Destroy the test object
     */
    ~TestObject()
    {
        // We need to un-register event callback here
        SRL::Core::OnVblank -= &vblankProxy;
    }

    /** @brief Print number of calls to a member function
     */
    void Print()
    {
        SRL::Debug::Print(5, this->row, "Instance calls: %d", this->counter);
    }
};

/** @brief Number of calls to our static function
 */
uint32_t staticCounter = 0;

/** @brief Static function that gets called each v-blank
 */
void StaticCounter()
{
    staticCounter++;
}

// Main program entry
int main()
{
    SRL::Core::Initialize(HighColor::Colors::Black);

    // Initialize two instance of our object
    TestObject obj1 = TestObject(7);
    TestObject obj2 = TestObject(8);

    // Attach static function callback
    SRL::Core::OnVblank += StaticCounter;

    // Main program loop
    while (1)
    {
        // Print number of calls to the static function
        SRL::Debug::Print(5,5, "Static calls: %d    ", staticCounter);

        // Print number of calls to the member function
        obj1.Print();
        obj2.Print();

        SRL::Core::Synchronize();
    }

    return 0;
}
