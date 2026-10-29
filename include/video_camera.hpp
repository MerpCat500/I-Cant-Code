/**
 * @file video_camera.hpp
 * @brief Linux (V4L2) video camera interface returning grayscale Images.
 */
#pragma once

#include <fcntl.h>            // For Fileopening flags
#include <libv4l2.h>          // For opening cameras
#include <linux/videodev2.h>  // Main V4L2 API features
#include <sys/mman.h>         // Mandatory Linux Header
#include <sys/select.h>       // Mandatory Linux Header
#include <unistd.h>           // Manditory Linux Header

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "image.hpp"

class VideoCamera
{
 private:
  struct Buffer
  {
    void* start = nullptr;
    std::size_t length = 0;
  };

  // Amount of buffers exposed to my code
  static constexpr std::size_t bufferCount = 4;

  int fd = -1;                   // Linux file descriptor for the camera
  std::size_t bytesPerLine = 0;  // Reported by the driver (may include padding)
  std::vector<Buffer> buffers;   // Buffers stored in a vector

  // Actual dimensions of camera
  std::size_t width = 0;
  std::size_t height = 0;

  // Tracks weather video is streaming
  bool streaming = false;

  /**
   * @brief Repeatably calls V4L2 ioctl until a response is returned
   *
   * @param fd - Linux camera file descriptor
   * @param request - Request to be passed to ioctl
   * @param arg - Arguments to request
   * @return int - Return result of ioctl
   */
  static int xioctl(int fd, unsigned long request, void* arg)
  {
    int result;
    // Repeat until result doesn't fail / isn't interupted
    do { result = v4l2_ioctl(fd, request, arg); } while (result == -1 && errno == EINTR);
    return result;
  }

  /**
   * @brief Releases all camera resources
   *
   */
  void cleanup() noexcept
  {
    // If the camera is streaming
    if (streaming)
    {
      v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

      // Request that streaming camera is turned off.
      xioctl(fd, VIDIOC_STREAMOFF, &type);

      streaming = false;  // Set streaming to false
    }

    // For each buffer in vector of buffers
    for (auto& buffer : buffers)
    {
      // If buffer data isn't null, unmap the buffer (start to end).
      if (buffer.start) { v4l2_munmap(buffer.start, buffer.length); }
    }

    // Clear vector of buffers
    buffers.clear();

    // If camera exists
    if (fd != -1)
    {
      // Close camera
      v4l2_close(fd);

      // Camera no longer exists
      fd = -1;
    }
  }

  /**
   * @brief Check the format of the camera at filepath.
   *
   * @param devicePath - Device filepath
   */
  void configureFormat(const std::string& devicePath)
  {
    v4l2_capability cap{};

    // If the ioctl fails, or the device isn't a video camera / streaming camera, throw an error.
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1 || !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
        !(cap.capabilities & V4L2_CAP_STREAMING))
    {
      throw std::runtime_error(devicePath + " does not support streaming video capture");
    }

    // Set up a check the format of the Frame
    v4l2_frmsizeenum frameSize{};
    frameSize.pixel_format = V4L2_PIX_FMT_YUYV;

    // Query the size of an image
    std::uint32_t largestWidth = 0;
    std::uint32_t largestHeight = 0;

    // For each camera frame size offered by camera
    for (frameSize.index = 0; xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSize) == 0;
         ++frameSize.index)
    {
      // Find the frame size
      std::uint32_t candidateWidth = 0;
      std::uint32_t candidateHeight = 0;

      if (frameSize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
      {
        candidateWidth = frameSize.discrete.width;
        candidateHeight = frameSize.discrete.height;
      }
      else
      {
        candidateWidth = frameSize.stepwise.max_width;
        candidateHeight = frameSize.stepwise.max_height;
      }

      // Check the FPS of frame size
      v4l2_frmivalenum frameInterval{};
      frameInterval.pixel_format = V4L2_PIX_FMT_YUYV;
      frameInterval.width = candidateWidth;
      frameInterval.height = candidateHeight;

      // Decide if frame size is atleast 30fps
      bool supports30Fps = false;
      for (frameInterval.index = 0; xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameInterval) == 0;
           ++frameInterval.index)
      {
        const v4l2_fract fastestInterval = frameInterval.type == V4L2_FRMIVAL_TYPE_DISCRETE
                                               ? frameInterval.discrete
                                               : frameInterval.stepwise.min;
        if (fastestInterval.numerator != 0 && fastestInterval.denominator != 0 &&
            static_cast<std::uint64_t>(fastestInterval.numerator) * 30 <=
                fastestInterval.denominator)
        {
          supports30Fps = true;
          break;
        }
      }

      // If candidate is applicable then store it for later
      if (supports30Fps && static_cast<std::uint64_t>(candidateWidth) * candidateHeight >
                               static_cast<std::uint64_t>(largestWidth) * largestHeight)
      {
        largestWidth = candidateWidth;
        largestHeight = candidateHeight;
      }
    }

    // Throw an error if no candidate is applicable
    if (largestWidth == 0 || largestHeight == 0)
    {
      throw std::runtime_error(devicePath + " does not support a YUYV mode at 30 FPS");
    }

    // Query the format of the capture
    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;       // Query a video capture camera
    fmt.fmt.pix.width = largestWidth;             // Set the size of the camera
    fmt.fmt.pix.height = largestHeight;           // Set the size of the camera
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // Y is every other byte
    fmt.fmt.pix.field =
        V4L2_FIELD_NONE;  // Requests progressive frames rather than interlaced frames

    // Set the video format for the camera, if it false throw an error
    if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
      throw std::runtime_error("Failed to set video format on " + devicePath);
    }

    // If the camera doesn't support luminance mode, throw an error
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
    {
      throw std::runtime_error(devicePath + " does not support YUYV capture");
    }

    // Set the size of the camera
    width = fmt.fmt.pix.width;
    height = fmt.fmt.pix.height;
    bytesPerLine = fmt.fmt.pix.bytesperline;

    // Ensure that bytes per line matches YUYV (2*width bytes per row)
    if (bytesPerLine < width * 2) { bytesPerLine = width * 2; }
  }

  /**
   * @brief Sets up buffers from the camera
   *
   * @param devicePath
   */
  void setupBuffers(const std::string& devicePath)
  {
    // Set up a request buffer query
    v4l2_requestbuffers req{};
    req.count = bufferCount;                 // Ask for bufferCount of buffer space
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  // Ensure that the buffer is a video capture type
    req.memory = V4L2_MEMORY_MMAP;           // Use memory mapping

    // Try to request buffers from video camera
    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1 || req.count < 2)
    {
      throw std::runtime_error("Failed to request buffers on " + devicePath);
    }

    // Resize the vector to store buffers
    buffers.resize(req.count);

    // For each buffer
    for (std::uint32_t i = 0; i < req.count; ++i)
    {
      // Set up query for buffers
      v4l2_buffer buf{};
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = i;

      // Request buffer
      if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
      {
        throw std::runtime_error("Failed to query buffer");
      }

      // Set the length and start of the buffer
      buffers[i].length = buf.length;
      buffers[i].start =
          v4l2_mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

      // If the buffer failed to be mapped, throw an error
      if (buffers[i].start == MAP_FAILED)
      {
        buffers[i].start = nullptr;
        throw std::runtime_error("Failed to mmap buffer");
      }

      // Attempt to queue buffer.
      if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
      {
        throw std::runtime_error("Failed to queue buffer");
      }
    }

    // Attempt to start streaming on the camera
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) == -1)
    {
      throw std::runtime_error("Failed to start streaming");
    }

    // Set streaming to true if query succeeds
    streaming = true;
  }

 public:
  /**
   * @brief Construct a new Video Camera
   *
   * @param devicePath Path to device
   */
  explicit VideoCamera(const std::string& devicePath)
  {
    // Attempt to open the filepath and throw an error if it cannot be opened.
    fd = v4l2_open(devicePath.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd == -1) { throw std::runtime_error("Failed to open video device: " + devicePath); }

    // Attempt to configure formating and setup buffers for device
    try
    {
      configureFormat(devicePath);
      setupBuffers(devicePath);
    }
    // If attempt fails then clean up and reset before throwing an error
    catch (...)
    {
      cleanup();
      throw;
    }
  }

  /**
   * @brief Destroy the Video Camera.
   *
   */
  ~VideoCamera() { cleanup(); }

  // Remove alternate constructors
  VideoCamera(const VideoCamera&) = delete;
  VideoCamera& operator=(const VideoCamera&) = delete;
  VideoCamera(VideoCamera&&) = delete;
  VideoCamera& operator=(VideoCamera&&) = delete;

  // Get the size of the camera
  std::size_t getWidth() const { return width; }
  std::size_t getHeight() const { return height; }

  /**
   * @brief Writes frame data to a image structure
   *
   * @param frame Pointer to frame to write on
   * @param timeoutSeconds Time before dropping the frame
   */
  void getFrame(const std::shared_ptr<Image>& frame, int timeoutSeconds = 2)
  {
    // If the frame buffer doesn't exist or is sized wrong, throw an error.
    if (!frame || frame->getWidth() != width || frame->getHeight() != height)
    {
      throw std::invalid_argument("Frame dimensions do not match camera dimensions");
    }

    // Device is O_NONBLOCK, so wait until a frame is ready.
    // Prepare a set containing the camera file descriptor
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    // Set the timeout
    timeval timeout{timeoutSeconds, 0};

    // wait until the camera can write a frame
    int ready;
    do
    {
      ready = select(fd + 1, &fds, nullptr, nullptr, &timeout);
    } while (ready == -1 && errno == EINTR);

    // If camera timed out, throw an error
    if (ready == 0) { throw std::runtime_error("Timed out waiting for frame"); }

    // If camera failed to give frame, throw an error
    if (ready == -1) { throw std::runtime_error("select() failed on camera"); }

    // Dequeue the frame
    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1)
    {
      throw std::runtime_error("Failed to dequeue frame buffer");
    }

    // YUYV: bytes are Y0 U Y1 V ..., so luma is every other byte.
    // Read the bytes from the camera
    const auto* src = static_cast<const std::uint8_t*>(buffers[buf.index].start);

    // Get reference to pixels in the frame
    auto& pixels = frame->getRawData();

    // Write the camera info into the frame buffer
    for (std::size_t y = 0; y < height; ++y)
    {
      const std::uint8_t* row = src + y * bytesPerLine;
      for (std::size_t x = 0; x < width; ++x) { pixels[y * width + x] = row[x * 2]; }
    }

    // Hand the buffer back to the driver before returning.
    if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
    {
      throw std::runtime_error("Failed to requeue frame buffer");
    }
  }

  /**
   * @brief Returns filepaths for nCamera cameras if they exist
   *
   * @param nCameras
   * @return std::vector<std::string>
   */
  static std::vector<std::string> getCameras(const std::size_t nCameras)
  {
    std::vector<std::string> devices;

    for (std::size_t i = 0; i < nCameras; ++i)
    {
      std::string devicePath = "/dev/video" + std::to_string(i);

      int tempFd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK, 0);
      if (tempFd == -1) { continue; }

      v4l2_capability cap{};
      if (xioctl(tempFd, VIDIOC_QUERYCAP, &cap) == 0 && (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
      {
        devices.push_back(devicePath);
      }
      close(tempFd);
    }

    return devices;
  }
};