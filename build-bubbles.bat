set VCPKG_ROOT=E:/projects/vcpkg
set INSTALL_PATH=E:/projects/mumble-install
set BUILD_NUMBER=0
set RELEASE_ID=0

rmdir /S /Q build
mkdir build
pushd build

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

cmake -G "Ninja" "-DVCPKG_TARGET_TRIPLET=x64-windows-static-md" "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" "-Dpackaging=ON" "-Dtranslations=OFF" "-DRELEASE_ID=%RELEASE_ID%" "-DBUILD_NUMBER=%BUILD_NUMBER%" "-DCMAKE_INSTALL_PREFIX=%INSTALL_PATH%" "-DCMAKE_BUILD_TYPE=Release" ..
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

cmake --build .
if %errorlevel% neq 0 (
    popd
    exit /b %errorlevel%
)

ninja windeployqt
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
