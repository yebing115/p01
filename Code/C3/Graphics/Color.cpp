#include "Color.h"

static Color s_red(1.f, 0.f, 0.f);
static Color s_green(0.f, 1.f, 0.f);
static Color s_blue(0.f, 0.f, 1.f);
static Color s_white(1.f, 1.f, 1.f);
static Color s_black(0.f, 0.f, 0.f);
static Color s_no_color(0.f, 0.f, 0.f, 0.f);

const Color& Color::RED = s_red;
const Color& Color::GREEN = s_green;
const Color& Color::BLUE = s_blue;
const Color& Color::WHITE = s_white;
const Color& Color::BLACK = s_black;
const Color& Color::NO_COLOR = s_no_color;
