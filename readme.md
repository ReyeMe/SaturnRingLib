<h1 align="center" id="heading">Saturn Ring Library</h1>
<p align="center">
 Easy to use SGL wrapper written in C++</br>
 <img src="https://github.com/ReyeMe/SaturnRingLib/blob/main/Documentation/resources/srl_logo.png"></br></br>
</p>

## Big thanks to these folk:
 - [jae686](https://github.com/jae686)
 - [robertoduarte](https://github.com/robertoduarte)
 - [7shades](https://github.com/seven-shades)

## Getting started
> **Work in progress**

## Code style
This block describes general code style for this project. Following this style will ensure readability and ease of creating documentation.

### Sections
- [Tab vs. Spaces](#tab-v-space)
- [Empty lines](#empty-line)
- [Curly braces](#curly-braces)
- [Documentation](#docs)
- [Variables & constants](#variables)
- [Function naming](#functions)
- [Referencing local variables](#references)
    - [Instance variables](#references-instance)
    - [Static/Constant variables](#references-static)

### Tab vs. Spaces<a name="tab-v-space"></a>
Tab characters should not be used, 4 spaces should be used instead of a single TAB.

### Empty lines<a name="empty-line"></a>
Do not use multiple empty lines.

**Noncompliant:**
```cpp
void MyFunc()
{
    int i;



    i = Compute();
}
```

**Compliant:**
```cpp
void MyFunc() {
    int i;

    i = Compute();
}
```

### Curly braces<a name="curly-braces"></a>
1) Curly braces should always be on a new line.
2) If there is an function call or variable definition/assignment before a starting curly bracket it must be preceded by a empty line, otherwise there should be no empty line before starting curly bracket.
3) Ending curly bracket must never be preceded by a empty line,

### Documentation<a name="docs"></a>
Every member (eg: ``namespace``, ``class``, ``variable``, ``function``, ...) must have [doxygen documentation](https://www.doxygen.nl/manual/commands.html) comment above it. (example: see [Variables & Constants - compliant case](#VariablesAndConstantsValid))

### Variables & constants<a name="variables"></a>
1) Public variables and constants must be names with **PascalCase** and use of '_' or single letter names is forbidden in every case.
2) Private variables must be named with **camelCase** and use of '_' or single letter names is forbidden in every case.

**Noncompliant:**
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

**Compliant:**<a name="VariablesAndConstantsValid"></a>
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
### Function naming<a name="functions"></a>
Function names are in **PascalCase** and use of '_' or single letter names is forbidden in every case.

**Noncompliant:**
```cpp
void _myFunction(int _t);
```
**Compliant:**
```cpp
/** @brief My function that does stuff
 * @param test Current test value
 */
void MyFunction(int test);
```

### Referencing local variables<a name="references"></a>
* **Instance variables:**<br/><a name="references-instance"></a>
    When referencing an instance variable within class be it public or private ``this->`` keyword must always be before it. This improves readability by showing which variable comes from within this class, which comes from within function or which comes from outside.

* **Static/Constant variables:**<br/><a name="references-static"></a>
    When referencing a constant or static variable we must put a name of class from where this variable came from before it always. This improves readability helping to distinguish between static/const and instance variables, it also helps to identify where variable is defined by just a glance.
    
#### Example:
> Documentation omitted to conserve space
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

    int TestFunc(int myParam)
    {
        return this->YouCanSeeThis +
            this->myAmazingVariable +
            myParam + 
            MyClass::MyAmazingHiddenConstant +
            MyClass::WowSoPublic;
    }
}
```
