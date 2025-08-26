#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

namespace ecx {

template <typename T>
class Vector {
 public:
  using SizeT = std::size_t;
  using ValueT = T;
  using PointerT = T*;
  using ReferenceT = T&;
  using ConstReferenceT = const T&;

  /**
   * Iterator class should be a nested class, and needs to contain a pointer
   * to the element it's currently at.
   * The core overloads are:
   * 1. T& operator*(): Dereference overload
   * 2. Iter& operator++(): prefix increment, optionally: Iter operator++(int)
   * 3. bool operator!=(): Inequality
   * 4. bool operator==(): Equality
   */
  class Iterator {
   public:
    explicit Iterator(PointerT ptr) : curr_(ptr) {}

    ReferenceT operator*() { return *curr_; }

    Iterator& operator++() {
      ++curr_;
      return *this;
    }

    Iterator operator++(int) {
      Iterator pre = *this;
      ++(*this);
      return pre;
    }

    bool operator==(const Iterator& other) const {
      return curr_ == other.curr_;
    }

    // TODO: Figure out why the operator== overload does not handle this.
    // Check out the starship operator <=>
    bool operator!=(const Iterator& other) const {
      return curr_ != other.curr_;
    }

    Iterator& operator+(SizeT x) {
      curr_ += x;
      return *this;
    }

   private:
    PointerT curr_;
  };

  using IteratorT = Iterator;
  using ReferenceIterator = Iterator&;
  using ConstReferenceIterator = const Iterator&;

  explicit Vector() noexcept : size_(0), capacity_(0), data_(nullptr) {}

  explicit Vector(SizeT n) : size_(n), capacity_(n), data_(new T[n]) {}

  explicit Vector(SizeT n, ConstReferenceT init)
      : size_(n), capacity_(n), data_(new T[n]) {
    // TODO:
  }

  Vector(std::initializer_list<ValueT> init) {
    // TODO:
  }

  Vector(ConstReferenceT other)
      : size_(other.size_),

        capacity_(other.capacity_),
        data_(new T[other.capacity_]) {
    for (SizeT i = 0; i < other.size_; ++i) {
      data_[i] = other.data_[i];
    }
  }

  // NOTE:
  // Can be optimised to not allocate memory if the current capacity
  // is sufficient. But this is succinct.
  ReferenceT operator=(ConstReferenceT other) {
    if (this != &other) {
      Vector temp(other);
      std::swap(data_, temp.data_);
      std::swap(size_, temp.size_);
      std::swap(capacity_, temp.capacity_);
    }

    return *this;
  }

  Vector(Vector&& other) noexcept
      : size_(std::exchange(other.size(), 0)),
        capacity_(std::exchange(other.capacity(), 0)),
        data_(std::exchange(other.data(), nullptr)) {}

  ReferenceT operator=(Vector&& other) noexcept {
    if (this != &other) {
      delete[] data_;

      size_ = std::exchange(other.size_, 0);
      capacity_ = std::exchange(other.capacity_, 0);
      data_ = std::exchange(other.data_, nullptr);
    }

    return *this;
  }

  ~Vector() {
    if (data_ != nullptr) {
      delete[] data_;
    }
  }

  // TODO: Const Iterators
  IteratorT begin() { return Iterator(data_); }

  IteratorT end() { return Iterator(data_ + size_); }

  /**
   * Increase the capacity of the vector (the total number of elements that the
   * vector can hold without requiring reallocation) to a value that's greater
   * or equal to new_cap. If new_cap is greater than the current capacity(), new
   * storage is allocated, otherwise the function does nothing.
   *
   * reserve() does not change the size of the vector.
   *
   * If new_cap is greater than capacity(), all iterators (including the end()
   * iterator) and all references to the elements are invalidated. Otherwise, no
   * iterators or references are invalidated.
   *
   * After a call to reserve(), insertions will not trigger reallocation unless
   * the insertion would make the size of the vector greater than the value of
   * capacity().
   */
  void reserve(SizeT newCapacity) {
    if (capacity_ >= newCapacity) {
      return;
    }

    // NOTE: Copy-and-Swap idiom for Exception Safety.
    // If an exception occurs especially in std::memcpy, then in the naive way
    // where only a buffer is created, there will be a memory leak.
    // Hence, we exploit RAII, creating a temporary Vector.
    Vector temp;
    temp.size_ = size_;
    temp.capacity_ = newCapacity;
    temp.data_ = new T[newCapacity];

    if constexpr (std::is_trivially_copyable_v<T>) {
      if (temp.size_ > 0) {
        std::memcpy(temp.data_, data_, temp.size_ * sizeof(T));
      }
    } else {
      for (SizeT i = 0; i < temp.size_; ++i) {
        temp.data_[i] = std::move_if_noexcept(data_[i]);
      }
    }

    // no need to explicitly delete data_, the other Vector will handle.
    std::swap(data_, temp.data_);
    std::swap(size_, temp.size_);
    std::swap(capacity_, temp.capacity_);
  }

  // TODO: shrink and expand
  /**
   * Resizes the container to contain count elements:
   * If count is equal to the current size, does nothing.
   * If the current size is greater than count, the container is reduced to its
   * first count elements.
   * If the current size is less than count, then:
   * 1) Additional copies of T()(until C++11)default-inserted elements(since
   *    C++11) are appended.
   * 2) Additional copies of value are appended.
   */
  void resize(SizeT newSize) {
    if (size_ == newSize) {
      return;
    }

    // shrink
    if (newSize < size_) {
      std::destroy_n(begin() + newSize, size_ - newSize);
    } else {
      // expand.
      reserve(newSize);
      std::uninitialized_default_construct_n(begin() + size_, newSize - size_);
    }
    size_ = newSize;
  }

  void resize(SizeT newSize, ConstReferenceT value) {}

  void push_back(ConstReferenceT elem) {
    if (size_ + 1 >= capacity_) {
      reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_++] = elem;
  }

  void push_back(T&& elem) {
    if (size_ + 1 >= capacity_) {
      reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_++] = std::move(elem);
  }

  template <typename... Args>
  ReferenceT emplace_back(Args&&... args) {}

  void pop_back() {}

  ReferenceT back() {
    // Not sure if an if-check is done here.
    return data_[size_ - 1];
  }

  ConstReferenceT back() const {
    // Not sure if an if-check is done here.
    return data_[size_ - 1];
  }

  SizeT size() const { return size_; }

  bool empty() const { return size_ == 0; }

  SizeT capacity() const { return capacity_; }

  PointerT data() const { return data_; }

  ReferenceT operator[](SizeT i) { return data_[i]; }

  ConstReferenceT operator[](SizeT i) const { return data_[i]; }

 private:
  SizeT size_{};

  SizeT capacity_{};

  PointerT data_;
};

}  // namespace ecx
