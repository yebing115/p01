in vec3 a_position;
in vec3 a_normal;
in vec2 a_texcoord0;

out vec3 normal_varying;
out vec2 texcoord_varying;

uniform mat4 u_model_view;
uniform mat4 u_model_view_proj;

void main() {
  vec4 pos = vec4(a_position, 1.0);
  vec4 pos_out = pos * u_model_view_proj;
  POSITION = pos_out;
  normal_varying = a_normal * mat3(u_model_view);
  texcoord_varying = a_texcoord0;
}
