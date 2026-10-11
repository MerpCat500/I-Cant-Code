/**
 * @file kernal.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Wrapper a std::array to provide a few helper functions for a kernal.
 * @version 1.0 (@LastMod : 2026/10/10 19:50:39)
 * @date 2026-10-09
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <array>
#include <initializer_list>

template <typename T, std::size_t kernalSize>
class Kernal
{
 private:
  std::array<T, kernalSize> u;
  std::array<T, kernalSize> v;

 public:
  Kernal(
      std::initializer_list<std::array<T, kernalSize>> inner,
      std::initializer_list<std::array<T, kernalSize>> outer)
      : u(inner), v(outer)
  {
  }

  const std::array<T, kernalSize>& getInner() const { return u; }
  const std::array<T, kernalSize>& getOuter() const { return v; }
  constexpr std::size_t size() const { return kernalSize; }
};