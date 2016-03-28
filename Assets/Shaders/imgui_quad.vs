in vec2 a_position;
in vec2 a_texcoord0;
in vec4 a_color0;
out vec2 texcoord_varying;
out vec4 color_varying;

uniform vec2 win_size;

void main() {
  POSITION = vec4(((a_position + vec2(0.5, 0.5)) / win_size) * vec2(2.0, -2.0) - vec2(1.0, -1.0), 0.0, 1.0);
  texcoord_varying = a_texcoord0;
  color_varying = a_color0;
}
