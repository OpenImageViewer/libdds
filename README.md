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

### Where libdds gains performance

#### Avoid moving the same pixels twice

An application may already hold both the DDS bytes and its final output buffer.
Loading through an owning image and decoding into another temporary image adds
storage work before those pixels reach their destination.

The *validated DDS views* commit removes the input copy for eligible layouts.
Its [view loader](DirectXTex/DirectXTexDDS.cpp#L2027) validates the DDS and creates
surface descriptors pointing into the caller's bytes. Native pixels can be consumed
there. Compressed pixels still need decoding, but their encoded data need not be
copied first. Layouts requiring repair or stronger alignment retain the owning
fallback, and borrowed input must stay alive and unchanged.

The *caller-owned decompression* and *caller-owned conversion* commits remove the
other intermediate: they write directly into the application's destination.
This avoids allocating and initializing a whole-image result only to copy it again.
Conversion can still allocate temporary scanlines. These storage savings require callers
to adopt the new APIs. Existing allocating overloads remain available.

#### Resolve setup once, then process pixels

After removing avoidable storage work, repeated format handling becomes another
source of cost. The *prepared scanline conversion* commit resolves format metadata
and transfer flags once per image in CPU conversion and BC encoding/decoding.
[Scanline processing](DirectXTex/DirectXTexConvert.cpp#L3154) reuses that state and
skips conversion arithmetic when numeric and channel semantics already match and
no transfer-function change remains.

The *decompression writer selection* commit similarly chooses the RGBA8 or generic
output path before the block loop. The *batched RGBA8 packing* commit gives
conversion and decompression a [shared writer](DirectXTex/DirectXTexConvert.cpp#L1655)
that packs four pixels at a time using SSE2 or NEON. It preserves the existing
rounding order and stores the remaining pixels individually. The *unity
grouping* commit makes these shared stores available for compiler inlining by
building compression and conversion together.

#### Reuse calculations within each block

BC4 and BC5 have another repeated calculation: pixels select from eight
interpolated values per channel. The *interpolation palette caching* commit
[computes those values once per block](DirectXTex/BC4BC5.cpp#L398), then indexes them
for each pixel. It retains the original interpolation formulas and signed-endpoint
rules. The earlier *unaligned BC4/BC5 payload safety* fix supplies safe local word
loads independently of this optimization.

These changes affect different workloads differently. Native images benefit mainly
from avoided allocation and copying. Compressed images also benefit from reduced
setup, packing and interpolation work, but still pay for decoding and writing every
output pixel. Their combined speedups cannot be attributed to any one commit.

### License

[MIT license](LICENSE). Bundled dependency licenses remain in their directories.
