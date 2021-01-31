set VCPKG_ROOT=E:/projects/vcpkg
set BUILD_NUMBER=0
set RELEASE_ID=0

rmdir /S /Q build
mkdir build
pushd build

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 exit /b %errorlevel%

cmake -G "NMake Makefiles" "-DVCPKG_TARGET_TRIPLET=x64-windows-static-md" "-Dstatic=ON" "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" "-Dclient=ON" "-Dserver=OFF" "-Dzeroconf=OFF" "-Ddbus=OFF" "-Doverlay=OFF" "-Dplugins=OFF" "-Dpackaging=ON" "-DRELEASE_ID=%RELEASE_ID%" "-DBUILD_NUMBER=%BUILD_NUMBER%" "-DCMAKE_BUILD_TYPE=Release" ..
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build .
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --install .
if %errorlevel% neq 0 exit /b %errorlevel%

popd
