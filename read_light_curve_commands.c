#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>

#define HEADER_OFFSET 21
#define DATA_OFFSET 11
#define DATA_LINE_LENGTH 66

//clang -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL lib/libraylib.a read_light_curve_commands.c -o read_lc

int main(void)
{
  char fname[] = "light_curve.lcc";

  char *file_contents = LoadFileText(fname);

  int line_count;
  // const char **file_lines = TextSplit(file_contents, (char) 92, &line_count);

  int model_index = TextFindIndex(file_contents, "Model File"); // Find first text occurrence within a string
  int instances_index = TextFindIndex(file_contents, "Instances"); // Find first text occurrence within a string
  int format_index = TextFindIndex(file_contents, "Format "); // Find first text occurrence within a string
  int reference_frame_index = TextFindIndex(file_contents, "Reference Frame"); // Find first text occurrence within a string
  int data_points_index = TextFindIndex(file_contents, "Data Points"); // Find first text occurrence within a string

  int begin_data_index = TextFindIndex(file_contents, "Begin data"); // Find first text occurrence within a string

  char delim[] = "\n";

  char *model_file_name = strtok(file_contents + model_index + HEADER_OFFSET, delim);
  int instances = atoi(strtok(file_contents + instances_index + HEADER_OFFSET, delim));
  char *format = strtok(file_contents + format_index + HEADER_OFFSET, delim);
  char *reference_frame = strtok(file_contents + reference_frame_index + HEADER_OFFSET, delim);
  int data_points = atoi(strtok(file_contents + data_points_index + HEADER_OFFSET, delim));

  printf("%s\n", model_file_name);
  printf("%d\n", instances);
  printf("%s\n", format);
  printf("%s\n", reference_frame);

  for(int line_num = 0; line_num < data_points; line_num++) {
    char *data = strtok(file_contents + begin_data_index + 10 + DATA_LINE_LENGTH * line_num, delim);

    if(line_num > 1) {
      data = strtok(file_contents + begin_data_index + 10 + (DATA_LINE_LENGTH) * line_num - line_num + 1, delim);
    }

    data = TextReplace(data, "   ", ",");
    data = TextReplace(data, "  ", ",");
    data = TextReplace(data, " ", ",");

    printf("%s\n", data);

    double sun_vector_x = atof(strtok(data, ","));
    double sun_vector_y = atof(strtok(NULL, ","));
    double sun_vector_z = atof(strtok(NULL, ","));

    double viewer_vector_x = atof(strtok(NULL, ","));
    double viewer_vector_y = atof(strtok(NULL, ","));
    double viewer_vector_z = atof(strtok(NULL, ","));

    Vector3 sun_vector = {sun_vector_x, sun_vector_y, sun_vector_z};
    Vector3 viewer_vector = {viewer_vector_x, viewer_vector_y, viewer_vector_z};
  }

  return 0;
}