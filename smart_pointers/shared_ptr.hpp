#include <iostream>
#include <memory>
#include <type_traits>

namespace tmp {
struct ControlBlockTag {};
}  // namespace tmp

template <typename U, typename T>
concept ConvertibleFromUStar = std::is_convertible_v<U*, T*>;

struct BaseControlBlock {
  size_t shared_count;
  size_t weak_count;
  BaseControlBlock() = default;
  BaseControlBlock(size_t shared, size_t weak);
  virtual ~BaseControlBlock() = default;

  virtual void destroy_control_block() = 0;
  virtual void destroy_obj() = 0;
  virtual void* get() = 0;
};

template <typename T, typename U, typename Deleter, typename Allocator>
struct ControlBlockWithType : public BaseControlBlock {
  U* true_ptr;
  Deleter deleter;
  Allocator allocator;

  ControlBlockWithType(U* ptr1, Deleter deleter, Allocator allocator);
  void destroy_control_block() override;
  void destroy_obj() noexcept override;
  void* get() override;
};

template <typename T, typename Allocator = std::allocator<T>>
struct ControlBlockMakeShared : public BaseControlBlock {
  alignas(alignof(T)) char storage[sizeof(T)];
  Allocator allocator;

  template <typename... Args>
  ControlBlockMakeShared(Allocator allocator1, Args&&... args);

  void destroy_obj() override;
  void destroy_control_block() override;
  void* get() override;
};

template <typename T>
class SharedPtr {
 public:
  template <typename U>
  friend class SharedPtr;
  template <typename U>
  friend class WeakPtr;

  SharedPtr() = default;
  SharedPtr(std::nullptr_t);
  SharedPtr& operator=(std::nullptr_t);
  SharedPtr(T* ptr);

  template <ConvertibleFromUStar<T> U>
  SharedPtr(U* ptr);

  SharedPtr(const SharedPtr& other);

  template <ConvertibleFromUStar<T> U>
  SharedPtr(const SharedPtr<U>& other);

  SharedPtr(SharedPtr&& other);

  template <ConvertibleFromUStar<T> U>
  SharedPtr(SharedPtr<U>&& other);

  template <ConvertibleFromUStar<T> U, typename Deleter>
  SharedPtr(U* ptr, Deleter deleter);

  template <ConvertibleFromUStar<T> U, typename Deleter, typename Allocator>
  SharedPtr(U* ptr, Deleter deleter, Allocator allocator);

  SharedPtr& operator=(const SharedPtr& other) noexcept;

  template <ConvertibleFromUStar<T> U>
  SharedPtr<T>& operator=(const SharedPtr<U>& other) noexcept;

  template <ConvertibleFromUStar<T> U>
  SharedPtr<T>& operator=(SharedPtr<U>&& other) noexcept;

  SharedPtr& operator=(SharedPtr&& other) noexcept;

  T& operator*() const;
  T* operator->() const;
  T* get() const;
  void reset();
  ~SharedPtr();
  size_t use_count() const;

  SharedPtr([[maybe_unused]] tmp::ControlBlockTag tag1, T* ptr,
            BaseControlBlock* new_cb);

 private:
  BaseControlBlock* cb_ = nullptr;
  T* ptr_ = nullptr;
  void shared_count_plus();
  void shared_count_minus();
};

template <typename T>
class WeakPtr {
 public:
  WeakPtr() = default;
  WeakPtr(const WeakPtr& other);

  template <ConvertibleFromUStar<T> U>
  WeakPtr(const WeakPtr<U>& other);

  WeakPtr(WeakPtr&& other);

  template <ConvertibleFromUStar<T> U>
  WeakPtr(WeakPtr<U>&& other);

  WeakPtr(const SharedPtr<T>& other);

  template <ConvertibleFromUStar<T> U>
  WeakPtr(const SharedPtr<U>& other);

  WeakPtr& operator=(const WeakPtr& other) noexcept;
  WeakPtr& operator=(WeakPtr&& other) noexcept;
  ~WeakPtr();
  bool expired() const;
  SharedPtr<T> lock();

 private:
  BaseControlBlock* cb_ = nullptr;
  void plus_weak_count();
  void minus_weak_count();
};

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args);

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> AllocateShared(const Alloc& alloc, Args&&... args);

BaseControlBlock::BaseControlBlock(size_t shared, size_t weak)
    : shared_count(shared), weak_count(weak) {}

template <typename T, typename U, typename Deleter, typename Allocator>
ControlBlockWithType<T, U, Deleter, Allocator>::ControlBlockWithType(
    U* ptr1, Deleter deleter, Allocator allocator)
    : BaseControlBlock{1, 0},
      true_ptr(ptr1),
      deleter(std::move(deleter)),
      allocator(std::move(allocator)) {}

template <typename T, typename U, typename Deleter, typename Allocator>
void ControlBlockWithType<T, U, Deleter, Allocator>::destroy_control_block() {
  using Cballoc = typename std::allocator_traits<
      Allocator>::template rebind_alloc<ControlBlockWithType>;
  Cballoc new_alloc(allocator);
  std::allocator_traits<Cballoc>::destroy(new_alloc, this);
  std::allocator_traits<Cballoc>::deallocate(new_alloc, this, 1);
}

template <typename T, typename U, typename Deleter, typename Allocator>
void ControlBlockWithType<T, U, Deleter, Allocator>::destroy_obj() noexcept {
  deleter(true_ptr);
  true_ptr = nullptr;
}

template <typename T, typename U, typename Deleter, typename Allocator>
void* ControlBlockWithType<T, U, Deleter, Allocator>::get() {
  return static_cast<void*>(true_ptr);
}

template <typename T, typename Allocator>
template <typename... Args>
ControlBlockMakeShared<T, Allocator>::ControlBlockMakeShared(
    Allocator allocator1, Args&&... args)
    : BaseControlBlock{1, 0}, allocator(std::move(allocator1)) {
  ::new (static_cast<void*>(storage)) T(std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
void ControlBlockMakeShared<T, Allocator>::destroy_obj() {
  reinterpret_cast<T*>(&storage)->~T();
}

template <typename T, typename Allocator>
void ControlBlockMakeShared<T, Allocator>::destroy_control_block() {
  using Cballoc = typename std::allocator_traits<
      Allocator>::template rebind_alloc<ControlBlockMakeShared>;
  Cballoc new_alloc(allocator);
  std::allocator_traits<Cballoc>::destroy(new_alloc, this);
  std::allocator_traits<Cballoc>::deallocate(new_alloc, this, 1);
}

template <typename T, typename Allocator>
void* ControlBlockMakeShared<T, Allocator>::get() {
  return reinterpret_cast<void*>(&storage);
}

template <typename T>
SharedPtr<T>::SharedPtr(std::nullptr_t) {}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(std::nullptr_t) {
  reset();
  return *this;
}

template <typename T>
SharedPtr<T>::SharedPtr(T* ptr) : ptr_(ptr) {
  cb_ =
      new ControlBlockWithType<T, T, std::default_delete<T>, std::allocator<T>>(
          ptr, std::default_delete<T>(), std::allocator<T>());
}

template <typename T>
template <ConvertibleFromUStar<T> U>
SharedPtr<T>::SharedPtr(U* ptr) : ptr_(ptr) {
  cb_ =
      new ControlBlockWithType<T, U, std::default_delete<U>, std::allocator<U>>(
          ptr, std::default_delete<U>(), std::allocator<U>());
}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) {
  ptr_ = other.ptr_;
  cb_ = other.cb_;
  shared_count_plus();
}

template <typename T>
template <ConvertibleFromUStar<T> U>
SharedPtr<T>::SharedPtr(const SharedPtr<U>& other) {
  ptr_ = other.ptr_;
  cb_ = other.cb_;
  shared_count_plus();
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) : ptr_(other.ptr_), cb_(other.cb_) {
  other.ptr_ = nullptr;
  other.cb_ = nullptr;
}

template <typename T>
template <ConvertibleFromUStar<T> U>
SharedPtr<T>::SharedPtr(SharedPtr<U>&& other)
    : ptr_(other.ptr_), cb_(other.cb_) {
  other.ptr_ = nullptr;
  other.cb_ = nullptr;
}

template <typename T>
template <ConvertibleFromUStar<T> U, typename Deleter>
SharedPtr<T>::SharedPtr(U* ptr, Deleter deleter) : ptr_(ptr) {
  cb_ = new ControlBlockWithType<T, U, Deleter, std::allocator<U>>(
      ptr, deleter, std::allocator<U>());
}

template <typename T>
template <ConvertibleFromUStar<T> U, typename Deleter, typename Allocator>
SharedPtr<T>::SharedPtr(U* ptr, Deleter deleter, Allocator allocator) {
  using Cballoc =
      typename std::allocator_traits<Allocator>::template rebind_alloc<
          ControlBlockWithType<T, U, Deleter, Allocator>>;
  Cballoc cballoc(allocator);
  ControlBlockWithType<T, U, Deleter, Allocator>* new_cb;
  new_cb = std::allocator_traits<Cballoc>::allocate(cballoc, 1);
  std::allocator_traits<Cballoc>::construct(
      cballoc, new_cb, ptr, std::move(deleter), std::move(allocator));
  cb_ = new_cb;
  ptr_ = ptr;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other) noexcept {
  if (&other != this) {
    shared_count_minus();
    cb_ = other.cb_;
    ptr_ = other.ptr_;
    shared_count_plus();
  }
  return *this;
}

template <typename T>
template <ConvertibleFromUStar<T> U>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<U>& other) noexcept {
  shared_count_minus();
  cb_ = other.cb_;
  ptr_ = other.ptr_;
  shared_count_plus();
  return *this;
}

template <typename T>
template <ConvertibleFromUStar<T> U>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<U>&& other) noexcept {
  if (&other != this) {
    shared_count_minus();
    cb_ = other.cb_;
    ptr_ = other.ptr_;
    other.cb_ = nullptr;
    other.ptr_ = nullptr;
  }
  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) noexcept {
  if (&other != this) {
    shared_count_minus();
    cb_ = other.cb_;
    ptr_ = other.ptr_;
    other.cb_ = nullptr;
    other.ptr_ = nullptr;
  }
  return *this;
}

template <typename T>
T& SharedPtr<T>::operator*() const {
  return *ptr_;
}

template <typename T>
T* SharedPtr<T>::operator->() const {
  return ptr_;
}

template <typename T>
T* SharedPtr<T>::get() const {
  return ptr_;
}

template <typename T>
void SharedPtr<T>::reset() {
  shared_count_minus();
  ptr_ = nullptr;
  cb_ = nullptr;
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
  shared_count_minus();
}

template <typename T>
size_t SharedPtr<T>::use_count() const {
  if (cb_ == nullptr) {
    return 0;
  }
  return cb_->shared_count;
}

template <typename T>
SharedPtr<T>::SharedPtr([[maybe_unused]] tmp::ControlBlockTag tag1, T* ptr,
                        BaseControlBlock* new_cb)
    : ptr_(ptr), cb_(new_cb) {}

template <typename T>
void SharedPtr<T>::shared_count_plus() {
  if (cb_ != nullptr) {
    ++cb_->shared_count;
  }
}

template <typename T>
void SharedPtr<T>::shared_count_minus() {
  if (cb_ != nullptr) {
    cb_->shared_count--;
    if (cb_->shared_count == 0) {
      cb_->destroy_obj();
      if (cb_->weak_count == 0) {
        cb_->destroy_control_block();
      }
    }
  }
}

template <typename T>
WeakPtr<T>::WeakPtr(const WeakPtr& other) : cb_(other.cb_) {
  plus_weak_count();
}

template <typename T>
template <ConvertibleFromUStar<T> U>
WeakPtr<T>::WeakPtr(const WeakPtr<U>& other) {
  cb_ = other.cb_;
  plus_weak_count();
}

template <typename T>
WeakPtr<T>::WeakPtr(WeakPtr&& other) : cb_(other.cb_) {
  other.ptr_ = nullptr;
  other.cb_ = nullptr;
}

template <typename T>
template <ConvertibleFromUStar<T> U>
WeakPtr<T>::WeakPtr(WeakPtr<U>&& other) : cb_(other.cb_) {
  other.ptr_ = nullptr;
  other.cb_ = nullptr;
}

template <typename T>
WeakPtr<T>::WeakPtr(const SharedPtr<T>& other) : cb_(other.cb_) {
  plus_weak_count();
}

template <typename T>
template <ConvertibleFromUStar<T> U>
WeakPtr<T>::WeakPtr(const SharedPtr<U>& other) {
  cb_ = other.cb_;
  plus_weak_count();
}

template <typename T>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr& other) noexcept {
  if (&other != this) {
    minus_weak_count();
    cb_ = other.cb_;
    plus_weak_count();
  }
  return *this;
}

template <typename T>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr&& other) noexcept {
  if (&other != this) {
    minus_weak_count();
    cb_ = other.cb_;
    other.cb_ = nullptr;
    other.ptr_ = nullptr;
  }
  return *this;
}

template <typename T>
WeakPtr<T>::~WeakPtr() {
  minus_weak_count();
}

template <typename T>
bool WeakPtr<T>::expired() const {
  return cb_ == nullptr || cb_->shared_count == 0;
}

template <typename T>
SharedPtr<T> WeakPtr<T>::lock() {
  ++cb_->shared_count;
  return SharedPtr<T>(tmp::ControlBlockTag{}, static_cast<T*>(cb_->get()), cb_);
}

template <typename T>
void WeakPtr<T>::plus_weak_count() {
  if (cb_ != nullptr) {
    ++cb_->weak_count;
  }
}

template <typename T>
void WeakPtr<T>::minus_weak_count() {
  if (cb_ != nullptr) {
    --cb_->weak_count;
    if (cb_->weak_count == 0 && cb_->shared_count == 0) {
      cb_->destroy_control_block();
    }
  }
}

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  auto* new_cb = new ControlBlockMakeShared<T>(std::allocator<T>(),
                                               std::forward<Args>(args)...);
  return SharedPtr<T>(tmp::ControlBlockTag{}, static_cast<T*>(new_cb->get()),
                      new_cb);
}

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> AllocateShared(const Alloc& alloc, Args&&... args) {
  using CbType = ControlBlockMakeShared<T, Alloc>;
  using CbAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<CbType>;
  CbAlloc cb_alloc(alloc);
  auto* new_cb = std::allocator_traits<CbAlloc>::allocate(cb_alloc, 1);
  std::allocator_traits<CbAlloc>::construct(cb_alloc, new_cb, std::move(alloc),
                                            std::forward<Args>(args)...);
  return SharedPtr<T>(tmp::ControlBlockTag{}, static_cast<T*>(new_cb->get()),
                      new_cb);
}
