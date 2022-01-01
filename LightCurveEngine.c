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

#define RLIGHTS_IMPLEMENTATION
#include "include/rlights.h"

#define GLSL_VERSION            330
#define MAX_INSTANCES          5 //no work: 2 5 6 10 11 (last 2 left) 12 17 (last 3 left) 18 (last 5 left) 19 (last 7 left) 20 (last 5 left)
                                  //not rendered count: 2 5 6 10 9 12 14 13 12 20

Image LoadImageFromScreenFixed(void);
void printMatrix(Matrix m);
Matrix CalculateMVPFromCamera(Camera light_camera, int screenWidth, int screenHeight, Vector3 offset);
Matrix CalculateMVPBFromMVP(Matrix mvp_light);
void SaveScreen(char fname[]);
float CalculateCameraArea(Camera cam, int screenWidth, int screenHeight);
float CalculateMeshScaleFactor(Mesh mesh, Camera cam, int screenWidth, int screenHeight); //Finds the factor required to scale all vertices down to fit the model in a unit cube
Mesh ApplyMeshScaleFactor(Mesh mesh, float sf);
Vector3 TransformOffsetToCameraPlane(Camera cam, Vector3 offset);
void GenerateTranslations(Vector3 *mesh_offsets, Camera cam, int screenWidth, int screenHeight);
void CalculateRightAndTop(Camera cam, int screenWidth, int screenHeight, float *right, float *top);

int main(void)
{
    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 800;

    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    InitWindow(screenWidth, screenHeight, "Light Curve Engine"); // A cool name for a cool app

    Camera viewer_camera = { 0 };                              // Define the viewer camera
    viewer_camera.position = (Vector3){ 2.0f, 0.0f, -6.0f };   // Camera position
    viewer_camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    viewer_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    viewer_camera.fovy = 4.0f;                                // Camera field-of-view Y
    viewer_camera.projection = CAMERA_ORTHOGRAPHIC;             // Camera mode type

    // Load plane model from a generated mesh
    Model model = LoadModel("models/bob_tri.obj");
    Mesh mesh = model.meshes[0];

    float mesh_scale_factor = CalculateMeshScaleFactor(mesh, viewer_camera, screenWidth, screenHeight);
    mesh = ApplyMeshScaleFactor(mesh, mesh_scale_factor);

    // Loading depth shader
    Shader depthShader = LoadShader("shaders/depth_texture.vs", "shaders/create_depth_texture.fs");
    Shader lighting_shader = LoadShader("shaders/base_shadowing.vs", "shaders/lighting.fs");
    Shader brightness_shader = LoadShader("shaders/brightness.vs", "shaders/brightness.fs");
    Shader light_curve_shader = LoadShader("shaders/light_curve_extraction.vs", "shaders/light_curve_extraction.fs");
    Shader min_shader = LoadShader("shaders/minimize.vs", "shaders/minimize.fs");
    
    depthShader.locs[0] = GetShaderLocation(depthShader, "viewPos");           //Location of the viewer position uniform for the depth shader
    depthShader.locs[1] = GetShaderLocation(depthShader, "light_mvp");         //Location of the light MVP matrix uniform for the depth shader
    depthShader.locs[2] = GetShaderLocation(depthShader, "lightPos");          //Location of the light position uniform for the depth shader
    depthShader.locs[3] = GetShaderLocation(depthShader, "model_id");          //Location of the model id uniform for the depth shader

    int depth_light_mvp_locs[MAX_INSTANCES];
    for (int i = 0; i < MAX_INSTANCES; i++)
    {
      const char *matName = TextFormat("light_mvps[%d].mat\0", i);
      depth_light_mvp_locs[i] = GetShaderLocation(depthShader, matName);
    }

    lighting_shader.locs[0] = GetShaderLocation(lighting_shader, "viewPos");   //Location of the viewer position uniform for the lighting shader
    lighting_shader.locs[1] = GetShaderLocation(lighting_shader, "lightPos");  //Location of the light position uniform for the lighting shader
    lighting_shader.locs[2] = GetShaderLocation(lighting_shader, "depthTex");  //Location of the depth texture uniform for the lighting shader
    lighting_shader.locs[3] = GetShaderLocation(lighting_shader, "mvp_from_script"); //Location of the light MVP matrix uniform for the lighting shader
    lighting_shader.locs[4] = GetShaderLocation(lighting_shader, "model_id");  //Location of the light MVP matrix uniform for the lighting shader
    lighting_shader.locs[5] = GetShaderLocation(lighting_shader, "light_mvp");

    int lighting_light_mvp_locs[MAX_INSTANCES];
    for (int i = 0; i < MAX_INSTANCES; i++)
    {
      const char *matName = TextFormat("light_mvps[%d].mat\0", i);
      lighting_light_mvp_locs[i] = GetShaderLocation(lighting_shader, matName);
    }

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
    RenderTexture2D minifiedLightCurveTex = LoadRenderTexture(1, screenHeight);              // Creates a RenderTexture2D (1x1) for the light curve texture

    SetTargetFPS(600);                       // Attempt to run at 60 fps

    // Main animation loop
    while (!WindowShouldClose())            // Detect window close button or ESC key
    {
      //----------------------------------------------------------------------------------
      // Update
      //----------------------------------------------------------------------------------
      sun.position = (Vector3) {2.0f*sin(GetTime()), 1.0, 2.0f*cos(GetTime())};

      viewer_camera.position = (Vector3) {2.0*cos(GetTime()), 10.0, 2.0*sin(GetTime()/2)};

      light_camera.position = (Vector3) {sun.position.x, sun.position.y, sun.position.z};

      Vector3 mesh_offsets[MAX_INSTANCES] = { 0 };

      GenerateTranslations(mesh_offsets, viewer_camera, screenWidth, screenHeight);

      Vector3 viewer_camera_transforms[MAX_INSTANCES] = { 0 };
      Vector3 light_camera_transforms[MAX_INSTANCES] = { 0 };
      Matrix mvp_lights[MAX_INSTANCES] = { 0 };
      Matrix mvp_viewer[MAX_INSTANCES] = { 0 };
      Matrix mvp_light_biases[MAX_INSTANCES] = { 0 };

      for(int i = 0; i < MAX_INSTANCES; i++) {
        viewer_camera_transforms[i] = TransformOffsetToCameraPlane(viewer_camera, mesh_offsets[i]);
        light_camera_transforms[i] = TransformOffsetToCameraPlane(light_camera, mesh_offsets[i]);
      }
      // UpdateCamera(&viewer_camera);              // Update camera
      UpdateLightValues(lighting_shader, sun);

      float viewerCameraPos[3] = { viewer_camera.position.x, viewer_camera.position.y, viewer_camera.position.z };
      float lightPos[3] = { sun.position.x, sun.position.y, sun.position.z };

      for(int i = 0; i < MAX_INSTANCES; i++) {
        mvp_lights[i] = CalculateMVPFromCamera(light_camera, screenWidth, screenHeight, mesh_offsets[i]); //Calculates the model-view-projection matrix for the light_camera
        mvp_viewer[i] = CalculateMVPFromCamera(viewer_camera, screenWidth, screenHeight, mesh_offsets[i]); //Calculates the model-view-projection matrix for the light_camera
        mvp_light_biases[i] = CalculateMVPBFromMVP(mvp_lights[i]); //Takes [-1, 1] -> [0, 1] for texture sampling
      }

      SetShaderValue(lighting_shader, lighting_shader.locs[1], lightPos, SHADER_UNIFORM_VEC3); //Sends the light position vector to the lighting shader
      
      SetShaderValue(depthShader, depthShader.locs[0], lightPos, SHADER_UNIFORM_VEC3);           //Sends the viewer position vector (lightPos for this camera) to the depth shader
      SetShaderValue(depthShader, depthShader.locs[2], lightPos, SHADER_UNIFORM_VEC3);         //Sends the light position vector to the depth shader
      
      rlUpdateVertexBuffer(mesh.vboId[0], mesh.vertices, mesh.vertexCount*3*sizeof(float), 0);    // Update vertex position
      rlUpdateVertexBuffer(mesh.vboId[2], mesh.normals, mesh.vertexCount*3*sizeof(float), 0);     // Update vertex normals

      //----------------------------------------------------------------------------------
      // Write to depth texture
      //----------------------------------------------------------------------------------
      BeginTextureMode(depthTex);                             // Enable drawing to texture
          ClearBackground(BLACK);                             // Clear texture background

          BeginMode3D(light_camera);                          // Begin 3d mode drawing
              model.materials[0].shader = depthShader;        // Assign depth texture shader to model
              for(int i = 0; i < MAX_INSTANCES; i++) {
                SetShaderValue(depthShader, depthShader.locs[3], &i, SHADER_UNIFORM_INT); //Sends the light position vector to the lighting shader
                SetShaderValueMatrix(depthShader, depth_light_mvp_locs[i], mvp_lights[i]);
                DrawMesh(mesh, model.materials[0], MatrixTranslate(light_camera_transforms[i].x, light_camera_transforms[i].y, light_camera_transforms[i].z));  
                
                // printf("%.1f %.1f %.1f\n", light_camera_transforms[i].x, light_camera_transforms[i].y, light_camera_transforms[i].z);
                // printMatrix(mvp_lights[i]);
              }

          EndMode3D();                                        // End 3d mode drawing, returns to orthographic 2d mode
      EndTextureMode();                                       // End drawing to texture


      //----------------------------------------------------------------------------------
      // Write to the rendered texture
      //----------------------------------------------------------------------------------
      BeginTextureMode(renderedTex);
        model.materials[0].shader = lighting_shader;             //Sets the model's shader to the lighting shader (was the depth shader)
        ClearBackground(BLACK);                             // Clear texture background
        DrawTextureRec(depthTex.texture, (Rectangle){ 0, 0, 0, 0}, (Vector2){ 0, 0 }, WHITE);
        SetShaderValueTexture(lighting_shader, lighting_shader.locs[2], depthTex.texture); //Sends depth texture to the main lighting shader    

        for(int i = 0; i < MAX_INSTANCES; i++) {
          BeginMode3D(viewer_camera);
                DrawTextureRec(depthTex.texture, (Rectangle){ 0, 0, 0, 0}, (Vector2){ 0, 0 }, WHITE);
                SetShaderValue(lighting_shader, lighting_shader.locs[4], &i, SHADER_UNIFORM_INT); //Sends the light position vector to the lighting shader
                
                SetShaderValueMatrix(lighting_shader, lighting_shader.locs[5], mvp_light_biases[i]);
                
                SetShaderValueMatrix(lighting_shader, lighting_shader.locs[3], mvp_viewer[i]);

                SetShaderValueTexture(lighting_shader, lighting_shader.locs[2], depthTex.texture); //Sends depth texture to the main lighting shader 
                DrawMesh(mesh, model.materials[0], MatrixTranslate(viewer_camera_transforms[i].x, viewer_camera_transforms[i].y, viewer_camera_transforms[i].z));     
          EndMode3D();
        }
      EndTextureMode();

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
          DrawTextureRec(brightnessTex.texture, (Rectangle){ 0, 0, (float) screenWidth, (float) -screenHeight }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
      EndTextureMode();

      Image light_curve_image = LoadImageFromTexture(minifiedLightCurveTex.texture);
      int total_pixels = brightnessTex.texture.width * brightnessTex.texture.height;

      // Image full_light_curve_image = LoadImageFromTexture(brightnessTex.texture); 

      // float lit_pixels = 0.0;
      // float sum_total_irrad = 0.0;

      // for(int i = 0; i < full_light_curve_image.width; i++) {
      //   for(int j = 0; j < full_light_curve_image.height; j++) {
      //     Color pix_color = GetImageColor(full_light_curve_image, i, j);

      //     if((float) pix_color.g > 0.0) {
      //       lit_pixels += 1.0;
      //     }
      //     sum_total_irrad += pix_color.r / 255.0;
      //   }
      // }

      float lit_rows = 0.0;
      float sum_total_irrad_est = 0.0;

      for(int i = 0; i < light_curve_image.height; i++) {
        Color pix_color = GetImageColor(light_curve_image, 0, i);
        
        if((float) pix_color.g > 0.0) { //for all lit rows
          lit_rows += 1.0;
          sum_total_irrad_est += (float) pix_color.r / 255.0 * brightnessTex.texture.width; //Represents the average irrad of each row * the fraction of lit pixels on that row
        }
      }
    
      float clippingArea = CalculateCameraArea(viewer_camera, screenWidth, screenHeight);

      UnloadImage(light_curve_image);
      // UnloadImage(full_light_curve_image);

      // float lightCurveFunction = sum_total_irrad / total_pixels * clippingArea; //running_average.x = (for all lit pixels, average irrad)
      float lightCurveFunctionEst = sum_total_irrad_est / total_pixels * clippingArea;

      //DRAWING
      BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureRec(depthTex.texture, (Rectangle){ 0, 0, depthTex.texture.width, (float) -depthTex.texture.height }, (Vector2){ 0, 0 }, WHITE);
        DrawTextureRec(renderedTex.texture, (Rectangle){ 0, 0, depthTex.texture.width, (float) -depthTex.texture.height }, (Vector2){ 0, 0 }, WHITE);
    

        DrawFPS(10, 10);

        DrawText(TextFormat("Clipping area = %.4f", clippingArea), 10, 40, 12, WHITE);
        // DrawText(TextFormat("Light Curve function = %.4f", lightCurveFunction), 10, 40, 12, WHITE);
        DrawText(TextFormat("Lite Curve function estimate = %.4f", lightCurveFunctionEst), 10, 60, 12, WHITE);
        // DrawText(TextFormat("Light Curve percent difference = %.2f\%", fabs((lightCurveFunctionEst - lightCurveFunction) / lightCurveFunction * 100)), 10, 80, 12, WHITE);
      EndDrawing();
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

float CalculateMeshScaleFactor(Mesh mesh, Camera cam, int screenWidth, int screenHeight) //Finds the factor required to scale all vertices down to fit the model in a unit cube
{
  float largest_disp = 0.0;
  for(int i = 0; i < mesh.vertexCount; i++) {
    Vector3 vertex = {mesh.vertices[i*3+0], mesh.vertices[i*3+1], mesh.vertices[i*3+2]};
    float vertex_disp = Vector3Length(vertex);

    if(vertex_disp > largest_disp) largest_disp = vertex_disp;
  }
  int square_below = (int) ceil(sqrt(MAX_INSTANCES));  
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

void GenerateTranslations(Vector3 *mesh_offsets, Camera cam, int screenWidth, int screenHeight) 
{
  float top;
  float right;
  CalculateRightAndTop(cam, screenWidth, screenHeight, &right, &top);

  int square_below = (int) ceil(sqrt(MAX_INSTANCES));

  int index = 0;
  for(int i = 0; i < square_below; i++) {
    for(int j = 0; j < square_below; j++) {
      Vector3 absolute_offset = {-right * (square_below - 1.0) + i * (square_below + (4.0 - square_below)), -top * (square_below - 1.0) + j * (square_below + (4.0 - square_below)), 0.0};

      mesh_offsets[index] = Vector3Scale(absolute_offset, 1.0 / (float) square_below);
      index++;
      if(index == MAX_INSTANCES) break;
    }
  }
}