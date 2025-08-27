#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <new>
#include <utility>

namespace ecx::stl {

template <typename T>
class Vector {
 public:
  using SizeT = std::size_t;
  using ValueT = T;
  using PointerT = T*;
  using ConstPointerT = const T*;
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
    // Iterator Traits
    using iterator_category = std::random_access_iterator_tag;
    using value_type = ValueT;
    using difference_type = std::ptrdiff_t;
    using pointer = PointerT;
    using reference = ReferenceT;

    explicit Iterator(pointer ptr) : curr_(ptr) {}

    reference operator*() const { return *curr_; }

    Iterator& operator++() {
      ++curr_;
      return *this;
    }

    Iterator operator++(int) {
      Iterator pre = *this;
      ++(*this);
      return pre;
    }

    // C++20, compiler will generate operator != if operator== is defined.
    bool operator==(const Iterator& other) const {
      return curr_ == other.curr_;
    }

    Iterator operator+(SizeT x) const { return Iterator(curr_ + x); }

   private:
    pointer curr_;
  };

  class ConstIterator {
   public:
    // Iterator Traits
    using iterator_category = std::random_access_iterator_tag;
    using value_type = ValueT;
    using difference_type = std::ptrdiff_t;
    using pointer = ConstPointerT;
    using reference = ConstReferenceT;

    explicit ConstIterator(pointer ptr) : curr_(ptr) {}

    ConstReferenceT operator*() const { return *curr_; }

    ConstIterator& operator++() {
      ++curr_;
      return *this;
    }

    ConstIterator operator++(int) {
      ConstIterator pre = *this;
      ++(*this);
      return pre;
    }

    bool operator==(const ConstIterator& other) const {
      return curr_ == other.curr_;
    }

    ConstIterator operator+(SizeT x) const { return ConstIterator(curr_ + x); }

   private:
    pointer curr_;
  };

  using IteratorT = Iterator;
  using ConstIteratorT = ConstIterator;
  using ReverseIteratorT = std::reverse_iterator<IteratorT>;
  using ConstReverseIteratorT = std::reverse_iterator<ConstIteratorT>;

  explicit Vector() noexcept : size_(0), capacity_(0), data_(nullptr) {}

  explicit Vector(SizeT n) : size_(n), capacity_(n), data_(allocate(n)) {
    std::uninitialized_default_construct(begin(), end());
  }

  explicit Vector(SizeT n, ConstReferenceT init)
      : size_(n), capacity_(n), data_(allocate(n)) {
    std::uninitialized_fill(begin(), end(), init);
  }

  Vector(std::initializer_list<ValueT> init) : Vector() {
    reserve(init.size());
    std::uninitialized_copy(init.begin(), init.end(), begin());
    size_ = init.size();
  }

  Vector(const Vector& other)
      : size_(other.size_),
        capacity_(other.capacity_),
        data_(allocate(other.capacity_)) {
    std::uninitialized_copy(other.begin(), other.end(), begin());
  }

  Vector& operator=(const Vector& other) {
    if (this != &other) {
      Vector temp(other);
      std::swap(*this, temp);
    }

    return *this;
  }

  Vector(Vector&& other) noexcept
      : size_(std::exchange(other.size_, 0)),
        capacity_(std::exchange(other.capacity_, 0)),
        data_(std::exchange(other.data_, nullptr)) {}

  Vector& operator=(Vector&& other) noexcept {
    if (this != &other) {
      std::destroy(begin(), end());
      ::operator delete(data_);

      steal(other);
    }

    return *this;
  }

  ~Vector() {
    std::destroy(begin(), end());
    ::operator delete(data_);
  }

  IteratorT begin() { return Iterator(data_); }

  IteratorT end() { return Iterator(data_ + size_); }

  ConstIteratorT begin() const { return ConstIterator(data_); }

  ConstIteratorT end() const { return ConstIterator(data_ + size_); }

  ReverseIteratorT rbegin() { return ReverseIteratorT(end()); }

  ReverseIteratorT rend() { return ReverseIteratorT(begin()); }

  ConstReverseIteratorT rbegin() const { return ConstReverseIteratorT(end()); }

  ConstReverseIteratorT rend() const { return ConstReverseIteratorT(begin()); }

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

    // NOTE:
    // Instead of doing the CAS idiom, we make use of std::uninitialized_move,
    // which essentially:
    // 1. move if noexcept
    // 2. copy otherwise,
    // This provides Exception Safety, since the moved-from buffer can be
    // destroyed without crashing.
    // It is crucial that we do not call the destructor on the moved-from buffer
    // as it may try to release any resources that it no longer owns.
    PointerT tempBuffer = allocate(newCapacity);
    if (data_) {
      std::uninitialized_move(begin(), end(), tempBuffer);
    }
    ::operator delete(data_);

    data_ = tempBuffer;
    capacity_ = newCapacity;
  }

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

  void resize(SizeT newSize, ConstReferenceT value) {
    if (size_ == newSize) {
      return;
    }

    if (newSize < size_) {
      std::destroy_n(begin() + newSize, size_ - newSize);
    } else {
      reserve(newSize);
      std::uninitialized_fill_n(begin() + size_, newSize - size_, value);
    }
    size_ = newSize;
  }

  void push_back(ConstReferenceT elem) {
    if (size_ >= capacity_) {
      reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }

    new (&data_[size_++]) ValueT(elem);
  }

  void push_back(T&& elem) {
    if (size_ >= capacity_) {
      reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }

    new (&data_[size_++]) ValueT(std::move(elem));
  }

  template <typename... Args>
  ReferenceT emplace_back(Args&&... args) {
    if (size_ >= capacity_) {
      reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }

    new (&data_[size_]) ValueT(std::forward<Args>(args)...);
    return data_[size_++];
  }

  void pop_back() { std::destroy_at(&data_[--size_]); }

  ReferenceT back() { return data_[size_ - 1]; }

  ConstReferenceT back() const { return data_[size_ - 1]; }

  SizeT size() const noexcept { return size_; }

  bool empty() const noexcept { return size_ == 0; }

  SizeT capacity() const noexcept { return capacity_; }

  PointerT data() const noexcept { return data_; }

  ReferenceT operator[](SizeT i) { return data_[i]; }

  ConstReferenceT operator[](SizeT i) const { return data_[i]; }

 private:
  PointerT allocate(SizeT n) {
    return static_cast<PointerT>(::operator new(n * sizeof(ValueT)));
  }

  void steal(Vector& other) {
    size_ = std::exchange(other.size_, 0);
    capacity_ = std::exchange(other.capacity_, 0);
    data_ = std::exchange(other.data_, nullptr);
  }

  SizeT size_{};
  SizeT capacity_{};
  PointerT data_;
};

}  // namespace ecx::stl
