# libdds

A high-performance, standalone CPU-only port of [DirectXTex 2.1.1](https://github.com/microsoft/DirectXTex),
designed to preserve as much of the original upstream code as possible. The commit
history clearly separates **unmodified extraction → correctness fixes → optimizations**.

It retains the DirectX namespace and CPU image-processing APIs without WIC, COM,
WRL or Direct3D runtime dependencies.

## History

| Layer | Contents |
| --- | --- |
| DirectXTex 2.1.1 extraction | 22 unmodified upstream CPU files and license, followed by bundled dependencies, with only line endings normalized. |
| Build and correctness | Portable build, alignment/rounding fixes, overflow checks, DDS validation, tests and CI. |

Upstream files are imported unmodified before portability changes, correctness
fixes and optimizations. Upstream updates follow the same order.

## Build

Requires CMake 3.20 and C++17. Dependencies are bundled.

```sh
cmake -S . -B build/libdds -DCMAKE_BUILD_TYPE=Release
cmake --build build/libdds --config Release
ctest --test-dir build/libdds -C Release --output-on-failure
```

For embedding, use `add_subdirectory` and link `libdds::libdds`. Include
`DirectXTex.h`. `LIBDDS_BUILD_TESTS` defaults on for standalone builds and off
when embedded. Do not link libdds and full DirectXTex into the same executable.

## Further reading

[MIT license](LICENSE). Bundled dependency licenses remain in their directories.
