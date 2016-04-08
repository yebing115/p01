in vec3 a_position;
in vec3 a_normal;

out vec3 normal_varying;

uniform mat4 u_model_view;
uniform mat4 u_model_view_proj;

void main() {
  vec4 pos = vec4(a_position, 1.0);
  vec4 pos_out = u_model_view_proj * pos;
  POSITION = pos_out;
  normal_varying = mat3(u_model_view) * a_normal;
}
