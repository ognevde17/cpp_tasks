#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>
template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;
  };
  struct Node : public BaseNode {
    T value;
    Node(const T& value) : value(value), BaseNode{nullptr, nullptr} {}
    Node() = default;
  };

 public:
  using value_type = T;
  using allocator_type = Allocator;

  List() : size_(0), allocator_(node_alloc()) {
    fakeNode_.prev = &fakeNode_;
    fakeNode_.next = &fakeNode_;
  }

  List(size_t count, const T& value, const Allocator& alloc = Allocator())
      : size_(0), allocator_(alloc) {
    fakeNode_.prev = &fakeNode_;
    fakeNode_.next = &fakeNode_;
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back(value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }
  List(size_t count, const Allocator& alloc = Allocator()) : List() {
    allocator_ = alloc;
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back();
      }
    } catch (...) {
      clear();
      throw;
    }
  }
  List(const List& other)
      : size_(0),
        allocator_(node_alloc_traits::select_on_container_copy_construction(
            other.allocator_)) {
    fakeNode_.prev = &fakeNode_;
    fakeNode_.next = &fakeNode_;
    try {
      for (const T& tmp : other) {
        push_back(tmp);
      }
    } catch (...) {
      clear();
      throw;
    }
  }
  List(std::initializer_list<T> init, const Allocator& allocator = Allocator())
      : size_(0) {
    fakeNode_.prev = &fakeNode_;
    fakeNode_.next = &fakeNode_;
    allocator_ = allocator;
    try {
      for (const auto& tmp : init) {
        push_back(tmp);
      }
    } catch (...) {
      while (size_ != 0) {
        pop_back();
      }
    }
  }
  void swap(List& other) noexcept {
    std::swap(size_, other.size_);
    std::swap(fakeNode_.next, other.fakeNode_.next);
    std::swap(fakeNode_.prev, other.fakeNode_.prev);
    if (size_ > 0) {
      fakeNode_.next->prev = &fakeNode_;
      fakeNode_.prev->next = &fakeNode_;
    } else {
      fakeNode_.next = &fakeNode_;
      fakeNode_.prev = &fakeNode_;
    }
    if (other.size_ > 0) {
      other.fakeNode_.next->prev = &other.fakeNode_;
      other.fakeNode_.prev->next = &other.fakeNode_;
    } else {
      other.fakeNode_.next = &other.fakeNode_;
      other.fakeNode_.prev = &other.fakeNode_;
    }
  }
  List& operator=(const List& other) {
    if (this != &other) {
      List tmp(other);
      if (node_alloc_traits::propagate_on_container_copy_assignment::value) {
        allocator_ = other.allocator_;
      }
      swap(tmp);
    }
    return *this;
  }

  ~List() { clear(); }

  T& front() { return static_cast<Node*>(fakeNode_.next)->value; }
  const T& front() const { return static_cast<Node*>(fakeNode_.next)->value; }

  T& back() { return static_cast<Node*>(fakeNode_.prev)->value; }
  const T& back() const { return static_cast<Node*>(fakeNode_.prev)->value; }

  bool empty() const { return size_ == 0; }
  size_t size() const { return size_; }
  void push_back() {
    Node* new_node = nullptr;
    try {
      new_node = construct_node();
      new_node->next = &fakeNode_;
      new_node->prev = fakeNode_.prev;
      fakeNode_.prev->next = new_node;
      fakeNode_.prev = new_node;
      size_++;
    } catch (...) {
      throw;
    }
  }
  void push_back(const T& value) {
    Node* new_node = nullptr;
    try {
      new_node = construct_node(value);
      new_node->next = &fakeNode_;
      new_node->prev = fakeNode_.prev;
      fakeNode_.prev->next = new_node;
      fakeNode_.prev = new_node;
      size_++;
    } catch (...) {
      throw;
    }
  }

  void push_front(const T& value) {
    try {
      Node* new_node = construct_node(value);
      new_node->prev = &fakeNode_;
      new_node->next = fakeNode_.next;
      fakeNode_.next->prev = new_node;
      fakeNode_.next = new_node;
      size_++;
    } catch (...) {
      throw;
    }
  }

  void pop_back() {
    if (size_ != 0) {
      Node* node = reinterpret_cast<Node*>(fakeNode_.prev);
      node->prev->next = &fakeNode_;
      fakeNode_.prev = node->prev;
      delete_node(node);
      --size_;
    }
  }

  void pop_front() {
    if (size_ != 0) {
      Node* node = reinterpret_cast<Node*>(fakeNode_.next);
      node->next->prev = &fakeNode_;
      fakeNode_.next = node->next;
      delete_node(node);
      --size_;
    }
  }

  template <bool IsConst>
  class Iterator {
   public:
    using value_type = T;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;

    Iterator(BaseNode* node) : node_(node) {}
    Iterator(const BaseNode* node) : node_(const_cast<BaseNode*>(node)) {}
    reference operator*() const { return static_cast<Node*>(node_)->value; }

    pointer operator->() const { return &(static_cast<Node*>(node_)->value); }

    Iterator& operator++() {
      node_ = node_->next;
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    Iterator& operator--() {
      node_ = node_->prev;
      return *this;
    }

    Iterator operator--(int) {
      Iterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const Iterator& other) const {
      return node_ == other.node_;
    }

    bool operator!=(const Iterator& other) const {
      return node_ != other.node_;
    }

   private:
    BaseNode* node_;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  iterator begin() { return iterator(fakeNode_.next); }

  iterator end() { return iterator(&fakeNode_); }

  const_iterator cbegin() const { return const_iterator(fakeNode_.next); }
  const_iterator cend() const { return const_iterator(&fakeNode_); }

  const_iterator begin() const { return const_iterator(fakeNode_.next); }
  const_iterator end() const { return const_iterator(&fakeNode_); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }

  allocator_type get_allocator() const noexcept { return allocator_; }

 private:
  using alloc_traits = std::allocator_traits<Allocator>;
  using node_alloc = typename alloc_traits::template rebind_alloc<Node>;
  using node_alloc_traits = typename alloc_traits::template rebind_traits<Node>;
  BaseNode fakeNode_;
  size_t size_ = 0;
  node_alloc allocator_;

  Node* construct_node(const T& value) {
    Node* node = node_alloc_traits::allocate(allocator_, 1);
    try {
      node_alloc_traits::construct(allocator_, node, value);
    } catch (...) {
      node_alloc_traits::deallocate(allocator_, node, 1);
      throw;
    }
    node->next = nullptr;
    node->prev = nullptr;
    return node;
  }
  Node* construct_node() {
    Node* node = node_alloc_traits::allocate(allocator_, 1);
    try {
      node_alloc_traits::construct(allocator_, node);
    } catch (...) {
      node_alloc_traits::deallocate(allocator_, node, 1);
      throw;
    }
    node->next = nullptr;
    node->prev = nullptr;
    return node;
  }
  void delete_node(Node* node) {
    if (node) {
      node_alloc_traits::destroy(allocator_, node);
      node_alloc_traits::deallocate(allocator_, node, 1);
    }
  }

  void clear() {
    while (size_ != 0) {
      pop_back();
    }
  }
};
