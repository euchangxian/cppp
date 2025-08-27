#pragma once

struct LifetimeTracker {
  inline static int constructions = 0;
  inline static int destructions = 0;
  inline static int copyConstructions = 0;
  inline static int moveConstructions = 0;
  inline static int copyAssignments = 0;
  inline static int moveAssignments = 0;

  static void reset() {
    constructions = 0;
    destructions = 0;
    copyConstructions = 0;
    moveConstructions = 0;
  }

  LifetimeTracker() { ++constructions; }
  ~LifetimeTracker() { ++destructions; }
  LifetimeTracker(const LifetimeTracker&) noexcept { ++copyConstructions; }
  LifetimeTracker(LifetimeTracker&&) noexcept { ++moveConstructions; }
  LifetimeTracker& operator=(const LifetimeTracker&) noexcept {
    ++copyAssignments;
    return *this;
  }
  LifetimeTracker& operator=(LifetimeTracker&&) noexcept {
    ++moveAssignments;
    return *this;
  }
};
