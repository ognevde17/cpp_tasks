#include <cassert>
#include <iostream>
#include <vector>

template <size_t N, size_t M, typename T = int64_t>
class Matrix {
 public:
  Matrix() : data_(N, std::vector<T>(M, T())) {};
  Matrix(const std::vector<std::vector<T>>& vec) : data_(vec) {};
  Matrix(const T& elem) : data_(N, std::vector<T>(M, elem)) {};
  T& operator()(size_t row, size_t column);
  const T& operator()(size_t row, size_t column) const;
  bool operator==(const Matrix<N, M, T>& other) const;
  Matrix<N, M, T>& operator+=(const Matrix<N, M, T>& other);
  Matrix<N, M, T> operator+(const Matrix<N, M, T>& other) const;
  Matrix<N, M, T>& operator-=(const Matrix<N, M, T>& other);
  Matrix<N, M, T> operator-(const Matrix<N, M, T>& other) const;
  Matrix<N, M, T> operator*(const T& elem) const;
  template <size_t Z>
  Matrix<N, Z, T> operator*(const Matrix<M, Z, T>& other);
  Matrix<M, N, T> Transposed() const;
  T Trace() const;

 private:
  std::vector<std::vector<T>> data_;
};

template <size_t N, size_t M, typename T>
T& Matrix<N, M, T>::operator()(size_t row, size_t column) {
  return data_[row][column];
}
template <size_t N, size_t M, typename T>
const T& Matrix<N, M, T>::operator()(size_t row, size_t column) const {
  return data_[row][column];
}
template <size_t N, size_t M, typename T>
bool Matrix<N, M, T>::operator==(const Matrix<N, M, T>& other) const {
  return data_ == other.data_;
}
template <size_t N, size_t M, typename T>
Matrix<N, M, T>& Matrix<N, M, T>::operator+=(const Matrix<N, M, T>& other) {
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      data_[i][j] += other(i, j);
    }
  }
  return *this;
}
template <size_t N, size_t M, typename T>
Matrix<N, M, T> Matrix<N, M, T>::operator+(const Matrix<N, M, T>& other) const {
  Matrix<N, M, T> new_matrix(data_);
  new_matrix += other;
  return new_matrix;
}
template <size_t N, size_t M, typename T>
Matrix<N, M, T>& Matrix<N, M, T>::operator-=(const Matrix<N, M, T>& other) {
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      data_[i][j] -= other(i, j);
    }
  }
  return *this;
}
template <size_t N, size_t M, typename T>
Matrix<N, M, T> Matrix<N, M, T>::operator-(const Matrix<N, M, T>& other) const {
  Matrix<N, M, T> new_matrix(data_);
  new_matrix -= other;
  return new_matrix;
}
template <size_t N, size_t M, typename T>
Matrix<N, M, T> Matrix<N, M, T>::operator*(const T& elem) const {
  Matrix<N, M, T> new_matrix;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      new_matrix(i, j) = data_[i][j] * elem;
    }
  }
  return new_matrix;
}
template <size_t N, size_t M, typename T>
template <size_t Z>
Matrix<N, Z, T> Matrix<N, M, T>::operator*(const Matrix<M, Z, T>& other) {
  Matrix<N, Z, T> new_matrix;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < Z; ++j) {
      for (size_t k = 0; k < M; ++k) {
        new_matrix(i, j) += data_[i][k] * other(k, j);
      }
    }
  }
  return new_matrix;
}
template <size_t N, size_t M, typename T>
Matrix<M, N, T> Matrix<N, M, T>::Transposed() const {
  Matrix<M, N, T> new_matrix;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      new_matrix(j, i) = data_[i][j];
    }
  }
  return new_matrix;
}
template <size_t N, size_t M, typename T>
T Matrix<N, M, T>::Trace() const {
  static_assert(N == M);
  T answer{};
  for (size_t i = 0; i < N; ++i) {
    answer += data_[i][i];
  }
  return answer;
}
