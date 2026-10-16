/**
 * @file kernel.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Wrapper a std::array to provide a few helper functions for a kernel.
 * @version 1.0 (@LastMod : 2026/10/15 12:13:23)
 * @date 2026-10-09
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <array>

template <typename T, std::size_t kernelSize>
class Kernel
{
 private:
  std::array<T, kernelSize> x;
  std::array<T, kernelSize> y;

 public:
  Kernel(
      const std::array<T, kernelSize>& inner,
      const std::array<T, kernelSize>& outer)
      : x(inner), y(outer)
  {
  }

  const std::array<T, kernelSize>& getInner() const { return x; }
  const std::array<T, kernelSize>& getOuter() const { return y; }
  constexpr std::size_t size() const { return kernelSize; }

  static Kernel<T, kernelSize> boxBlur()
  {
    std::array<T, kernelSize> x{};
    std::array<T, kernelSize> y{};
    const T coefficient = static_cast<T>(1) / static_cast<T>(kernelSize);
    x.fill(coefficient);
    y.fill(coefficient);

    return Kernel<T, kernelSize>(x, y);
  }
};