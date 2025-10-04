#pragma once

#include <SDL3/SDL.h>

#include <cstdlib>
#include <symphony_lite/all_symphony.hpp>

inline float randMinusOneToOne() {
  return 1.0 - (2.0 * ((float)std::rand() / RAND_MAX));
}

inline SDL_FRect AARectToSdlFRect(Symphony::Math::AARect2d inp) {
  SDL_FRect out;
  out.x = inp.center.x - inp.half_size.x;
  out.y = inp.center.y - inp.half_size.y;
  out.w = inp.half_size.x * 2;
  out.h = inp.half_size.y * 2;
  return out;
}
