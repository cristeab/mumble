set QT5_ROOT=C:/Qt/5.15.2/msvc2019_64
set INSTALL_PATH=E:/projects/mumble-install

rem rmdir /S /Q build
rmdir /S /Q "%INSTALL_PATH%" 
rem mkdir build
pushd build

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

cmake -G "Ninja" .. -DCMAKE_PREFIX_PATH=%QT5_ROOT% -DCMAKE_INSTALL_PREFIX=%INSTALL_PATH%
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

ninja package
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

popd
