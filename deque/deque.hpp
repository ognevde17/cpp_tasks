#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

const int kSize = 32;

template <typename T, typename Allocator = std::allocator<T>>
class Deque {
 public:
  using allocator_type = Allocator;

  Deque() = default;
  Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator());
  explicit Deque(const Allocator& alloc) : alloc_(alloc) {}
  Deque(const Deque& other);
  Deque(Deque&& other) noexcept;
  Deque(size_t count, const Allocator& alloc = Allocator());
  Deque(size_t count, const T& value, const Allocator& alloc = Allocator());
  ~Deque();

  Deque& operator=(const Deque& other);
  Deque& operator=(Deque&& other) noexcept;
  void swap(Deque& new_deque) noexcept;

  size_t size() const;
  bool empty() const;

  T& operator[](size_t index);
  const T& operator[](size_t index) const;
  T& at(size_t index);
  const T& at(size_t index) const;
  void push_back();
  void push_back(const T& element);
  void push_back(T&& element);
  void pop_back();
  void push_front(const T& element);
  void push_front(T&& element);
  void pop_front();
  template <typename... Args>
  void emplace_back(Args&&... args);
  template <typename... Args>
  void emplace_front(Args&&... args);
  allocator_type get_allocator() const { return alloc_; }

  template <bool IsConst>
  class Iterator {
   public:
    using value_type = T;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using iterator_category = std::random_access_iterator_tag;

    Iterator(T** bucket_ptr, int bucket, int index, int bucket_size);
    Iterator(const T* const* bucket_ptr, int bucket, int index,
             int bucket_size);

    reference operator*() const;
    pointer operator->() const;

    Iterator& operator++();
    Iterator operator++(int);
    Iterator& operator--();
    Iterator operator--(int);

    Iterator& operator+=(size_t n);
    Iterator operator+(size_t n) const;
    Iterator& operator-=(size_t n);
    Iterator operator-(size_t n) const;
    int operator-(const Iterator& other) const;

    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;
    bool operator<(const Iterator& other) const;
    bool operator>(const Iterator& other) const;
    bool operator<=(const Iterator& other) const;
    bool operator>=(const Iterator& other) const;

   private:
    size_t index_;
    size_t bucket_;
    T** bucket_ptr_;
    size_t bucket_size_;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  iterator begin();
  const_iterator cbegin() const;
  iterator end();
  const_iterator cend() const;
  reverse_iterator rbegin();
  reverse_iterator rend();

  iterator insert(iterator pos, const T& value);
  iterator insert(iterator pos, T&& value);
  void erase(iterator pos);

 private:
  std::vector<T*> buckets_;
  size_t bucket_size_ = kSize;
  size_t first_bucket_ = 0;
  size_t last_bucket_ = 0;
  size_t first_index_ = 0;
  size_t last_index_ = 0;
  size_t size_ = 0;
  [[no_unique_address]] Allocator alloc_;

  using alloc_traits = std::allocator_traits<Allocator>;

  void new_bucket(int index);
  void delete_bucket(int index);
  void realloc();
  void initialize_first_element();
  void clear();
};
template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(std::initializer_list<T> init,
                           const Allocator& alloc)
    : alloc_(alloc) {
  try {
    for (auto elem : init) {
      push_back(std::move(elem));
    }
  } catch (...) {
    clear();
    throw;
  }
}
template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(const Deque& other)
    : alloc_(alloc_traits::select_on_container_copy_construction(other.alloc_)),
      buckets_(other.buckets_.size(), nullptr) {
  if (other.empty()) {
    return;
  }
  size_ = 0;
  first_bucket_ = last_bucket_ = first_index_ = last_index_ = 0;
  try {
    for (size_t i = 0; i < other.size(); ++i) {
      push_back(other[i]);
    }
  } catch (...) {
    clear();
    throw;
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(Deque&& other) noexcept
    : alloc_(std::move(other.alloc_)),
      buckets_(std::move(other.buckets_)),
      bucket_size_(other.bucket_size_),
      first_bucket_(other.first_bucket_),
      last_bucket_(other.last_bucket_),
      first_index_(other.first_index_),
      last_index_(other.last_index_),
      size_(other.size_) {
  other.size_ = 0;
  other.first_bucket_ = 0;
  other.last_bucket_ = 0;
  other.first_index_ = 0;
  other.last_index_ = 0;
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(size_t count, const Allocator& alloc)
    : alloc_(alloc) {
  if (count == 0) {
    return;
  }
  try {
    for (size_t i = 0; i < count; ++i) {
      push_back();
    }
  } catch (...) {
    clear();
    throw;
  }
}
template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(size_t count, const T& value, const Allocator& alloc)
    : alloc_(alloc), size_(count) {
  if (count == 0) {
    buckets_ = std::vector<T*>(0);
    return;
  }
  buckets_ = std::vector<T*>(0);
  first_bucket_ = 0;
  last_bucket_ = 0;
  first_index_ = 0;
  last_index_ = 0;
  size_ = 0;
  try {
    for (size_t i = 0; i < count; ++i) {
      push_back(value);
    }
  } catch (...) {
    clear();
    throw;
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>::~Deque() {
  clear();
}

template <typename T, typename Allocator>
Deque<T, Allocator>& Deque<T, Allocator>::operator=(const Deque& other) {
  if (this != &other) {
    Deque tmp(other);
    if (alloc_traits::propagate_on_container_copy_assignment::value) {
      alloc_ = other.alloc_;
    }
    swap(tmp);
  }
  return *this;
}

template <typename T, typename Allocator>
Deque<T, Allocator>& Deque<T, Allocator>::operator=(Deque&& other) noexcept {
  if (&other == this) {
    return *this;
  }
  if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
    alloc_ = std::move(other.alloc_);
  }
  Deque tmp(std::move(other));
  swap(tmp);
  return *this;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::swap(Deque& new_deque) noexcept {
  std::swap(buckets_, new_deque.buckets_);
  std::swap(size_, new_deque.size_);
  std::swap(first_bucket_, new_deque.first_bucket_);
  std::swap(last_bucket_, new_deque.last_bucket_);
  std::swap(first_index_, new_deque.first_index_);
  std::swap(last_index_, new_deque.last_index_);
  if constexpr (alloc_traits::propagate_on_container_swap::value) {
    std::swap(alloc_, new_deque.alloc_);
  }
}

template <typename T, typename Allocator>
size_t Deque<T, Allocator>::size() const {
  return size_;
}

template <typename T, typename Allocator>
bool Deque<T, Allocator>::empty() const {
  return size_ == 0;
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::operator[](size_t index) {
  if (last_bucket_ == first_bucket_) {
    return buckets_[first_bucket_][first_index_ + index];
  }
  size_t elem_first_bucket = bucket_size_ - first_index_;
  if (index + 1 <= elem_first_bucket) {
    return buckets_[first_bucket_][first_index_ + index];
  }
  index -= elem_first_bucket;
  return buckets_[first_bucket_ + 1 + index / bucket_size_]
                 [index % bucket_size_];
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::operator[](size_t index) const {
  if (last_bucket_ == first_bucket_) {
    return buckets_[first_bucket_][first_index_ + index];
  }
  size_t elem_first_bucket = bucket_size_ - first_index_;
  if (index + 1 <= elem_first_bucket) {
    return buckets_[first_bucket_][first_index_ + index];
  }
  index -= elem_first_bucket;
  return buckets_[first_bucket_ + 1 + index / bucket_size_]
                 [index % bucket_size_];
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::at(size_t index) {
  if (index >= size_) {
    throw std::out_of_range("bad index");
  }
  return (*this)[index];
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::at(size_t index) const {
  if (index >= size_) {
    throw std::out_of_range("bad index");
  }
  return (*this)[index];
}
template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back() {
  emplace_back();
}
template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back(const T& element) {
  emplace_back(element);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back(T&& element) {
  emplace_back(std::move(element));
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::pop_back() {
  if (buckets_[last_bucket_] != nullptr) {
    alloc_traits::destroy(alloc_, buckets_[last_bucket_] + last_index_);
  }
  if (size_ == 1) {
    clear();
    return;
  }
  if (last_index_ == 0) {
    last_index_ = bucket_size_ - 1;
    if (first_bucket_ != last_bucket_ && buckets_[last_bucket_] != nullptr) {
      delete_bucket(last_bucket_);
    }
    last_bucket_--;
  } else {
    --last_index_;
  }
  --size_;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_front(const T& element) {
  emplace_front(element);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_front(T&& element) {
  emplace_front(std::move(element));
}
template <typename T, typename Allocator>
void Deque<T, Allocator>::pop_front() {
  if (buckets_[first_bucket_] != nullptr) {
    alloc_traits::destroy(alloc_, buckets_[first_bucket_] + first_index_);
  }
  if (size_ == 1) {
    clear();
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
template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::emplace_back(Args&&... args) {
  if (empty()) {
    initialize_first_element();
    try {
      alloc_traits::construct(alloc_, buckets_[last_bucket_] + last_index_,
                              std::forward<Args>(args)...);
      size_ = 1;
      return;
    } catch (...) {
      delete_bucket(last_bucket_);
      buckets_.clear();
      size_ = 0;
      throw;
    }
  }
  if (last_bucket_ == buckets_.size() - 1) {
    realloc();
  }
  ++last_index_;
  ++size_;
  if (last_index_ == bucket_size_) {
    ++last_bucket_;
    last_index_ = 0;
    new_bucket(last_bucket_);
  }
  try {
    if (buckets_[last_bucket_] == nullptr) {
      new_bucket(last_bucket_);
    }
    alloc_traits::construct(alloc_, buckets_[last_bucket_] + last_index_,
                            std::forward<Args>(args)...);
  } catch (...) {
    if (last_index_ == 0) {
      last_index_ = bucket_size_;
      if (buckets_[last_bucket_] != nullptr && last_bucket_ != first_bucket_) {
        delete_bucket(last_bucket_);
      }
      last_bucket_--;
    }
    --last_index_;
    --size_;
    throw;
  }
}
template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::emplace_front(Args&&... args) {
  if (empty()) {
    initialize_first_element();
    try {
      alloc_traits::construct(alloc_, buckets_[first_bucket_] + first_index_,
                              std::forward<Args>(args)...);
      size_ = 1;
      return;
    } catch (...) {
      clear();
      throw;
    }
  }
  if (first_bucket_ == 0) {
    realloc();
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
    alloc_traits::construct(alloc_, buckets_[first_bucket_] + first_index_,
                            std::forward<Args>(args)...);
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
    throw;
  }
}
template <typename T, typename Allocator>
template <bool IsConst>
Deque<T, Allocator>::Iterator<IsConst>::Iterator(T** bucket_ptr, int bucket,
                                                 int index, int bucket_size)
    : bucket_ptr_(bucket_ptr),
      index_(index),
      bucket_(bucket),
      bucket_size_(bucket_size) {}

template <typename T, typename Allocator>
template <bool IsConst>
Deque<T, Allocator>::Iterator<IsConst>::Iterator(const T* const* bucket_ptr,
                                                 int bucket, int index,
                                                 int bucket_size)
    : bucket_ptr_(const_cast<T**>(bucket_ptr)),
      index_(index),
      bucket_(bucket),
      bucket_size_(bucket_size) {}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>::reference
Deque<T, Allocator>::Iterator<IsConst>::operator*() const {
  return bucket_ptr_[bucket_][index_];
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>::pointer
Deque<T, Allocator>::Iterator<IsConst>::operator->() const {
  return &bucket_ptr_[bucket_][index_];
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator++() {
  ++index_;
  if (index_ == bucket_size_) {
    index_ = 0;
    ++bucket_;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator++(int) {
  Iterator tmp = *this;
  ++(*this);
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator--() {
  if (index_ == 0) {
    index_ = bucket_size_ - 1;
    --bucket_;
  } else {
    --index_;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator--(int) {
  Iterator tmp = *this;
  --(*this);
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator+=(size_t n) {
  int tmp = bucket_ * bucket_size_ + index_ + n;
  bucket_ = (tmp) / bucket_size_;
  index_ = tmp % bucket_size_;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator+(size_t n) const {
  Iterator tmp = *this;
  tmp += n;
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator-=(size_t n) {
  size_t tmp = bucket_ * bucket_size_ + index_ - n;
  bucket_ = (tmp) / bucket_size_;
  index_ = tmp % bucket_size_;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator-(size_t n) const {
  Iterator tmp = *this;
  tmp -= n;
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
int Deque<T, Allocator>::Iterator<IsConst>::operator-(
    const Iterator& other) const {
  return (bucket_ * bucket_size_ + index_) -
         (other.bucket_ * bucket_size_ + other.index_);
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator==(
    const Iterator& other) const {
  return bucket_ == other.bucket_ && index_ == other.index_ &&
         bucket_ptr_ == other.bucket_ptr_;
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator!=(
    const Iterator& other) const {
  return !(*this == other);
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator<(
    const Iterator& other) const {
  return bucket_ < other.bucket_ ||
         (bucket_ == other.bucket_ && index_ < other.index_);
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator>(
    const Iterator& other) const {
  return other < *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator<=(
    const Iterator& other) const {
  return !(other < *this);
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator>=(
    const Iterator& other) const {
  return !(*this < other);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::begin() {
  return iterator(buckets_.data(), first_bucket_, first_index_, bucket_size_);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::const_iterator Deque<T, Allocator>::cbegin()
    const {
  return const_iterator(buckets_.data(), first_bucket_, first_index_,
                        bucket_size_);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::end() {
  if (size_ == 0) {
    return begin();
  }
  if (last_index_ == bucket_size_ - 1) {
    return iterator(buckets_.data(), last_bucket_ + 1, 0, bucket_size_);
  }
  return iterator(buckets_.data(), last_bucket_, last_index_ + 1, bucket_size_);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::const_iterator Deque<T, Allocator>::cend() const {
  if (size_ == 0) {
    return cbegin();
  }
  if (last_index_ == bucket_size_ - 1) {
    return const_iterator(buckets_.data(), last_bucket_ + 1, 0, bucket_size_);
  }
  return const_iterator(buckets_.data(), last_bucket_, last_index_ + 1,
                        bucket_size_);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::reverse_iterator Deque<T, Allocator>::rbegin() {
  return reverse_iterator(end());
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::reverse_iterator Deque<T, Allocator>::rend() {
  return reverse_iterator(begin());
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::insert(
    iterator pos, const T& value) {
  if (pos == end()) {
    push_back(value);
    return end() - 1;
  }
  if (pos == begin()) {
    push_front(value);
    return begin();
  }
  size_t index = pos - begin();
  push_back(value);
  for (size_t i = size_ - 1; i > index; --i) {
    (*this)[i] = std::move((*this)[i - 1]);
  }
  (*this)[index] = value;
  return begin() + index;
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::insert(iterator pos,
                                                                   T&& value) {
  if (pos == end()) {
    push_back(std::move(value));
    return end() - 1;
  }
  if (pos == begin()) {
    push_front(std::move(value));
    return begin();
  }
  size_t index = pos - begin();
  push_back(std::move(value));
  for (size_t i = size_ - 1; i > index; --i) {
    (*this)[i] = std::move((*this)[i - 1]);
  }
  (*this)[index] = std::move(value);
  return begin() + index;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::erase(iterator pos) {
  if (pos == end() - 1) {
    pop_back();
    return;
  }
  for (auto it = pos; it != end() - 1; ++it) {
    *it = std::move(*(it + 1));
  }
  pop_back();
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::new_bucket(int index) {
  buckets_[index] = alloc_traits::allocate(alloc_, bucket_size_);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::delete_bucket(int index) {
  alloc_traits::deallocate(alloc_, buckets_[index], bucket_size_);
  buckets_[index] = nullptr;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::realloc() {
  std::vector<T*> new_vec(buckets_.size() * 3, nullptr);
  size_t offset = buckets_.size();
  for (int i = 0; i < static_cast<int>(buckets_.size()); ++i) {
    new_vec[offset + i] = buckets_[i];
  }
  first_bucket_ += offset;
  last_bucket_ += offset;
  buckets_ = new_vec;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::initialize_first_element() {
  buckets_.resize(1);
  if (buckets_[0] == nullptr) {
    new_bucket(0);
  }
  first_bucket_ = last_bucket_ = 0;
  first_index_ = last_index_ = 0;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::clear() {
  if (empty()) {
    if (buckets_.size() == 1) {
      delete_bucket(0);
      buckets_.clear();
    }
    return;
  }
  for (size_t i = first_bucket_; i <= last_bucket_; ++i) {
    if (buckets_[i] != nullptr) {
      for (size_t j = 0; j < bucket_size_; ++j) {
        if ((i == first_bucket_ && j < first_index_) ||
            (i == last_bucket_ && j > last_index_)) {
          continue;
        }
        alloc_traits::destroy(alloc_, buckets_[i] + j);
      }
    }
    delete_bucket(i);
  }
  first_bucket_ = 0;
  last_bucket_ = 0;
  first_index_ = 0;
  last_index_ = 0;
  size_ = 0;
  buckets_.clear();
}
