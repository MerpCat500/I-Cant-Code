#include <poll.h>
#include <unistd.h>

// #include <chrono>
#include <iostream>
#include <string>

#include "display.hpp"
#include "localize.hpp"

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
  Display display;

  unsigned long threshold = 10;

  std::cout << "Enter a threshold value for FAST feature detection (or "
               "'exit' to quit): "
            << std::flush;

  while (true)
  {
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

    const auto& errorFrame = camera.template getErrorFrame<5>(threshold, 10);
    if (!display.processEvents()) { return 0; }

    display.show(errorFrame);
  }
  return 0;
}