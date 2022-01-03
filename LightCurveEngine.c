// clang -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL lib/libraylib.a LightCurveEngine.c -o LightCurveEngine
/*******************************************************************************************
*
*   raylib [shaders] example - basic lighting
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version.
*
*   NOTE: Shaders used in this example are #version 330 (OpenGL 3.3).
*
*   This example has been created using raylib 3.8 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Example contributed by Chris Camacho (@codifies, http://bedroomcoders.co.uk/) and 
*   reviewed by Ramon Santamaria (@raysan5)
*
*   Copyright (c) 2019-2021 Chris Camacho (@codifies) and Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"           // OpenGL abstraction layer to OpenGL 1.1, 2.1, 3.3+ or ES2
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//User-defined
#include "include/read_light_curve_commands.c"

#define RLIGHTS_IMPLEMENTATION
#include "include/rlights.h"

#define GLSL_VERSION            330
#define MAX_INSTANCES          25 //no work: 2 5 6 10 11 (last 2 left) 12 17 (last 3 left) 18 (last 5 left) 19 (last 7 left) 20 (last 5 left)
                                  //not rendered count: 2 5 6 10 9 12 14 13 12 20

Image LoadImageFromScreenFixed(void);
void printMatrix(Matrix m);
Matrix CalculateMVPFromCamera(Camera light_camera, int screenWidth, int screenHeight, Vector3 offset);
Matrix CalculateMVPBFromMVP(Matrix mvp_light);
void SaveScreen(char fname[]);
float CalculateCameraArea(Camera cam, int screenWidth, int screenHeight);
float CalculateMeshScaleFactor(Mesh mesh, Camera cam, int screenWidth, int screenHeight, int instances); //Finds the factor required to scale all vertices down to fit the model in a unit cube
Mesh ApplyMeshScaleFactor(Mesh mesh, float sf);
Vector3 TransformOffsetToCameraPlane(Camera cam, Vector3 offset);
void GenerateTranslations(Vector3 *mesh_offsets, Camera cam, int screenWidth, int screenHeight, int instances);
void CalculateRightAndTop(Camera cam, int screenWidth, int screenHeight, float *right, float *top);
void InitializeViewerCamera(Camera *cam);
void GetLCShaderLocations(Shader *depthShader, Shader *lighting_shader, Shader *brightness_shader, Shader *light_curve_shader, Shader *min_shader, int depth_light_mvp_locs[], int lighting_light_mvp_locs[], int instances);
void CalculateLightCurveValues(float lightCurveFunctionTrue[], float lightCurveFunctionEst[], RenderTexture2D minifiedLightCurveTex, RenderTexture2D brightnessTex, float clipping_area, int instances);
void printVector3(Vector3 vec, char name[]);

int main(void)
{
    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 800;

    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    InitWindow(screenWidth, screenHeight, "Light Curve Engine"); // A cool name for a cool app

    char filename[] = "light_curve.lcc"; 
    Model model; 
    int instances; 
    Vector3 sun_vectors[1000];
    Vector3 viewer_vectors[1000]; 
    int data_points;

    ReadLightCurveCommandFile(filename, &model, &instances, sun_vectors, viewer_vectors, &data_points);
    int gridWidth = (int) ceil(sqrt(instances));

    Camera viewer_camera;                            // Define the viewer camera
    InitializeViewerCamera(&viewer_camera);

    // Load plane model from a generated mesh
    Mesh mesh = model.meshes[0];

    float mesh_scale_factor = CalculateMeshScaleFactor(mesh, viewer_camera, screenWidth, screenHeight, instances);
    mesh = ApplyMeshScaleFactor(mesh, mesh_scale_factor);

    // Loading depth shader
    Shader depthShader = LoadShader("shaders/depth_texture.vs", "shaders/create_depth_texture.fs");
    Shader lighting_shader = LoadShader("shaders/base_shadowing.vs", "shaders/lighting.fs");
    Shader brightness_shader = LoadShader("shaders/brightness.vs", "shaders/brightness.fs");
    Shader light_curve_shader = LoadShader("shaders/light_curve_extraction.vs", "shaders/light_curve_extraction.fs");
    Shader min_shader = LoadShader("shaders/minimize.vs", "shaders/minimize.fs");

    int depth_light_mvp_locs[MAX_INSTANCES];
    int lighting_light_mvp_locs[MAX_INSTANCES];
    GetLCShaderLocations(&depthShader, &lighting_shader, &brightness_shader, &light_curve_shader, &min_shader, depth_light_mvp_locs, lighting_light_mvp_locs, instances);

    Light sun = CreateLight(LIGHT_POINT, (Vector3) { 2.0f, 2.0f, 2.0f }, Vector3Zero(), WHITE, lighting_shader);

    Camera light_camera = { 0 };                        // The camera that views the scene from the light's perspective
    light_camera.position = sun.position;         // Camera position
    light_camera.target = sun.target;             // Camera looking at point
    light_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };    // Camera up vector (rotation towards target)
    light_camera.fovy = 4.0f;                           // Camera field-of-view Y
    light_camera.projection = CAMERA_ORTHOGRAPHIC;      // Camera mode type

    RenderTexture2D depthTex = LoadRenderTexture(screenWidth, screenHeight);      // Creates a RenderTexture2D for the depth texture
    RenderTexture2D renderedTex = LoadRenderTexture(screenWidth, screenHeight);   // Creates a RenderTexture2D for the rendered texture
    RenderTexture2D brightnessTex = LoadRenderTexture(screenWidth, screenHeight); // Creates a RenderTexture2D for the brightness texture
    RenderTexture2D lightCurveTex = LoadRenderTexture(screenWidth, screenHeight); // Creates a RenderTexture2D for the light curve texture
    RenderTexture2D minifiedLightCurveTex = LoadRenderTexture(ceil(sqrt(instances)), screenHeight);              // Creates a RenderTexture2D (1x1) for the light curve texture

    SetTargetFPS(1);                       // Attempt to run at 60 fps

    int frame_number = 0;
    // Main animation loop
    while (!WindowShouldClose())            // Detect window close button or ESC key
    {
      //----------------------------------------------------------------------------------
      // Update
      //----------------------------------------------------------------------------------
      BeginTextureMode(depthTex);                             // Enable drawing to texture
          ClearBackground(BLACK);                             // Clear texture background
      EndTextureMode();
      
      BeginTextureMode(renderedTex);                             // Enable drawing to texture
          ClearBackground(BLACK);                             // Clear texture background
      EndTextureMode();

      rlUpdateVertexBuffer(mesh.vboId[0], mesh.vertices, mesh.vertexCount*3*sizeof(float), 0);    // Update vertex position
      rlUpdateVertexBuffer(mesh.vboId[2], mesh.normals, mesh.vertexCount*3*sizeof(float), 0);     // Update vertex normals
      
      for(int instance = 0; instance < instances; instance++) {
        // int render_index = instance + frame_number * instances;
        int render_index = instance;

        sun.position = sun_vectors[render_index % data_points];
        viewer_camera.position = viewer_vectors[render_index % data_points];

        light_camera.position = (Vector3) {sun.position.x, sun.position.y, sun.position.z};
        UpdateLightValues(lighting_shader, sun);
        UpdateLightValues(depthShader, sun);

        Vector3 mesh_offsets[MAX_INSTANCES] = { 0 };
        GenerateTranslations(mesh_offsets, viewer_camera, screenWidth, screenHeight, instances);

        Vector3 viewer_camera_transforms[MAX_INSTANCES] = { 0 };
        Vector3 light_camera_transforms[MAX_INSTANCES] = { 0 };
        Matrix mvp_lights[MAX_INSTANCES] = { 0 };
        Matrix mvp_viewer[MAX_INSTANCES] = { 0 };
        Matrix mvp_light_biases[MAX_INSTANCES] = { 0 };

        viewer_camera_transforms[instance] = TransformOffsetToCameraPlane(viewer_camera, mesh_offsets[instance]);
        light_camera_transforms[instance] = TransformOffsetToCameraPlane(light_camera, mesh_offsets[instance]);

        float viewerCameraPos[3] = { viewer_camera.position.x, viewer_camera.position.y, viewer_camera.position.z };
        float lightPos[3] = { sun.position.x, sun.position.y, sun.position.z };

        mvp_lights[instance] = CalculateMVPFromCamera(light_camera, screenWidth, screenHeight, mesh_offsets[instance]); //Calculates the model-view-projection matrix for the light_camera
        mvp_viewer[instance] = CalculateMVPFromCamera(viewer_camera, screenWidth, screenHeight, mesh_offsets[instance]); //Calculates the model-view-projection matrix for the light_camera
        mvp_light_biases[instance] = CalculateMVPBFromMVP(mvp_lights[instance]); //Takes [-1, 1] -> [0, 1] for texture sampling

        SetShaderValue(lighting_shader, lighting_shader.locs[1], lightPos, SHADER_UNIFORM_VEC3); //Sends the light position vector to the lighting shader

        //----------------------------------------------------------------------------------
        // Write to depth texture
        //----------------------------------------------------------------------------------
        BeginTextureMode(depthTex);                             // Enable drawing to texture

            BeginMode3D(light_camera);                          // Begin 3d mode drawing
                model.materials[0].shader = depthShader;        // Assign depth texture shader to model

                SetShaderValue(depthShader, depthShader.locs[3], &instance, SHADER_UNIFORM_INT); //Sends the light position vector to the lighting shader
                SetShaderValueMatrix(depthShader, depth_light_mvp_locs[instance], mvp_lights[instance]);
                
                SetShaderValue(depthShader, depthShader.locs[2], lightPos, SHADER_UNIFORM_VEC3);         //Sends the light position vector to the depth shader
                DrawMesh(mesh, model.materials[0], MatrixTranslate(light_camera_transforms[instance].x, light_camera_transforms[instance].y, light_camera_transforms[instance].z));  

            EndMode3D();                                        // End 3d mode drawing, returns to orthographic 2d mode
        EndTextureMode();                                       // End drawing to texture

        //----------------------------------------------------------------------------------
        // Write to the rendered texture
        //----------------------------------------------------------------------------------
        BeginTextureMode(renderedTex);
          model.materials[0].shader = lighting_shader;             //Sets the model's shader to the lighting shader (was the depth shader)

          DrawTextureRec(depthTex.texture, (Rectangle){ 0, 0, 0, 0}, (Vector2){ 0, 0 }, WHITE);
          
          SetShaderValueTexture(lighting_shader, lighting_shader.locs[2], depthTex.texture); //Sends depth texture to the main lighting shader    

          BeginMode3D(viewer_camera);
                DrawTextureRec(depthTex.texture, (Rectangle){ 0, 0, 0, 0}, (Vector2){ 0, 0 }, WHITE);
                
                SetShaderValue(lighting_shader, lighting_shader.locs[4], &instance, SHADER_UNIFORM_INT); //Sends the light position vector to the lighting shader
                SetShaderValueMatrix(lighting_shader, lighting_shader.locs[5], mvp_light_biases[instance]);
                SetShaderValueMatrix(lighting_shader, lighting_shader.locs[3], mvp_viewer[instance]);
                SetShaderValueTexture(lighting_shader, lighting_shader.locs[2], depthTex.texture); //Sends depth texture to the main lighting shader 
                
                DrawMesh(mesh, model.materials[0], MatrixTranslate(viewer_camera_transforms[instance].x, viewer_camera_transforms[instance].y, viewer_camera_transforms[instance].z));     

                printVector3(light_camera_transforms[instance], "Viewer camera transform");
          EndMode3D();

        EndTextureMode();

      }

      BeginTextureMode(brightnessTex);
        ClearBackground(BLACK);                             // Clear texture background
        BeginShaderMode(brightness_shader);
          DrawTextureRec(renderedTex.texture, (Rectangle){ 0, 0, (float) screenWidth, (float) -screenHeight }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
      EndTextureMode();

      BeginTextureMode(lightCurveTex);
        ClearBackground(BLACK);                             // Clear texture background
        BeginShaderMode(light_curve_shader);
          DrawTextureRec(brightnessTex.texture, (Rectangle){ 0, 0, (float) screenWidth, (float) -screenHeight }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
      EndTextureMode();

      BeginTextureMode(minifiedLightCurveTex);
        ClearBackground(BLACK);                             // Clear texture background
        BeginShaderMode(min_shader);
          SetShaderValue(min_shader, min_shader.locs[0], &gridWidth, SHADER_UNIFORM_INT); //Sends the light position vector to the lighting shader
          DrawTextureRec(brightnessTex.texture, (Rectangle){ 0, 0, (float) screenWidth, (float) -screenHeight }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
      EndTextureMode();

      float clipping_area = CalculateCameraArea(viewer_camera, screenWidth, screenHeight);

      float lightCurveFunctionTrue[MAX_INSTANCES];
      float lightCurveFunctionEst[MAX_INSTANCES];
      CalculateLightCurveValues(lightCurveFunctionTrue, lightCurveFunctionEst, minifiedLightCurveTex, brightnessTex, clipping_area, instances);

      //DRAWING
      BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureRec(depthTex.texture, (Rectangle){ 0, 0, depthTex.texture.width, (float) -depthTex.texture.height }, (Vector2){ 0, 0 }, WHITE);
        DrawTextureRec(renderedTex.texture, (Rectangle){ 0, 0, depthTex.texture.width, (float) -depthTex.texture.height }, (Vector2){ 0, 0 }, WHITE);
        DrawTextureRec(minifiedLightCurveTex.texture, (Rectangle){ 0, 0, minifiedLightCurveTex.texture.width, (float) -minifiedLightCurveTex.texture.height }, (Vector2){ 0, 0 }, WHITE);

        DrawFPS(10, 10);

        DrawText(TextFormat("Clipping area = %.4f", clipping_area), 10, 40, 12, WHITE);
        // for(int i = 0; i < instances; i++) {
        //   DrawText(TextFormat("Light Curve function estimate instance %d = %.4f (dev %.4f real)", i, lightCurveFunctionEst[i], (lightCurveFunctionEst[i] - lightCurveFunctionTrue[i])), 10, 60 + 20*i, 12, WHITE);
        // }
      EndDrawing();

      frame_number++;
    }

    //----------------------------------------------------------------------------------
    // Unloading GPU components
    //--------------------------------------------------------------------------------------
    UnloadModel(model);                 // Unload the model

    UnloadShader(lighting_shader);      // Unload shader
    UnloadShader(depthShader);          // Unload depth texture shader
    UnloadShader(brightness_shader);    // Unload brightness shader
    UnloadShader(light_curve_shader);   // Unload light curve shader
    UnloadShader(min_shader);           // Unload minimize shader

    UnloadRenderTexture(depthTex);      // Unload depth texture
    UnloadRenderTexture(renderedTex);   // Unload rendered texture
    UnloadRenderTexture(brightnessTex); // Unload brightnesss texture
    UnloadRenderTexture(lightCurveTex); // Unload light curve texture
    UnloadRenderTexture(minifiedLightCurveTex); // Unload minified light curve texture

    CloseWindow();                      // Close window and OpenGL context

    return 0;
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

Matrix CalculateMVPFromCamera(Camera cam, int screenWidth, int screenHeight, Vector3 offset) //Calculates the MVP matrix for a camera
{
        Matrix matView = MatrixLookAt(cam.position, cam.target, cam.up); //Calculate camera view matrix

        float top;
        float right;
        CalculateRightAndTop(cam, screenWidth, screenHeight, &right, &top);
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

float CalculateCameraArea(Camera cam, int screenWidth, int screenHeight)
{
  float top;
  float right;
  CalculateRightAndTop(cam, screenWidth, screenHeight, &right, &top);
  return 4.0*top*right;
}

void CalculateRightAndTop(Camera cam, int screenWidth, int screenHeight, float *right, float *top)
{
  float aspect = (float)screenWidth / (float)screenHeight; //Screen aspect ratio
  *top = (float) cam.fovy/2.0; //Half-height of the clipping plane
  *right = *top*aspect; //Half-width of the clipping plane
}

void SaveScreen(char fname[]) //Saves the current screen image to a file
{
    Image screen_image = LoadImageFromScreenFixed();

    // int success = ExportImage(screen_image, fname);
    printf("Saved image!\n");
}

float CalculateMeshScaleFactor(Mesh mesh, Camera cam, int screenWidth, int screenHeight, int instances) //Finds the factor required to scale all vertices down to fit the model in a unit cube
{
  float largest_disp = 0.0;
  for(int i = 0; i < mesh.vertexCount; i++) {
    Vector3 vertex = {mesh.vertices[i*3+0], mesh.vertices[i*3+1], mesh.vertices[i*3+2]};
    float vertex_disp = Vector3Length(vertex);

    if(vertex_disp > largest_disp) largest_disp = vertex_disp;
  }
  int square_below = (int) ceil(sqrt(instances));  
  float top;
  float right;
  CalculateRightAndTop(cam, screenWidth, screenHeight, &right, &top);

  return (largest_disp / top) * square_below;
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

void GenerateTranslations(Vector3 *mesh_offsets, Camera cam, int screenWidth, int screenHeight, int instances) 
{
  float top;
  float right;
  CalculateRightAndTop(cam, screenWidth, screenHeight, &right, &top);

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

void CalculateLightCurveValues(float lightCurveFunctionTrue[], float lightCurveFunctionEst[], RenderTexture2D minifiedLightCurveTex, RenderTexture2D brightnessTex, float clipping_area, int instances) {
    int gridWidth = (int) ceil(sqrt(instances));

    Image light_curve_image = LoadImageFromTexture(minifiedLightCurveTex.texture);
    int total_pixels = brightnessTex.texture.width * brightnessTex.texture.height;

    Image full_light_curve_image = LoadImageFromTexture(brightnessTex.texture); 

    float instance_total_irrad_est[MAX_INSTANCES];
    float instance_total_irrad_true[MAX_INSTANCES];

    int grid_pixel_height = light_curve_image.height / gridWidth;

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
    
    // for(int i = 0; i < full_light_curve_image.width; i++) {
    //   for(int j = 0; j < full_light_curve_image.height; j++) {
    //     Color pix_color = GetImageColor(full_light_curve_image, i, j);

    //     if((float) pix_color.g > 0.0) {
    //       lit_pixels += 1.0;
    //     }
    //     sum_total_irrad += pix_color.r / 255.0;
    //   }
    // }
    
    //CALCULATING SHADED LC VALUES
    for(int col = 0; col < light_curve_image.width; col++) {
      for(int row_instance = 0; row_instance < gridWidth; row_instance++) {
        float lit_rows = 0.0;
        float sum_total_irrad_est = 0.0;
        for(int row_instance_pixel = row_instance * grid_pixel_height; row_instance_pixel < (row_instance + 1) * grid_pixel_height; row_instance_pixel++) {
          Color pix_color = GetImageColor(light_curve_image, col, row_instance_pixel);
          
          if((float) pix_color.g > 0.0) { //for all lit rows
            lit_rows += 1.0;
            sum_total_irrad_est += (float) pix_color.r / 255.0 * grid_pixel_height; //Represents the average irrad of each row * the fraction of lit pixels on that row
          }
        }
        instance_total_irrad_est[row_instance + gridWidth * col] = sum_total_irrad_est;
      }
    }
  
    UnloadImage(light_curve_image);
    UnloadImage(full_light_curve_image);
    
    for(int i = 0; i < instances; i++) {
      lightCurveFunctionTrue[i] = instance_total_irrad_true[i] / total_pixels * clipping_area; //running_average.x = (for all lit pixels, average irrad)
    }
    for(int i = 0; i < instances; i++) {
      lightCurveFunctionEst[i] = instance_total_irrad_est[i] / total_pixels * clipping_area;
    }
}

void printVector3(Vector3 vec, char name[])
{
  printf("%s: %.4f, %.4f, %.4f\n", name, vec.x, vec.y, vec.z);
}