#pragma once

class Color final {
public:
  Color() {}
  Color(float r, float g, float b, float a = 1.f) {
    _c[0] = r;
    _c[1] = g;
    _c[2] = b;
    _c[3] = a;
  }
  static const Color& RED;
  static const Color& GREEN;
  static const Color& BLUE;
  static const Color& WHITE;
  static const Color& BLACK;
  static const Color& NO_COLOR;
private:
  float _c[4];
};
