
#include <chrono>
#include <memory>

#include "imagev2.hpp"

int main()
{
  auto image = std::make_unique<Image<4032, 2268>>("image.jpg");

  image->save_jpg("grey-image.jpg");

  while (true)
  {
    auto extremes = std::make_unique<Image<4032, 2268>>();
    std::cout << "Enter a threshold value for FAST feature detection (or "
                 "'exit' to quit): "
              << std::flush;

    std::string input;
    std::getline(std::cin, input);

    if (input == "exit") { return 0; }

    if (input.empty())
    {
      std::cerr << "No input provided. Exiting." << std::endl;
      return 1;
    }

    std::chrono::high_resolution_clock::time_point startTime =
        std::chrono::high_resolution_clock::now();

    image->getFeaturesFAST(*image, *extremes, std::stoi(input));
    extremes->save_png("extremes.png");

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - startTime)
                     .count()
              << " milliseconds" << std::endl;
  }

  return 0;
}