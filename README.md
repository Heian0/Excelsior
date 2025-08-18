A high performance market data parser and disseminator (SPMC) for NASDAQ equities. The parser lives in include/parser/ItchParser.hpp and src/parser/ItchParser.cpp and the SPMC queue used to disseminate the parsed messages lives in the utils folder.

Some optimization techniques I used are:

mmap + sequential scan: lets the kernel prefetch pages efficiently. Memory gets walked linearly which helps to be more I-cache/D-cache friendly and minimizes syscall overhead compared to if I was to use streams.

inline constexpr readers ask the compiler to inline byte-swaps and address arithmetic.

I used a dispatch table instead of a large switch with many branches for distinguishing NASDAQ message types so I get constant-time lookup by uint8_t type. Helpful for branch prediction and I-cache locality.

No allocations in the hot path, i construct plain old data on the stack and then use a single memcpy into the ring/buffer. Keeps working set of addresses small.
