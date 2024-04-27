# Saturn Ring Library
> **Work in progress**

## Big thanks to these folk:
 - [jae686](https://github.com/jae686)
 - [robertoduarte](https://github.com/robertoduarte)
 - [7shades](https://github.com/seven-shades)

## Code style
This block describes general code style for this project. Following this style will ensure readability and ease of creating documentation.

### Tab vs. Spaces
Tab characters should not be used, 4 spaces should be used instead of a single TAB.

### Documentation
Every member (eg: ``namespace``, ``class``, ``variable``, ``function``, ...) must have [doxygen documentation](https://www.doxygen.nl/manual/commands.html) comment above it. (example: see [Variables & Constants - valid case](#VariablesAndConstantsValid))

### Variables & constants
1) Public variables and constants must be names with **PascalCase** and use of '_' or single letter names is forbidden in every case.
2) Private variables must be named with **camelCase** and use of '_' or single letter names is forbidden in every case.

**Inalid:**
```cpp
class MyClass
{
private:
    static const _myConstant = 15;
    int _myValue;

public:
    int myTest;

    // ...
};
```

**Valid:**<a name="VariablesAndConstantsValid"></a>
```cpp
/** @brief My example class
 */
class MyClass
{
private:
    /** @brief My private constant that defines stuff
     */
    static const MyConstant = 15;
    
    /** @brief My private variable that stores stuff
     */
    int myValue;

public:
    /** @brief My public variable that stores stuff
     */
    int MyTest;

    // ...
};
```
### Function naming
Function names are in **PascalCase** and use of '_' or single letter names is forbidden in every case.

**Inalid:**
```cpp
void _myFunction(int _t);
```
**Valid:**
```cpp
/** @brief My function that does stuff
 * @param test Current test value
 */
void MyFunction(int test);
```

### Referencing local variables
* **Instance variables:**<br/>
    When referencing an instance variable within class be it public or private ``this->`` keyword must always be before it. This improves readability by showing which variable comes from within this class, which comes from within function or which comes from outside.

* **Static/Constant variables:**<br/>
    When referencing a constant or static variable we must put a name of class from where this variable came from before it always. This improves readability helping to distinguish between static/const and instance variables, it also helps to identifie where variable is defined by just a glance.
    
#### Example:
> Documentation ommited to conserve space
```cpp
class MyClass
{
private:
    static const int MyAmazingHiddenConstant;
    int myAmazingVariable;

public:
    static const int WowSoPublic;
    int YouCanSeeThis;

    // Constructors...

    int TestFunct(int myParam)
    {
        return this->YouCanSeeThis +
            this->myAmazingVariable +
            myParam + 
            MyClass::MyAmazingHiddenConstant +
            MyClass::WowSoPublic;
    }
}
```