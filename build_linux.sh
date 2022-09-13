CLEAN_BUILD=NO
# parse command line args
# https://stackoverflow.com/a/14203146/12003165
for i in "$@"; do
  case $i in
    --clean)
      CLEAN_BUILD=YES
      ;;
    -*|--*)
      echo "Unknown option $i"
      exit 1
      ;;
    *)
      ;;
  esac
done

if [ "$CLEAN_BUILD" = "YES" ]; then
  rm -fr ./build
  echo "removed build directory"
  exit 0
fi


# Taichi related paths
BACKEND_NAME="cuda" # cuda, x64, vulkan
TAICHI_REPO="/home/yuzhang/Work/taichi-1" # example: /home/taichigraphics/workspace/taichi
AOT_DIRECTORY="/tmp/aot_files"
RUNTIME_LIB="${TAICHI_REPO}/python/taichi/_lib/runtime"
PACKAGE_PATH="${TAICHI_REPO}/python/taichi"

mkdir build
cd build

cmake .. -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_TOOLCHAIN_FILE=../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DTAICHI_REPO=${TAICHI_REPO} \
  -DCMAKE_BUILD_TYPE=Debug

make -j

TI_LIB_DIR=${RUNTIME_LIB} ./xpbd_engine ${AOT_DIRECTORY} ${PACKAGE_PATH} ${BACKEND_NAME}