#pragma once

#include <math.h>

#include <format>
#include <iostream>

namespace Symphony {
namespace Math {
class Vector2d {
 public:
  Vector2d() : x(0.0f), y(0.0f) {}

  Vector2d(float new_x, float new_y) : x(new_x), y(new_y) {}

  static Vector2d Zero() { return Vector2d(0.0f, 0.0f); }

  static Vector2d X() { return Vector2d(1.0f, 0.0f); }

  static Vector2d Y() { return Vector2d(0.0f, 1.0f); }

  Vector2d operator-() const { return Vector2d(-x, -y); }

  Vector2d operator+(const Vector2d& rhv) const {
    return Vector2d(x + rhv.x, y + rhv.y);
  }

  Vector2d operator-(const Vector2d& rhv) const {
    return Vector2d(x - rhv.x, y - rhv.y);
  }

  Vector2d operator*(float v) const { return Vector2d(x * v, y * v); }

  float operator*(const Vector2d& v) const { return x * v.x + y * v.y; }

  float GetLengthSq() const { return x * x + y * y; }

  float GetLength() const { return sqrtf(GetLengthSq()); }

  void MakeNormalized(float eps) {
    float l = GetLength();
    if (l < eps) {
      x = 0.0f;
      y = 0.0f;
    } else {
      x = x / l;
      y = y / l;
    }
  }

  void MakeNormalized() {
    float l = GetLength();
    x = x / l;
    y = y / l;
  }

  Vector2d GetNormalized() const {
    Vector2d v(x, y);
    v.MakeNormalized();
    return v;
  }

  void Rotate(float angle_rad) { Rotate(cosf(angle_rad), sinf(angle_rad)); }

  Vector2d GetRotated(float angle_rad) const {
    Vector2d v(x, y);
    v.Rotate(angle_rad);
    return v;
  }

  void Rotate(float cos_val, float sin_val) {
    float new_x = cos_val * x - sin_val * y;
    float new_y = sin_val * x + cos_val * y;
    x = new_x;
    y = new_y;
  }

  Vector2d GetRotated(float cos_val, float sin_val) const {
    Vector2d v(x, y);
    v.Rotate(cos_val, sin_val);
    return v;
  }

  friend std::ostream& operator<<(std::ostream& os, const Vector2d& p) {
    os << "Vector2d(x: " << p.x << ", y: " << p.y << ")";
    return os;
  }

  float x;
  float y;
};

}  // namespace Math
}  // namespace Symphony

namespace std {
template <>
struct formatter<::Symphony::Math::Vector2d> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(const ::Symphony::Math::Vector2d& v, format_context& ctx) const {
    return format_to(ctx.out(), "Vector2d(x:{}, y:{})", v.x, v.y);
  }
};
}  // namespace std
