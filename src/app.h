#ifndef APP_H
#define APP_H

#include "gs.h"

typedef struct
{
  uint32_t width;
  uint32_t height;
  gs_command_buffer_t command_buffer;
  struct
  {
    int z, x, left, right, up, down;
  } input;
} app_t;

extern app_t app;

#endif  /* APP_H */

