/**
 * @file localize.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Classes for localizing from cameras
 * @version 1.0 (@LastMod : 2026/10/29 11:33:20)
 * @date 2026-10-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "image.hpp"
#include "video_camera.hpp"

#pragma once

struct Match
{
  constexpr static float cameraSpacing = 4.0f;
  constexpr static float focalLength = 711.1111f;

  const ImageView<32, 32>* left;
  const ImageView<32, 32>* right;
  float disparityX;

  float getDistance() const
  {
    if (distance == 0.0f) { distance = focalLength * cameraSpacing / disparityX; }

    return distance;
  }

  mutable float distance = 0.0f;
};

class StereoCamera
{
 private:
  VideoCamera leftCamera;
  VideoCamera rightCamera;

  std::array<std::shared_ptr<Image>, 2> frames;
  std::array<std::vector<ImageView<32, 32>>, 2> featureBuffers;

 public:
  StereoCamera(const std::string& leftCameraPath, const std::string& rightCameraPath)
      : leftCamera(leftCameraPath), rightCamera(rightCameraPath)
  {
    if (leftCamera.getWidth() != rightCamera.getWidth() ||
        leftCamera.getHeight() != rightCamera.getHeight())
    {
      throw std::runtime_error("Stereo cameras negotiated different frame sizes");
    }

    frames = {
        Image::create(this->getWidth(), this->getHeight()),
        Image::create(this->getWidth(), this->getHeight())};
  }

  ~StereoCamera() = default;

  StereoCamera(const StereoCamera&) = delete;
  StereoCamera& operator=(const StereoCamera&) = delete;
  StereoCamera(StereoCamera&&) = delete;
  StereoCamera& operator=(StereoCamera&&) = delete;

  std::size_t getWidth() const { return leftCamera.getWidth(); }
  std::size_t getHeight() const { return leftCamera.getHeight(); }

  const std::array<std::vector<ImageView<32, 32>>, 2>& getFeatureBuffers()
  { return featureBuffers; }

  std::array<std::shared_ptr<Image>, 2> getFrames()
  {
    leftCamera.getFrame(frames[0]);
    rightCamera.getFrame(frames[1]);
    return frames;
  }

  template <std::size_t selectionSize>
  std::vector<Match> getMatches(
      std::size_t threshold, std::size_t consecutivePixels, std::size_t errorThreshold)
  {
    getFrames();

    std::vector<Match> output;

    for (std::size_t i = 0; i < frames.size(); ++i)
    {
      featureBuffers[i] = frames[i]->getFeaturesFast<32>(threshold, consecutivePixels);
    }

    std::mutex outputMutex;
    std::atomic<std::size_t> nextLeftFeature{0};
    std::vector<std::thread> tasks;
    const std::size_t hardwareThreads = std::thread::hardware_concurrency();
    const std::size_t workerCount =
        std::min(featureBuffers[0].size(), hardwareThreads == 0 ? std::size_t{1} : hardwareThreads);
    tasks.reserve(workerCount);

    for (std::size_t worker = 0; worker < workerCount; ++worker)
    {
      tasks.emplace_back([this, &nextLeftFeature, errorThreshold, &output, &outputMutex]() {
        std::vector<Match> workerMatches;

        while (true)
        {
          const std::size_t leftIndex = nextLeftFeature.fetch_add(1);
          if (leftIndex >= featureBuffers[0].size()) { break; }

          const auto& leftFeature = featureBuffers[0][leftIndex];

          for (const auto& rightFeature : featureBuffers[1])
          {
            if (rightFeature.getYOffset() < leftFeature.getYOffset()) { continue; }
            if (rightFeature.getYOffset() > leftFeature.getYOffset()) { break; }

            float error = this->getError2(leftFeature, rightFeature, errorThreshold);

            if (error < errorThreshold)
            {
              Match match{
                  &leftFeature,
                  &rightFeature,
                  static_cast<float>(leftFeature.getXOffset()) - rightFeature.getXOffset()};

              if (match.getDistance() > 0 && !std::isinf(match.getDistance()))
                workerMatches.push_back(match);
            }
          }
        }

        std::lock_guard lock(outputMutex);
        output.insert(output.end(), workerMatches.begin(), workerMatches.end());
      });
    }

    // Ensure workers finish before output is returned.
    for (auto& task : tasks) { task.join(); }

    return output;
  }

  template <std::size_t selectionSize>
  float getError2(
      const ImageView<selectionSize, selectionSize>& leftSelection,
      const ImageView<selectionSize, selectionSize>& rightSelection,
      std::size_t earlyExitThreshold) const
  {
    float errorSum = 0.0f;

    float intensityLeft = 0.0f;
    float yIntensityLeft = 0.0f;
    float xIntensityLeft = 0.0f;

    float intensityRight = 0.0f;
    float yIntensityRight = 0.0f;
    float xIntensityRight = 0.0f;

    for (std::size_t y = 0; y < selectionSize; ++y)
    {
      for (std::size_t x = 0; x < selectionSize; ++x)
      {
        const auto& leftPixel = leftSelection.getPixel(x, y);
        intensityLeft += leftPixel;
        yIntensityLeft += leftPixel * y;
        xIntensityLeft += leftPixel * x;

        const auto& rightPixel = rightSelection.getPixel(x, y);
        intensityRight += rightPixel;
        yIntensityRight += rightPixel * y;
        xIntensityRight += rightPixel * x;
      }
    }

    float distanceFromCenter = (selectionSize - 1) / 2.0f;

    float centroidXLeft = xIntensityLeft / intensityLeft - distanceFromCenter;  // Opposite
    float centroidYLeft = yIntensityLeft / intensityLeft - distanceFromCenter;  // Adjacent

    float centroidXRight = xIntensityRight / intensityRight - distanceFromCenter;
    float centroidYRight = yIntensityRight / intensityRight - distanceFromCenter;

    float leftLength = std::hypot(centroidXLeft, centroidYLeft);
    float rightLength = std::hypot(centroidXRight, centroidYRight);

    float cosLeft = centroidXLeft / leftLength;
    float cosRight = centroidXRight / rightLength;

    float sinLeft = centroidYLeft / leftLength;
    float sinRight = centroidYRight / rightLength;

    float relativeCos = cosRight * cosLeft + sinRight * sinLeft;
    float relativeSin = sinRight * cosLeft - cosRight * sinLeft;

    int leftAverage = 0;
    int leftMax = 1;
    int rightAverage = 0;
    int rightMax = 1;

    for (std::size_t y = 0; y < selectionSize; ++y)
    {
      for (std::size_t x = 0; x < selectionSize; ++x)
      {
        const auto& leftPixel = leftSelection.getPixel(x, y);
        const auto& rightPixel = rightSelection.getPixel(x, y);

        leftAverage += leftPixel;
        rightAverage += rightPixel;

        if (leftPixel > leftMax) { leftMax = leftPixel; }
        if (rightPixel > rightMax) { rightMax = rightPixel; }
      }
    }

    leftAverage /= selectionSize * selectionSize;
    rightAverage /= selectionSize * selectionSize;

    for (std::size_t y = 0; y < selectionSize; ++y)
    {
      for (std::size_t x = 0; x < selectionSize; ++x)
      {
        const float centeredX = static_cast<float>(x) - distanceFromCenter;
        const float centeredY = static_cast<float>(y) - distanceFromCenter;

        // Rotate left coordinates into right-patch coordinates.
        const float rotatedX =
            distanceFromCenter + centeredX * relativeCos - centeredY * relativeSin;

        const float rotatedY =
            distanceFromCenter + centeredX * relativeSin + centeredY * relativeCos;

        if (rotatedX < 0.0f || rotatedX >= selectionSize - 1 || rotatedY < 0.0f ||
            rotatedY >= selectionSize - 1)
        {
          continue;
        }

        const std::size_t rightX = static_cast<std::size_t>(std::round(rotatedX));
        const std::size_t rightY = static_cast<std::size_t>(std::round(rotatedY));

        const int leftPixel = leftSelection.getPixel(x, y);
        const int rightPixel = rightSelection.getPixel(rightX, rightY);

        // errorSum += std::popcount(
        //     static_cast<uint32_t>(leftPixel - leftAverage) ^ (rightPixel - rightAverage));

        errorSum +=
            (std::abs(
                 (leftPixel - leftAverage) / leftMax - (rightPixel + rightAverage) / rightMax) *
                 1.0f +
             0.0f * std::abs(leftPixel - rightPixel)) *
            ((1.0f - relativeCos) * 0.5f + 1.0f);

        // errorSum += (std::abs(leftPixel - leftAverage - rightPixel + rightAverage) * 0.8f +
        //              0.2f * std::abs(leftPixel - rightPixel)) *
        //             ((1.0f - relativeCos) * 0.5f + 1.0f);

        if (errorSum > earlyExitThreshold) { break; }
      }
    }

    return errorSum;
  }

  template <std::size_t selectionSize>
  float getError(
      const ImageView<selectionSize, selectionSize>& leftSelection,
      const ImageView<selectionSize, selectionSize>& rightSelection,
      std::size_t earlyExitThreshold) const
  {
    std::uint32_t errorSum = 0;

    int leftAverage = 0;
    int rightAverage = 0;

    for (std::size_t y = 0; y < selectionSize; ++y)
    {
      for (std::size_t x = 0; x < selectionSize; ++x)
      {
        leftAverage += leftSelection.getPixel(x, y);
        rightAverage += rightSelection.getPixel(x, y);
      }
    }

    leftAverage /= selectionSize * selectionSize;
    rightAverage /= selectionSize * selectionSize;

    for (std::size_t y = 0; y < selectionSize; ++y)
    {
      for (std::size_t x = 0; x < selectionSize; ++x)
      {
        errorSum += (std::abs(
            (static_cast<int>(leftSelection.getPixel(x, y)) - leftAverage) -
            (static_cast<int>(rightSelection.getPixel(x, y)) - rightAverage)));

        // errorSum += std::popcount(
        //     (leftSelection.getPixel(x, y) - leftAverage) ^
        //     (rightSelection.getPixel(x, y) - rightAverage));

        if (errorSum >= earlyExitThreshold) { return static_cast<float>(errorSum); }
      }
    }

    return static_cast<float>(errorSum);
  }
};