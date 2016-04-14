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

uniform int light_type;
uniform vec3 light_color;
uniform vec4 light_falloff;

in vec3 normal_varying;
#if USE_NORMAL_MAP
in vec3 tangent_varying;
#endif
in vec2 texcoord_varying;

in vec3 light_ref_dir_varying; 	 // light's center dir (world or tangent space)
in vec3 light_vec_varying;		 // from light's pos to fragment's pos (world or tangent space)
in vec3 eye_vec_varying;		 // from fragment's pos to camera

out vec4 color_out;

//const vec3 AMBIENT_LIGHT = vec3(0.01, 0.01, 0.01);
//const vec3 SUN_COLOR = vec3(0.06299, 0.374256, 1.0); // 64, 156, 255
//const vec3 SUN_COLOR = vec3(1.0, 1.0, 1.0);
//const vec3 SUN_COLOR = vec3(1.0, 0.68, 0.41); // 255, 214, 170

float compute_light(int light_type, vec3 light_ref_dir, vec3 light_vec, vec4 falloff) {
	float intensity = 0.0;
	if (light_type == 0) { // directional light
		intensity = 1.0;
	} if (light_type == 1) { // point light
		float dist = length(light_vec);
		intensity = max(0.0, 1.0 - (dist - falloff.x) / (falloff.y - falloff.x));
		intensity *= intensity;
	} else if (light_type == 2) { // spot light
		float dist = length(light_vec);
		float p1 = max(0.0, 1.0 - (dist - falloff.x) / (falloff.y - falloff.x));
		float p2 = clamp((dot(light_ref_dir, light_vec / dist) - falloff.w) / (falloff.z - falloff.w), 0.0, 1.0);
		intensity = p1 * p1 * p2;
	}
	return intensity;
}

void main() {
  vec3 diff = texture(diffuse_tex, texcoord_varying).rgb * diffuse_color;
  float alpha = texture(opacity_tex, texcoord_varying).r * opacity;
  if (alpha < 0.5) discard;
  
  vec3 light_dir = normalize(light_vec_varying);
  vec3 eye_dir = normalize(eye_vec_varying);
  vec3 n;
#if USE_NORMAL_MAP
  n = normalize(texture(normal_tex, texcoord_varying).rgb * 2.0 - vec3(1.0));
#else
  n = normalize(normal_varying);
#endif
  float intensity = compute_light(light_type, normalize(light_ref_dir_varying), light_vec_varying, light_falloff);
  vec3 lc = light_color * intensity;
  vec3 color = diff * lc * max(0.0, dot(n, -light_dir));
#if USE_SPECULAR_MAP
  vec4 spec = texture(specular_tex, texcoord_varying);
  vec3 r = reflect(light_dir, n);
  color += spec.rgb * lc * pow(max(0.0, dot(eye_dir, r)), specular_power);
#endif  
  color_out.rgb = clamp(color, vec3(0.0), vec3(1.0)) * alpha;
  color_out.a = alpha;
}

