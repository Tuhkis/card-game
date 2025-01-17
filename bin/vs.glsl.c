const char *str_src_glsl_vs_glsl =
"#version 440 core\n"
"\n"
"layout (location = 0) in vec4 a_quad;\n"
"\n"
"out vec2 v_uv;\n"
"\n"
"uniform mat4 u_proj;\n"
"\n"
"const vec2 quad_positions[6] = {\n"
"  vec2(0, 0),\n"
"  vec2(1, 0),\n"
"  vec2(1, 1),\n"
"\n"
"  vec2(0, 0),\n"
"  vec2(1, 1),\n"
"  vec2(0, 1),\n"
"};\n"
"\n"
"void main() {\n"
"  vec2 position = vec2(a_quad.x, a_quad.y);\n"
"  vec2 scale = vec2((floatBitsToUint(a_quad.z) & 0xFFFF0000) >> 16, (floatBitsToUint(a_quad.z) & 0x0000FFFF));\n"
"  gl_Position = u_proj * vec4(((scale * quad_positions[gl_VertexID]) + position), 0.f, 1.f);\n"
"\n"
"  uint frame_data = floatBitsToUint(a_quad.w);\n"
"  float byte0 = float((frame_data & 0x000000FF));\n"
"  float byte1 = float((frame_data & 0x0000FF00) >> 8);\n"
"  float byte2 = float((frame_data & 0x00FF0000) >> 16);\n"
"  float byte3 = float((frame_data & 0xFF000000) >> 24);\n"
"\n"
"  // Combine bytes into vec2 components\n"
"  vec2 frame = vec2(byte0, byte1);\n"
"  vec2 max_frames = vec2(byte2, byte3);\n"
"\n"
"  const vec2 quad_uvs[6] = {\n"
"    vec2(frame.x * (1.f / max_frames.x), frame.y * (1.f / max_frames.y)),\n"
"    vec2((1.f + frame.x) * (1.f / max_frames.x), frame.y * (1.f / max_frames.y)),\n"
"    vec2((1.f + frame.x) * (1.f / max_frames.x), (1.f + frame.y) * (1.f / max_frames.y)),\n"
"\n"
"    vec2(frame.x * (1.f / max_frames.x), frame.y * (1.f / max_frames.y)),\n"
"    vec2((1.f + frame.x) * (1.f / max_frames.x), (1.f + frame.y) * (1.f / max_frames.y)),\n"
"    vec2(frame.x * (1.f / max_frames.x), (1.f + frame.y) * (1.f / max_frames.y)),\n"
"  };\n"
"  v_uv = quad_uvs[gl_VertexID];\n"
"}\n";
