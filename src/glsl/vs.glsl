#version 440 core

layout (location = 0) in vec4 a_quad;

out vec2 v_uv;

uniform mat4 u_proj;

const vec2 quad_positions[6] = {
  vec2(0, 0),
  vec2(1, 0),
  vec2(1, 1),

  vec2(0, 0),
  vec2(1, 1),
  vec2(0, 1),
};

void main() {
  vec2 position = vec2(a_quad.x, a_quad.y);
  vec2 scale = vec2((floatBitsToUint(a_quad.z) & 0xFFFF0000) >> 16, (floatBitsToUint(a_quad.z) & 0x0000FFFF));
  gl_Position = u_proj * vec4(((scale * quad_positions[gl_VertexID]) + position), 0.f, 1.f);

  uint frame_data = floatBitsToUint(a_quad.w);
  float byte0 = float((frame_data & 0x000000FF));
  float byte1 = float((frame_data & 0x0000FF00) >> 8);
  float byte2 = float((frame_data & 0x00FF0000) >> 16);
  float byte3 = float((frame_data & 0xFF000000) >> 24);

  // Combine bytes into vec2 components
  vec2 frame = vec2(byte0, byte1);
  vec2 max_frames = vec2(byte2, byte3);

  const vec2 quad_uvs[6] = {
    vec2(frame.x * (1.f / max_frames.x), frame.y * (1.f / max_frames.y)),
    vec2((1.f + frame.x) * (1.f / max_frames.x), frame.y * (1.f / max_frames.y)),
    vec2((1.f + frame.x) * (1.f / max_frames.x), (1.f + frame.y) * (1.f / max_frames.y)),

    vec2(frame.x * (1.f / max_frames.x), frame.y * (1.f / max_frames.y)),
    vec2((1.f + frame.x) * (1.f / max_frames.x), (1.f + frame.y) * (1.f / max_frames.y)),
    vec2(frame.x * (1.f / max_frames.x), (1.f + frame.y) * (1.f / max_frames.y)),
  };
  v_uv = quad_uvs[gl_VertexID];
}
