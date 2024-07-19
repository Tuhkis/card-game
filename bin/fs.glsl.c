const char *str_src_glsl_fs_glsl =
"#version 440 core\n"
"\n"
"in vec2 v_uv;\n"
"\n"
"layout (location = 0) out vec4 f_color;\n"
"\n"
"uniform sampler2D u_texture;\n"
"\n"
"void main() {\n"
"  f_color = texture(u_texture, v_uv);\n"
"}\n";
