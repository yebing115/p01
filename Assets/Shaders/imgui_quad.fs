in vec2 texcoord_varying;
in vec4 color_varying;

out vec4 color_out;

uniform sampler2D tex0;

void main() {
  color_out = color_varying;
  color_out.a *= texture(tex0, texcoord_varying).r;
}
