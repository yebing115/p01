const vec4 COLOR = vec4(1.0, 1.0, 1.0, 1.0);

in vec3 normal_varying;

out vec4 color_out;

void main() {
  vec4 color = COLOR;
  if (color.a == 0.0) discard;
  vec3 n = normalize(normal_varying);
  color_out = clamp(vec4(0.1, 0.1, 0.1, 0.1) + color * max(0.0, dot(n, vec3(-0.5, 0.5, 0.707))), vec4(0.0), vec4(1.0));
}

