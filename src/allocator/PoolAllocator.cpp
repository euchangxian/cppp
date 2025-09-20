#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

constexpr uint64_t nearestPowerOfTwo(uint64_t n) {
  if (n == 0 || n == 1) {
    return 1;
  }

  return 1ULL << (64 - __builtin_clzll(n - 1));
}

// PoolAllocator manages memory allocation for objects of type T using memory
// pools.
template <typename T, size_t PoolSize = 64>
class PoolAllocator {
 private:
  // Represents a block of memory that can be allocated.
  struct Chunk {
    // When a Chunk is free, the next ptr contains the address of
    // the next free chunk.
    Chunk* next;
  };

  // Represents the size in bytes of each of the Memory Pool to allocate
  const std::size_t mPoolSize_;

  // Represents the size of the alignment. For aligned allocation
  const std::size_t mAlignment_;

  // Represents the number of Chunks per Memory Pool
  const std::size_t mChunksPerPool_;

  // Represents the list of Memory Pools in use. Allows efficient deallocation
  // of an entire pool.
  std::vector<void*> mPools_;

  // Represents the list of available memory blocks. Allows for constant-time
  // allocation and deallocation on a per-object level.
  Chunk* mFreelist;

  // Acquires a new Memory Pool and updates the freelist.
  void allocatePool() noexcept {
    void* newPool = std::aligned_alloc(mAlignment_, mPoolSize_);
    if (newPool == nullptr) {
      // Early return if memory allocation fails
      return;
    }

    // Update the list of Pools being in use.
    mPools_.push_back(newPool);

    // Splice this whole list onto the freelist. Order is important! Otherwise
    // we risk leaking memory.
    // A reassignment is okay here, since allocatePool is only invoked when
    // there are no free Chunks.
    assert(mFreelist == nullptr);
    mFreelist = reinterpret_cast<Chunk*>(newPool);

    // Split the newly acquired memory into segments of mAlignment and add them
    // to the freelist.
    Chunk* chunk = mFreelist;
    for (size_t i = 1; i < mChunksPerPool_; ++i) {
      // First reinterpret_cast the Chunk* to a byte ptr, then add sizeof(T)
      // bytes to it. The result will be the start of the next available Chunk.
      // reinterpret_cast the address back to a Chunk*.
      chunk->next = reinterpret_cast<Chunk*>(reinterpret_cast<byte*>(chunk) +
                                             mAlignment_);
      chunk = chunk->next;
    }
    // Set the end of the list to null
    chunk->next = nullptr;

    return;
  }

 public:
  PoolAllocator(const size_t poolSize)
      : mPoolSize_(poolSize),
        mAlignment_(std::max(64ULL, nearestPowerOfTwo(sizeof(T)))),
        mChunksPerPool_(mPoolSize_ / mAlignment_),
        mFreelist(nullptr) {
    // Initial pool allocation can be deferred until first use. Lazy allocation
  }

  ~PoolAllocator() {
    for (void* pool : mPools_) {
      std::free(pool);
    }
  }

  /**
   * Returns a Pointer to the allocated memory.
   *
   * @param n Number of objects to allocate. Must be 1.
   * @return Pointer to the allocated memory or nullptr if allocation fails.
   */
  T* allocate(size_t n = 1) noexcept {
    assert(n == 1 && "PoolAllocator can only allocate one object at a time.");

    // If there are available memory blocks
    if (mFreelist) {
      T* addr = reinterpret_cast<T*>(mFreelist);
      mFreelist = mFreelist->next;
      return addr;
    }

    // Otherwise, acquire a new Memory Pool
    allocatePool();

    if (mFreelist == nullptr) {
      // Failed to allocate memory
      return nullptr;
    }

    // Allocate the memory from the newly acquired pool
    T* addr = reinterpret_cast<T*>(mFreelist);
    mFreelist = mFreelist->next;
    return addr;
  }

  /**
   * Reclaims the block of memory pointed to by the ptr.
   *
   * @param ptr Pointer to the memory to deallocate.
   * @param n Number of objects to deallocate. Must be 1.
   */
  void deallocate(T* ptr, size_t n = 1) {
    assert(n == 1 && "PoolAllocator can only deallocate one object at a time.");

    if (ptr == nullptr) {
      return;
    }

#ifndef NDEBUG
    // Check for the validity of the ptr to be deallocated
    bool fromPools{false};
    for (void* pool : mPools_) {
      if (ptr >= pool && reinterpret_cast<byte*>(ptr) <
                             static_cast<byte*>(pool) + mPoolSize_) {
        fromPools = true;
        break;
      }
    }

    assert(fromPools && "Deallocating memory not from memory pool");
#endif

    // Assume that the ptr is a valid address within the pool.
    Chunk* newlyFreed = reinterpret_cast<Chunk*>(ptr);

    // Insert to the front of the list.
    newlyFreed->next = mFreelist;
    mFreelist = newlyFreed;
  }
};

class TestClass {
 private:
  static PoolAllocator<TestClass> allocator;

  int32_t a;
  double b;

 public:
  TestClass(int32_t a, double b) : a(a), b(b) {}

  void* operator new(std::size_t size) { return allocator.allocate(1); }

  void operator delete(void* ptr) {
    allocator.deallocate(static_cast<TestClass*>(ptr));
  }
};

// Initialize the static allocator with a pool size.
// For example, 1024 chunks * sizeof(TestClass).
// Adjust poolSize based on the expected number of allocations and
// sizeof(TestClass).
constexpr size_t poolSize = 4 * 1024;  // 4096 Bytes = 4KB
PoolAllocator<TestClass> TestClass::allocator(poolSize);

int main() {
  // Demonstration of allocating and deallocating TestClass objects using
  // PoolAllocator

  // Allocate a single TestClass object
  TestClass* obj1 = new TestClass(42, 3.14);

  // Allocate another TestClass object
  TestClass* obj2 = new TestClass(7, 2.718);

  // Use the objects (for demonstration purposes, we'll print their addresses)
  std::cout << "obj1 address: " << obj1 << "\n";
  std::cout << "obj2 address: " << obj2 << "\n";

  // Deallocate the objects
  delete obj1;
  delete obj2;

  // Allocate more objects to demonstrate reuse of freed memory
  TestClass* obj3 = new TestClass(100, 1.618);
  TestClass* obj4 = new TestClass(256, 0.577);

  // obj3 should reuse obj2's address
  std::cout << "obj3 address: " << obj3 << "\n";

  // obj4 should reuse obj1's address
  std::cout << "obj4 address: " << obj4 << "\n";

  delete obj3;
  delete obj4;

  return 0;
}
