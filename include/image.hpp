/**
 * @file image.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief
 * @version 2.0 (@LastMod : 2026/10/15 22:52:09)
 * @date 2026-10-04
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "kernel.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

template <std::size_t width, std::size_t height>
class Image
{
 private:
  std::array<std::uint8_t, width * height> data{};
  constexpr static size_t readChannels = 4;   // RGBA
  constexpr static size_t writeChannels = 1;  // Grayscale
  // Note: The offsets are ordered to maximize the chance of early termination
  // in the FAST algorithm.
  constexpr static std::array<std::pair<int, int>, 16> fastOffsets = {{
      {0, -3},   //  0: top
      {1, -3},   //  1: top-right slight
      {2, -2},   //  2: upper-right
      {3, -1},   //  3: right-upper slight
      {3, 0},    //  4: right
      {3, 1},    //  5: right-lower slight
      {2, 2},    //  6: lower-right
      {1, 3},    //  7: bottom-right slight
      {0, 3},    //  8: bottom
      {-1, 3},   //  9: bottom-left slight
      {-2, 2},   // 10: lower-left
      {-3, 1},   // 11: left-lower slight
      {-3, 0},   // 12: left
      {-3, -1},  // 13: left-upper slight
      {-2, -2},  // 14: upper-left
      {-1, -3},  // 15: top-left slight
  }};

 public:
  Image(const char* filename, int* bpp = nullptr)
  {
    int sourceWidth = 0, sourceHeight = 0;

    // Get Raw Data
    stbi_uc* decodedData =
        stbi_load(filename, &sourceWidth, &sourceHeight, bpp, readChannels);

    // If there is no data, then print image load error and return nothing.
    if (!decodedData)
    {
      std::cerr << "Failed to load image: " << filename << std::endl;
      return;
    }

    if (sourceWidth != width || sourceHeight != height)
    {
      std::cerr << "Image dimensions do not match the Image type: "
                << sourceWidth << "x" << sourceHeight << std::endl;
      stbi_image_free(decodedData);
      return;
    }

    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        const std::size_t index = (y * width + x) * readChannels;
        data[y * width + x] =
            (decodedData[index] * 114 + decodedData[index + 1] * 587 +
             decodedData[index + 2] * 299) /
            1000;
      }
    }
    // Free the decoded data
    stbi_image_free(decodedData);
  }

  Image(std::array<std::uint8_t, width * height> data) : data(std::move(data))
  {
  }

  Image(const std::uint8_t fillColor) : data() { data.fill(fillColor); }

  Image() : data() {}

  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;
  Image(Image&&) noexcept = default;
  Image& operator=(Image&&) noexcept = default;

  constexpr std::size_t getWidth() const { return width; }
  constexpr std::size_t getHeight() const { return height; }

  std::uint8_t& operator()(std::size_t x, std::size_t y)
  { return data[checkedIndex(x, y)]; }

  const std::uint8_t& operator()(std::size_t x, std::size_t y) const
  { return data[checkedIndex(x, y)]; }

  const std::uint8_t& getPixel(std::size_t x, std::size_t y) const
  { return (*this)(x, y); }

  template <std::size_t selectionWidth, std::size_t selectionHeight>
  Image<selectionWidth, selectionHeight> getSelection(
      std::size_t x, std::size_t y) const
  {
    if (x > width - selectionWidth || y > height - selectionHeight)
    {
      throw std::out_of_range("Selection is outside the image");
    }

    std::array<std::uint8_t, selectionWidth * selectionHeight> selectionData{};

    for (std::size_t j = 0; j < selectionHeight; ++j)
    {
      for (std::size_t i = 0; i < selectionWidth; ++i)
      {
        selectionData[j * selectionWidth + i] = (*this)(x + i, y + j);
      }
    }

    return Image<selectionWidth, selectionHeight>(selectionData);
  }

  void save_png(const char* filename) const
  {
    if (stbi_write_png(
            filename,
            width,
            height,
            writeChannels,
            data.data(),
            width * writeChannels) == 0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
    }
  }

  void save_jpg(const char* filename, int quality = 90) const
  {
    if (stbi_write_jpg(
            filename, width, height, writeChannels, data.data(), quality) == 0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
    }
  }

  void save_bmp(const char* filename) const
  {
    if (stbi_write_bmp(filename, width, height, writeChannels, data.data()) ==
        0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
    }
  }

  void operator+=(const Image& B)
  {
    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t& outputPixel = (*this)(x, y);
        const std::uint8_t& rhsPixel = B(x, y);

        outputPixel = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(outputPixel) + rhsPixel, 0, 255));
      }
    }
  }

  void operator-=(const Image& B)
  {
    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t& outputPixel = (*this)(x, y);
        const std::uint8_t& rhsPixel = B(x, y);

        outputPixel = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(outputPixel) - rhsPixel, 0, 255));
      }
    }
  }

  void operator*=(const float B)
  {
    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t& outputPixel = (*this)(x, y);

        outputPixel = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(outputPixel) * B, 0.0f, 255.0f));
      }
    }
  }

  Image frobeniusInnerProduct(const Image& B) const
  {
    Image outputImage;

    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        const std::uint8_t& lhsPixel = (*this)(x, y);
        const std::uint8_t& rhsPixel = B(x, y);
        std::uint8_t& outputPixel = outputImage(x, y);

        outputPixel = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(lhsPixel) * rhsPixel, 0, 255));
      }
    }
    return outputImage;
  }

  Image operator*(const float B) const
  {
    Image outputImage;

    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        const std::uint8_t& lhsPixel = (*this)(x, y);
        std::uint8_t& outputPixel = outputImage(x, y);

        outputPixel = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(lhsPixel) * B, 0.0f, 255.0f));
      }
    }
    return outputImage;
  }

  Image operator+(const Image& B) const
  {
    Image outputImage;

    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t& outputPixel = outputImage(x, y);
        const std::uint8_t& lhsPixel = (*this)(x, y);
        const std::uint8_t& rhsPixel = B(x, y);

        outputPixel = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(lhsPixel) + rhsPixel, 0, 255));
      }
    }
    return outputImage;
  }

  Image operator-(const Image& B) const
  {
    Image outputImage;

    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t& outputPixel = outputImage(x, y);
        const std::uint8_t& lhsPixel = (*this)(x, y);
        const std::uint8_t& rhsPixel = B(x, y);

        outputPixel = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(lhsPixel) - rhsPixel, 0, 255));
      }
    }
    return outputImage;
  }

  template <std::size_t kernelSize, typename kernelType = float>
  static Image<width - kernelSize + 1, height - kernelSize + 1> convolve(
      const Image& inputBuffer, const Kernel<kernelType, kernelSize>& kernel)
  {
    static_assert(kernelSize > 0, "The kernel size must be greater than zero");
    static_assert(
        kernelSize <= width && kernelSize <= height,
        "The kernel must fit inside the input image");

    Image<width - kernelSize + 1, height - kernelSize + 1> outputBuffer;
    constexpr std::size_t outputWidth = width - kernelSize + 1;
    constexpr std::size_t outputHeight = height - kernelSize + 1;

    const auto& horizontalKernel = kernel.getInner();
    const auto& verticalKernel = kernel.getOuter();
    std::vector<long double> horizontalPass(outputWidth * height, 0.0L);

    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < outputWidth; ++x)
      {
        long double horizontalSum = 0.0L;
        for (std::size_t column = 0; column < kernelSize; ++column)
        {
          horizontalSum +=
              static_cast<long double>(inputBuffer(x + column, y)) *
              static_cast<long double>(horizontalKernel[column]);
        }
        horizontalPass[y * outputWidth + x] = horizontalSum;
      }
    }

    for (std::size_t outputY = 0; outputY < outputHeight; ++outputY)
    {
      for (std::size_t outputX = 0; outputX < outputWidth; ++outputX)
      {
        long double convolutionResult = 0.0L;
        for (std::size_t row = 0; row < kernelSize; ++row)
        {
          convolutionResult +=
              horizontalPass[(outputY + row) * outputWidth + outputX] *
              static_cast<long double>(verticalKernel[row]);
        }

        outputBuffer(outputX, outputY) = static_cast<std::uint8_t>(
            std::clamp(convolutionResult, 0.0L, 255.0L));
      }
    }

    return outputBuffer;
  }

  /**
   * @brief Get the Features Accelerated Segment Test (FAST)
   *
   */
  static void getFeaturesFAST(
      const Image<width, height>& inputBuffer,
      Image<width, height>& outputBuffer,
      const std::size_t threshold,
      const std::size_t consecutivePixels = 12)
  {
    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t& outputPixel = outputBuffer(x, y);
        const std::uint8_t& inputPixel = inputBuffer(x, y);

        std::size_t isExtreme = 0;
        std::size_t firstConsecutivePixels = 0;
        bool firstConsecutivePixelsSet = false;
        bool lastBrighter = true;
        bool lastDarker = true;

        for (auto offset : fastOffsets)
        {
          int neighborX = static_cast<int>(x) + offset.first;
          int neighborY = static_cast<int>(y) + offset.second;

          if (neighborX < static_cast<int>(width) &&
              neighborY < static_cast<int>(height) && neighborX >= 0 &&
              neighborY >= 0)
          {
            const std::uint8_t& neighborPixel =
                inputBuffer(neighborX, neighborY);

            bool isBrighter = neighborPixel > static_cast<int>(inputPixel) +
                                                  static_cast<int>(threshold);
            bool isDarker = neighborPixel < static_cast<int>(inputPixel) -
                                                static_cast<int>(threshold);
            bool isNotExtreme =
                !(isBrighter && lastBrighter) && !(isDarker && lastDarker);

            if (isBrighter && lastBrighter) { ++isExtreme; }
            else if (isDarker && lastDarker) { ++isExtreme; }
            else if (isNotExtreme && !firstConsecutivePixelsSet)
            {
              firstConsecutivePixels = isExtreme;
              firstConsecutivePixelsSet = true;
            }
            else
            {
              isExtreme = 0;
            }

            lastBrighter = isBrighter;
            lastDarker = isDarker;
          }

          if (offset == fastOffsets.back())
          {
            isExtreme +=
                firstConsecutivePixels;  // Wrap around to the first pixels
          }

          if (isExtreme >= consecutivePixels)
          {
            outputPixel = 255;
            break;
          }

          outputPixel = 255 / 16 * isExtreme;
        }
      }
    }
  }

 private:
  std::size_t checkedIndex(std::size_t x, std::size_t y) const
  {
    if (x >= width || y >= height)
    {
      throw std::out_of_range("Pixel coordinates are out of bounds");
    }
    return y * width + x;
  }
};
