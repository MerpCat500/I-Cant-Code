/**
 * @file video_camera.hpp
 * @brief Linux (V4L2) video camera interface returning grayscale Images.
 */
#pragma once

#include <errno.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "image.hpp"

template <std::size_t width, std::size_t height>
class VideoCamera
{
 private:
  struct Buffer
  {
    void* start = nullptr;
    std::size_t length = 0;
  };

  static constexpr std::size_t bufferCount = 4;

  int fd = -1;
  std::size_t bytesPerLine = 0;  // Reported by the driver (may include padding)
  std::vector<Buffer> buffers;
  bool streaming = false;

  // ioctl wrapper that retries when interrupted by a signal.
  static int xioctl(int fd, unsigned long request, void* arg)
  {
    int result;
    do
    {
      result = v4l2_ioctl(fd, request, arg);
    } while (result == -1 && errno == EINTR);
    return result;
  }

  void cleanup() noexcept
  {
    if (streaming)
    {
      v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      xioctl(fd, VIDIOC_STREAMOFF, &type);
      streaming = false;
    }
    for (auto& buffer : buffers)
    {
      if (buffer.start) { v4l2_munmap(buffer.start, buffer.length); }
    }
    buffers.clear();
    if (fd != -1)
    {
      v4l2_close(fd);
      fd = -1;
    }
  }

  void configureFormat(const std::string& devicePath)
  {
    v4l2_capability cap{};
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1 ||
        !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
        !(cap.capabilities & V4L2_CAP_STREAMING))
    {
      throw std::runtime_error(
          devicePath + " does not support streaming video capture");
    }

    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = static_cast<std::uint32_t>(width);
    fmt.fmt.pix.height = static_cast<std::uint32_t>(height);
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // Y is every other byte
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
      throw std::runtime_error("Failed to set video format on " + devicePath);
    }

    // The driver may adjust the request; read back what we actually got.
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
    {
      throw std::runtime_error(devicePath + " does not support YUYV capture");
    }
    if (fmt.fmt.pix.width != width || fmt.fmt.pix.height != height)
    {
      throw std::runtime_error(
          "Camera size mismatch: requested " + std::to_string(width) + "x" +
          std::to_string(height) + " but device provides " +
          std::to_string(fmt.fmt.pix.width) + "x" +
          std::to_string(fmt.fmt.pix.height));
    }

    bytesPerLine = fmt.fmt.pix.bytesperline;
    if (bytesPerLine < width * 2) { bytesPerLine = width * 2; }
  }

  void setupBuffers(const std::string& devicePath)
  {
    v4l2_requestbuffers req{};
    req.count = bufferCount;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1 || req.count < 2)
    {
      throw std::runtime_error("Failed to request buffers on " + devicePath);
    }

    buffers.resize(req.count);
    for (std::uint32_t i = 0; i < req.count; ++i)
    {
      v4l2_buffer buf{};
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = i;
      if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
      {
        throw std::runtime_error("Failed to query buffer");
      }

      buffers[i].length = buf.length;
      buffers[i].start = v4l2_mmap(
          nullptr,
          buf.length,
          PROT_READ | PROT_WRITE,
          MAP_SHARED,
          fd,
          buf.m.offset);
      if (buffers[i].start == MAP_FAILED)
      {
        buffers[i].start = nullptr;
        throw std::runtime_error("Failed to mmap buffer");
      }

      if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
      {
        throw std::runtime_error("Failed to queue buffer");
      }
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) == -1)
    {
      throw std::runtime_error("Failed to start streaming");
    }
    streaming = true;
  }

 public:
  explicit VideoCamera(const std::string& devicePath)
  {
    fd = v4l2_open(devicePath.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd == -1)
    {
      throw std::runtime_error("Failed to open video device: " + devicePath);
    }

    try
    {
      configureFormat(devicePath);
      setupBuffers(devicePath);
    }
    catch (...)
    {
      cleanup();
      throw;
    }
  }

  ~VideoCamera() { cleanup(); }

  VideoCamera(const VideoCamera&) = delete;
  VideoCamera& operator=(const VideoCamera&) = delete;
  VideoCamera(VideoCamera&&) = delete;
  VideoCamera& operator=(VideoCamera&&) = delete;

  constexpr std::size_t getWidth() const { return width; }
  constexpr std::size_t getHeight() const { return height; }

  /**
   * @brief Capture one frame and return it as a grayscale Image.
   * @param timeoutSeconds Max time to wait for a frame.
   */
  Image<width, height> getFrame(int timeoutSeconds = 2)
  {
    // Device is O_NONBLOCK, so wait until a frame is ready.
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeval timeout{timeoutSeconds, 0};

    int ready;
    do
    {
      ready = select(fd + 1, &fds, nullptr, nullptr, &timeout);
    } while (ready == -1 && errno == EINTR);

    if (ready == 0) { throw std::runtime_error("Timed out waiting for frame"); }
    if (ready == -1) { throw std::runtime_error("select() failed on camera"); }

    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1)
    {
      throw std::runtime_error("Failed to dequeue frame buffer");
    }

    // YUYV: bytes are Y0 U Y1 V ..., so luma is every other byte.
    const auto* src =
        static_cast<const std::uint8_t*>(buffers[buf.index].start);
    std::array<std::uint8_t, width * height> pixels{};
    for (std::size_t y = 0; y < height; ++y)
    {
      const std::uint8_t* row = src + y * bytesPerLine;
      for (std::size_t x = 0; x < width; ++x)
      {
        pixels[y * width + x] = row[x * 2];
      }
    }

    // Hand the buffer back to the driver before returning.
    if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
    {
      throw std::runtime_error("Failed to requeue frame buffer");
    }

    return Image<width, height>(std::move(pixels));
  }

  static std::vector<std::string> getCameras(const std::size_t nCameras)
  {
    std::vector<std::string> devices;

    for (std::size_t i = 0; i < nCameras; ++i)
    {
      std::string devicePath = "/dev/video" + std::to_string(i);

      int tempFd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK, 0);
      if (tempFd == -1) { continue; }

      v4l2_capability cap{};
      if (xioctl(tempFd, VIDIOC_QUERYCAP, &cap) == 0 &&
          (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
      {
        devices.push_back(devicePath);
      }
      close(tempFd);
    }

    return devices;
  }
};