#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

template <typename T>
class Deque {
 public:
  Deque()
      : buckets_(1, nullptr),
        first_bucket_(0),
        last_bucket_(0),
        first_index_(0),
        last_index_(0),
        size_(0) {
    new_bucket(0);
  }

  Deque(const Deque& other)
      : size_(other.size_),
        buckets_(other.buckets_.size(), nullptr),
        first_bucket_(other.first_bucket_),
        last_bucket_(other.last_bucket_),
        first_index_(other.first_index_),
        last_index_(other.last_index_) {
    try {
      for (int i = first_bucket_; i <= last_bucket_; ++i) {
        new_bucket(i);
        for (int j = 0; j < bucket_size_; ++j) {
          if ((i == first_bucket_ && j < first_index_) ||
              (i == last_bucket_ && j > last_index_)) {
            continue;
          }
          new (buckets_[i] + j) T(other.buckets_[i][j]);
        }
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  Deque(size_t count) {
    size_ = count;
    if (count == 0) {
      buckets_ = std::vector<T*>(1, nullptr);
      first_bucket_ = 0;
      last_bucket_ = 0;
      first_index_ = 0;
      last_index_ = 0;
      new_bucket(0);
      return;
    }
    buckets_ = std::vector<T*>((count - 1) / bucket_size_ + 1, nullptr);
    first_bucket_ = 0;
    last_bucket_ = (count - 1) / bucket_size_;
    first_index_ = 0;
    last_index_ = ((count % bucket_size_ - 1) + bucket_size_) % bucket_size_;
    try {
      for (int i = 0; i <= last_bucket_; ++i) {
        new_bucket(i);
        for (int j = 0; j < bucket_size_; ++j) {
          if (i == last_bucket_ && j == last_index_ + 1) {
            break;
          } 
          new (buckets_[i] + j) T();
        }
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  Deque(size_t count, const T& value) {
    size_ = count;
    if (count == 0) {
      buckets_ = std::vector<T*>(1, nullptr);
      first_bucket_ = 0;
      last_bucket_ = 0;
      first_index_ = 0;
      last_index_ = 0;
      new_bucket(0);
      return;
    }
    buckets_ = std::vector<T*>((count - 1) / bucket_size_ + 1, nullptr);
    first_bucket_ = 0;
    last_bucket_ = (count - 1) / bucket_size_;
    first_index_ = 0;
    last_index_ = ((count % bucket_size_ - 1) + bucket_size_) % bucket_size_;
    try {
      for (int i = 0; i <= last_bucket_; ++i) {
        new_bucket(i);
        for (int j = 0; j < bucket_size_; ++j) {
          if (i == last_bucket_ && j == last_index_ + 1) {
            break;
          }
          new (buckets_[i] + j) T(value);
        }
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  ~Deque() { clear(); }

  Deque& operator=(const Deque<T>& other) {
    Deque new_deque(other);
    std::swap(new_deque.buckets_, buckets_);
    std::swap(new_deque.size_, size_);
    std::swap(new_deque.first_bucket_, first_bucket_);
    std::swap(new_deque.last_bucket_, last_bucket_);
    std::swap(new_deque.first_index_, first_index_);
    std::swap(new_deque.last_index_, last_index_);
    return *this;
  }

  size_t size() const { return size_; }

  bool empty() { return size_ == 0; }

  T& operator[](int index) {
    if (last_bucket_ == first_bucket_) {
      return buckets_[first_bucket_][first_index_ + index];
    }
    int elem_first_bucket = bucket_size_ - first_index_;
    if (index + 1 <= elem_first_bucket) {
      return buckets_[first_bucket_][first_index_ + index];
    }
    index -= elem_first_bucket;
    return buckets_[first_bucket_ + 1 + index / bucket_size_]
                   [index % bucket_size_];
  }

  const T& operator[](int index) const {
    if (last_bucket_ == first_bucket_) {
      return buckets_[first_bucket_][first_index_ + index];
    }
    int elem_first_bucket = bucket_size_ - first_index_;
    if (index + 1 <= elem_first_bucket) {
      return buckets_[first_bucket_][first_index_ + index];
    }
    index -= elem_first_bucket;
    return buckets_[first_bucket_ + 1 + index / bucket_size_]
                   [index % bucket_size_];
  }

  T& at(size_t index) {
    if (index >= (size_t)size_) {
      throw std::out_of_range("");
    }
    return (*this)[index];
  }

  const T& at(size_t index) const {
    if (index >= (size_t)size_) {
      throw std::out_of_range("");
    }
    return (*this)[index];
  }

  void push_back(T element) {
    if (last_bucket_ == (int)buckets_.size() - 1) {
      realloc();
    }
    if (size_ == 0) {
      size_++;
      try {
        if (buckets_[last_bucket_] == nullptr) {
          new_bucket(last_bucket_);
        }
        new (buckets_[last_bucket_] + last_index_) T(element);
      } catch (...) {
        --size_;
        throw;
      }
      return;
    }
    if (last_index_ == bucket_size_ - 1) {
      ++last_bucket_;
      last_index_ = -1;
      new_bucket(last_bucket_);
    }
    ++last_index_;
    ++size_;
    try {
      if (buckets_[last_bucket_] == nullptr) {
        new_bucket(last_bucket_);
      }
      new (buckets_[last_bucket_] + last_index_) T(element);
    } catch (...) {
      if (last_index_ == 0) {
        last_index_ = bucket_size_;
        if (buckets_[last_bucket_] != nullptr &&
            last_bucket_ != first_bucket_) {
          delete_bucket(last_bucket_);
        }
        last_bucket_--;
      }
      --last_index_;
      --size_;
      throw;
    }
  }

  void pop_back() {
    if (buckets_[last_bucket_] != nullptr) {
      buckets_[last_bucket_][last_index_].~T();
    }
    if (size_ == 1) {
      size_--;
      return;
    }
    --last_index_;
    if (last_index_ == -1) {
      last_index_ = bucket_size_ - 1;
      if (first_bucket_ != last_bucket_ && buckets_[last_bucket_] != nullptr) {
        delete_bucket(last_bucket_);
      }
      last_bucket_--;
    }
    --size_;
  }

  void push_front(T element) {
    if (first_bucket_ == 0) {
      realloc();
    }
    if (size_ == 0) {
      size_++;
      try {
        if (buckets_[first_bucket_] == nullptr) {
          new_bucket(first_bucket_);
        }
        new (buckets_[last_bucket_] + last_index_) T(element);
      } catch (...) {
        --size_;
        throw;
      }
      return;
    }
    if (size_ == 0) {
      return;
    }
    if (first_index_ == 0) {
      --first_bucket_;
      new_bucket(first_bucket_);
      first_index_ = bucket_size_;
    }
    first_index_--;
    ++size_;
    try {
      if (buckets_[first_bucket_] == nullptr) {
        new_bucket(first_bucket_);
      }
      new (buckets_[first_bucket_] + first_index_) T(element);
    } catch (...) {
      if (first_index_ == bucket_size_ - 1) {
        if (buckets_[first_bucket_] != nullptr) {
          delete_bucket(first_bucket_);
        }
        ++first_bucket_;
        first_index_ = -1;
      }
      ++first_index_;
      --size_;
    }
  }

  void pop_front() {
    if (buckets_[first_bucket_] != nullptr) {
      buckets_[first_bucket_][first_index_].~T();
    }
    if (size_ == 1) {
      size_--;
      return;
    }
    ++first_index_;
    if (first_index_ == bucket_size_) {
      first_index_ = 0;
      if (first_bucket_ != last_bucket_ && buckets_[first_bucket_] != nullptr) {
        delete_bucket(first_bucket_);
      }
      first_bucket_++;
    }
    --size_;
  }

  template <bool IsConst>
  class Iterator {
   public:
    using value_type = T;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using iterator_category = std::random_access_iterator_tag;

    Iterator(std::vector<T*>* bucket_ptr, int bucket, int index,
             int bucket_size)
        : bucket_ptr_(bucket_ptr),
          index_(index),
          bucket_(bucket),
          bucket_size_(bucket_size) {}
    Iterator(const std::vector<T*>* bucket_ptr, int bucket, int index,
             int bucket_size)
        : bucket_ptr_(const_cast<std::vector<T*>*>(bucket_ptr)),
          index_(index),
          bucket_(bucket),
          bucket_size_(bucket_size) {}
    reference operator*() const { return (*bucket_ptr_)[bucket_][index_]; }

    pointer operator->() const { return &(*bucket_ptr_)[bucket_][index_]; }

    Iterator& operator++() {
      ++index_;
      if (index_ == bucket_size_) {
        index_ = 0;
        ++bucket_;
      }
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    Iterator& operator--() {
      --index_;
      if (index_ == -1) {
        index_ = bucket_size_ - 1;
        --bucket_;
      }
      return *this;
    }

    Iterator operator--(int) {
      Iterator tmp = *this;
      --(*this);
      return tmp;
    }

    Iterator& operator+=(size_t n) {
      int tmp = bucket_ * bucket_size_ + index_ + n;
      bucket_ = (tmp) / bucket_size_;
      index_ = tmp % bucket_size_;
      return *this;
    }

    Iterator operator+(size_t n) const {
      Iterator tmp = *this;
      tmp += n;
      return tmp;
    }

    Iterator& operator-=(size_t n) {
      int tmp = bucket_ * bucket_size_ + index_ - n;
      bucket_ = (tmp) / bucket_size_;
      index_ = tmp % bucket_size_;
      return *this;
    }

    Iterator operator-(size_t n) const {
      Iterator tmp = *this;
      tmp -= n;
      return tmp;
    }

    int operator-(const Iterator& other) const {
      return (bucket_ * bucket_size_ + index_) -
             (other.bucket_ * bucket_size_ + other.index_);
    }

    bool operator==(const Iterator& other) const {
      return bucket_ == other.bucket_ && index_ == other.index_ &&
             bucket_ptr_ == other.bucket_ptr_;
    }

    bool operator!=(const Iterator& other) const { return !(*this == other); }

    bool operator<(const Iterator& other) const {
      return bucket_ < other.bucket_ ||
             (bucket_ == other.bucket_ && index_ < other.index_);
    }

    bool operator>(const Iterator& other) const { return other < *this; }

    bool operator<=(const Iterator& other) const { return !(other < *this); }

    bool operator>=(const Iterator& other) const { return !(*this < other); }

   private:
    int index_;
    int bucket_;
    std::vector<T*>* bucket_ptr_;
    int bucket_size_;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  iterator begin() {
    return iterator(&buckets_, first_bucket_, first_index_, bucket_size_);
  }

  const_iterator cbegin() const {
    return const_iterator(&buckets_, first_bucket_, first_index_, bucket_size_);
  }

  iterator end() {
    if (size_ == 0) {
      return begin();
    }
    if (last_index_ == bucket_size_ - 1) {
      return iterator(&buckets_, last_bucket_ + 1, 0, bucket_size_);
    }
    return iterator(&buckets_, last_bucket_, last_index_ + 1, bucket_size_);
  }

  const_iterator cend() const {
    if (size_ == 0) {
      return cbegin();
    }
    if (last_index_ == bucket_size_ - 1) {
      return const_iterator(&buckets_, last_bucket_ + 1, 0, bucket_size_);
    }
    return const_iterator(&buckets_, last_bucket_, last_index_ + 1,
                          bucket_size_);
  }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  reverse_iterator rend() { return reverse_iterator(begin()); }
  iterator insert(iterator pos, const T& value) {
    if (pos == end()) {
      push_back(value);
      return end() - 1;
    }
    if (pos == begin()) {
      push_front(value);
      return begin();
    }
    int index = pos - begin();
    push_back(value);
    for (int i = size_ - 1; i > index; --i) {
      (*this)[i] = (*this)[i - 1];
    }
    (*this)[index] = value;
    return begin() + index;
  }
  void erase(iterator pos) {
    if (pos == end() - 1) {
      pop_back();
      return;
    }
    for (auto it = pos; it != end() - 1; ++it) {
      *it = *(it + 1);
    }
    pop_back();
  }

 private:
  std::vector<T*> buckets_;
  int bucket_size_ = 2 * 2 * 2 * 2 * 2;
  int first_bucket_ = 0;
  int last_bucket_ = 0;
  int first_index_ = 0;
  int last_index_ = 0;
  int size_ = 0;

  void new_bucket(int index) {
    buckets_[index] =
        reinterpret_cast<T*>(new int8_t[bucket_size_ * sizeof(T)]);
  }

  void delete_bucket(int index) {
    delete[] reinterpret_cast<int8_t*>(buckets_[index]);
    buckets_[index] = nullptr;
  }

  void realloc() {
    std::vector<T*> new_vec(buckets_.size() * 3, nullptr);
    size_t offset = buckets_.size();
    for (int i = 0; i < (int)buckets_.size(); ++i) {
      new_vec[offset + i] = buckets_[i];
    }
    first_bucket_ += offset;
    last_bucket_ += offset;
    buckets_ = new_vec;
  }

  void clear() {
    for (int i = first_bucket_; i <= last_bucket_; ++i) {
      if (buckets_[i] != nullptr) {
        for (int j = 0; j < bucket_size_; ++j) {
          if ((i == first_bucket_ && j < first_index_) ||
              (i == last_bucket_ && j > last_index_)) {
            continue;
          }
          buckets_[i][j].~T();
        }
      }
      delete_bucket(i);
    }
    first_bucket_ = 0;
    last_bucket_ = 0;
    first_index_ = 0;
    last_index_ = 0;
    buckets_.clear();
  }
};
