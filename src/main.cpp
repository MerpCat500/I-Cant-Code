#include <iostream>
#include <memory>

#include "imagev2.hpp"

int main()
{
  auto image = std::make_unique<Image<678, 452>>("image.png");

  image->save_jpg("image.jpg");
  return 0;
}