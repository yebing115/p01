uniform vec3 diffuse_color;
uniform sampler2D diffuse_tex;
uniform float opacity;
uniform sampler2D opacity_tex;
#if USE_SPECULAR_MAP
uniform sampler2D specular_tex;
uniform float specular_power;
#endif
#if USE_NORMAL_MAP
uniform sampler2D normal_tex;
#endif

in vec3 normal_varying;
#if USE_NORMAL_MAP
in vec3 tangent_varying;
#endif
in vec2 texcoord_varying;

in vec3 light_dir_varying;
in vec3 eye_dir_varying;

out vec4 color_out;

const vec3 AMBIENT_LIGHT = vec3(0.01, 0.01, 0.01);
//const vec3 SUN_COLOR = vec3(0.06299, 0.374256, 1.0); // 64, 156, 255
//const vec3 SUN_COLOR = vec3(1.0, 1.0, 1.0);
const vec3 SUN_COLOR = vec3(1.0, 0.68, 0.41); // 255, 214, 170

void main() {
  vec3 diff = texture(diffuse_tex, texcoord_varying).rgb * diffuse_color;
  float alpha = texture(opacity_tex, texcoord_varying).r * opacity;
  if (alpha < 0.5) discard;
  
  vec3 light_dir = normalize(light_dir_varying);
  vec3 n;
#if USE_NORMAL_MAP
  n = normalize(texture(normal_tex, texcoord_varying).rgb * 2.0 - vec3(1.0));
#else
  n = normalize(normal_varying);
#endif
  vec3 color = AMBIENT_LIGHT + diff * SUN_COLOR * max(0.0, dot(n, light_dir));
#if USE_SPECULAR_MAP
  vec3 eye_dir = normalize(eye_dir_varying);
  vec4 spec = texture(specular_tex, texcoord_varying);
  vec3 r = reflect(-light_dir, n);
  color += spec.rgb * SUN_COLOR * pow(max(0.0, dot(eye_dir, r)), specular_power);
#endif  
  color_out.rgb = clamp(color, vec3(0.0), vec3(1.0)) * alpha;
  color_out.a = alpha;
}

