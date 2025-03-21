#include "string.hpp"

String::String(size_t size, char character) {
  str_ = new char[size + 1];
  str_[size] = '\0';
  for (size_t i = 0; i < size; ++i) {
    str_[i] = character;
  }
  size_ = size;
  capacity_ = size;
}

String::String(const char* str)
    : capacity_(strlen(str)), size_(strlen(str)), str_(nullptr) {
  str_ = new char[capacity_ + 1];
  str_[size_] = '\0';
  for (size_t i = 0; i < size_; ++i) {
    str_[i] = str[i];
  }
}

String::~String() { delete[] str_; }

String::String(const String& str) : capacity_(str.capacity_), size_(str.size_) {
  str_ = new char[str.capacity_ + 1];
  str_[str.size_] = '\0';
  for (size_t i = 0; i < size_; ++i) {
    str_[i] = str.str_[i];
  }
}

String& String::operator=(const String& str) {
  if (this == &str) {
    return *this;
  }
  delete[] str_;
  str_ = new char[str.capacity_ + 1];
  size_ = str.size_;
  str_[size_] = '\0';
  capacity_ = str.capacity_;
  for (size_t i = 0; i < size_; ++i) {
    str_[i] = str.str_[i];
  }
  return *this;
}

void String::Clear() {
  if (str_ == nullptr) {
    return;
  }
  size_ = 0;
  str_[size_] = '\0';
}

void String::PushBack(char character) {
  if (capacity_ == 0) {
    capacity_ = 1;
    delete[] str_;
    str_ = new char[2];
    str_[0] = character;
    str_[1] = '\0';
    ++size_;
    return;
  }
  if (size_ == capacity_) {
    str_ = Realloc(capacity_ * 2, str_, size_ + 1);
    capacity_ *= 2;
  }
  ++size_;
  str_[size_ - 1] = character;
  str_[size_] = '\0';
}

void String::PopBack() {
  if (size_ != 0) {
    size_--;
    str_[size_] = '\0';
  }
}

void String::Resize(size_t new_size) {
  if (new_size <= capacity_) {
    size_ = new_size;
    str_[size_] = '\0';
  } else {
    str_ = Realloc(new_size + 1, str_, size_);
    size_ = new_size;
    capacity_ = new_size;
  }
}

void String::Resize(size_t new_size, char character) {
  if (new_size <= capacity_) {
    if (size_ < new_size) {
      for (size_t i = size_; i < new_size; ++i) {
        str_[i] = character;
      }
    }
    size_ = new_size;
    str_[size_] = '\0';
  } else {
    str_ = Realloc(new_size, str_, size_);
    for (size_t i = size_; i < new_size; ++i) {
      str_[i] = character;
    }
    size_ = new_size;
    capacity_ = new_size;
  }
}

void String::Reserve(size_t new_cap) {
  if (new_cap > capacity_) {
    str_ = Realloc(new_cap, str_, size_);
    capacity_ = new_cap;
  }
}

void String::ShrinkToFit() {
  if (capacity_ > size_) {
    str_ = Realloc(size_, str_, size_);
    capacity_ = size_;
  }
}

void String::Swap(String& other) {
  size_t tmp_size = other.size_;
  size_t tmp_capacity = other.capacity_;
  char* tmp_str = other.str_;
  other.capacity_ = capacity_;
  other.size_ = size_;
  other.str_ = str_;
  size_ = tmp_size;
  capacity_ = tmp_capacity;
  str_ = tmp_str;
}

const char& String::operator[](int index) const { return str_[index]; }

char& String::operator[](int index) { return str_[index]; }

const char& String::Front() const { return str_[0]; }

char& String::Front() { return str_[0]; }

const char& String::Back() const { return str_[size_ - 1]; }

char& String::Back() { return str_[size_ - 1]; }

bool String::Empty() const { return size_ == 0; }

size_t String::Size() const { return size_; }

size_t String::Capacity() const { return capacity_; }

const char* String::Data() const { return str_; }

char* String::Data() { return str_; }

String& String::operator+=(const String& second) {
  size_t new_capacity = capacity_;
  if (size_ + second.size_ > capacity_) {
    new_capacity = size_ + second.size_;
    str_ = Realloc(new_capacity, str_, size_);
  }
  for (size_t i = size_; i < size_ + second.size_; ++i) {
    str_[i] = second[i - size_];
  }
  size_ += second.size_;
  capacity_ = new_capacity;
  str_[size_] = '\0';
  return *this;
}

String String::operator*(size_t num) const {
  String new_string(*this);
  new_string *= num;
  return new_string;
}

String& String::operator*=(size_t num) {
  size_t new_capacity = capacity_;
  if (capacity_ < size_ * num) {
    new_capacity = size_ * num;
    str_ = Realloc(new_capacity, str_, size_);
  }
  str_[size_ * num] = '\0';
  capacity_ = new_capacity;
  for (size_t i = size_; i < size_ * num; ++i) {
    str_[i] = str_[i % size_];
  }
  size_ *= num;
  return *this;
}
std::vector<String> String::Split(const String& delim) const {
  if (delim.Empty()) {
    return {*this};
  }
  String current;
  std::vector<String> res;
  for (int i = 0; i < (int)size_; ++i) {
    if (tmp::Check(*this, delim, i)) {
      res.push_back(current);
      current.Clear();
      i += delim.Size() - 1;
    } else {
      current.PushBack(str_[i]);
    }
  }
  res.push_back(current);
  return res;
}

String String::Join(const std::vector<String>& strings) const {
  if (strings.empty()) {
    return String();
  }
  String ans(strings[0]);
  for (int i = 1; i < (int)strings.size(); ++i) {
    ans += *this;
    ans += strings[i];
  }
  return ans;
}
char* String::Realloc(size_t need_capacity, const char* current,
                      size_t original_size) {
  char* new_string = new char[need_capacity + 1];
  new_string[original_size] = '\0';
  for (size_t i = 0; i < original_size; ++i) {
    new_string[i] = current[i];
  }
  delete[] current;
  return new_string;
}
std::ostream& operator<<(std::ostream& os, const String& str) {
  os << str.Data();
  return os;
}
std::istream& operator>>(std::istream& is, String& str) {
  str.Clear();
  char character;
  while (is.get(character)) {
    if (static_cast<bool>(std::isspace(character))) {
      if (str.Size() > 0) {
        break;
      }
    } else {
      str.PushBack(character);
    }
  }
  return is;
}
bool tmp::Check(const String& place, const String& elem, const size_t& start) {
  if (elem.Size() > place.Size() - start) {
    return false;
  }
  for (int i = start; i < (int)(start + elem.Size()); ++i) {
    if (place[i] != elem[i - start]) {
      return false;
    }
  }
  return true;
}
bool operator<(const String& first, const String& second) {
  size_t ind = 0;
  while (ind < first.Size() && ind < second.Size() &&
         first[ind] == second[ind]) {
    ++ind;
  }
  if (ind < first.Size() && ind < second.Size()) {
    return first[ind] < second[ind];
  }
  return (ind == first.Size() && ind < second.Size());
}

bool operator==(const String& first, const String& second) {
  if (first.Size() != second.Size()) {
    return false;
  }
  for (size_t i = 0; i < first.Size(); ++i) {
    if (first[i] != second[i]) {
      return false;
    }
  }
  return true;
}

bool operator>(const String& first, const String& second) {
  return (second < first);
}

bool operator>=(const String& first, const String& second) {
  return (!(first < second));
}

bool operator<=(const String& first, const String& second) {
  return (!(first > second));
}

bool operator!=(const String& first, const String& second) {
  return (!(first == second));
}

String operator+(const String& first, const String& second) {
  String res(first);
  res += second;
  return res;
}
