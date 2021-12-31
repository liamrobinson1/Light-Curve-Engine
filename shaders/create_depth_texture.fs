#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;

out vec4 fragColor;

uniform sampler2D texture0;
uniform vec3 lightPos;

// Input lighting values

void main()
{
    // NOTE: Implement here your fragment shader code
    float x1 = fragPosition.x; //world coordinates of the fragment of interest
    float y1 = fragPosition.y;
    float z1 = fragPosition.z;

    float A = lightPos.x; //for the equation of the light's plane as A(x-x0) + B(y-y0) + C(z-z0) = 0
    float B = lightPos.y;
    float C = lightPos.z;

    float x0 = lightPos.x; //coordinates of a point we know lies in the plane
    float y0 = lightPos.y;
    float z0 = lightPos.z;

    float d = -dot(vec3(x1-x0, y1-y0, z1-z0), vec3(A, B, C))/40.0;

    fragColor = vec4(d, d, d, 1.0);
}
