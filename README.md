# XenoScan
*XenoScan* is a memory scanner which can be used to scan the memory of processes to locate the specific locations of important values. These types of tools are typically used when hacking video games, as they allow one to locate the values representing the game's state in memory.

*XenoScan* is written in C++ with a Lua frontend, and I've been working on advanced functionality that goes beyond anything that has been in any other memory scanners I've seen. Notably, it has a way to enumerate and return all complex data structures (such as std::list and std::map) in the target's memory space, and it can even scan for any class instances and group the discovered instances by their underlying types.

## Sub-projects

### XenoLua
*XenoLua* is a wrapper around Lua that provides a ton of functionality. Most notably, it provides a `LuaVariant` class which wraps the functionality of converting between `C`/`C++` and `Lua` types. Additionally, it has helper functions for working with Lua in the `LuaPrimitive` class.

### XenoScanEngine
*XenoScanEngine* is the meat of the project. It contains the code for the scanning, data structure detection, and everything else.

### XenoScanLua
*XenoScanLua* ties *XenoScanEngine* to *XenoLua* to provide a Lua-scriptable frontend for the scanner. Currently, this is the only entry-point to the scanner.

Additionally, this project contains some test code that ensures everything is working properly. A test is a combination of a '.cpp', a '.h', and a '.lua' file. For examples on how to use the scanner, you can check out the `.lua` test files.

## Compiling
The *XenoScan* project files are currently for the Visual Studio 2010 IDE. You should be able to compile the code with newer versions of Visual Studio, however.

Before you can compile, you will need to make sure you've checked out the submodules. Once that's done, you'll also have to build the `luajit-2.0` submodule so *XenoScan* can link against the libraries.

## Platform
The code is designed to be platform-agnostic. Theoretically, to compile on any other platform, you would need to

1. Create project/make files for your target IDE/compiler.
2. Remove the `ScannerTargetWindows.cpp` and `ScannerTargetWindows.h` files from the project.
3. Implement the `ScannerTarget` interface for your platform.
4. Add your implementation to the project.
5. ???? *profit*

## Features
**Basic scanning functionality supports the following types:**
- Integral types\*:
    - `int8_t`
    - `uint8_t`
    - `int16_t`
    - `uint16_t`
    - `int32_t`
    - `uint32_t`
    - `int64_t`
    - `uint64_t`
  - `float`
  - `double`
- ascii strings
- wide strings
- Custom data structures (think `C++` `struct`)
    - Can consist of any combination integral and decimal types

<sub>\* *Lua frontend may choke on 64-bit integers, but the scanner library supports them.*</sub>

**Additionally, there is functionality to detect all instances of the following types:**
- `std::map`
- `std::list`
- Any class with a virtual-function table
