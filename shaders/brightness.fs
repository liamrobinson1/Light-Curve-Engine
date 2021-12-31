#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);

    // NOTE: Implement here your fragment shader code
    float areaUnit = 0.0;

    if(texelColor.r > 0.0) {
      areaUnit = 1.0; //Indicates that area is present here
    }

    finalColor = vec4(texelColor.r, areaUnit, texelColor.b, 1.0); //Indicates that irradiance is present here
}