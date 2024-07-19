#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Please give input files\n");
    return 1;
  }
  char *begin = "const char *str_";
  char *path_begin = "bin/";

  for (int i = 1; i < argc; ++i)
  {
    FILE *fp = fopen(argv[i], "r");
    if (!fp)
    {
      printf("Could not open file %s\n", argv[i]);
    }

    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);
    char *src = malloc(sz);
    rewind(fp);
    fread(src, 1, sz, fp);
    fclose(fp);

    int newlines = 0;
    for (int ii = 0; ii < strlen(src); ++ii)
      newlines += src[ii] == '\n';

    sz += newlines * 4;
    sz += strlen(argv[i]) + strlen(begin) + 3;

    char *final = malloc(sz);
    char *tmp = src;

    int offset = strlen(argv[i]) + strlen(begin) + 3;
    for (int ii = 0; ii < strlen(begin); ++ii)
      final[ii] = begin[ii];

    for (int ii = 0; ii < strlen(argv[i]); ++ii)
    {
      if (argv[i][ii] == '.') final[strlen(begin) + ii] = '_';
      else if (argv[i][ii] == '/') final[strlen(begin) + ii] = '_';
      else final[strlen(begin) + ii] = argv[i][ii];
    }

    final[offset - 1] = '\n';
    final[offset - 2] = '=';
    final[offset - 3] = ' ';

    final[offset] = '\"';
    {
      int ii = 0;
      while (*tmp != '\0')
      {
        if (*tmp != '\n')
        {
          final[offset + ii + 1 + (tmp - src)] = *tmp;
        }
        else
        {
          final[offset + ii + 1 + (tmp - src)] = '\\';
          ++ii;
          final[offset + ii + 1 + (tmp - src)] = 'n';
          ++ii;
          final[offset + ii + 1 + (tmp - src)] = '"';
          ++ii;
          final[offset + ii + 1 + (tmp - src)] = '\n';
          ++ii;
          final[offset + ii + 1 + (tmp - src)] = '"';
        }
        ++tmp;
      }
    }

    final[strlen(final) - 2] = ';';
    final[strlen(final) - 1] = '\0';
    // printf("%s", final);

    {
      int slashes_in_path = 0;
      for (int ii = 0; ii < strlen(argv[i]); ++ii)
        newlines += argv[i][ii] == '/';


      char *path = malloc(strlen(path_begin) + strlen(argv[i]) + 3);
      memset(path, 0, strlen(path_begin) + strlen(argv[i]) + 3);
      for (int ii = 0; ii < strlen(path_begin); ++ii)
        path[ii] = path_begin[ii];

      int slashes_encountered = 0;
      int path_offset = 0;
      for (int ii = 0; ii < strlen(argv[i]); ++ii)
      {
        ++path_offset;
        if (argv[i][ii] == '/') ++slashes_encountered;
        if (slashes_encountered > slashes_in_path + 1) break;
      }

      for (int ii = 0; ii < strlen(argv[i]) - path_offset; ++ii)
        path[strlen(path_begin) + ii] = argv[i][ii + path_offset];
      path[strlen(path)] = '.';
      path[strlen(path)] = 'c';

      fp = fopen(path, "w");
      fprintf(fp, "%s\n", final);
      fclose(fp);
      free(path);
    }

    free(src);
    free(final);
  }

  return 0;
}

