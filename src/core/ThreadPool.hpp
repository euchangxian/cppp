#include <bits/stdc++.h>

template <std::size_t NumThreads = std::thread::hardware_concurrency()>
class ThreadPool {
 public:
 private:
  std::array<std::jthread, NumThreads> pool_;
};
