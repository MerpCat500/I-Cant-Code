#include <memory>

#include "imagev2.hpp"

int main()
{
  auto image = std::make_unique<Image<4032, 1816>>("image.jpg");

  image->save_png("image.png");
  return 0;
}