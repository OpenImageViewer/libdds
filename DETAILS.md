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
