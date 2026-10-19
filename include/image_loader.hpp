/**
 * @file image_loader.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief This class has been separated from image v1 as the libraries can be excluded from final
 * binary if loading / saving an image isn't pertinent.
 * @version 1.0 (@LastMod : 2026/10/19 13:58:52)
 * @date 2026-10-18
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <filesystem>

#include "image.hpp"
#include "stb_image.h"
#include "stb_image_write.h"

// Compile-time hash function (FNV-1a algorithm)
constexpr unsigned int hashString(std::string_view str)
{
  unsigned int hash = 2166136261u;
  for (char c : str)
  {
    hash ^= static_cast<unsigned int>(c);
    hash *= 16777619u;
  }
  return hash;
}

inline void saveImage(const Image& image, std::string filepath, std::uint8_t quality = 100)
{
  std::filesystem::path path(filepath);
  switch (hashString(path.extension().string()))
  {
    case hashString(".png"):
      stbi_write_png(
          path.string().c_str(),
          image.getWidth(),
          image.getHeight(),
          1,  // Grayscale
          &image.getRawData(),
          image.getWidth());
      break;

    case hashString(".jpg"):
      stbi_write_jpg(
          path.string().c_str(),
          image.getWidth(),
          image.getHeight(),
          1,  // Grayscale
          &image.getRawData(),
          quality);
      break;

    case hashString(".bmp"):
      stbi_write_bmp(
          path.string().c_str(),
          image.getWidth(),
          image.getHeight(),
          1,  // Grayscale
          &image.getRawData());
      break;
  }
}

inline std::shared_ptr<Image> loadImage(std::string filepath)
{
  int x, y, channels;
  auto data = stbi_load(filepath.c_str(), &x, &y, &channels, 1);

  return Image::create(x, y, data);
}