/**
 * @file display.hpp
 * @author Andrew Hilton - andrew.hilton@usu.edu
 * @brief Displays a image buffer to the screen
 * @version 1.0 (@LastMod : 2026/10/19 14:06:07)
 * @date 2026-10-17
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <SDL2/SDL.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "image.hpp"

class Display
{
 private:
  static constexpr int imageWidth = 640;
  static constexpr int imageHeight = 480;
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  std::vector<SDL_Texture*> textures;
  std::chrono::steady_clock::time_point lastPresent;
  bool hasPresented = false;

  void cleanup() noexcept
  {
    for (SDL_Texture* texture : textures)
    {
      if (texture) { SDL_DestroyTexture(texture); }
    }
    textures.clear();
    if (renderer) { SDL_DestroyRenderer(renderer); }
    if (window) { SDL_DestroyWindow(window); }
    renderer = nullptr;
    window = nullptr;
    SDL_Quit();
  }

  void updateTexture(SDL_Texture* texture, const Image& frame)
  {
    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0)
    {
      throw std::runtime_error(SDL_GetError());
    }

    for (int y = 0; y < imageHeight; ++y)
    {
      auto* row = reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(pixels) + y * pitch);
      for (int x = 0; x < imageWidth; ++x)
      {
        const std::uint8_t value = frame(x, y);
        row[x] = 0xFF000000u | (static_cast<std::uint32_t>(value) << 16) |
                 (static_cast<std::uint32_t>(value) << 8) | value;
      }
    }
    SDL_UnlockTexture(texture);
  }

  void createTextures(std::size_t count)
  {
    while (textures.size() < count)
    {
      SDL_Texture* texture = SDL_CreateTexture(
          renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, imageWidth, imageHeight);
      if (!texture) { throw std::runtime_error(SDL_GetError()); }
      textures.push_back(texture);
    }
  }

  void limitFrameRate()
  {
    constexpr auto frameInterval = std::chrono::microseconds(33334);
    if (hasPresented) { std::this_thread::sleep_until(lastPresent + frameInterval); }
  }

 public:
  explicit Display(const char* title = "Image Display")
  {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { throw std::runtime_error(SDL_GetError()); }

    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        imageWidth,
        imageHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED) : nullptr;
    if (!window || !renderer)
    {
      const std::string error = SDL_GetError();
      cleanup();
      throw std::runtime_error(error);
    }
  }

  ~Display() { cleanup(); }

  Display(const Display&) = delete;
  Display& operator=(const Display&) = delete;

  bool processEvents()
  {
    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT) { return false; }
    }
    return true;
  }

  template <typename FrameContainer>
  void show(const FrameContainer& frames)
  {
    if (frames.empty()) { return; }

    limitFrameRate();

    createTextures(frames.size());

    std::size_t columns = 1;
    while (columns * columns < frames.size()) { ++columns; }
    const std::size_t rows = (frames.size() + columns - 1) / columns;
    const int windowWidth = static_cast<int>(columns * imageWidth);
    const int windowHeight = static_cast<int>(rows * imageHeight);
    SDL_SetWindowSize(window, windowWidth, windowHeight);
    if (SDL_RenderSetLogicalSize(renderer, windowWidth, windowHeight) != 0)
    {
      throw std::runtime_error(SDL_GetError());
    }

    std::size_t index = 0;
    for (const auto& frame : frames)
    {
      updateTexture(textures[index], *frame);
      ++index;
    }

    SDL_RenderClear(renderer);
    for (std::size_t i = 0; i < frames.size(); ++i)
    {
      SDL_Rect destination{
          static_cast<int>(i % columns) * imageWidth,
          static_cast<int>(i / columns) * imageHeight,
          imageWidth,
          imageHeight};
      SDL_RenderCopy(renderer, textures[i], nullptr, &destination);
    }
    SDL_RenderPresent(renderer);
    lastPresent = std::chrono::steady_clock::now();
    hasPresented = true;
  }
};
