/**
 * @file video_camera.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Class containing the video camera interfacing for linux devices.
 * @version 1.0 (@LastMod : 2026/10/12 13:45:25)
 * @date 2026-10-12
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <string>
#include <vector>

class VideoCamera
{
 private:
 public:
  static std::vector<std::string> getCameras(const std ::size_t nCameras)
  {
    std::vector<std::string> devices;

    for (std ::size_t i = 0; i < nCameras; ++i)
    {
      std::string devicePath = "/dev/video" + std::to_string(i);

      int fd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK, 0);
      if (fd == -1) { continue; }

      struct v4l2_capability cap;
      if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0)
      {
        if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        {
          devices.push_back(devicePath);
        }
      }

      close(fd);
    }

    return devices;
  }

 private:
};