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

mkdir build
cd build
cmake .. -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE=../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake
make -j