uniform vec4 diffuse_color;
uniform sampler2D diffuse_tex;

in vec3 normal_varying;
in vec2 texcoord_varying;

out vec4 color_out;

const vec3 LIGHT_DIR = vec3(-0.5, 0.5, 0.7071);

void main() {
  vec4 color = texture(diffuse_tex, texcoord_varying);
  if (color.a == 0.0) discard;
  vec3 n = normalize(normal_varying);
  color_out = clamp(vec4(0.1, 0.1, 0.1, 0.1) + color * max(0.0, dot(n, LIGHT_DIR)), vec4(0.0), vec4(1.0));
}

