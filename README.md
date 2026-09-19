# libdds

A standalone CPU-only extraction of DirectXTex, used by CodecDDS as an external
submodule. The library retains the DirectX namespace and CPU implementations.
It does not link WRL, COM, WIC or Direct3D. Windows file I/O still uses Win32;
Linux and macOS use the existing C++ filesystem/stream implementations.

## Build

```sh
cmake -S . -B build/libdds -DCMAKE_BUILD_TYPE=Release
cmake --build build/libdds --config Release
ctest --test-dir build/libdds -C Release --output-on-failure
```

CMake 3.20 and C++17 are required. Dependencies are bundled, so configuration and
compilation require no downloads. Add this directory with `add_subdirectory`
and link `libdds::libdds`. Tests default on for standalone builds and off when
embedded; `LIBDDS_BUILD_TESTS` controls them explicitly. The library is static
with position-independent code. OpenMP is not enabled by this build.

## API and behavior

Include `DirectXTex.h`. The CPU API keeps `HRESULT` return codes, `DXGI_FORMAT`,
`Image`, `TexMetadata` and `ScratchImage` ownership. HRESULT is signed 32-bit on
all targets. Load DDS with `LoadFromDDSMemory` or `LoadFromDDSFile`; compressed
input remains compressed until `Decompress` is called. Arrays, cubemaps, volumes
and mip chains retain upstream layout and indexing.

CPU compression, HDR/TGA I/O, conversion, resizing, mipmap generation, normal maps,
alpha processing and image evaluation/transforms are retained. WIC and GPU
operations are unavailable; conversion and filtering use CPU paths. DirectXMath
is header-only CPU math, not a Direct3D runtime dependency.

Do not link this extraction and full DirectXTex into the same executable: they
preserve the same symbols. Keep the public header and library from the same
revision. Windows consumers may still include SDK graphics headers; libdds does
not provide their runtime operations.

## Correctness layer

The platform bridge and WIC guards isolate the CPU subset. Private resource-limit
constants retain upstream DDS validation limits. Vendor source formatting is
preserved apart from CRLF normalization.

- BC7 encoding: align colors for packed-vector loads and define the zero rounding bias at eight-bit precision.
- RGB/BGR swizzling: use unaligned-safe word copies for serialized pixel storage, including TGA data.
- Pitch calculation: reject addition overflow in alignment and planar-height calculations, in addition to upstream multiplication checks.
- 24-bit DDS writing: follow the actual encoded header choice, including implicit DX10 arrays; validate source rows, slices and temporary-buffer width. Final source rows need not include trailing padding.
- BC4/BC5 decoding: copy encoded words to aligned local storage before accessing them. This safety fix does not cache interpolation palettes.
- DDS parsing: read serialized magic and headers without alignment assumptions.

## Upstream and licenses

- DirectXTex: `868198cb4bcbc4e359372e7ba38d7a6dda3a6afa` (2.1.1), MIT.
- DirectXMath: `d837578297c6c93849573858182350ede04987dc` (`feb2024`, 3.19), MIT.
- `dxgiformat.h`: DirectX-Headers `c94b9b23aaadc2034dd1cad656a5a69f1526f98a`, MIT.
- Portable SAL annotations: dotnet/runtime `v8.0.0`, MIT.

Licenses remain in `LICENSE`, `External/DirectXMath/LICENSE` and
`External/SAL/LICENSE`. `upstream.json` records normalized source hashes before
local patches.

The imported 2.1.1 CPU subset includes bounded HDR header/RLE parsing, checked pitch
multiplication, input validation, public `CalculateMipLevels` and
`CalculateMipLevels3D`, permissive dimension-zero DX10 loading, and
`DDS_FLAGS_FORCE_24BPP_RGB`. CodecDDS uses strict DDS loading. R8_UNORM/A8_UNORM
rounding can differ from 2.0.8 by one code value, and invalid inputs may receive
different errors. These are upstream changes. GPU, WIC, tool-only and optional
format-library updates are outside this subset.

## Validation

Standalone tests cover known BC pixels/alpha, every BC family, signed/HDR data,
odd dimensions, DDS arrays/cubes/volumes/mips, malformed data, HDR/TGA memory/file
I/O and retained image processing. They require no external framework or assets.
Each later feature includes focused regression coverage or reuses the appropriate
earlier parity tests.

On Windows, pass `-DLIBDDS_REFERENCE_SOURCE_DIR=path/to/pinned/DirectXTex` to
compile untouched upstream CPU sources in a separate executable. The parity test
compares binary snapshots with WIC disabled; the reference checkout is not modified.
