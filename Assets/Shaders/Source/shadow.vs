in vec3 a_position;

uniform mat4 u_model_view_proj;

void main() {
  POSITION = vec4(a_position, 1.0) * u_model_view_proj;
}
