/**
 * @file localize.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Classes for localizing from cameras
 * @version 1.0 (@LastMod : 2026/10/15 22:54:37)
 * @date 2026-10-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <string>
#include <utility>

#include "image.hpp"
#include "video_camera.hpp"

template <std::size_t width, std::size_t height>
class StereoCamera
{
 private:
  VideoCamera<width, height> leftCamera;
  VideoCamera<width, height> rightCamera;

 public:
  StereoCamera(
      const std::string& leftCameraPath, const std::string& rightCameraPath)
      : leftCamera(leftCameraPath), rightCamera(rightCameraPath)
  {
  }

  ~StereoCamera() = default;

  StereoCamera(const StereoCamera&) = delete;
  StereoCamera& operator=(const StereoCamera&) = delete;
  StereoCamera(StereoCamera&&) = delete;
  StereoCamera& operator=(StereoCamera&&) = delete;

  constexpr std::size_t getWidth() const { return leftCamera.getWidth(); }
  constexpr std::size_t getHeight() const { return leftCamera.getHeight(); }

  std::array<Image<width, height>, 2> getFrames()
  {
    auto leftFrame = leftCamera.getFrame();
    auto rightFrame = rightCamera.getFrame();
    return {std::move(leftFrame), std::move(rightFrame)};
  }

  Image<width, height> getErrorFrame(std::size_t threshold)
  {
    std::array<Image<width, height>, 2> frames = getFrames();
    std::array<Image<width, height>, 3> outputBuffers = {
        Image<width, height>(), Image<width, height>(), Image<width, height>()};

    for (std::size_t i = 0; i < 2; ++i)
    {
      Image<width, height>::getFeaturesFAST(
          frames[i], outputBuffers[i], threshold);
    }

    for (std::size_t y = 0; y < height; ++y)
    {
      for (std::size_t x = 0; x < width; ++x)
      {
        std::uint8_t leftPixel = outputBuffers[0](x, y);

        Image leftSelection = outputBuffers[0].getSelection<3, 3>(x, y);

        if (leftPixel > 0)
        {
          for (std::size_t j = 0; j < width; ++j)
          {
            std::uint8_t rightPixel = outputBuffers[1](j, y);
            if (rightPixel > 0)
            {
              Image rightSelection = outputBuffers[1].getSelection<3, 3>(j, y);

              auto error =
                  (leftSelection - rightSelection)
                      .frobeniusInnerProduct(leftSelection - rightSelection) *
                  1.0 / 9.0;

              outputBuffers[2](x, y) = static_cast<std::uint8_t>(
                  std::clamp(static_cast<int>(error), 0, 255));
            }
          }
        }
      }
    }
    return outputBuffers[2];
  }
};