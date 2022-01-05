#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>

#define HEADER_OFFSET 21
#define DATA_OFFSET 11
#define DATA_LINE_LENGTH 66
#define GLSL_VERSION            330

#define MAX_INSTANCES          25

void ReadLightCurveCommandFile(char *filename, char *model_name, int *instances, int *screen_pixels, 
     Vector3 sun_vectors[], Vector3 viewer_vectors[], int *data_points, char *results_file, bool *use_gpu, int *frame_rate, Vector3 model_augmentations[]);
Image LoadImageFromScreenFixed(void);
void printMatrix(Matrix m);
Matrix CalculateMVPFromCamera(Camera light_camera, Vector3 offset);
Matrix CalculateMVPBFromMVP(Matrix mvp_light);
void SaveScreen(char fname[]);
float CalculateCameraArea(Camera cam);
float CalculateMeshScaleFactor(Mesh mesh, Camera cam, int instances); //Finds the factor required to scale all vertices down to fit the model in a unit cube
Mesh ApplyMeshScaleFactor(Mesh mesh, float sf);
Vector3 TransformOffsetToCameraPlane(Camera cam, Vector3 offset);
void GenerateTranslations(Vector3 *mesh_offsets, Camera cam, int instances);
void CalculateRightAndTop(Camera cam, float *right, float *top);
void InitializeViewerCamera(Camera *cam);
void GetLCShaderLocations(Shader *depthShader, Shader *lighting_shader, Shader *brightness_shader, Shader *light_curve_shader, Shader *min_shader, int depth_light_mvp_locs[], int lighting_light_mvp_locs[], int instances);
void CalculateLightCurveValues(float lightCurveFunction[], RenderTexture2D minifiedLightCurveTex, RenderTexture2D brightnessTex, float clipping_area, int instances, float scale_factor, bool gpu_light_curve_compute);
void printVector3(Vector3 vec, const char name[]);
void WriteLightCurveResults(char results_file[], float light_curve_results[], int data_points);

void ReadLightCurveCommandFile(char *filename, char *model_name, int *instances, int *screen_pixels, 
      Vector3 sun_vectors[], Vector3 viewer_vectors[], int *data_points, char *results_file, bool *use_gpu, int *frame_rate, Vector3 model_augmentations[])
{

  char *file_contents = LoadFileText(filename);
  int line_count;

  //Header parsing
  int model_index = TextFindIndex(file_contents, "Model File"); // Find first text occurrence within a string
  int instances_index = TextFindIndex(file_contents, "Instances"); // Find first text occurrence within a string
  int dimensions_index = TextFindIndex(file_contents, "Square Dimensions"); // Find first text occurrence within a string
  int format_index = TextFindIndex(file_contents, "Format "); // Find first text occurrence within a string
  int reference_frame_index = TextFindIndex(file_contents, "Reference Frame"); // Find first text occurrence within a string
  int data_points_index = TextFindIndex(file_contents, "Data Points"); // Find first text occurrence within a string
  int computation_method_index = TextFindIndex(file_contents, "Computation Method"); // Find first text occurrence within a string
  int results_file_index = TextFindIndex(file_contents, "Expected .lcr Name"); // Find first text occurrence within a string
  int frame_rate_index = TextFindIndex(file_contents, "Target Framerate"); // Find first text occurrence within a string
  
  int begin_model_aug_index = TextFindIndex(file_contents, "Begin model augmentation"); // Find first text occurrence within a string
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
  *frame_rate = atoi(strtok(file_contents + frame_rate_index + HEADER_OFFSET, delim));

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

  //Model augmentation parsing
  char *model_aug_data = strtok(file_contents + begin_model_aug_index + 24, delim);
  model_aug_data = TextReplace(model_aug_data, "   ", ",");
  model_aug_data = TextReplace(model_aug_data, "  ", ",");
  model_aug_data = TextReplace(model_aug_data, " ", ",");

  int vertex_index = atoi(strtok(model_aug_data, ","));
  double dx = atof(strtok(NULL, ","));
  double dy = atof(strtok(NULL, ","));
  double dz = atof(strtok(NULL, ","));

  model_augmentations[vertex_index] = (Vector3) {dx, dy, dz};

  //Sun/viewer data parsing
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

  return;
}

// Load image from screen buffer and (screenshot)
Image LoadImageFromScreenFixed(void)
{
    Image image = { 0 };

    image.width = GetScreenWidth() * 2; //Probably an artifact of the retina display... to do: coeck on other devices
    image.height = GetScreenHeight() * 2;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    image.data = rlReadScreenPixels(image.width, image.height);

    return image;
}

void printMatrix(Matrix m) 
{
    printf("\n[ %f %f %f %f\n %f %f %f %f\n %f %f %f %f\n %f %f %f %f ]\n", 
    m.m0, m.m4, m.m8, m.m12, 
    m.m1, m.m5, m.m9, m.m13,  
    m.m2, m.m6, m.m10, m.m14,  
    m.m3, m.m7, m.m11, m.m15);
}

Matrix CalculateMVPFromCamera(Camera cam, Vector3 offset) //Calculates the MVP matrix for a camera
{
        Matrix matView = MatrixLookAt(cam.position, cam.target, cam.up); //Calculate camera view matrix

        float top;
        float right;
        CalculateRightAndTop(cam, &right, &top);
        Matrix matProj = MatrixOrtho(-right, right, -top, top, 0.01, 1000.0); // Calculate camera projection matrix

        return MatrixMultiply(MatrixMultiply(matView, MatrixTranslate(offset.x, offset.y, offset.z)), matProj); //Computes the light MVP matrix
}

Matrix CalculateMVPBFromMVP(Matrix MVP) //Calculates the biased MVP matrix for texture sampling
{
    Matrix bias_matrix = {0.5, 0.0, 0.0, 0.0,
                          0.0, 0.5, 0.0, 0.0,
                          0.0, 0.0, 0.5, 0.0,
                          0.5, 0.5, 0.5, 1.0}; //Row-major form of the bias matrix (takes homogeneous coords [-1, 1] -> texture coords [0, 1])

    return MatrixMultiply(MVP, MatrixTranspose(bias_matrix)); //Adds the bias for texture sampling
}

float CalculateCameraArea(Camera cam)
{
  float top;
  float right;
  CalculateRightAndTop(cam, &right, &top);
  return 4.0*top*right;
}

void CalculateRightAndTop(Camera cam, float *right, float *top)
{
  float aspect = 1; //Screen aspect ratio
  *top = (float) cam.fovy/2.0; //Half-height of the clipping plane
  *right = *top*aspect; //Half-width of the clipping plane
}

void SaveScreen(char fname[]) //Saves the current screen image to a file
{
    Image screen_image = LoadImageFromScreenFixed();

    // int success = ExportImage(screen_image, fname);
    printf("Saved image!\n");
}

float CalculateMeshScaleFactor(Mesh mesh, Camera cam, int instances) //Finds the factor required to scale all vertices down to fit the model in a unit cube
{
  float largest_disp = 0.0;
  for(int i = 0; i < mesh.vertexCount; i++) {
    Vector3 vertex = {mesh.vertices[i*3+0], mesh.vertices[i*3+1], mesh.vertices[i*3+2]};
    float vertex_disp = Vector3Length(vertex);

    if(vertex_disp > largest_disp) largest_disp = vertex_disp;
  }
  int grid_width = (int) ceil(sqrt(instances));  
  float top;
  float right;
  CalculateRightAndTop(cam, &right, &top);

  return (largest_disp / top) * grid_width;
}

Mesh ApplyMeshScaleFactor(Mesh mesh, float sf)
{  
  for(int i = 0; i < mesh.vertexCount; i++) {
    mesh.vertices[i*3] = mesh.vertices[i*3]/sf;
    mesh.vertices[i*3+1] = mesh.vertices[i*3+1]/sf;
    mesh.vertices[i*3+2] = mesh.vertices[i*3+2]/sf;
  }
  return mesh;
}

Vector3 TransformOffsetToCameraPlane(Camera cam, Vector3 offset)
{
  Vector3 basis1 = cam.up;
  Vector3 normal = Vector3Scale(cam.position, 1.0 / Vector3Length(cam.position));
  Vector3 basis2 = Vector3CrossProduct(basis1, normal);
  Matrix camera_basis = MatrixTranspose((Matrix) {basis2.x, basis2.y, basis2.z, 0.0,
                  basis1.x, basis1.y, basis1.z, 0.0,
                  normal.x, normal.y, normal.z, 0.0,
                  0.0, 0.0, 0.0, 1.0});

  Vector3 transformed_offset = Vector3Transform(offset, camera_basis);
  return transformed_offset;
}

void GenerateTranslations(Vector3 *mesh_offsets, Camera cam, int instances) 
{
  float top;
  float right;
  CalculateRightAndTop(cam, &right, &top);

  int square_below = (int) ceil(sqrt(instances));

  int index = 0;
  for(int i = 0; i < square_below; i++) {
    for(int j = 0; j < square_below; j++) {
      Vector3 absolute_offset = {-right * (square_below - 1.0) + i * (square_below + (4.0 - square_below)), -top * (square_below - 1.0) + j * (square_below + (4.0 - square_below)), 0.0};

      mesh_offsets[index] = Vector3Scale(absolute_offset, 1.0 / (float) square_below);
      index++;
      if(index == instances) break;
    }
  }
}

void InitializeViewerCamera(Camera *cam)
{
    cam->position = (Vector3){ 2.0f, 0.0f, -6.0f };   // Camera position
    cam->target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    cam->up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    cam->fovy = 4.0f;                                // Camera field-of-view Y
    cam->projection = CAMERA_ORTHOGRAPHIC;             // Camera mode type
}

void GetLCShaderLocations(Shader *depthShader, Shader *lighting_shader, Shader *brightness_shader, Shader *light_curve_shader, Shader *min_shader, int depth_light_mvp_locs[], int lighting_light_mvp_locs[], int instances) {
    depthShader->locs[0] = GetShaderLocation(*depthShader, "viewPos");           //Location of the viewer position uniform for the depth shader
    depthShader->locs[1] = GetShaderLocation(*depthShader, "light_mvp");         //Location of the light MVP matrix uniform for the depth shader
    depthShader->locs[2] = GetShaderLocation(*depthShader, "lightPos");          //Location of the light position uniform for the depth shader
    depthShader->locs[3] = GetShaderLocation(*depthShader, "model_id");          //Location of the model id uniform for the depth shader

    for (int i = 0; i < instances; i++)
    {
      const char *matName = TextFormat("light_mvps[%d].mat\0", i);
      depth_light_mvp_locs[i] = GetShaderLocation(*depthShader, matName);
    }

    lighting_shader->locs[0] = GetShaderLocation(*lighting_shader, "viewPos");   //Location of the viewer position uniform for the lighting shader
    lighting_shader->locs[1] = GetShaderLocation(*lighting_shader, "lightPos");  //Location of the light position uniform for the lighting shader
    lighting_shader->locs[2] = GetShaderLocation(*lighting_shader, "depthTex");  //Location of the depth texture uniform for the lighting shader
    lighting_shader->locs[3] = GetShaderLocation(*lighting_shader, "mvp_from_script"); //Location of the light MVP matrix uniform for the lighting shader
    lighting_shader->locs[4] = GetShaderLocation(*lighting_shader, "model_id");  //Location of the light MVP matrix uniform for the lighting shader
    lighting_shader->locs[5] = GetShaderLocation(*lighting_shader, "light_mvp");

    min_shader->locs[0] = GetShaderLocation(*min_shader, "grid_width");

    // for (int i = 0; i < instances; i++)
    // {
    //   const char *matName = TextFormat("light_mvps[%d].mat\0", i);
    //   lighting_light_mvp_locs[i] = GetShaderLocation(*lighting_shader, matName);
    // }
}

void CalculateLightCurveValues(float lightCurveFunction[], RenderTexture2D minifiedLightCurveTex, RenderTexture2D brightnessTex, float clipping_area, int instances, float scale_factor, bool use_gpu) {
    int gridWidth = (int) ceil(sqrt(instances));

    Image light_curve_image = LoadImageFromTexture(minifiedLightCurveTex.texture);
    Image full_light_curve_image = LoadImageFromTexture(minifiedLightCurveTex.texture);
    int total_pixels = brightnessTex.texture.width * brightnessTex.texture.height;

    float instance_total_irrad_est[MAX_INSTANCES];
    float instance_total_area_est[MAX_INSTANCES];

    float instance_total_irrad_true[MAX_INSTANCES];

    int grid_pixel_height = light_curve_image.height / gridWidth;
    
    if(!use_gpu) {
      full_light_curve_image = LoadImageFromTexture(brightnessTex.texture); 
      //CALCULATING TRUE LC VALUES
      int instance = 0;
      for(int col_instance = 0; col_instance < gridWidth; col_instance++) {
        for(int row_instance = 0; row_instance < gridWidth; row_instance++) {

          float lit_rows = 0.0;
          float sum_total_irrad_true = 0.0;

          for(int row_instance_pixel = row_instance * grid_pixel_height; row_instance_pixel < (row_instance + 1) * grid_pixel_height; row_instance_pixel++) {
            for(int col_instance_pixel = col_instance * grid_pixel_height; col_instance_pixel < (col_instance + 1) * grid_pixel_height; col_instance_pixel++) {
              
              Color pix_color = GetImageColor(full_light_curve_image, col_instance_pixel, row_instance_pixel);
              
              if((float) pix_color.g > 0.0) { //for all lit rows
                lit_rows += 1.0;
                sum_total_irrad_true += (float) pix_color.r / 255.0; //Represents the average irrad of each row * the fraction of lit pixels on that row
              }
            }
          }
          instance_total_irrad_true[instance++] = sum_total_irrad_true;
        }
      }
    }
    
    //CALCULATING SHADED LC VALUES
    for(int col = 0; col < light_curve_image.width; col++) {
      for(int row_instance = 0; row_instance < gridWidth; row_instance++) {
        float lit_pixels = 0.0;
        float lighting_factor = 0.0;
        float light_curve_function = 0.0;
        for(int row_instance_pixel = row_instance * grid_pixel_height; row_instance_pixel < (row_instance + 1) * grid_pixel_height; row_instance_pixel++) {
          Color pix_color = GetImageColor(light_curve_image, col, row_instance_pixel);
          
          if((float) pix_color.g > 0.0) { //for all lit rows
            lit_pixels += (float) pix_color.g / 255.0 * grid_pixel_height; //number of lit pixels in row
            lighting_factor += (float) pix_color.r / 255.0 * grid_pixel_height; //Represents the average irrad of each row * the fraction of lit pixels on that row
          }
        }
        float fraction_of_pixels_lit = 1 / pow((double) grid_pixel_height, 2.0);
        float instance_clipping_area = 1.0 / (float) (gridWidth * gridWidth) * clipping_area;
        float apparent_model_lit_area_scaled = instance_clipping_area * fraction_of_pixels_lit;
        float apparent_model_lit_area_unscaled = apparent_model_lit_area_scaled * scale_factor * scale_factor;//removing the mesh scale factor

        instance_total_irrad_est[row_instance + gridWidth * col] = lighting_factor;
        // instance_total_area_est[row_instance + gridWidth * col] = average_irrad * model_lit_area_unscaled / PI; // I think this is right
        
        instance_total_area_est[row_instance + gridWidth * col] = lighting_factor * apparent_model_lit_area_unscaled / PI;
      }
    }
  
    UnloadImage(light_curve_image);
    UnloadImage(full_light_curve_image);

    if(!use_gpu) {
      for(int i = 0; i < instances; i++) {
        lightCurveFunction[i] = instance_total_irrad_true[i] / total_pixels * clipping_area * gridWidth * gridWidth; //running_average.x = (for all lit pixels, average irrad)
      }
    }

    if(use_gpu) {
      for(int i = 0; i < instances; i++) {
        float screen_irrad_percent_filled = instance_total_irrad_est[i]; // amount of irrad seen / total irrad possible
        lightCurveFunction[i] = screen_irrad_percent_filled / PI;
        lightCurveFunction[i] = instance_total_area_est[i];
      }
    }
}

void printVector3(Vector3 vec, const char name[])
{
  printf("%s: %.4f, %.4f, %.4f\n", name, vec.x, vec.y, vec.z);
}

void WriteLightCurveResults(char results_file[], float light_curve_results[], int data_points)
{
  FILE *fptr;
  fptr = fopen(results_file, "w");

  for(int i = 0; i < data_points; i++) {    
    fprintf(fptr, "%f\n", light_curve_results[i]);
  }

  fclose(fptr);
}