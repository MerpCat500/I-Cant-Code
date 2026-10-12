/**
 * @file image.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief
 * @version 2.0 (@LastMod : 2026/10/12 14:51:09)
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

#include "kernal.hpp"

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
      {0, 3},    // Above Center
      {0, -3},   // Below Center
      {3, 0},    // Right Center
      {-3, 0},   // Left Center
      {2, 2},    // Diagonal Center Above Right
      {2, -2},   // Diagonal Center Below Right
      {-2, 2},   // Diagonal Center Above Left
      {-2, -2},  // Diagonal Center Below Left
      {1, 3},    // Above Center Slight Right
      {-1, 3},   // Above Center Slight Left
      {3, 1},    // Right Center Slight Above
      {3, -1},   // Right Center Slight Below
      {1, -3},   // Below Center Slight Right
      {-1, -3},  // Below Center Slight Left
      {-3, 1},   // Left Center Slight Above
      {-3, -1},  // Left Center Slight Below
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

  template <std::size_t kernalSize, typename kernalType = float>
  static void convolve(
      const Image& inputBuffer,
      Image& outputBuffer,
      const Kernal<kernalType, kernalSize>& kernal)
  {
  }

  /**
   * @brief Get the Features Accelerated Segment Test (FAST)
   *
   */
  static void getFeaturesFAST(
      const Image<width, height>& inputBuffer,
      Image<width, height>& outputBuffer,
      const std::size_t threshold)
  {
    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t& outputPixel = outputBuffer(x, y);
        const std::uint8_t& inputPixel = inputBuffer(x, y);

        std::size_t isExtreme = 0;

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

            if (std::abs(
                    static_cast<int>(inputPixel) -
                    static_cast<int>(neighborPixel)) > threshold)
            {
              isExtreme++;
            }
          }
          if (isExtreme >= 12)
          {
            outputPixel = 255;  // Mark as a feature
            break;
          }
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
