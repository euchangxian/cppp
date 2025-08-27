#include "src/stl/Vector.hpp"

#include <gtest/gtest.h>

#include "src/testutil/LifetimeTracker.hpp"

namespace ecx::stl {
namespace test {

TEST(VectorTest, DefaultConstructor) {
  Vector<int> underTest;

  EXPECT_EQ(underTest.size(), 0);
  EXPECT_EQ(underTest.capacity(), 0);
  EXPECT_EQ(underTest.data(), nullptr);
  EXPECT_TRUE(underTest.empty());
}

TEST(VectorTest, ConstructorWithSizeOnlyShouldDefaultConstruct) {
  Vector<int> underTest(10);

  EXPECT_EQ(underTest.size(), 10);
  EXPECT_EQ(underTest.capacity(), 10);
  EXPECT_NE(underTest.data(), nullptr);

  for (int x : underTest) {
    EXPECT_EQ(x, 0);
  }
}

TEST(VectorTest, ConstructorWithSizeAndSeedShouldFillWithSeed) {
  int seed = 20;
  Vector<int> underTest(10, seed);

  EXPECT_EQ(underTest.size(), 10);
  EXPECT_EQ(underTest.capacity(), 10);
  EXPECT_NE(underTest.data(), nullptr);

  for (int x : underTest) {
    EXPECT_EQ(x, seed);
  }
}

TEST(VectorTest, ConstructorWithInitializerList) {
  Vector<int> underTest{0, 1, 2};

  EXPECT_EQ(underTest.size(), 3);
  EXPECT_EQ(underTest.capacity(), 3);
  EXPECT_NE(underTest.data(), nullptr);

  EXPECT_EQ(underTest[0], 0);
  EXPECT_EQ(underTest[1], 1);
  EXPECT_EQ(underTest[2], 2);
}

TEST(VectorTest, CopyConstructorCreatesDeepCopy) {
  Vector<std::string> original{"hello", "world"};
  Vector<std::string> copy(original);

  EXPECT_EQ(original.size(), copy.size());
  EXPECT_EQ(original[0], copy[0]);

  // modified
  copy[0] = "greetings";
  EXPECT_EQ(original[0], "hello");
  EXPECT_EQ(copy[0], "greetings");
}

TEST(VectorTest, CopyAssignmentSelfAssignmentShouldDoNothing) {
  Vector<int> underTest{1, 2, 3};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
  underTest = underTest;
#pragma GCC diagnostic pop

  EXPECT_EQ(underTest.size(), 3);
  EXPECT_EQ(underTest[0], 1);
  EXPECT_EQ(underTest[2], 3);
}

TEST(VectorTest, MoveConstructorStealsResourcesAndLeavesSourceEmpty) {
  Vector<std::string> source{"hello", "world"};
  Vector<std::string> destination(std::move(source));

  EXPECT_EQ(destination.size(), 2);
  EXPECT_EQ(destination.capacity(), 2);
  EXPECT_EQ(destination[0], "hello");

  EXPECT_EQ(source.size(), 0);
  EXPECT_EQ(source.capacity(), 0);
  EXPECT_EQ(source.data(), nullptr);
}

TEST(VectorTest, MoveAssignmentSelfAssignment) {
  Vector<int> underTest{1, 2, 3};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
  underTest = std::move(underTest);
#pragma GCC diagnostic pop

  // Per the standard, a self-move leaves the object in a "valid but
  // unspecified state". The most important thing is that it doesn't crash.
  // Asserting the size remains unchanged is a reasonable check for this case.
  EXPECT_EQ(underTest.size(), 3);
  EXPECT_EQ(underTest[0], 1);
}

TEST(VectorTest, NonTrivialTypeDestructorsCalledOnResizeShrink) {
  LifetimeTracker::reset();
  {
    Vector<LifetimeTracker> underTest(5);
    EXPECT_EQ(LifetimeTracker::constructions, 5);
    EXPECT_EQ(LifetimeTracker::destructions, 0);

    underTest.resize(2);  // This should destroy 3 objects
    EXPECT_EQ(LifetimeTracker::constructions, 5);
    EXPECT_EQ(LifetimeTracker::destructions, 3);
  }  // The remaining 2 objects are destroyed here
  EXPECT_EQ(LifetimeTracker::constructions, LifetimeTracker::destructions);
}

TEST(VectorTest, MoveOnlyTypeCanPushBackAndReserve) {
  // This test will fail to compile if move semantics are wrong
  Vector<std::unique_ptr<int>> underTest;

  underTest.push_back(std::make_unique<int>(10));
  underTest.push_back(std::make_unique<int>(20));

  // Trigger a reallocation, which requires moving move-only elements
  underTest.reserve(10);
  underTest.push_back(std::make_unique<int>(30));

  EXPECT_EQ(underTest.size(), 3);
  EXPECT_EQ(*underTest[0], 10);
  EXPECT_EQ(*underTest[2], 30);
}

TEST(VectorTest, PushBackOnceGrows) {
  Vector<int> underTest;
  underTest.push_back(0);

  EXPECT_EQ(underTest.size(), 1);
  EXPECT_EQ(underTest.capacity(), 1);
  EXPECT_NE(underTest.data(), nullptr);
  EXPECT_FALSE(underTest.empty());
}

TEST(VectorTest, PushBackTwiceDoublesCapacity) {
  Vector<int> underTest;
  underTest.push_back(0);
  underTest.push_back(1);

  EXPECT_EQ(underTest.size(), 2);
  EXPECT_EQ(underTest.capacity(), 2);
  EXPECT_NE(underTest.data(), nullptr);
}

TEST(VectorTest, PushBackThriceDoublesCapacity) {
  Vector<int> underTest;
  underTest.push_back(0);
  underTest.push_back(1);
  underTest.push_back(2);

  EXPECT_EQ(underTest.size(), 3);
  EXPECT_EQ(underTest.capacity(), 4);
  EXPECT_NE(underTest.data(), nullptr);
}

}  // namespace test
}  // namespace ecx::stl
