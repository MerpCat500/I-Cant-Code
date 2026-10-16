#include <poll.h>
#include <unistd.h>

// #include <chrono>
#include <iostream>
#include <string>

#include "localize.hpp"
#include "video_camera.hpp"

// Returns true if a full line was read, false if nothing is ready.
bool tryReadLine(std::string& line, int timeoutMs = 0)
{
  pollfd pfd{STDIN_FILENO, POLLIN, 0};
  if (poll(&pfd, 1, timeoutMs) > 0 && (pfd.revents & POLLIN))
  {
    return static_cast<bool>(std::getline(std::cin, line));
  }
  return false;
}

int main()
{
  StereoCamera<640, 480> camera("/dev/video4", "/dev/video6");

  unsigned long threshold = 10;

  std::cout << "Enter a threshold value for FAST feature detection (or "
               "'exit' to quit): "
            << std::flush;

  while (true)
  {
    auto [frame, frame2] = camera.getFrames();

    Image<640, 480> outputBuffer;
    Image<640, 480> outputBuffer2;

    std::string input;

    if (tryReadLine(input))
    {
      if (input == "exit") { return 0; }

      try
      {
        threshold = std::stoul(input);
      }
      catch (const std::invalid_argument&)
      {
        std::cerr << "Threshold must be an unsigned integer.\n";
      }
      catch (const std::out_of_range&)
      {
        std::cerr << "Threshold is too large.\n";
      }

      std::cout << "Enter a threshold value for FAST feature detection (or "
                   "'exit' to quit): "
                << std::flush;
    }

    // std ::chrono::steady_clock::time_point startTime =
    //     std::chrono::steady_clock::now();

    Image<640, 480>::getFeaturesFAST(
        frame,  //.convolve(frame, Kernel<float, 3>::boxBlur()),
        outputBuffer,
        threshold,
        12);

    Image<640, 480>::getFeaturesFAST(
        frame2,  //.convolve(frame2, Kernel<float, 3>::boxBlur()),
        outputBuffer2,
        threshold,
        12);

    // std ::chrono::steady_clock::time_point endTime =
    //     std::chrono::steady_clock::now();

    // std ::cout << "FAST feature detection took "
    //            << std::chrono::duration_cast<std::chrono::milliseconds>(
    //                   endTime - startTime)
    //                   .count()
    //            << " ms" << std::endl;

    frame.save_png("frame.png");
    frame2.save_png("frame2.png");
    outputBuffer.save_png("output.png");
    outputBuffer2.save_png("output2.png");
  }
  return 0;
}