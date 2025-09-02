#include <memory>
#include <utility>

namespace ecx::stl {

namespace v1 {

template <typename T, typename DeleterFn = std::default_delete<T>>
class UniquePointer {
 public:
  using PointerT = T*;

  constexpr explicit UniquePointer(PointerT ptr = nullptr,
                                   DeleterFn deleter = DeleterFn()) noexcept
      : ptr_(ptr), deleter_(std::move(deleter)) {}

  constexpr explicit UniquePointer(const UniquePointer&) = delete;

  constexpr UniquePointer& operator=(const UniquePointer&) = delete;

  constexpr explicit UniquePointer(UniquePointer&& other) noexcept
      : ptr_(std::exchange(other.ptr_, nullptr)),
        deleter_(std::move(other.deleter_)) {}

  constexpr UniquePointer& operator=(UniquePointer&& other) noexcept {
    // Recall: while other is passed in as an rvalue, it binds to other as an
    // lvalue. Hence, moving it again is necessary.
    if (this == &other) {
      return *this;
    }

    reset(other.release());
    deleter_ = std::move(other.deleter_);
    return *this;
  }

  constexpr ~UniquePointer() noexcept(
      std::is_nothrow_invocable_v<DeleterFn, PointerT>) {
    reset();
  }

  constexpr void reset(PointerT p = nullptr) noexcept(
      std::is_nothrow_invocable_v<DeleterFn, PointerT>) {
    if (ptr_) {
      deleter_(ptr_);
    }
    ptr_ = p;
  }
  constexpr PointerT get() const noexcept { return ptr_; }
  constexpr PointerT release() noexcept { return std::exchange(ptr_, nullptr); }

  constexpr T& operator*() noexcept { return *ptr_; }
  constexpr const T& operator*() const noexcept { return *ptr_; }
  constexpr PointerT operator->() const noexcept { return ptr_; }
  constexpr explicit operator bool() const noexcept { return !!ptr_; }

 private:
  PointerT ptr_;
  DeleterFn deleter_;
};

template <typename T, typename... Args>
constexpr UniquePointer<T> makeUnique(Args&&... args) {
  return UniquePointer<T>(new T(std::forward<Args>(args)...));
}

}  // namespace v1

inline namespace v2 {

// Empty Base Class optimisation
template <typename T, typename Deleter = std::default_delete<T>>
class UniquePointer : private Deleter {
 public:
  using PointerT = T*;
  using DeleterT = Deleter;

  static constexpr auto isNoThrowDeleter =
      std::is_nothrow_invocable_v<DeleterT, PointerT>;

  constexpr explicit UniquePointer(PointerT ptr = nullptr,
                                   DeleterT deleter = DeleterT()) noexcept
      : DeleterT(std::move(deleter)), ptr_(ptr) {}

  constexpr explicit UniquePointer(const UniquePointer&) = delete;

  constexpr UniquePointer& operator=(const UniquePointer&) = delete;

  constexpr explicit UniquePointer(UniquePointer&& other) noexcept
      : DeleterT(std::move(other.getDeleter())), ptr_(other.release()) {}

  constexpr UniquePointer& operator=(UniquePointer&& other) noexcept(
      isNoThrowDeleter) {
    if (this == &other) {
      return *this;
    }

    reset(other.release());
    getDeleter() = std::move(other.getDeleter());
    return *this;
  }

  constexpr ~UniquePointer() noexcept(isNoThrowDeleter) { reset(); }

  constexpr void reset(PointerT p = nullptr) noexcept(isNoThrowDeleter) {
    // NOTE: the order is important for exception safety.
    // If delete is done first, and an exception is thrown, then the passed-in
    // pointer will dangle.
    PointerT temp = std::exchange(ptr_, p);
    if (temp) {
      getDeleter()(temp);
    }
  }
  constexpr PointerT get() const noexcept { return ptr_; }
  constexpr PointerT release() noexcept { return std::exchange(ptr_, nullptr); }

  constexpr T& operator*() noexcept { return *ptr_; }
  constexpr const T& operator*() const noexcept { return *ptr_; }
  constexpr PointerT operator->() const noexcept { return ptr_; }
  constexpr explicit operator bool() const noexcept { return !!ptr_; };

 private:
  constexpr DeleterT& getDeleter() noexcept {
    return static_cast<DeleterT&>(*this);
  }

  constexpr const DeleterT& getDeleter() const noexcept {
    return static_cast<DeleterT&>(*this);
  }

  PointerT ptr_;
};

template <typename T, typename... Args>
constexpr UniquePointer<T> makeUnique(Args&&... args) {
  return UniquePointer<T>(new T(std::forward<Args>(args)...));
}

}  // namespace v2
}  // namespace ecx::stl
