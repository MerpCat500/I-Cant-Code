/**
 * @file localize.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Classes for localizing from cameras
 * @version 1.0 (@LastMod : 2026/10/15 23:47:24)
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
  StereoCamera(const std::string& leftCameraPath, const std::string& rightCameraPath)
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

  template <std::size_t selectionSize>
  Image<width, height>& getErrorFrame(std::size_t threshold)
  {
    std::array<Image<width, height>, 2> frames = getFrames();
    std::array<Image<width, height>, 3> outputBuffers = {
        Image<width, height>(), Image<width, height>(), Image<width, height>()};

    std::size_t selectionSizeHalf = selectionSize / 2;

    for (std::size_t i = 0; i < frames.size(); ++i)
    {
      Image<width, height>::getFeaturesFAST(frames[i], outputBuffers[i], threshold);
    }

    for (std::size_t y = selectionSizeHalf; y < height - selectionSizeHalf; ++y)
    {
      for (std::size_t x = selectionSizeHalf; x < width - selectionSizeHalf; ++x)
      {
        std::uint8_t leftPixel = outputBuffers[0](x, y);

        const auto& leftSelection =
            outputBuffers[0].template getSelection<selectionSize, selectionSize>(x, y);

        if (leftPixel > 0)
        {
          for (std::size_t j = selectionSizeHalf; j < width - selectionSizeHalf; ++j)
          {
            std::uint8_t rightPixel = outputBuffers[1](j, y);
            if (rightPixel > 0)
            {
              const auto& rightSelection =
                  outputBuffers[1].template getSelection<selectionSize, selectionSize>(j, y);

              auto error = Image<selectionSize, selectionSize>::meanStandardDeviation(
                  leftSelection, rightSelection);

              outputBuffers[2](x, y) =
                  static_cast<std::uint8_t>(255 - std::clamp(static_cast<int>(error), 0, 255));
            }
          }
        }
      }
    }

    for (size_t i = 0; i < outputBuffers.size(); ++i)
    {
      outputBuffers[i].save_png(("error_frame" + std::to_string(i) + ".png").c_str());
    }

    for (size_t i = 0; i < frames.size(); ++i)
    {
      frames[i].save_png(("frame" + std::to_string(i) + ".png").c_str());
    }

    return outputBuffers[2];
  }
};