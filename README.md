# XenoScan
*XenoScan* is a memory scanner which can be used to scan the memory of processes to locate the specific locations of important values. These types of tools are typically used when hacking video games, as they allow one to locate the values representing the game's state in memory.

*XenoScan* is written in C++ with a Lua frontend, and I've been working on advanced functionality that goes beyond anything that has been in any other memory scanners I've seen. Notably, it has a way to enumerate and return all complex data structures (such as std::list and std::map) in the target's memory space, and it can even scan for any class instances and group the discovered instances by their underlying types.

## Communication
If you need to get in touch with me, want a place to chat, or have a question, my [Discord](https://discord.gg/2cfR9AA) is the best place.

## Sub-projects

### XenoLua
*XenoLua* is a wrapper around Lua that provides a ton of functionality. Most notably, it provides a `LuaVariant` class which wraps the functionality of converting between `C`/`C++` and `Lua` types. Additionally, it has helper functions for working with Lua in the `LuaPrimitive` class.

### XenoScanEngine
*XenoScanEngine* is the meat of the project. It contains the code for the scanning, data structure detection, and everything else.

### XenoScanLua
*XenoScanLua* ties *XenoScanEngine* to *XenoLua* to provide a Lua-scriptable frontend for the scanner. Currently, this is the only entry-point to the scanner.

Additionally, this project contains some test code that ensures everything is working properly. A test is a combination of a `.cpp`, a `.h`, and a `.lua` file. For examples on how to use the scanner, you can check out the `.lua` test files.

## Compiling
*XenoScan* uses *CMake*, and has been tested with Visual Studio 2017. In theory, you should be able to build the code with any modernish compiler, as long as you use CMake to generate the project files. Before you can compile, you will need to make sure you've checked out the submodules. Once that's done, you'll also have to build the *luajit* submodule so *XenoScan* can link against the libraries.

By modernish, I mean anything that supports C++17, as the code uses that standard. Additionally, your CMake version should be at least 3.10.

If you're using Visual Studio, this should be easy. Simply run `buildmsvc2017.bat` from a *Developer Command Prompt for VS*. As an example, to build a project for *Visual Studio 2017*, I run

```
cd C:\path\to\XenoScan
buildmsvc2017.bat
```
Which would make a file named `XenoScan.sln` appear in my `build` directory (e.g. `C:\path\to\XenoScan\build`).

The main development of XenoScan is done on this version of Visual Studio.

If you're on another system or using another compiler or IDE, you'll have to [build *luajit* on your own](http://luajit.org/install.html) and run *CMake* manually.

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

**Scanning supports the following types of matching:**
- Equal to
- Greater than
- Greater than or equal to
- Less than
- Less than or equal to
- Ranges (`min <= check <= max`)


**Additionally, there is functionality to detect all instances of the following types:**
- `std::map`
- `std::list`
- Any class with a virtual-function table
