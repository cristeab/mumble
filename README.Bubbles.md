Bubbles - Mumble clone with touch screen support for Windows
=======================================

## Compilation Instructions

- Use vcpkg to install dependences for Windows (see build instructions for Mumble on Windows)
- Install Qt5 for Visual Studio 2019
- Install NSIS
- Make sure that the following environment variables point to the correct location:
    * VCPKG_PATH: contains the path where external dependences are installed with vcpkg, e.g. E:/projects/vcpkg/installed/x64-windows-static-md
    * MUMBLE_PROTOC: path to protoc compiler, e.g. E:\projects\vcpkg\installed\x64-windows-static-md\tools\protobuf\protoc.exe
    * PROTOBUF_PATH: path to the root folder where protobuf include and lib folders reside, e.g. E:\projects\vcpkg\installed\x64-windows-static-md
    * OPENSSL_PATH: path to openSSL libraries. It is recommended to use the version bundled with Qt5, e.g. C:\Qt\Tools\OpenSSL\Win_x64
- In QtCreator select the Release build and make sure that the build directory is set to <root>/build folder
- In order to compile the application, open in QtCreator the main project file, main.pro and start the compilation. The application executable and additional DLLs are generated in build folder.
- Generate the installer by starting from a command window build-bubbles.bat script. The installer can also be found in build folder.


## Translations Generation

"C:\Qt\5.15.2\msvc2019_64\bin\lupdate.exe" E:\projects\mumble\src\mumble\mumble.qrc src/mumble/ServerTableModel.h src/mumble/ServerTableModel.cpp src/mumble/AudioDeviceModel.h src/mumble/AudioDeviceModel.cpp src/mumble/TokensModel.h src/mumble/TokensModel.cpp src/mumble/CertificateModel.h src/mumble/CertificateModel.cpp src/mumble/main.cpp src/mumble/Messages.cpp src/mumble/Log.cpp src/mumble/MainWindow.cpp -ts bubbles_da_DK.ts
"C:\Qt\5.15.2\msvc2019_64\bin\lrelease.exe" bubbles_da_DK.ts