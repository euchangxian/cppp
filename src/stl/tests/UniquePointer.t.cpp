#include "src/stl/UniquePointer.hpp"

#include <gtest/gtest.h>

namespace ecx::stl {
namespace test {

struct UniquePointerTest : ::testing::Test {
  struct CustomDeleter {
    void operator()(auto* ptr) const { delete ptr; }
  };

  struct StatefulDeleter {
    int state = 0;
    void operator()(auto* ptr) const { delete ptr; }
  };
};

TEST_F(UniquePointerTest, StatelessDeleterIsOptimisedOut) {
  using UniquePointerDefaultDeleter = UniquePointer<int>;
  using UniquePointerCustomDeleter = UniquePointer<int, CustomDeleter>;

  static_assert(sizeof(int*) == 8);
  static_assert(sizeof(UniquePointerDefaultDeleter) == sizeof(int*));
  static_assert(sizeof(UniquePointerCustomDeleter) == sizeof(int*));
}

TEST_F(UniquePointerTest, DeleterIsPartOfType) {
  using UniquePointerDefaultDeleter = UniquePointer<int>;
  using UniquePointerCustomDeleter = UniquePointer<int, CustomDeleter>;

  static_assert(
      !std::is_same_v<UniquePointerDefaultDeleter, UniquePointerCustomDeleter>);
}

TEST_F(UniquePointerTest, DefaultConstructorCreatesEmptyPointer) {
  UniquePointer<int> underTest;

  EXPECT_EQ(underTest.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(underTest));
}

TEST_F(UniquePointerTest, ConstructorWithRawPointerTakesOwnership) {
  UniquePointer<int> underTest(new int(100));

  EXPECT_NE(underTest.get(), nullptr);
  EXPECT_TRUE(static_cast<bool>(underTest));
  EXPECT_EQ(*underTest, 100);
}

TEST_F(UniquePointerTest, MoveConstructorTransfersOwnership) {
  int* ptr = new int(100);

  UniquePointer<int> original(ptr);
  UniquePointer<int> moved(std::move(original));

  EXPECT_EQ(moved.get(), ptr);
  EXPECT_EQ(*moved, 100);
  EXPECT_TRUE(static_cast<bool>(moved));

  EXPECT_EQ(original.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(original));
}

TEST_F(UniquePointerTest, MoveAssignmentTransfersOwnership) {
  UniquePointer<int> original(new int(100));
  UniquePointer<int> destination(new int(200));

  int* originalRawPtr = original.get();

  destination = std::move(original);

  EXPECT_EQ(destination.get(), originalRawPtr);
  EXPECT_EQ(*destination, 100);

  EXPECT_EQ(original.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(original));
}

TEST_F(UniquePointerTest, ReleaseReturnsRawPointerAndRelinquishesOwnership) {
  UniquePointer<int> ptr(new int(100));
  int* rawPtr = ptr.release();

  EXPECT_NE(rawPtr, nullptr);
  EXPECT_EQ(*rawPtr, 100);

  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(ptr));

  delete rawPtr;
}

TEST_F(UniquePointerTest, ResetFreesOldAndTakesNewPointer) {
  UniquePointer<int> ptr(new int(100));
  int* newPtr = new int(200);
  ptr.reset(newPtr);

  EXPECT_EQ(ptr.get(), newPtr);
  EXPECT_EQ(*ptr, 200);

  ptr.reset();
  EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(UniquePointerTest, AccessorsReturnCorrectValues) {
  UniquePointer<int> ptr(new int(100));
  EXPECT_EQ(*ptr, 100);
  EXPECT_EQ(ptr.operator->(), ptr.get());
  EXPECT_EQ(*(ptr.operator->()), 100);
  EXPECT_TRUE(static_cast<bool>(ptr));
}

TEST_F(UniquePointerTest,
       UniquePointerWithStatefulDeleterHasNonEmptyBaseClassSize) {
  using UniquePointerStatefulDeleter = UniquePointer<int, StatefulDeleter>;
  static_assert(sizeof(UniquePointerStatefulDeleter) == 2 * sizeof(int*));
}

TEST_F(UniquePointerTest, SelfMoveAssignmentIsHandled) {
  UniquePointer<int> ptr(new int(100));
  int* rawPtr = ptr.get();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
  ptr = std::move(ptr);
#pragma GCC diagnostic pop

  EXPECT_NE(ptr.get(), nullptr);
  EXPECT_EQ(ptr.get(), rawPtr);
  EXPECT_EQ(*ptr, 100);
}

TEST_F(UniquePointerTest,
       MakeUniqueCorrectlyConstructsAndReturnsUniquePointer) {
  UniquePointer<int> ptr = makeUnique<int>(200);
  EXPECT_NE(ptr.get(), nullptr);
  EXPECT_EQ(*ptr, 200);

  struct NonTrivial {
    int a;
    std::string b;
    NonTrivial(int val, std::string str) : a(val), b(std::move(str)) {}
  };

  UniquePointer<NonTrivial> complexPtr =
      makeUnique<NonTrivial>(10, "test string");
  EXPECT_EQ(complexPtr->a, 10);
  EXPECT_EQ(complexPtr->b, "test string");
}

}  // namespace test
}  // namespace ecx::stl
