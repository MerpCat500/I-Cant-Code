#include <poll.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>

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
  VideoCamera<1280, 720> camera("/dev/video0");

  unsigned long threshold = 12;

  while (true)
  {
    auto frame = camera.getFrame();
    Image<1280, 720> outputBuffer;

    std::string input;

    if (tryReadLine(input))
    {
      threshold = std::stoul(input);
      std::cout << "Enter a threshold value for FAST feature detection (or "
                   "'exit' to quit): "
                << std::flush;
    }

    if (input == "exit") { return 0; }

    frame.getFeaturesFAST(frame, outputBuffer, threshold);

    frame.save_png("frame.png");
    outputBuffer.save_png("output.png");
  }
  return 0;
}