in vec3 a_position;
in vec3 a_normal;
#if USE_NORMAL_MAP
in vec4 a_tangent;
#endif
in vec2 a_texcoord0;

out vec3 normal_varying;
#if USE_NORMAL_MAP
out vec3 tangent_varying;
#endif
out vec2 texcoord_varying;

out vec3 light_dir_varying;
out vec3 eye_dir_varying;

uniform mat4 u_model;
uniform mat4 u_view_proj;
uniform vec3 u_eye;

const vec3 LIGHT_DIR = vec3(0.1294095, 0.9659258, 0.2241439);
//const vec3 LIGHT_DIR = vec3(0.0, 1.0, 0.0);

void main() {
  vec4 pos = vec4(a_position, 1.0) * u_model;
  vec4 pos_out = pos * u_view_proj;
  POSITION = pos_out;
  normal_varying = a_normal * mat3(u_model);
#if USE_NORMAL_MAP
  tangent_varying = a_tangent.rgb;
  mat3 w2t = transpose(mat3(u_model)) * transpose(mat3(a_tangent.rgb, cross(a_normal, a_tangent.rgb) * a_tangent.a, a_normal));
  light_dir_varying = LIGHT_DIR * w2t;
  eye_dir_varying = (u_eye - pos.xyz) * w2t;
#else
  light_dir_varying = LIGHT_DIR;
  eye_dir_varying = u_eye - pos.xyz;
#endif
  texcoord_varying = a_texcoord0;
}
