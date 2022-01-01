#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 matView;
uniform mat4 matProjection;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec4 ShadowCoord;
out vec3 lightPosition;

// NOTE: Add here your custom variables
uniform mat4 mvp_from_script;
uniform int model_id;
uniform vec3 lightPos;

#define MAX_MODELS   16
struct MatArr {
    mat4 mat;
};
uniform MatArr light_mvps[MAX_MODELS];

void main()
{
    mat4 light_mvp_from_arr = light_mvps[model_id].mat;
    mat4 mvp_manual = matProjection * matView * matModel;
    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));
    fragPosition = vertexPosition;

    fragTexCoord = vertexTexCoord;

    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    gl_Position = mvp_from_script*vec4(vertexPosition, 1.0);
    ShadowCoord = light_mvp_from_arr*vec4(vertexPosition, 1.0);
    // lightPosition = vec3(matModel*vec4(lightPos, 1.0));
    lightPosition = lightPos;
}
