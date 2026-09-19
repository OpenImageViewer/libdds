# Where libdds gains performance

## Avoid moving the same pixels twice

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

## Resolve setup once, then process pixels

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
rounding order and stores the remaining pixels individually.

## Reuse calculations within each block

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
