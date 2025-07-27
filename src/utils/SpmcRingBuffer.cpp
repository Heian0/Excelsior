#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <array>
#include <optional>
#include <cstdint>
#include <functional>
#include <atomic>
#include <memory>
#include <new>

template <size_t N>
concept PowerOfTwo = (N & (N - 1)) == 0 && N > 0;

template <typename T, std::size_t N = 1024>
requires PowerOfTwo<N>
class SPMC_RingBuffer {
public:
    SPMC_RingBuffer() : writeIdx_(0) {}

    // Producer only
    void write(const T& data) {
        size_t idx = writeIdx_++ & (N - 1);
        Slot& slot = buffer_[idx];

        slot.seq.store(1, std::memory_order_release); // odd = write in progress
        std::memcpy(&slot.data, &data, sizeof(T));    // memcpy ensures no UB for non-trivial types
        slot.seq.store(2, std::memory_order_release); // even = write complete
    }

    // Consumer manages ConsumerState in their own thread
    std::optional<T> tryRead(size_t readIdx) const {
        size_t idx = readIdx & (N - 1);
        const Slot& slot = buffer_[idx];

        uint32_t seq0 = slot.seq.load(std::memory_order_acquire);
        if (seq0 & 1) return std::nullopt; // write in progress

        T tmp;
        std::memcpy(&tmp, &slot.data, sizeof(T));

        uint32_t seq1 = slot.seq.load(std::memory_order_acquire);
        if (seq0 != seq1) return std::nullopt; // invalid read

        return tmp;
    }

    constexpr std::size_t capacity() const noexcept {
        return N;
    }

    size_t getwriteIndex() const noexcept {
        return writeIdx_.load(std::memory_order_acquire);
    }

private:
    // Use Seqlocks for synchronization
    struct Slot {
        std::atomic<uint32_t> seq{0};
        alignas(64) T data;
    };
    
    alignas(64) std::array<Slot, N> buffer_;
    alignas(64) std::atomic<size_t> writeIdx_;
};

struct alignas(64) ConsumerState {
    size_t readIdx = 0;
};  

using BlockVersion = uint32_t;
using PayloadSize = uint32_t;
using WriteCallback = std::function<void(uint8_t* data)>;

struct Block
{
    // Local block versions reduce contention for the queue
    std::atomic<BlockVersion> version{0};
    // Size of the data
    std::atomic<PayloadSize> payloadSize{0};
    // 64 byte payload
    alignas(std::hardware_destructive_interference_size) uint8_t payload[64]{};
};

struct Header
{
    // Block count
    alignas(std::hardware_destructive_interference_size) std::atomic<uint64_t> writeIdx {0};
};


class SPMC_Queue {
public:
    SPMC_Queue(size_t size): size_(size), blocks_(std::make_unique<Block[]>(size)) {}
    ~SPMC_Queue() = default;  

    void Write(PayloadSize size, WriteCallback write){
        // the next block index to write to
        uint64_t blockIndex = header_.writeIdx.fetch_add(1, std::memory_order_acquire) % size_;
        Block &block = blocks_[blockIndex];

        BlockVersion currentVersion = block.version.load(std::memory_order_acquire) + 1;
        // If the block has been written to before, it has an odd version
        // we need to make its version even before writing begins to indicate that writing is in progress
        if (block.version.load(std::memory_order_acquire) % 2 == 1){
            // make the version even
            block.version.store(currentVersion, std::memory_order_release);
            // store the newVersion used for after the writing is done
            currentVersion++;
        }
        // store the size
        block.payloadSize.store(size, std::memory_order_release);
        // perform write using the callback function
        write(block.payload);
        // store the new odd version
        block.version.store(currentVersion, std::memory_order_release);
    }

    bool Read (uint64_t blockIndex, uint8_t* data, PayloadSize& size) const {
        // Block
        Block &block = blocks_[blockIndex];
        // Block version
        BlockVersion version = block.version.load(std::memory_order_acquire);
        // Read when version is odd
        if(version % 2 == 1){
            // Size of the data
            size = block.payloadSize.load(std::memory_order_acquire);
            // Perform the read
            std::memcpy(data, block.payload, size);
            // Indicate that a read has occurred by adding a 2 to the version
            // However do not block subsequent reads
            block.version.store(version + 2, std::memory_order_release);
            return true;
        }
        return false;
    }

    constexpr size_t size () const {
        return size_;
    }

private:
    Header header_;
    size_t size_;
    std::unique_ptr<Block[]> blocks_;
};

