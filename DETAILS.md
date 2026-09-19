# Where libdds gains performance

## Avoid moving the same pixels twice

An application may already hold both the DDS bytes and its final output buffer.
Loading through an owning image and decoding into another temporary image adds
storage work before those pixels reach their destination.

The *caller-owned decompression* commit writes decoded pixels directly into the
application's destination. This avoids allocating and initializing a whole-image
result only to copy it again. Existing allocating overloads remain available.
