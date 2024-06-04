# WinCPPToolkit

WinCPPToolkit is an integrated suite of popular C++ build tools for Windows, designed to enhance the efficiency of cross-platform projects being developed within the Windows environment.

This toolkit is designed for cross-platform C/C++ projects, making building more convenient. It can also be integrated into IDEs such as CLion ([See it](https://github.com/ChillMagic/WinCPPToolkit?tab=readme-ov-file#clion-setup)), offering an alternative toolchain option besides *Visual Studio* and *MinGW*.

You can use it on Windows in a way similar to Linux like this:
```
mkdir build
cd build
cmake ..
ninja
```

Currently integrated or in the process of integration tools:
- clang+llvm
- cmake
- ninja
- git
- meson

## Requirements
- Windows 10 or later
- [VC++ Build Tools](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=buildtools) (Recommended) or [Visual Studio](https://visualstudio.microsoft.com/)

## Setup

To setup this tool, you typically follow these steps after ensuring Python is installed on your system:

1. **Clone the repository**:
   Clone the repository to your local machine using Git.
   ```bash
   git clone https://github.com/ChillMagic/WinCPPToolkit.git
   ```

2. **Navigate to the project directory**:
   Change into the project directory.
   ```bash
   cd WinCPPToolkit
   ```

3. **Setup Python environment**
   Run `get-python` script to setup.
   ```
   .\get-python.bat
   ```

4. **Run the init script**:
   Execute the script or command to init the tool.
   ```batch
   python\python init.py
   ```

## Usage

### Shell Setup

**Setup `PATH` variable**:
   Now, by adding toolkit to the `PATH`, you can directly use the commands.

   *Powershell*:
   ```
   $env:PATH += ";" + (Join-Path (Get-Location) "bin")
   ```
   *CMD*:
   ```
   set PATH=%PATH%;%cd%\bin
   ```

After setting up the `PATH`, you can directly use commands like `cmake` for building. This toolkit utilizes clang/llvm + cmake + ninja as the toolset, which can be used as follows:

```
cmake ..
ninja
```

### CLion Setup

In CLion, you can use it by simply making the following settings:

![](CLion-Toolchains.png)
![](CLion-Meson.png)

### Compatibility

#### MinGW
In some projects, although Windows cmake scripts are configured, the MinGW toolchain is used. Compatibility can be achieved by adding the `__MINGW32__` macro, be like:
```
cmake .. -DCMAKE_CXX_FLAGS=-D__MINGW32__
```
