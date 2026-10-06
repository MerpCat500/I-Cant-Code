/**
 * @file image.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief
 * @version 1.0 (@LastMod : 2026/10/05 13:39:20)
 * @date 2026-10-04
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Pixel
{
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;

  bool operator>(const Pixel& other) const
  { return (r > other.r) && (g > other.g) && (b > other.b) && (a > other.a); }
  bool operator<(const Pixel& other) const
  { return (r < other.r) && (g < other.g) && (b < other.b) && (a < other.a); }
  bool operator==(const Pixel& other) const
  {
    return (r == other.r) && (g == other.g) && (b == other.b) && (a == other.a);
  }
};

static_assert(sizeof(Pixel) == 4, "Pixel must contain four bytes");

class Image
{
 private:
  int width = 0;
  int height = 0;
  int bpp = 0;
  std::unique_ptr<Pixel[]> data;
  constexpr static size_t channels = 4;  // RGBA

 public:
  Image(const char* filename)
  {
    // Get Raw Data
    stbi_uc* decodedData = stbi_load(filename, &width, &height, &bpp, channels);

    // If there is no data, then print image load error and return nothing.
    if (!decodedData)
    {
      std::cerr << "Failed to load image: " << filename << std::endl;
      return;
    }

    // Get count of pixels (Cast to uint first to allow for larger images
    // [avoids overflows])
    const std::size_t pixelCount = static_cast<std::size_t>(width) * height;

    // Allocate memory for the image data
    data = std::make_unique<Pixel[]>(pixelCount);

    // Copy the decoded data into the pixel array
    std::memcpy(data.get(), decodedData, pixelCount * sizeof(Pixel));

    // Free the decoded data
    stbi_image_free(decodedData);
  }

  Image(int width, int height, int bpp, std::unique_ptr<Pixel[]> data)
      : width(width), height(height), bpp(bpp), data(std::move(data))
  {
  }

  Image(int width, int height, int bpp, const Pixel fillColor)
      : width(width),
        height(height),
        bpp(bpp),
        data(
            std::make_unique<Pixel[]>(static_cast<std::size_t>(width) * height))
  {
    std::fill(
        data.get(),
        data.get() + static_cast<std::size_t>(width) * height,
        fillColor);
  }

  Image(int width, int height, int bpp)
      : width(width),
        height(height),
        bpp(bpp),
        data(
            std::make_unique<Pixel[]>(static_cast<std::size_t>(width) * height))
  {
  }

  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;
  Image(Image&&) noexcept = default;
  Image& operator=(Image&&) noexcept = default;

  int getWidth() const { return width; }
  int getHeight() const { return height; }
  int getOriginalBPP() const { return bpp; }

  Pixel& operator()(int x, int y) { return data[checkedIndex(x, y)]; }

  const Pixel& operator()(int x, int y) const
  { return data[checkedIndex(x, y)]; }

  const Pixel& getPixel(int x, int y) const { return (*this)(x, y); }

  Image getSelection(
      int x, int y, int selectionWidth, int selectionHeight) const
  {
    if (x < 0 || y < 0 || x + selectionWidth > width ||
        y + selectionHeight > height)
    {
      throw std::out_of_range("Selection is outside the image");
    }

    std::unique_ptr<Pixel[]> selectionData = std::make_unique<Pixel[]>(
        static_cast<std::size_t>(selectionWidth) * selectionHeight);

    for (int j = 0; j < selectionHeight; ++j)
    {
      for (int i = 0; i < selectionWidth; ++i)
      {
        selectionData[static_cast<std::size_t>(j) * selectionWidth + i] =
            (*this)(x + i, y + j);
      }
    }

    return Image(
        selectionWidth, selectionHeight, bpp, std::move(selectionData));
  }

  void save_png(const char* filename) const
  {
    if (!data)
    {
      std::cerr << "No image data to save." << std::endl;
      return;
    }

    if (stbi_write_png(
            filename, width, height, channels, data.get(), width * channels) ==
        0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
    }
  }

  void save_jpg(const char* filename, int quality = 90) const
  {
    if (!data)
    {
      std::cerr << "No image data to save." << std::endl;
      return;
    }

    if (stbi_write_jpg(
            filename, width, height, channels, data.get(), quality) == 0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
    }
  }

  void save_bmp(const char* filename) const
  {
    if (!data)
    {
      std::cerr << "No image data to save." << std::endl;
      return;
    }

    if (stbi_write_bmp(filename, width, height, channels, data.get()) == 0)
    {
      std::cerr << "Failed to save image: " << filename << std::endl;
    }
  }

  template <size_t kernalSize>
  void convolve(
      std::array<std::array<float, kernalSize>, kernalSize> kernel,
      Image& OutputBuffer) const
  {
    if (kernalSize % 2 == 0)
    {
      throw std::invalid_argument("Kernel size must be odd");
    }

    int offset = kernalSize / 2;

    int outputWidth = OutputBuffer.getWidth();
    int outputHeight = OutputBuffer.getHeight();

    for (int y = 0; y < outputHeight; ++y)
    {
      for (int x = 0; x < outputWidth; ++x)
      {
        const float sourceX =
            (static_cast<float>(x) + 0.5f) * width / outputWidth - 0.5f;
        const float sourceY =
            (static_cast<float>(y) + 0.5f) * height / outputHeight - 0.5f;
        float r = 0.0f, g = 0.0f, b = 0.0f;

        for (int ky = -offset; ky <= offset; ++ky)
        {
          for (int kx = -offset; kx <= offset; ++kx)
          {
            const Pixel p = sampleBilinear(sourceX + kx, sourceY + ky);
            const float weight = kernel[ky + offset][kx + offset];

            r += p.r * weight;
            g += p.g * weight;
            b += p.b * weight;
          }
        }

        OutputBuffer(x, y) = Pixel{
            static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f)),
            sampleBilinear(sourceX, sourceY).a};
      }
    }
  }

  void operator-=(const Image& B)
  {
    if (width != B.getWidth() || height != B.getHeight())
    {
      throw std::invalid_argument(
          "Images must have the same dimensions for subtraction");
    }

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        Pixel& pixelA = (*this)(x, y);
        const Pixel& pixelB = B(x, y);

        pixelA.r = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(pixelA.r) - pixelB.r, 0, 255));
        pixelA.g = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(pixelA.g) - pixelB.g, 0, 255));
        pixelA.b = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(pixelA.b) - pixelB.b, 0, 255));
      }
    }
  }

  void operator+=(const Image& B)
  {
    if (width != B.getWidth() || height != B.getHeight())
    {
      throw std::invalid_argument(
          "Images must have the same dimensions for subtraction");
    }

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        Pixel& pixelA = (*this)(x, y);
        const Pixel& pixelB = B(x, y);

        pixelA.r = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(pixelA.r) + pixelB.r, 0, 255));
        pixelA.g = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(pixelA.g) + pixelB.g, 0, 255));
        pixelA.b = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(pixelA.b) + pixelB.b, 0, 255));
      }
    }
  }

  Image operator-(const Image& B) const
  {
    if (width != B.getWidth() || height != B.getHeight())
    {
      throw std::invalid_argument(
          "Images must have the same dimensions for subtraction");
    }

    Image outputImage(width, height, bpp);

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        Pixel& pixelA = outputImage(x, y);
        const Pixel& sourcePixel = (*this)(x, y);
        const Pixel& pixelB = B(x, y);

        pixelA.r = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(sourcePixel.r) - pixelB.r, 0, 255));
        pixelA.g = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(sourcePixel.g) - pixelB.g, 0, 255));
        pixelA.b = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(sourcePixel.b) - pixelB.b, 0, 255));
        pixelA.a = sourcePixel.a;
      }
    }
    return outputImage;
  }

  Image operator+(const Image& B) const
  {
    if (width != B.getWidth() || height != B.getHeight())
    {
      throw std::invalid_argument(
          "Images must have the same dimensions for subtraction");
    }

    Image outputImage(width, height, bpp);

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        Pixel& pixelA = outputImage(x, y);
        const Pixel& sourcePixel = (*this)(x, y);
        const Pixel& pixelB = B(x, y);

        pixelA.r = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(sourcePixel.r) + pixelB.r, 0, 255));
        pixelA.g = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(sourcePixel.g) + pixelB.g, 0, 255));
        pixelA.b = static_cast<std::uint8_t>(
            std::clamp(static_cast<int>(sourcePixel.b) + pixelB.b, 0, 255));
        pixelA.a = sourcePixel.a;
      }
    }
    return outputImage;
  }

  void operator*=(const float B)
  {
    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        Pixel& pixelA = (*this)(x, y);

        pixelA.r = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(pixelA.r) * B, 0.0f, 255.0f));
        pixelA.g = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(pixelA.g) * B, 0.0f, 255.0f));
        pixelA.b = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(pixelA.b) * B, 0.0f, 255.0f));
      }
    }
  }

  Image operator*(const float B) const
  {
    Image outputImage(width, height, bpp);

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        const Pixel& sourcePixel = (*this)(x, y);
        Pixel& pixelA = outputImage(x, y);

        pixelA.r = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(sourcePixel.r) * B, 0.0f, 255.0f));
        pixelA.g = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(sourcePixel.g) * B, 0.0f, 255.0f));
        pixelA.b = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(sourcePixel.b) * B, 0.0f, 255.0f));
        pixelA.a = sourcePixel.a;
      }
    }

    return outputImage;
  }

  void toGrayScale()
  {
    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        Pixel& pixel = (*this)(x, y);
        std::uint8_t grayValue = static_cast<std::uint8_t>(
            0.299f * pixel.r + 0.587f * pixel.g + 0.114f * pixel.b);
        pixel.r = grayValue;
        pixel.g = grayValue;
        pixel.b = grayValue;
      }
    }
  }

 private:
  Pixel sampleBilinear(float x, float y) const
  {
    const int x0 = std::clamp(static_cast<int>(x), 0, width - 1);
    const int y0 = std::clamp(static_cast<int>(y), 0, height - 1);
    const int x1 = std::min(x0 + 1, width - 1);
    const int y1 = std::min(y0 + 1, height - 1);
    const float xFraction = std::clamp(x - x0, 0.0f, 1.0f);
    const float yFraction = std::clamp(y - y0, 0.0f, 1.0f);

    const Pixel& topLeft = (*this)(x0, y0);
    const Pixel& topRight = (*this)(x1, y0);
    const Pixel& bottomLeft = (*this)(x0, y1);
    const Pixel& bottomRight = (*this)(x1, y1);

    const auto interpolate = [&](std::uint8_t topLeftChannel,
                                 std::uint8_t topRightChannel,
                                 std::uint8_t bottomLeftChannel,
                                 std::uint8_t bottomRightChannel) {
      const float top =
          topLeftChannel * (1.0f - xFraction) + topRightChannel * xFraction;
      const float bottom = bottomLeftChannel * (1.0f - xFraction) +
                           bottomRightChannel * xFraction;
      return static_cast<std::uint8_t>(std::clamp(
          top * (1.0f - yFraction) + bottom * yFraction, 0.0f, 255.0f));
    };

    return Pixel{
        interpolate(topLeft.r, topRight.r, bottomLeft.r, bottomRight.r),
        interpolate(topLeft.g, topRight.g, bottomLeft.g, bottomRight.g),
        interpolate(topLeft.b, topRight.b, bottomLeft.b, bottomRight.b),
        interpolate(topLeft.a, topRight.a, bottomLeft.a, bottomRight.a)};
  }

  std::size_t checkedIndex(int x, int y) const
  {
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
      throw std::out_of_range("Pixel coordinates are outside the image");
    }

    return static_cast<std::size_t>(y) * width + x;
  }
};
