#include <atomic>
#include <memory>
#include <utility>

#include "src/stl/UniquePointer.hpp"

namespace ecx::stl {

// The problem right now is, there are many ways to create a shared pointer.
// Each of these different ways require managing the memory of the object and
// the control block differently.
//
// makeShared:
// Memory for Object and Control Block are allocated in one allocation.
// Object is destroyed when sharedCount is 0.
// Only when weakCount is 0, will the CB be destroyed, and the memory for both
// released.
// I suppose this will use ::operator new
//
// Constructor:
// Memory for Object is allocated outside of the SharedPointer (actually, we
// dont know if the memory was allocated using new, or operator new?)
// Allocate memory for the Control Block.
// SharedCount = 0 => destroy and release the memory for the object
// WeakCount = 0 => destroy and release memory for Control Block
template <typename T>
class SharedPointer {
 public:
  using pointer = T*;

 protected:
  template <typename Deleter = std::default_delete<T>>
  struct ControlBlock : Deleter {
    std::atomic<std::size_t> sharedCount;
    std::atomic<std::size_t> weakCount;
  };

 public:
  explicit SharedPointer(pointer ptr = nullptr);

  SharedPointer(const SharedPointer& other);
  SharedPointer(SharedPointer&& other) noexcept;

  template <typename Deleter>
  SharedPointer(UniquePointer<T, Deleter>&& other);

  SharedPointer operator=(const SharedPointer& other);
  SharedPointer operator=(SharedPointer&& other) noexcept;

  ~SharedPointer();

  // Exchange the stored pointer values and the ownership of *this and other.
  // Reference counts, if any, are not adjusted.
  void swap(SharedPointer& other) noexcept;

  // Replaces the managed object with an object pointed to by other.
  void reset() noexcept;
  template <typename Deleter>
  void reset(pointer other, Deleter deleter = std::default_delete<T>());

  pointer get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; };
  T& operator->() const noexcept { return *ptr_; };

  // Returns the number of SharedPointer objects referring to the same managed
  // object.
  auto useCount() const noexcept;

  explicit operator bool() const noexcept { return !!ptr_; }

 private:
  pointer ptr_;
  std::shared_ptr<int> sptr_;
};

template <typename T, typename... Args>
SharedPointer<T> makeShared(Args&&... args) {
  return SharedPointer<T>(new T(std::forward<Args>(args)...));
}

template <typename Deleter, typename T>
Deleter* getDeleter(SharedPointer<T> ptr) noexcept;

}  // namespace ecx::stl
