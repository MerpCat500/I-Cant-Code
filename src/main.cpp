#include <poll.h>
#include <unistd.h>

// #include <chrono>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
  StereoCamera camera("/dev/video4", "/dev/video6");
  Display display;
  auto leftFast = Image::create(camera.getWidth(), camera.getHeight());
  auto rightFast = Image::create(camera.getWidth(), camera.getHeight());

  unsigned long fastThreshold = 80;
  unsigned long errorThreshold = 8'000;
  unsigned long consecutiveMatches = 4;

  std::cout << "I-Cant-Code>" << std::flush;

  while (true)
  {
    std::string input;

    if (tryReadLine(input))
    {
      std::istringstream iss(input);
      std::vector<std::string> tokens;
      std::string token;

      while (std::getline(iss, token, ' ')) { tokens.push_back(token); }

      try
      {
        switch (tokens.size())
        {
          case 3:
            std::cout << "Setting required consecutive matches to " << tokens[2] << ". ";
            consecutiveMatches = std::stoul(tokens[2]);
            [[fallthrough]];
          case 2:
            std::cout << "Setting error threshold to " << tokens[1] << ". ";
            errorThreshold = std::stoul(tokens[1]);
            [[fallthrough]];
          case 1:
            if (tokens[0] == "exit") { return 0; }
            std::cout << "Setting fast threshold to " << tokens[0] << ". ";
            fastThreshold = std::stoul(tokens[0]);
            break;
          case 0:
            break;
          default:
            std::cout << "Extra Parameter Given, Skipped. ";
            break;
        }
      }
      catch (const std::invalid_argument&)
      {
        std::cerr << "Thresholds must be an unsigned integer.\n";
      }
      catch (const std::out_of_range&)
      {
        std::cerr << "Threshold(s) is/are too large.\n";
      }

      std::cout << "\nI-Cant-Code>" << std::flush;
    }

    const auto& matches = camera.getMatches<5>(fastThreshold, consecutiveMatches, errorThreshold);
    if (!display.processEvents()) { return 0; }

    leftFast->clear();
    rightFast->clear();

    for (auto match : camera.getFeatureBuffers()[0])
    {
      (*leftFast)(match.getXOffset() + 32 / 2, match.getYOffset() + 32 / 2) = 255;
    }
    for (auto match : camera.getFeatureBuffers()[1])
    {
      (*rightFast)(match.getXOffset() + 32 / 2, match.getYOffset() + 32 / 2) = 255;
    }

    auto frames = camera.getFrames();

    std::array<std::shared_ptr<Image>, 4> allFrames{frames[0], frames[1], leftFast, rightFast};
    display.show(allFrames, matches, camera.getFeatureBuffers());
  }
  return 0;
}