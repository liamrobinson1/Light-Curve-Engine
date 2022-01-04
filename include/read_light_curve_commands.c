#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>

#define HEADER_OFFSET 21
#define DATA_OFFSET 11
#define DATA_LINE_LENGTH 66

void ReadLightCurveCommandFile(char *filename, char *model_name, int *instances, int *screen_pixels, Vector3 sun_vectors[], Vector3 viewer_vectors[], int *data_points, char *results_file, bool *use_gpu);

//clang -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL lib/libraylib.a read_light_curve_commands.c -o read_lc

// int main(void)
// {

//   char fname[] = "light_curve.lcc";
//   Vector3 *sun_vectors[1000] = { 0 };
//   Vector3 *viewer_vectors[1000] = { 0 };
//   Model model;
//   int instances;
//   int data_points;

//   ReadLightCurveCommandFile(fname, &model, &instances, sun_vectors, viewer_vectors, &data_points);

//   printf("%d\n", instances);

//   UnloadModel(model);                 // Unload the model

//   return 0;
// }

void ReadLightCurveCommandFile(char *filename, char *model_name, int *instances, int *screen_pixels, Vector3 sun_vectors[], Vector3 viewer_vectors[], int *data_points, char *results_file, bool *use_gpu)
{

  char *file_contents = LoadFileText(filename);

  int line_count;
  // const char **file_lines = TextSplit(file_contents, (char) 92, &line_count);

  int model_index = TextFindIndex(file_contents, "Model File"); // Find first text occurrence within a string
  int instances_index = TextFindIndex(file_contents, "Instances"); // Find first text occurrence within a string
  int dimensions_index = TextFindIndex(file_contents, "Square Dimensions"); // Find first text occurrence within a string
  int format_index = TextFindIndex(file_contents, "Format "); // Find first text occurrence within a string
  int reference_frame_index = TextFindIndex(file_contents, "Reference Frame"); // Find first text occurrence within a string
  int data_points_index = TextFindIndex(file_contents, "Data Points"); // Find first text occurrence within a string
  int computation_method_index = TextFindIndex(file_contents, "Computation Method"); // Find first text occurrence within a string
  int results_file_index = TextFindIndex(file_contents, "Expected .lcr Name"); // Find first text occurrence within a string
  int begin_data_index = TextFindIndex(file_contents, "Begin data"); // Find first text occurrence within a string

  char delim[] = "\n";

  char *model_file_name = TextReplace(strtok(file_contents + model_index + HEADER_OFFSET, delim), " ", "");
  *instances = atoi(strtok(file_contents + instances_index + HEADER_OFFSET, delim));
  *screen_pixels = atoi(strtok(file_contents + dimensions_index + HEADER_OFFSET, delim));
  char *format = TextReplace(strtok(file_contents + format_index + HEADER_OFFSET, delim), " ", "");
  char *reference_frame = TextReplace(strtok(file_contents + reference_frame_index + HEADER_OFFSET, delim), " ", "");
  *data_points = atoi(strtok(file_contents + data_points_index + HEADER_OFFSET, delim));
  char *computation_method = TextReplace(strtok(file_contents + computation_method_index + HEADER_OFFSET, delim), " ", "");
  char *results_file_temp = TextReplace(strtok(file_contents + results_file_index + HEADER_OFFSET, delim), " ", "");

  for(int i = 0; i < strlen(results_file_temp); i++) {
    results_file[i] = results_file_temp[i];
  } 
  
  for(int i = 0; i < strlen(model_file_name); i++) {
    model_name[i] = model_file_name[i];
  }

  if(TextIsEqual(computation_method, "GPU")) *use_gpu = true;
  if(TextIsEqual(computation_method, "CPU")) *use_gpu = false;

  printf("%s\n", model_file_name);
  printf("%d\n", *instances);
  printf("%s\n", format);
  printf("%s\n", reference_frame);
  printf("%s\n", results_file);

  for(int line_num = 0; line_num < *data_points; line_num++) {
    char *data = strtok(file_contents + begin_data_index + 10 + DATA_LINE_LENGTH * line_num, delim);

    if(line_num > 1) {
      data = strtok(file_contents + begin_data_index + 10 + (DATA_LINE_LENGTH) * line_num - line_num + 1, delim);
    }

    data = TextReplace(data, "   ", ",");
    data = TextReplace(data, "  ", ",");
    data = TextReplace(data, " ", ",");

    // printf("%s\n", data);

    double sun_vector_x = atof(strtok(data, ","));
    double sun_vector_y = atof(strtok(NULL, ","));
    double sun_vector_z = atof(strtok(NULL, ","));

    double viewer_vector_x = atof(strtok(NULL, ","));
    double viewer_vector_y = atof(strtok(NULL, ","));
    double viewer_vector_z = atof(strtok(NULL, ","));

    Vector3 sun_vector = {sun_vector_x, sun_vector_y, sun_vector_z};
    Vector3 viewer_vector = {viewer_vector_x, viewer_vector_y, viewer_vector_z};

    sun_vectors[line_num] = sun_vector;
    viewer_vectors[line_num] = viewer_vector;
  }

  // printf("%.2f, %.2f, %.2f\n", sun_vectors[5].x, sun_vectors[5].y, sun_vectors[5].z);  
  return;
}