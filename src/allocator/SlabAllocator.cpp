#include <array>
#include <cstddef>
#include <list>
#include <vector>

constexpr size_t kPageSizeBytes = 4 * 1024;  // 4KB

template <typename T,
          size_t NSlabSize = kPageSizeBytes,
          size_t NObjSize = sizeof(T)>
class SlabAllocator {
 private:
  struct Slab {};

  // Allocate to partially-filled Slabs first
  Slab* slabs;

 public:
  SlabAllocator() {}

  ~SlabAllocator() {}

  // Copy Constructor
  SlabAllocator(const SlabAllocator<T>& other) {}

  // Copy Assigmnet
  SlabAllocator& operator=(const SlabAllocator<T>& other) {}

  // Move Constructor
  SlabAllocator(SlabAllocator<T>&& other) noexcept {}

  // Move Assignment
  SlabAllocator&& operator=(SlabAllocator<T>&& other) noexcept {}

  T* allocate() {}

  void deallocate(T* ptr) {}
};
