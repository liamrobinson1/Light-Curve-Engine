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
#include "include/lightcurvelib.c"

#define RLIGHTS_IMPLEMENTATION
#include "include/rlights.h"

#define MAX_INSTANCES          25
#define MAX_DATA_POINTS        1000
#define MAX_FNAME_LENGTH       100

int main(void)
{
    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    int screenPixels;
    char command_filename[] = "light_curve.lcc"; 
    int instances; 
    Vector3 sun_vectors[MAX_DATA_POINTS];
    Vector3 viewer_vectors[MAX_DATA_POINTS]; 
    int data_points;
    char results_file[MAX_FNAME_LENGTH];
    char model_name[MAX_FNAME_LENGTH];
    int frame_rate;

    bool rendering = true;

    ReadLightCurveCommandFile(command_filename, model_name, &instances, &screenPixels, sun_vectors, viewer_vectors, &data_points, results_file, &frame_rate);
    ClearLightCurveResults(results_file);

    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    InitWindow(screenPixels, screenPixels, "Light Curve Engine"); // A cool name for a cool app

    int gridWidth = (int) ceil(sqrt(instances));

    Model model = LoadModel(TextFormat("models/%s", model_name));
    Mesh mesh = model.meshes[0];

    Camera viewer_camera;                            // Define the viewer camera
    InitializeViewerCamera(&viewer_camera);
    
    float mesh_scale_factor = CalculateMeshScaleFactor(mesh, viewer_camera, instances);
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

    Light sun = CreateLight(LIGHT_DIRECTIONAL, (Vector3) { 2.0f, 2.0f, 2.0f }, Vector3Zero(), WHITE, lighting_shader);

    Camera light_camera = { 0 };                        // The camera that views the scene from the light's perspective
    light_camera.position = sun.position;         // Camera position
    light_camera.target = sun.target;             // Camera looking at point
    light_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };    // Camera up vector (rotation towards target)
    light_camera.fovy = 4.0f;                           // Camera field-of-view Y
    light_camera.projection = CAMERA_ORTHOGRAPHIC;      // Camera mode type

    RenderTexture2D depthTex = LoadRenderTexture(screenPixels, screenPixels);      // Creates a RenderTexture2D for the depth texture
    RenderTexture2D renderedTex = LoadRenderTexture(screenPixels, screenPixels);   // Creates a RenderTexture2D for the rendered texture
    RenderTexture2D brightnessTex = LoadRenderTexture(screenPixels, screenPixels); // Creates a RenderTexture2D for the brightness texture
    RenderTexture2D lightCurveTex = LoadRenderTexture(screenPixels, screenPixels); // Creates a RenderTexture2D for the light curve texture
    RenderTexture2D minifiedLightCurveTex = LoadRenderTexture(ceil(sqrt(instances)), screenPixels);              // Creates a RenderTexture2D (1x1) for the light curve texture

    SetTargetFPS(frame_rate);                       // Attempt to run at 60 fps

    float light_curve_results[MAX_DATA_POINTS];
    int frame_number = 0;
    // Main animation loop
    while (!WindowShouldClose() && rendering)            // Detect window close button or ESC key
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
        int render_index = instance + (frame_number * instances) % data_points;
        // int render_index = 19;

        sun.position = sun_vectors[render_index];
        viewer_camera.position = viewer_vectors[render_index];

        light_camera.position = (Vector3) {sun.position.x, sun.position.y, sun.position.z};
        UpdateLightValues(lighting_shader, sun);
        UpdateLightValues(depthShader, sun);

        Vector3 mesh_offsets[MAX_INSTANCES] = { 0 };
        GenerateTranslations(mesh_offsets, viewer_camera, instances);

        Vector3 viewer_camera_transforms[MAX_INSTANCES] = { 0 };
        Vector3 light_camera_transforms[MAX_INSTANCES] = { 0 };
        Matrix mvp_lights[MAX_INSTANCES] = { 0 };
        Matrix mvp_viewer[MAX_INSTANCES] = { 0 };
        Matrix mvp_light_biases[MAX_INSTANCES] = { 0 };

        viewer_camera_transforms[instance] = TransformOffsetToCameraPlane(viewer_camera, mesh_offsets[instance]);
        light_camera_transforms[instance] = TransformOffsetToCameraPlane(light_camera, mesh_offsets[instance]);

        float viewerCameraPos[3] = { viewer_camera.position.x, viewer_camera.position.y, viewer_camera.position.z };
        float lightPos[3] = { sun.position.x, sun.position.y, sun.position.z };

        mvp_lights[instance] = CalculateMVPFromCamera(light_camera, mesh_offsets[instance]); //Calculates the model-view-projection matrix for the light_camera
        mvp_viewer[instance] = CalculateMVPFromCamera(viewer_camera, mesh_offsets[instance]); //Calculates the model-view-projection matrix for the light_camera
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
          EndMode3D();

        EndTextureMode();
      }

      BeginTextureMode(brightnessTex);
        ClearBackground(BLACK);                             // Clear texture background
        BeginShaderMode(brightness_shader);
          DrawTextureRec(renderedTex.texture, (Rectangle){ 0, 0, (float) screenPixels, (float) -screenPixels }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
      EndTextureMode();

      BeginTextureMode(lightCurveTex);
        ClearBackground(BLACK);                             // Clear texture background
        BeginShaderMode(light_curve_shader);
          DrawTextureRec(brightnessTex.texture, (Rectangle){ 0, 0, (float) screenPixels, (float) -screenPixels }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
      EndTextureMode();

      BeginTextureMode(minifiedLightCurveTex);
        ClearBackground(BLACK);                             // Clear texture background
        BeginShaderMode(min_shader);
          SetShaderValue(min_shader, min_shader.locs[0], &gridWidth, SHADER_UNIFORM_INT); //Sends the light position vector to the lighting shader
          DrawTextureRec(brightnessTex.texture, (Rectangle){ 0, 0, (float) screenPixels, (float) -screenPixels }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
      EndTextureMode();

      float clipping_area = CalculateCameraArea(viewer_camera);

      float lightCurveFunction[MAX_INSTANCES];
      CalculateLightCurveValues(lightCurveFunction, minifiedLightCurveTex, brightnessTex, clipping_area, instances, mesh_scale_factor);
      
      //STORING LIGHT CURVE RESULTS
      for(int i = 0; i < instances; i++) {
        int data_point_index = (frame_number * instances) % data_points + i;
        light_curve_results[data_point_index] = lightCurveFunction[i];

        if(data_point_index + 1 == data_points) {
          WriteLightCurveResults(results_file, light_curve_results, data_points);
          rendering = false;
        }
      }

      //DRAWING
      BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureRec(depthTex.texture, (Rectangle){ 0, 0, depthTex.texture.width, (float) -depthTex.texture.height }, (Vector2){ 0, 0 }, WHITE);
        DrawTextureRec(renderedTex.texture, (Rectangle){ 0, 0, depthTex.texture.width, (float) -depthTex.texture.height }, (Vector2){ 0, 0 }, WHITE);
        DrawTextureRec(minifiedLightCurveTex.texture, (Rectangle){ 0, 0, minifiedLightCurveTex.texture.width, (float) -minifiedLightCurveTex.texture.height }, (Vector2){ 0, 0 }, WHITE);

        DrawFPS(10, 10);

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