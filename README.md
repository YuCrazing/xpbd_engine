# xpbd_engine
A XPBD simulation framework.


## Setting up Environment

```bash
git submodule update --init --recursive
./third_party/vcpkg/bootstrap-vcpkg.sh
```

## Building && Running

Clean up:
```bash
./build_linux.sh --clean
```

Build:
```bash
./build_linux.sh
```

Run:
```bash
./build/xpbd_engine
```