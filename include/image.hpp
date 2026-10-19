/**
 * @file image.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Image Class and Image View Class
 * @version 3.0 (@LastMod : 2026/10/19 14:50:38)
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "kernel.hpp"

template <std::size_t width, std::size_t height>
class ImageView;  // Forward Declaration

/**
 * @brief Image class for storing grayscale images.
 *
 */
class Image : public std::enable_shared_from_this<Image>
{
 private:
  std::vector<std::uint8_t> data;
  std::size_t width;
  std::size_t height;
  constexpr static std::array<std::pair<int, int>, 16> fastOffsets = {{
      {0, -3},
      {1, -3},
      {2, -2},
      {3, -1},
      {3, 0},
      {3, 1},
      {2, 2},
      {1, 3},
      {0, 3},
      {-1, 3},
      {-2, 2},
      {-3, 1},
      {-3, 0},
      {-3, -1},
      {-2, -2},
      {-1, -3},
  }};

 private:
  /**
   * @brief Empty Image Constructor
   *
   * @param width - Width in pixels of image
   * @param height - Height in pixels of image
   */
  Image(std::size_t width, std::size_t height) : data(width * height), width(width), height(height)
  {
  }

  /**
   * @brief Construct Image with existing data
   *
   * @param width - Width of image in pixels.
   * @param height - Height of image in pixels.
   * @param inputData - Existing data to be put in Image.
   */
  Image(std::size_t width, std::size_t height, const std::vector<std::uint8_t>& inputData)
      : data(inputData), width(width), height(height)
  {
  }

  /**
   * @brief Construct a Image given a C-Style memory buffer.
   *
   * @param width - Width of image in pixels.
   * @param height - Height of image in pixels.
   * @param inputData - Existing data to be put in Image.
   */
  Image(std::size_t width, std::size_t height, const uint8_t* inputData)
      : data(inputData, inputData + width * height), width(width), height(height)
  {
  }

 public:
  /**
   * @brief Creates a shared pointer to an image.
   *
   * @param width - Width of the image.
   * @param height - Height of the image.
   * @return std::shared_ptr<Image> - Counted reference to the image.
   */
  static std::shared_ptr<Image> create(std::size_t width, std::size_t height)
  { return std::shared_ptr<Image>(new Image(width, height)); }

  /**
   * @brief Creates a shared pointer to an image.
   *
   * @param width - Width of the image.
   * @param height - Height of the image.
   * @param inputData - Data to be referenced from image class.
   * @return std::shared_ptr<Image> - Counted reference to the image.
   */
  static std::shared_ptr<Image> create(
      std::size_t width, std::size_t height, const std::vector<std::uint8_t>& inputData)
  { return std::shared_ptr<Image>(new Image(width, height, inputData)); }

  /**
   * @brief Creates a shared pointer to an image.
   *
   * @param width - Width of the image.
   * @param height - Height of the image.
   * @param inputData - Data to be referenced from the image class.
   * @return std::shared_ptr<Image> - Counted reference to the image.
   */
  static std::shared_ptr<Image> create(
      std::size_t width, std::size_t height, const uint8_t* inputData)
  { return std::shared_ptr<Image>(new Image(width, height, inputData)); }

 public:
  /**
   * @brief Get the width of the image.
   *
   * @return std::size_t - Width of the image
   */
  std::size_t getWidth() const { return width; }

  /**
   * @brief Get the height of the image.
   *
   * @return std::size_t - Height of the image
   */
  std::size_t getHeight() const { return height; }

  /**
   * @brief Get a non-const reference to pixel in image. This will throw std::out_of_range() if
   * pixel index is out of range.
   *
   * @param x - X-Coordinate of Pixel
   * @param y - Y-Coordinate of Pixel
   * @return std::uint8_t& - Pixel Reference (upon success)
   */
  std::uint8_t& operator()(std::size_t x, std::size_t y)
  {
    if (!this->inImage(x, y))
    {
      throw std::out_of_range(
          "Image: " + std::to_string(x) + ", " + std::to_string(y) + " is out of range.");
    }
    return data[y * width + x];
  }

  /**
   * @brief Get a const reference to pixel in image. This will throw std::out_of_range() if pixel
   * index is out of range.
   *
   * @param x - X-Coordinate of Pixel
   * @param y - Y-Coordinate of Pixel
   * @return const std::uint8_t& - Pixel Reference (upon success)
   */
  const std::uint8_t& operator()(std::size_t x, std::size_t y) const
  {
    if (!this->inImage(x, y))
    {
      throw std::out_of_range(
          "Image: " + std::to_string(x) + ", " + std::to_string(y) + " is out of range.");
    }
    return data[y * width + x];
  }

  /**
   * @brief Get an ImageView inside of the image.
   *
   * @tparam selectionWidth - Width of selection box
   * @tparam selectionHeight - Height of selection box
   * @param x - X-Coordinate in image (top-left coordinate of selection box)
   * @param y - Y-Coordinate in image (top-left coordinate of selection box)
   * @return ImageView<selectionWidth, selectionHeight> - View of image
   */
  template <std::size_t selectionWidth, std::size_t selectionHeight>
  ImageView<selectionWidth, selectionHeight> getSelection(std::size_t x, std::size_t y)
  {
    if (x + selectionWidth > width || y + selectionHeight > height)
    {
      throw std::runtime_error(
          "ImageView<" + std::to_string(selectionWidth) + ", " + std::to_string(selectionHeight) +
          "> couldn't be made at (" + std::to_string(y) + ", " + std::to_string(y) + ").");
    }

    return ImageView<selectionWidth, selectionHeight>(x, y, shared_from_this());
  }

  /**
   * @brief Get a constant pointer to the raw data stored under this class
   *
   * @return const std::vector<std::uint8_t>& data (single channel - grayscale)
   */
  const std::vector<std::uint8_t>& getRawData() const { return this->data; }

  std::vector<std::uint8_t>& getRawData() { return this->data; }

  /**
   * @brief Is pixel in image?
   *
   * @param x - X-Coordinate to check.
   * @param y - Y-Coordinate to check.
   * @return true - Coordinate is in image.
   * @return false - Coordinate is out of image.
   */
  bool inImage(std::size_t x, std::size_t y) const { return (x < width && y < height); }

  /**
   * @brief Create a new image containing the output of a FAST-16 Algorithm. This will also create a
   * output buffer. //!DO NOT USE THIS IN A LOOP!
   *
   * @param threshold - Threshold to be considered lighter / darker than pixel
   * @param consecutivePixels - Consecutive pixels required to quit early and mark as corner.
   * @return std::shared_ptr<Image>
   */
  std::shared_ptr<Image> getFeaturesFast(std::size_t threshold, std::size_t consecutivePixels)
  {
    auto outputBuffer = Image::create(this->getWidth(), this->getHeight());
    getFeaturesFast(threshold, consecutivePixels, outputBuffer);
    return outputBuffer;
  }

  /**
   * @brief Run FAST-16 without creating a output buffer
   *
   * @param threshold - Threshold to be considered lighter / darker than pixel
   * @param consecutivePixels - Consecutive Pixels required to quit early and mark as corner
   * @param outputBuffer - Buffer to be modified by FAST (Avoids creating buffers in a loop).
   */
  void getFeaturesFast(
      std::size_t threshold,
      std::size_t consecutivePixels,
      const std::shared_ptr<Image>& outputBuffer)
  {
    if (!outputBuffer || outputBuffer->getWidth() != width || outputBuffer->getHeight() != height)
    {
      throw std::invalid_argument("Feature output dimensions do not match image dimensions");
    }

    const auto& inputPixels = data;
    auto& outputPixels = outputBuffer->data;

    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t& outputPixel = outputPixels[y * width + x];
        const std::uint8_t inputPixel = inputPixels[y * width + x];

        std::size_t isExtreme = 0;
        std::size_t firstConsecutivePixels = 0;
        bool firstConsecutivePixelsSet = false;
        bool lastBrighter = true;
        bool lastDarker = true;

        for (auto offset : fastOffsets)
        {
          int neighborX = static_cast<int>(x) + offset.first;
          int neighborY = static_cast<int>(y) + offset.second;

          if (neighborX < static_cast<int>(width) && neighborY < static_cast<int>(height) &&
              neighborX >= 0 && neighborY >= 0)
          {
            const std::uint8_t neighborPixel = inputPixels
                [static_cast<std::size_t>(neighborY) * width + static_cast<std::size_t>(neighborX)];

            bool isBrighter =
                neighborPixel > static_cast<int>(inputPixel) + static_cast<int>(threshold);
            bool isDarker =
                neighborPixel < static_cast<int>(inputPixel) - static_cast<int>(threshold);
            bool isNotExtreme = !(isBrighter && lastBrighter) && !(isDarker && lastDarker);

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
            isExtreme += firstConsecutivePixels;  // Wrap around to the first pixels
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

  template <typename kernelType, std::size_t kernelSize>
  std::shared_ptr<Image> convolve(const Kernel<kernelType, kernelSize>& kernel)
  {
    // TODO: Clean and check this algorithm, it was taken from V1 without review
    std::size_t outputWidth = width - kernelSize + 1;
    std::size_t outputHeight = height - kernelSize + 1;

    auto outputBuffer = Image::create(outputWidth, outputHeight);

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
          horizontalSum += static_cast<long double>((*this)(x + column, y)) *
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
          convolutionResult += horizontalPass[(outputY + row) * outputWidth + outputX] *
                               static_cast<long double>(verticalKernel[row]);
        }

        (*outputBuffer)(outputX, outputY) =
            static_cast<std::uint8_t>(std::clamp(convolutionResult, 0.0L, 255.0L));
      }
    }

    return outputBuffer;
  }
};

/**
 * @brief ImageView class allows for selecting a section of an image without copy.
 *
 * @tparam width Width of Selection (Must be known at compile time)
 * @tparam height Height of Selection (Must be known at compile time)
 */
template <std::size_t width, std::size_t height>
class ImageView
{
 private:
  // Offset to the start of the selection within the parent image.
  std::size_t m_xOffset, m_yOffset;

  // Pointer to Parent. Will keep parent alive.
  std::shared_ptr<Image> m_pParent;

 public:
  /**
   * @brief Get const value of pixel inside of view
   *
   * @param x - X-Coordinate relative to view
   * @param y - Y-Coordinate relative to view
   * @return const std::uint8_t&
   */
  const std::uint8_t& operator()(std::size_t x, std::size_t y) const
  {
    if (!this->inView(x, y))
    {
      throw std::runtime_error(
          "Pixel coordinates are out of bounds for the image view: " + std::to_string(x) + "," +
          std::to_string(y) + ", starting at (" + std::to_string(m_xOffset) + ", " +
          std::to_string(m_yOffset) + ")");
    }

    return (*m_pParent)(x + m_xOffset, y + m_yOffset);
  }

  /**
   * @brief Get const value of pixel inside of view
   *
   * @param x - X-Coordinate relative to view
   * @param y - Y-Coordinate relative to view
   * @return const std::uint8_t&
   */
  const std::uint8_t& getPixel(std::size_t x, std::size_t y) const
  {
    if (!this->inView(x, y))
    {
      throw std::runtime_error(
          "Pixel coordinates are out of bounds for the image view: " + std::to_string(x) + "," +
          std::to_string(y) + ", starting at (" + std::to_string(m_xOffset) + ", " +
          std::to_string(m_yOffset) + ")");
    }

    return (*this)(x, y);
  }

  /**
   * @brief Get the Width of ImageView
   *
   * @return constexpr std::size_t
   */
  constexpr std::size_t getWidth() const { return width; }

  /**
   * @brief Get the Height of ImageView
   *
   * @return constexpr std::size_t
   */
  constexpr std::size_t getHeight() const { return height; }

  /**
   * @brief Returns whether a pixel is inside of view.
   *
   * @param x - X-Coordinate
   * @param y - Y-Coordinate
   * @return true - Is in Image View
   * @return false - Is out of Image View
   */
  bool inView(std::size_t x, std::size_t y) const
  { return (x <= m_xOffset + width && y <= m_yOffset + height); }

 protected:
  /**
   * @brief Image view constructor. Can be constructed by friendly classes.
   *
   * @param Offset X-Coordinate offset of the view within the parent image.
   * @param yOffset Y-Coordinate offset of the view within the parent image.
   * @param parent Shared pointer to the parent image.
   */
  ImageView(std::size_t Offset, std::size_t yOffset, std::shared_ptr<Image> pParent)
      : m_xOffset(Offset), m_yOffset(yOffset), m_pParent(pParent)
  {
  }

  friend Image;  // Allows Image Class to create image views
};
