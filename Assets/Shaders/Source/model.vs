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

out vec3 light_ref_dir_varying;
out vec3 light_vec_varying;
out vec3 eye_vec_varying;

uniform mat4 u_model;
uniform mat4 u_view_proj;
uniform vec3 u_eye;

uniform int light_type;
uniform vec3 light_pos;
uniform vec3 light_dir;

//const vec3 LIGHT_DIR = vec3(0.1294095, 0.9659258, 0.2241439);
//const vec3 LIGHT_DIR = vec3(0.0, 1.0, 0.0);

void main() {
  vec4 pos = vec4(a_position, 1.0) * u_model;
  vec4 pos_out = pos * u_view_proj;
  POSITION = pos_out;
  normal_varying = a_normal * mat3(u_model);
#if USE_NORMAL_MAP
  tangent_varying = a_tangent.rgb;
  mat3 w2t = transpose(mat3(u_model)) * transpose(mat3(a_tangent.rgb, cross(a_normal, a_tangent.rgb) * a_tangent.a, a_normal));
  if (light_type == 0) light_vec_varying = light_dir * w2t;
  else light_vec_varying = (pos.xyz - light_pos) * w2t;
  eye_vec_varying = (u_eye - pos.xyz) * w2t;
  light_ref_dir_varying = normalize(light_dir * w2t);
#else
  if (light_type == 0) light_vec_varying = light_dir;
  else light_vec_varying = pos.xyz - light_pos;
  eye_vec_varying = u_eye - pos.xyz;
  light_ref_dir_varying = light_dir;
#endif
  texcoord_varying = a_texcoord0;
}
