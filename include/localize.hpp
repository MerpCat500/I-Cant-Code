/**
 * @file localize.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Classes for localizing from cameras
 * @version 1.0 (@LastMod : 2026/10/19 14:59:28)
 * @date 2026-10-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <array>
#include <cmath>
#include <limits>
#include <string>

#include "image.hpp"
#include "video_camera.hpp"

class StereoCamera
{
 private:
  VideoCamera leftCamera;
  VideoCamera rightCamera;
  std::array<std::shared_ptr<Image>, 2> frames;
  std::array<std::shared_ptr<Image>, 2> featureBuffers;
  std::shared_ptr<Image> errorBuffer;

 public:
  StereoCamera(const std::string& leftCameraPath, const std::string& rightCameraPath)
      : leftCamera(leftCameraPath), rightCamera(rightCameraPath)
  {
    if (leftCamera.getWidth() != rightCamera.getWidth() ||
        leftCamera.getHeight() != rightCamera.getHeight())
    {
      throw std::runtime_error("Stereo cameras negotiated different frame sizes");
    }
    frames = {Image::create(getWidth(), getHeight()), Image::create(getWidth(), getHeight())};
    featureBuffers = {
        Image::create(getWidth(), getHeight()), Image::create(getWidth(), getHeight())};
    errorBuffer = Image::create(getWidth(), getHeight());
  }

  ~StereoCamera() = default;

  StereoCamera(const StereoCamera&) = delete;
  StereoCamera& operator=(const StereoCamera&) = delete;
  StereoCamera(StereoCamera&&) = delete;
  StereoCamera& operator=(StereoCamera&&) = delete;

  std::size_t getWidth() const { return leftCamera.getWidth(); }
  std::size_t getHeight() const { return leftCamera.getHeight(); }

  std::array<std::shared_ptr<Image>, 2> getFrames()
  {
    leftCamera.getFrame(frames[0]);
    rightCamera.getFrame(frames[1]);
    return frames;
  }

  template <std::size_t selectionSize>
  std::array<std::shared_ptr<Image>, 5> getErrorFrame(
      std::size_t threshold, std::size_t consecutivePixels)
  {
    getFrames();
    const std::size_t frameWidth = getWidth();
    const std::size_t frameHeight = getHeight();

    for (std::size_t i = 0; i < frames.size(); ++i)
    {
      frames[i]->getFeaturesFast(threshold, consecutivePixels, featureBuffers[i]);
    }

    auto& errorPixels = errorBuffer->getRawData();
    std::fill(errorPixels.begin(), errorPixels.end(), 0);
    const auto& leftFeatures = featureBuffers[0]->getRawData();
    const auto& rightFeatures = featureBuffers[1]->getRawData();
    const auto& leftPixels = frames[0]->getRawData();
    const auto& rightPixels = frames[1]->getRawData();

    for (std::size_t y = 0; y < frameHeight - selectionSize; ++y)
    {
      for (std::size_t x = 0; x < frameWidth - selectionSize; ++x)
      {
        // For a rectified left/right stereo pair, a point moves left in the right image.
        if (leftFeatures[y * frameWidth + x] > 0)
        {
          std::uint32_t bestError = std::numeric_limits<std::uint32_t>::max();
          std::uint32_t secondBestError = std::numeric_limits<std::uint32_t>::max();
          std::size_t bestX = x;

          for (std::size_t j = 0; j <= x; ++j)
          {
            if (rightFeatures[y * frameWidth + j] > 0)
            {
              std::uint32_t error = 0;
              for (std::size_t selectionY = 0; selectionY < selectionSize; ++selectionY)
              {
                for (std::size_t selectionX = 0; selectionX < selectionSize; ++selectionX)
                {
                  error += std::abs(
                      static_cast<int>(leftPixels[(y + selectionY) * frameWidth + x + selectionX]) -
                      static_cast<int>(
                          rightPixels[(y + selectionY) * frameWidth + j + selectionX]));
                }
              }

              if (error < bestError)
              {
                secondBestError = bestError;
                bestError = error;
                bestX = j;
              }
              else if (error < secondBestError) { secondBestError = error; }
            }
          }

          constexpr std::uint32_t maxError = selectionSize * selectionSize * 255;
          constexpr std::uint32_t acceptedError = maxError / 3;
          const bool hasDistinctMatch =
              secondBestError == std::numeric_limits<std::uint32_t>::max() ||
              bestError * 5 < secondBestError * 4;
          if (bestError <= acceptedError && hasDistinctMatch)
          {
            errorPixels[y * frameWidth + bestX] =
                static_cast<std::uint8_t>(255 - (bestError * 255 / maxError));
          }
        }
      }
    }

    return {featureBuffers[0], featureBuffers[1], errorBuffer, frames[0], frames[1]};
  }

  template <std::size_t selectionSize>
  float getError(
      ImageView<selectionSize, selectionSize> leftSelection,
      ImageView<selectionSize, selectionSize> rightSelection)
  {
    std::uint32_t errorSum = 0;

    for (std::size_t y = 0; y < selectionSize; ++y)
    {
      for (std::size_t x = 0; x < selectionSize; ++x)
      {
        errorSum += std::abs(
            static_cast<int>(leftSelection.getPixel(x, y)) -
            static_cast<int>(rightSelection.getPixel(x, y)));
      }
    }

    return errorSum;
  }
};