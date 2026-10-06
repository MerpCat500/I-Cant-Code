#include <array>

#include "image.hpp"

constexpr float expApprox(float value)
{
  float result = 1.0f;
  float term = 1.0f;

  for (int i = 1; i <= 24; ++i)
  {
    term *= value / i;
    result += term;
  }

  return result;
}

template <std::size_t Size>
constexpr std::array<std::array<float, Size>, Size> makeGaussianKernel()
{
  static_assert(
      Size > 0 && Size % 2 == 1, "Kernel size must be positive and odd");

  std::array<std::array<float, Size>, Size> kernel{};
  constexpr float sigma = static_cast<float>(Size) / 3.0f;
  constexpr float denominator = 2.0f * sigma * sigma;
  float total = 0.0f;
  const int offset = static_cast<int>(Size / 2);

  for (std::size_t row = 0; row < Size; ++row)
  {
    for (std::size_t column = 0; column < Size; ++column)
    {
      const float x = static_cast<float>(static_cast<int>(column) - offset);
      const float y = static_cast<float>(static_cast<int>(row) - offset);
      const float weight = expApprox(-(x * x + y * y) / denominator);
      kernel[row][column] = weight;
      total += weight;
    }
  }

  for (auto& row : kernel)
  {
    for (float& weight : row) { weight /= total; }
  }

  return kernel;
}

template <std::size_t size>
Image getFeatures(std::array<Image, size> images, float threshold = 0.07f)
{
  std::size_t width = images[0].getWidth();
  std::size_t height = images[0].getHeight();
  Image outputImage(width, height, images[0].getOriginalBPP());

  for (std::size_t i = 0; i < width; ++i)
  {
    for (std::size_t j = 0; j < height; ++j)
    {
      Pixel pixel = images[0].getPixel(i, j);
      for (std::size_t k = 0; k < size; ++k)
      {
        if (images[k].getPixel(i, j) > pixel)
        {
          pixel = images[k].getPixel(i, j);
        }
      }
      outputImage(i, j) = pixel;
    }
  }
  for (std::size_t i = 0; i < width; ++i)
  {
    for (std::size_t j = 0; j < height; ++j)
    {
      Pixel pixel = outputImage.getPixel(i, j);
      if (pixel.r < threshold * 255 && pixel.g < threshold * 255 &&
          pixel.b < threshold * 255)
      {
        outputImage(i, j) = Pixel{0, 0, 0, pixel.a};
      }
    }
  }

  return outputImage;
}

int main()
{
  Image image("image.jpg");

  std::cout << "Image dimensions: " << image.getWidth() << "x"
            << image.getHeight() << " (" << image.getOriginalBPP() << " BPP)"
            << std::endl;
  // image.toGrayScale();

  Image outputImage1(
      image.getWidth(), image.getHeight(), image.getOriginalBPP());
  Image outputImage2(
      image.getWidth(), image.getHeight(), image.getOriginalBPP());
  Image outputImage3(
      image.getWidth(), image.getHeight(), image.getOriginalBPP());
  Image outputImage4(
      image.getWidth(), image.getHeight(), image.getOriginalBPP());

  constexpr auto std1 = makeGaussianKernel<7>();
  constexpr auto std2 = makeGaussianKernel<15>();
  constexpr auto std3 = makeGaussianKernel<31>();
  constexpr auto std4 = makeGaussianKernel<63>();

  image.convolve(std1, outputImage1);
  image.convolve(std2, outputImage2);
  image.convolve(std3, outputImage3);
  image.convolve(std4, outputImage4);

  ((outputImage2 - outputImage1) * 10).save_png("build/output1.png");
  ((outputImage3 - outputImage2) * 10).save_png("build/output2.png");
  ((outputImage4 - outputImage3) * 10).save_png("build/output3.png");

  (getFeatures<3>(
       {(outputImage4 - outputImage3),
        (outputImage3 - outputImage2),
        (outputImage2 - outputImage1)}) *
   10)
      .save_png("build/output4.png");

  return 0;
}