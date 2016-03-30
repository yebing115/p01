#pragma once
#include "Data/DataType.h"

/// Greatest common divisor.
template <typename T>
inline T num_gcd(T a, T b) {
  do {
    T tmp = a % b;
    a = b;
    b = tmp;
  } while (b);

  return a;
}

/// Least common multiple.
template <typename T>
inline T num_lcm(T a, T b) { return a * (b / num_gcd(a, b)); }

/// Align number
template <typename P, typename Q>
inline P num_align(P x, Q a) {
  Q m = x % a;
  if (!m) return x;
  return x + (a - m);
}

template <typename T>
inline bool test_bit(T n, u8 bit) { return (n & (1 << bit)) != 0; }
template <typename T>
inline void set_bit(T& n, u8 bit) { n |= (1 << bit); }
template <typename T>
inline void clear_bit(T& n, u8 bit) { n &= ~(1 << bit); }
template <typename T>
inline void flip_bit(T& n, u8 bit) { n ^= (1 << bit); }

template <typename T>
T clamp(const T& value, const T& min_value, const T& max_value) {
  return max<T>(min<T>(value, max_value), min_value);
}
