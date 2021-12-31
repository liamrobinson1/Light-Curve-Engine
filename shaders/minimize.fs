#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main()
{
    ivec2 texSize = textureSize(texture0, 0);
    vec2 size = vec2(float(texSize.x), float(texSize.y));
    // Texel color fetching from texture sampler
    vec4 texelColor = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 acculumatedColor = vec3(0.0, 0.0, 0.0);

    for(float i = 0; i < size.x; i++) {
        vec3 texCol = texture(texture0, vec2(i / size.x, fragTexCoord.y)).rgb;
        acculumatedColor = acculumatedColor + texCol;
    }
    
    acculumatedColor = acculumatedColor / size.x;
    texelColor = vec4(acculumatedColor, 1.0);

    // NOTE: Implement here your fragment shader code
    finalColor = texelColor;
}