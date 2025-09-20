#include <cassert>
#include <cstdint>
#include <iostream>

namespace runtime {
struct Unit {
  virtual void process(std::int64_t) = 0;
  virtual std::int64_t result() const = 0;
};

struct Store : Unit {
  std::int64_t m_result;

  void process(std::int64_t x) { m_result = x; }

  std::int64_t result() const { return m_result; }
};

struct Doubler : Unit {
  Unit* m_next;
  Doubler(Unit* next) : m_next(next) {}
  void process(std::int64_t x) { return m_next->process(x * 2); }
  std::int64_t result() const { return m_next->result(); }
};

struct ShiftRighter : Unit {
  Unit* m_next;
  ShiftRighter(Unit* next) : m_next(next) {}
  void process(std::int64_t x) { return m_next->process(x >> 1); }
  std::int64_t result() const { return m_next->result(); }
};

struct Pipeline {
  Store store;
  Doubler doubler{&store};
  ShiftRighter shiftRighter{&doubler};
} storage;

Unit* pipeline = &storage.shiftRighter;
auto usePipeline() {
  pipeline->process(42);
  return pipeline->result();
}

static_assert(sizeof(Pipeline) == 48, "");
}  // namespace runtime

namespace compiletime {
struct Store {
  std::int64_t m_result;

  void process(std::int64_t x) { m_result = x; }

  std::int64_t result() const { return m_result; }
};

template <typename Next>
struct Doubler : Next {
  void process(std::int64_t x) { Next::process(x * 2); }
};

template <typename Next>
struct ShiftRighter : Next {
  void process(std::int64_t x) { Next::process(x >> 1); }
};

using Pipeline = ShiftRighter<Doubler<Store>>;

auto usePipeline() {
  Pipeline pipeline;
  pipeline.process(42);
  return pipeline.result();
}
static_assert(sizeof(Pipeline) == 8, "");
}  // namespace compiletime

int main() {
  std::cout << compiletime::usePipeline() << '\n';
  std::cout << runtime::usePipeline() << '\n';
}
