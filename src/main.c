#include "fe.h"
#include "gs.h"

#include "app.h"

#include "bin/vs.glsl.c"
#include "bin/fs.glsl.c"

app_t app = {0};

static gs_handle(gs_graphics_shader_t) shader;
static gs_handle(gs_graphics_pipeline_t) pip;
static gs_handle(gs_graphics_vertex_buffer_t) vbo;
static gs_handle(gs_graphics_uniform_t) u_proj = {0};
static gs_handle(gs_graphics_uniform_t) u_tex = {0};
static gs_handle(gs_graphics_texture_t) tex = {0};

static gs_mat4 proj;

#define FE_ARENA_SIZE (1024 * 32)
static char fe_arena[FE_ARENA_SIZE];
static fe_Context *fe_ctx;
static int fe_gc;
static fe_Object *fe_tick[2];

static int tex_hframes = 1;
static int tex_vframes = 1;

typedef struct
{
  struct
  {
    float x;
    float y;
  } position;
  struct
  {
    uint16_t width;
    uint16_t height;
  } scale;
  struct
  {
    uint8_t hframe;
    uint8_t vframe;
    uint8_t max_hframes;
    uint8_t max_vframes;
  } frame_data;
} rquad_t;

gs_dyn_array(rquad_t) quads = NULL;

static inline rquad_t create_quad(gs_vec2 pos, uint16_t width, uint16_t height, uint8_t hframe, uint8_t vframe, uint8_t max_hframes, uint8_t max_vframes)
{
  return (rquad_t) {
    .position.x = pos.x,
    .position.y = pos.y,

    .scale.width = width,
    .scale.height = height,

    .frame_data.hframe = hframe,
    .frame_data.vframe = vframe,
    .frame_data.max_hframes = max_hframes,
    .frame_data.max_vframes = max_vframes,
  };
}

static fe_Object *fe_push_quad(fe_Context *ctx, fe_Object *arg)
{
  float x = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  float y = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  uint16_t w = floorf(fe_tonumber(ctx, fe_nextarg(ctx, &arg)));
  uint16_t h = floorf(fe_tonumber(ctx, fe_nextarg(ctx, &arg)));
  int hf = (int)floorf(fe_tonumber(ctx, fe_nextarg(ctx, &arg)));
  int vf = (int)floorf(fe_tonumber(ctx, fe_nextarg(ctx, &arg)));
  gs_dyn_array_push(quads, create_quad((gs_vec2_t) {x, y}, w, h, hf, vf, tex_hframes, tex_vframes));
  return fe_bool(ctx, 0);
}

static fe_Object *fe_key_down(fe_Context *ctx, fe_Object *arg)
{
  int offset = (int)fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  return fe_bool(ctx, ((int*)(&app.input.z))[offset]);
}

static fe_Object *fe_lerp(fe_Context *ctx, fe_Object *arg)
{
  float a = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  float b = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  float ht = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  float dt = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  float ret = 0;
#define EPSILON (1.5f)
  if (a > b - EPSILON && a < b + EPSILON)
  {
    ret = b;
  }
  else
  {
    ret = b + (a - b) * exp2(-dt / ht);
  }
#undef EPSILON
  return fe_number(ctx, ret);
}

static fe_Object *fe_modulus(fe_Context *ctx, fe_Object *arg)
{
  int a = floorf(fe_tonumber(ctx, fe_nextarg(ctx, &arg)));
  int b = floorf(fe_tonumber(ctx, fe_nextarg(ctx, &arg)));
  return fe_number(ctx, (float)(a % b));
}

static fe_Object *fe_exec_file(fe_Context *ctx, fe_Object *arg)
{
  char path[64];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), path, 64);
  FILE *fp = fopen(path, "r");
  for (;;) {
    fe_Object *obj = fe_readfp(ctx, fp);
    if (!obj) { break; }
    fe_eval(ctx, obj);
    fe_restoregc(ctx, fe_gc);
  }
  fclose(fp);
  return fe_bool(ctx, 0);
}

static fe_Object *fe_load_atlas(fe_Context *ctx, fe_Object *arg)
{
  char path[64];
  fe_tostring(ctx, fe_nextarg(ctx, &arg), path, 64);
  gs_graphics_texture_destroy(tex);

  tex_hframes = fe_tonumber(ctx, fe_nextarg(ctx, &arg));
  tex_vframes = fe_tonumber(ctx, fe_nextarg(ctx, &arg));

  int32_t width, height;
  uint32_t num_comps;
  void *data;
  bool32_t ret = gs_util_load_texture_data_from_file(path, &width, &height, &num_comps, &data, true);

  tex = gs_graphics_texture_create (&(gs_graphics_texture_desc_t) {
    .type = GS_GRAPHICS_TEXTURE_2D,
    .width = width,
    .height = height,
    .data = data,
    .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
    .min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
    .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST
  });
  return fe_bool(ctx, ret);
}

static void init()
{
  uint32_t handle = gs_platform_main_window();
  gs_platform_window_size(handle, &app.width, &app.height);

  proj = gs_mat4_ortho(0, app.width, 0, app.height, -.1f, 1.f);

  app.command_buffer = gs_command_buffer_new();

  #define ROW_COL_CT (8)
  gs_color_t c0 = GS_COLOR_YELLOW;
  gs_color_t c1 = gs_color(20, 50, 150, 255);
  gs_color_t c2 = GS_COLOR_RED;
  gs_color_t c3 = GS_COLOR_MAGENTA;
  gs_color_t pixels[ROW_COL_CT * ROW_COL_CT] = gs_default_val();
  for (uint32_t r = 0; r < ROW_COL_CT; ++r) {
    for (uint32_t c = 0; c < ROW_COL_CT; ++c) {
      const bool re = (r % 2) == 0;
      const bool ce = (c % 2) == 0;
      uint32_t idx = r * ROW_COL_CT + c;
      pixels[idx] = (re && ce) ? c0 : (re) ? c3 : (ce) ? c1 : c2;
    }
  }

  tex = gs_graphics_texture_create (&(gs_graphics_texture_desc_t) {
    .type = GS_GRAPHICS_TEXTURE_2D,
    .width = ROW_COL_CT,
    .height = ROW_COL_CT,
    .data = pixels,
    .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
    .min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
    .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST
  });

  vbo = gs_graphics_vertex_buffer_create(&(gs_graphics_vertex_buffer_desc_t) {
      .data = quads,
      .size = sizeof(quad_t) * 128
  });

  shader = gs_graphics_shader_create(&(gs_graphics_shader_desc_t) {
    .sources = (gs_graphics_shader_source_desc_t[]) {
      { .type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = str_src_glsl_vs_glsl },
      { .type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = str_src_glsl_fs_glsl }
    },
    .size = 2 * sizeof(gs_graphics_shader_source_desc_t)
  });

  gs_assert(sizeof(rquad_t) == sizeof(gs_vec4_t));
  pip = gs_graphics_pipeline_create(&(gs_graphics_pipeline_desc_t) {
    .raster = {
      .shader = shader,
      .primitive = GS_GRAPHICS_PRIMITIVE_TRIANGLES
    },
    .layout = {
      .attrs = (gs_graphics_vertex_attribute_desc_t[]) {
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4, .name = "a_quad", .divisor = 1},
      },
      .size = sizeof(gs_graphics_vertex_attribute_desc_t)
    },
    .blend = {
      .func = GS_GRAPHICS_BLEND_EQUATION_ADD,
      .src = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
      .dst = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA
    }
  });

  u_tex = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t) {
    .stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
    .name = "u_texture",
    .layout = &(gs_graphics_uniform_layout_desc_t) {.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
  });
  u_proj = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t) {
    .stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
    .name = "u_proj",
    .layout = &(gs_graphics_uniform_layout_desc_t) {.type = GS_GRAPHICS_UNIFORM_MAT4}
  });

  // fe_arena = gs_malloc(FE_ARENA_SIZE);
  fe_ctx = fe_open(fe_arena, FE_ARENA_SIZE);
  fe_gc = fe_savegc(fe_ctx);
  fe_set(fe_ctx, fe_symbol(fe_ctx, "rect"), fe_cfunc(fe_ctx, fe_push_quad));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "button"), fe_cfunc(fe_ctx, fe_key_down));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "lerp"), fe_cfunc(fe_ctx, fe_lerp));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "eval_file"), fe_cfunc(fe_ctx, fe_exec_file));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "load_textures"), fe_cfunc(fe_ctx, fe_load_atlas));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "%"), fe_cfunc(fe_ctx, fe_modulus));

  fe_set(fe_ctx, fe_symbol(fe_ctx, "UP"), fe_number(fe_ctx, 4));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "Z"), fe_number(fe_ctx, 0));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "X"), fe_number(fe_ctx, 1));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "LEFT"), fe_number(fe_ctx, 2));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "RIGHT"), fe_number(fe_ctx, 3));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "DOWN"), fe_number(fe_ctx, 5));

  fe_set(fe_ctx, fe_symbol(fe_ctx, "WIDTH"), fe_number(fe_ctx, app.width));
  fe_set(fe_ctx, fe_symbol(fe_ctx, "HEIGHT"), fe_number(fe_ctx, app.height));

  FILE *fp = fopen("main.fe", "r");
  for (;;) {
    fe_Object *obj = fe_readfp(fe_ctx, fp);
    if (!obj) { break; }
    fe_eval(fe_ctx, obj);
    fe_restoregc(fe_ctx, fe_gc);
  }
  fclose(fp);
  fe_tick[0] = fe_symbol(fe_ctx, "tick");
}

static void tick()
{
  app.input.up = gs_platform_key_down(GS_KEYCODE_UP);
  app.input.down = gs_platform_key_down(GS_KEYCODE_DOWN);
  app.input.left = gs_platform_key_down(GS_KEYCODE_LEFT);
  app.input.right = gs_platform_key_down(GS_KEYCODE_RIGHT);
  app.input.z = gs_platform_key_down(GS_KEYCODE_Z);
  app.input.x = gs_platform_key_down(GS_KEYCODE_X);

  gs_dyn_array_clear(quads);

  float dt = gs_platform_delta_time();
  if (dt < 0.1)
  {
    fe_tick[1] = fe_number(fe_ctx, dt);
    fe_eval(fe_ctx, fe_list(fe_ctx, fe_tick, 2));
    fe_restoregc(fe_ctx, fe_gc);
  }
  gs_dyn_array_push(quads, create_quad((gs_vec2_t) {0, 0}, 32, 32, 0, 0, 1, 1));

  /* render */
  gs_graphics_vertex_buffer_update(vbo, &(gs_graphics_vertex_buffer_desc_t) {
    .data = quads,
    .size = sizeof(quad_t) * gs_dyn_array_size(quads)
  });
  gs_graphics_bind_uniform_desc_t uniforms[] = {
    (gs_graphics_bind_uniform_desc_t) { .uniform = u_proj, .data = &proj },
    (gs_graphics_bind_uniform_desc_t) { .uniform = u_tex, .data = &tex, .binding = 0 },
  };

  gs_graphics_bind_desc_t binds = {
    .vertex_buffers = { &(gs_graphics_bind_vertex_buffer_desc_t) { .buffer = vbo } },
    .uniforms = { .desc = uniforms, .size = sizeof(gs_graphics_bind_uniform_desc_t) * 2 }
  };

  gs_graphics_renderpass_begin(&app.command_buffer, GS_GRAPHICS_RENDER_PASS_DEFAULT);
    gs_graphics_clear(&app.command_buffer, &(gs_graphics_clear_desc_t) {
      .actions = &(gs_graphics_clear_action_t) { .color = {0.05f, 0.05f, 0.05f, 1.f} }
    });
    gs_graphics_pipeline_bind(&app.command_buffer, pip);
    gs_graphics_apply_bindings(&app.command_buffer, &binds);
    gs_graphics_draw(&app.command_buffer, &(gs_graphics_draw_desc_t) { .start = 0, .count = 6, .instances = gs_dyn_array_size(quads) });
  gs_graphics_renderpass_end(&app.command_buffer);
  gs_graphics_command_buffer_submit(&app.command_buffer);
}

static void shutdown()
{
  fe_close(fe_ctx);
  // gs_free(fe_arena);

  gs_dyn_array_free(quads);
  gs_command_buffer_free(&app.command_buffer);
  gs_graphics_pipeline_destroy(pip);
  gs_graphics_vertex_buffer_destroy(vbo);
  gs_graphics_uniform_destroy(u_proj);
  gs_graphics_uniform_destroy(u_tex);
}

gs_app_desc_t gs_main(int32_t argc, char **argv)
{
  return (gs_app_desc_t) {
    .window.width = 600,
    .window.height = 600,
    .window.title = "Guncards",
    .window.frame_rate = 120.0f,
    .window.flags = GS_WINDOW_FLAGS_NO_RESIZE,

    .init = init,
    .update = tick,
    .shutdown = shutdown
  };
}

