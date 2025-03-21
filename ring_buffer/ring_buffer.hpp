#include <cstddef>
#include <vector>
class RingBuffer {
 public:
  explicit RingBuffer(size_t capacity) : kMaxSize(capacity) {
    left_stack_.reserve(capacity);
    right_stack_.reserve(capacity);
  }
  size_t Size() const { return left_stack_.size() + right_stack_.size(); }
  bool Empty() const { return Size() == 0; }
  bool TryPush(int element) {
    if (left_stack_.size() + right_stack_.size() == kMaxSize) {
      return false;
    }
    left_stack_.push_back(element);
    return true;
  }

  bool TryPop(int* element) {
    if (Empty()) {
      return false;
    }
    if (right_stack_.empty()) {
      while (!left_stack_.empty()) {
        right_stack_.push_back(left_stack_.back());
        left_stack_.pop_back();
      }
    }
    *element = right_stack_.back();
    right_stack_.pop_back();
    return true;
  }

 private:
  std::vector<int> left_stack_;
  std::vector<int> right_stack_;
  const size_t kMaxSize;
};
