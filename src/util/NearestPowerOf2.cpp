#include <cstdint>
#include <iostream>

uint64_t nearestPowerOfTwo(uint64_t n) {
  if (n == 0 || n == 1) {
    return 1;
  }
  return static_cast<uint64_t>(1) << (64 - __builtin_clzll(n - 1));
}

int main() {
  std::cout << "nearestPowerOfTwo(100): " << nearestPowerOfTwo(100)
            << ", expected: 128\n";

  std::cout << "nearestPowerOfTwo(128): " << nearestPowerOfTwo(128)
            << ", expected: 128\n";

  std::cout << "nearestPowerOfTwo(0): " << nearestPowerOfTwo(0)
            << ", expected: 1\n";
}
