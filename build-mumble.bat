set VCPKG_ROOT=E:/projects/vcpkg

rmdir /S /Q build
mkdir build
pushd build

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 exit /b %errorlevel%

cmake -G "NMake Makefiles" "-DVCPKG_TARGET_TRIPLET=x64-windows-static-md" "-Dstatic=ON" "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" "-DIce_HOME=%VCPKG_ROOT%/installed/x64-windows-static-md" "-Dclient=ON" "-Dserver=OFF" "-Dzeroconf=OFF" "-Ddbus=OFF" "-DCMAKE_BUILD_TYPE=Release" ..

popd
