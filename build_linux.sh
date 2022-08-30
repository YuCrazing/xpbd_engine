rm -fr ./build
mkdir build
cd build
cmake .. -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE=/home/yuzhang/Work/xpbd_engine/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
make -j