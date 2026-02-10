# OpenJPH JNI Native Library

Native JNI bridge for HTJ2K encoding via [OpenJPH](https://github.com/aous72/OpenJPH).

## Building

### Prerequisites
- CMake 3.15+
- C++11 compiler (GCC, MSVC, Clang)
- JDK 17+ (for JNI headers)

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The output is a single shared library (`libopenjph_jni.so` / `openjph_jni.dll` / `libopenjph_jni.dylib`) with OpenJPH statically linked.

### Cross-platform CI/CD

GitHub Actions builds for all supported platforms are configured in `.github/workflows/build.yml`. Tagged releases are published as Maven artifacts to the dcm4che Maven repository.

## Maven Artifact Coordinates

| Platform | GroupId | ArtifactId | Type | Classifier |
|----------|---------|------------|------|------------|
| Linux x86-64 | org.dcm4che | dcm4che-openjph-native | so | linux-x86-64 |
| Linux aarch64 | org.dcm4che | dcm4che-openjph-native | so | linux-aarch64 |
| Windows x86-64 | org.dcm4che | dcm4che-openjph-native | dll | windows-x86-64 |
| macOS aarch64 | org.dcm4che | dcm4che-openjph-native | dylib | macosx-aarch64 |
