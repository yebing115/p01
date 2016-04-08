const vec4 COLOR = vec4(0.6, 0.4, 0.1, 1.0);

in vec3 normal_varying;

out vec4 color_out;

void main() {
  vec4 color = COLOR;
  if (color.a == 0.0) discard;
  color_out = color * clamp(dot(normal_varying, vec3(-0.5, 0.5, 0.707)), 0.0, 1.0);
  color_out.a = 0.3;
}

