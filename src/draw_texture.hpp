#pragma once

#include <SDL3/SDL.h>

#include <memory>

namespace gameLD58 {
namespace {
inline int GetNearestPow2(int v) {
  if (v <= 1) {
    return 1;
  } else if (v <= 2) {
    return 2;
  } else if (v <= 4) {
    return 4;
  } else if (v <= 8) {
    return 8;
  } else if (v <= 16) {
    return 16;
  } else if (v <= 32) {
    return 32;
  } else if (v <= 64) {
    return 64;
  } else if (v <= 128) {
    return 128;
  } else if (v <= 256) {
    return 256;
  } else if (v <= 512) {
    return 512;
  } else if (v <= 1024) {
    return 1024;
  } else if (v <= 2048) {
    return 2048;
  }

  return 0;
}
}  // namespace

void RenderTexture(std::shared_ptr<SDL_Renderer> renderer,
                   std::shared_ptr<SDL_Texture> texture, SDL_FRect* srcrect,
                   const SDL_FRect* dstrect, const SDL_FColor* color) {
  SDL_Vertex vertices[4];
  int indices[6];

#if defined __PSP__
  float texture_width = (float)GetNearestPow2(texture.get()->w);
  float texture_height = (float)GetNearestPow2(texture.get()->h);
#else
  float texture_width = (float)texture.get()->w;
  float texture_height = (float)texture.get()->h;
#endif

  vertices[0].position.x = dstrect->x;
  vertices[0].position.y = dstrect->y;
  vertices[0].tex_coord.x = srcrect->x / texture_width;
  vertices[0].tex_coord.y = srcrect->y / texture_height;
  vertices[0].color = *color;

  vertices[1].position.x = dstrect->x;
  vertices[1].position.y = dstrect->y + dstrect->h;
  vertices[1].tex_coord.x = srcrect->x / texture_width;
  vertices[1].tex_coord.y = (srcrect->y + srcrect->h) / texture_height;
  vertices[1].color = *color;

  vertices[2].position.x = dstrect->x + dstrect->w;
  vertices[2].position.y = dstrect->y + dstrect->h;
  vertices[2].tex_coord.x = (srcrect->x + srcrect->w) / texture_width;
  vertices[2].tex_coord.y = (srcrect->y + srcrect->h) / texture_height;
  vertices[2].color = *color;

  vertices[3].position.x = dstrect->x + dstrect->w;
  vertices[3].position.y = dstrect->y;
  vertices[3].tex_coord.x = (srcrect->x + srcrect->w) / texture_width;
  vertices[3].tex_coord.y = srcrect->y / texture_height;
  vertices[3].color = *color;

  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 0;
  indices[4] = 2;
  indices[5] = 3;

  SDL_RenderGeometry(renderer.get(), texture.get(), vertices, 4, indices, 6);
}
};  // namespace gameLD58
