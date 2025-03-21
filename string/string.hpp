#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

class String;
namespace tmp {
bool Check(const String& place, const String& elem, const size_t& start);
}
class String {
 public:
  String() = default;
  String(size_t size, char character);
  String(const char* str);
  String& operator=(const String& str);
  ~String();
  String(const String& str);

  void Clear();
  void PushBack(char character);
  void PopBack();
  void Resize(size_t new_size);
  void Resize(size_t new_size, char character);
  void Reserve(size_t new_cap);
  void ShrinkToFit();
  void Swap(String& other);

  const char& operator[](int index) const;
  char& operator[](int index);
  const char& Front() const;
  char& Front();
  const char& Back() const;
  char& Back();

  bool Empty() const;
  size_t Size() const;
  size_t Capacity() const;
  char* Data();
  const char* Data() const;

  String& operator+=(const String& second);
  String operator*(size_t num) const;
  String& operator*=(size_t num);
  std::vector<String> Split(const String& delim = " ") const;
  String Join(const std::vector<String>& strings) const;

 private:
  size_t capacity_ = 0;
  size_t size_ = 0;
  char* str_ = nullptr;
  static char* CharCopy(const char* start, char* final);
  static char* Realloc(size_t need_capacity, const char* current,
                       size_t original_size);
};

std::ostream& operator<<(std::ostream& os, const String& str);
std::istream& operator>>(std::istream& is, String& str);
bool operator<(const String& first, const String& second);
bool operator==(const String& first, const String& second);
bool operator>(const String& first, const String& second);
bool operator>=(const String& first, const String& second);
bool operator<=(const String& first, const String& second);
bool operator!=(const String& first, const String& second);
String operator+(const String& first, const String& second);
