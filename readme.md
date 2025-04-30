<h1 align="center" id="heading">Saturn Ring Library</h1>
<p align="center">
 Easy to use SGL wrapper written in C++</br>
 <a href="https://srl.reye.me/"><b>Documentation</b></a></br></br>
 <img src="https://github.com/ReyeMe/SaturnRingLib/blob/main/Documentation/resources/srl_logo.png"></br></br>
 <a href="https://github.com/ReyeMe/SaturnRingLib/issues">
    <img src="https://img.shields.io/github/issues/ReyeMe/SaturnRingLib.svg" alt="Issues"/>
  </a>
  <a href="https://github.com/ReyeMe/SaturnRingLib/commits/main">
    <img src="https://img.shields.io/github/last-commit/ReyeMe/SaturnRingLib/main.svg" alt="Last commit"/>
  </a>
</p>

## Brought to you by
<table>
  <tr>
    <td><a href="https://github.com/ReyeMe" target="_blank"><img src="https://github.com/ReyeMe.png" width="32px;"/></a></td>
    <td><a href="https://github.com/ReyeMe" target="_blank">ReyeMe</a></td>
  </tr>
  <tr>
    <td><a href="https://github.com/robertoduarte" target="_blank"><img src="https://github.com/robertoduarte.png" width="32px;"/></a></td>
    <td><a href="https://github.com/robertoduarte" target="_blank">robertoduarte</a></td>
  </tr>
  <tr>
    <td><a href="https://github.com/seven-shades" target="_blank"><img src="https://github.com/seven-shades.png" width="32px;"/></a></td>
    <td><a href="https://github.com/seven-shades" target="_blank">7shades</a></td>
  </tr>
  <tr>
    <td><a href="https://github.com/willll" target="_blank"><img src="https://github.com/willll.png" width="32px;"/></a></td>
    <td><a href="https://github.com/willll" target="_blank">willll</a></td>
  </tr>
  <tr>
    <td><a href="https://github.com/nemesis-saturn" target="_blank"><img src="https://github.com/nemesis-saturn.png" width="32px;"/></a></td>
    <td><a href="https://github.com/nemesis-saturn" target="_blank">nemesis-saturn</a></td>
  </tr>
  <tr>
    <td><a href="https://github.com/jae686" target="_blank"><img src="https://github.com/jae686.png" width="32px;"/></a></td>
    <td><a href="https://github.com/jae686" target="_blank">jae686</a></td>
  </tr>
</table>

## Getting started
### Installing SRL
#### Download
<details>
    <summary>Using pre-packaged release</summary>

Go to the [Releases](https://github.com/ReyeMe/SaturnRingLib/releases) section and download latest .zip release.

</details>
<details>
    <summary>Using git repository</summary>
Clone git repository by using:

```
git clone --recurse-submodules https://github.com/ReyeMe/SaturnRingLib.git
```

> __Note:__ It is important to not forget the ``--recurse-submodules`` otherwise some submodules (SaturnMath++ and TLSF memory allocator) will not get downloaded.
</details>

#### Setting up
1. Download toolchain. This can be done by running ``setup_compiler.bat``.  
2. Download [mednafen](https://mednafen.github.io/) and put it into ``emulators/mednafen/`` folder (or install it with a package manager if on linux).  
You will also need to obtain bios file (``mpr-17933.bin``) and put it in the mednafen ``firmware`` folder. 

> __Note:__ ``.bat`` scripts used within SRL can be run natively on Windows, Linux or Mac.
<details>
  <summary>Linux dependencies</summary>

Use your preferred package manager to install the following:
- `make` - for compilation
- `unzip` - used during compiler installation
- `wget` - to download the compiler
- `sox` - to convert audio
    - `libsox-fmt-mp3` - for sox to support mp3 files
- `xorriso` - to build cue/bin

</details>

#### Build and run samples
Samples and project in SRL can be build and run from VSCode or manually by starting ``.bat`` scripts in the sample/project directory
<details>
    <summary>With VSCode (recommended)</summary>

1. Open the folder of a project/sample (folder contains .vscode sub folder) with VSCode.  
2. Open tasks menu using ``CTRL+SHIFT+B``.
3. Click on one of the ``compile`` tasks to build the project, or ``run with`` task to start emulator.  
Projects can be compiled with DEBUG or RELEASE target.

</details>
<details>
    <summary>Manually</summary>

1. Open the folder of a project/sample.
2. To build just run ``compile.bat`` or to run a built project in an emulator use one of the ``run with`` ``.bat`` files.

</details>

#### Creating a project
To create a new custom project just copy one of the samples to the projects folder.  
The name of the project can than be changed within the ``makefile``.

## Contributing to the project
This block describes general code style for this project.  
Following this style will ensure readability, maintainability and ease of creating documentation.

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
void MyFunc()
{
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
