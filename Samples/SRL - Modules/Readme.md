# SampleModule – SRL Module Example
SampleModule is an example of an **SRL** extra module using the `modules_extra` system. 

It demonstrates how to structure a module, integrate it into a project, and distribute assets and build logic.

## This module shows how to:
- Add external modules via a project Makefile
- Organize module files for SRL
- Use module.mk for custom build behavior
- Automatically copy module data into a project
- Chain multiple modules together
- Write header-only modules (recommended)

## 1) Adding a Module to Your Project
To include a module in your project, add this line to your project’s Makefile:
```
MODULES_EXTRA = samplemodule
```
Replace SampleModule with the name of your module.

To include multiple modules, separate them with spaces:
```
MODULES_EXTRA = SampleModule AnotherModule MyModule
```
## 2) Module Location
Place your module inside the SRL modules_extra directory:
```
saturnringlib/
└── modules_extra/
    └── SampleModule/
```
## 3) Module Structure (SRL Style)
A typical SRL module layout:
```
SampleModule/
├── INC/
│   └── samplemodule.hpp
├── cd/
│   └── data/
│       └── example.bin
└── module.mk
```
### INC Folder (Important)
All module source files go in **INC**.
Header-only modules (.hpp) are highly encouraged.
If you place .c or .cpp files in **INC**, SRL will automatically compile them.
This allows modules to be simple, portable, and easy to integrate.

### Example:
```
#include <samplemodule.hpp>
SampleModule::HelloWorld::ShowMessage();
```
### cd/data Folder (Automatic Copy)
If your module needs to provide files to the project (e.g. binaries, assets, drivers), place them in:
```
SampleModule/cd/data/
```
During the build, these files are automatically copied to the project’s:
```
project_root/cd/data/
```
### Typical use cases:
- Sound drivers (e.g. SDRV.BIN)
- Binary assets
- Textures, maps, fonts, etc.

## 4) Custom Build Logic (module.mk)
Each module can define a **module.mk** file to extend the build process.

You can use module.mk to:
- Add compiler flags
- Add include paths
- Define custom copy rules
- Add build steps or dependencies
- Customize how the module integrates with SRL

## 5) Using Multiple Modules (Module Chaining)
Modules can be combined freely.
To use multiple modules, list them in the project Makefile:
```
MODULES_EXTRA = samplemodule dependencymodule anothermodule
```
Don't forget to include dependencies in your module:
```
#include <dependencymodule.hpp>
```
There is no special syntax required—modules are resolved through the same mechanism.
A module can depend on another module as long as it is listed in `MODULES_EXTRA`.

## 6) Design Philosophy
Modules should be lightweight and header-focused, making them portable and easy to drop into projects.

