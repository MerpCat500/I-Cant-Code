/**
 * @file image.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief
 * @version 2.0 (@LastMod : 2026/10/09 18:21:26)
 * @date 2026-10-04
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

template <std::size_t width, std::size_t height>
class Image
{
 private:
  std::array<std::uint8_t, width * height> data{};
  constexpr static size_t readChannels = 4;  // RGBA
  constexpr static size_t writeChannels = 1;  // Grayscale


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

  Image(std::array<std::uint8_t, width * height> data) : data(std::move(data)) {}

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
    if (stbi_write_png(filename, width, height, writeChannels, data.data(), width * writeChannels ) ==
        0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
    }
  }

  void save_jpg(const char* filename, int quality = 90) const
  {
    if (stbi_write_jpg(filename, width, height, writeChannels, data.data(), quality) == 0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
    }
  }

  void save_bmp(const char* filename) const
  {
    if (stbi_write_bmp(filename, width, height, writeChannels, data.data()) == 0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
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
