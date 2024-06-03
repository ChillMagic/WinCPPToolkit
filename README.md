# WinCPPToolkit

WinCPPToolkit is an integrated suite of popular C++ build tools for Windows, designed to enhance the efficiency of cross-platform projects being developed within the Windows environment.

It will use Python as the program interface, facilitating the invocation of build tools.

Currently integrated or in the process of integration tools:
- clang+llvm
- cmake
- ninja
- git

## Requirements
- Windows 10 or later
- Python 3 (Optional)

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

3. **Setup Python environment (Optional)**
   If Python is not installed, it can be installed using the `get-python` script:
   ```
   .\get-python.bat
   ```
   Then, add the Python command to the environment variables.

   *Powershell*:
   ```
   $env:PATH += ";" + (Join-Path (Get-Location) "python")
   ```
   *CMD*:
   ```
   set PATH=%PATH%;%cd%\python
   ```

4. **Run the init script**:
   Execute the script or command to init the tool.
   ```bash
   python init.py
   ```

5. **Setup `PATH` variable**:
   Now, by adding toolkit to the `PATH`, you can directly use the commands.

   *Powershell*:
   ```
   $env:PATH += ";" + (Join-Path (Get-Location) "bin")
   ```
   *CMD*:
   ```
   set PATH=%PATH%;%cd%\bin
   ```
