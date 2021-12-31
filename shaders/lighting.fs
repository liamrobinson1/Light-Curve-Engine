#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec4 ShadowCoord;
in vec3 lightPosition;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

#define     MAX_LIGHTS              4
#define     LIGHT_DIRECTIONAL       0
#define     LIGHT_POINT             1

struct MaterialProperty {
    vec3 color;
    int useSampler;
    sampler2D sampler;
};

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};

// Input lighting values
uniform Light lights[MAX_LIGHTS];
uniform vec3 viewPos;
uniform sampler2D depthTex;

void main()
{
    // Texel color fetching from texture sampler
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 specular = vec3(0.0);

    // NOTE: Implement here your fragment shader code

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 1)
        {
            vec3 light = vec3(0.0);

            if (lights[i].type == LIGHT_DIRECTIONAL)
            {
                light = -normalize(lights[i].target - lights[i].position);
            }

            if (lights[i].type == LIGHT_POINT)
            {
                light = normalize(lights[i].position - fragPosition);
            }

            float NdotL = max(dot(normal, light), 0.0);
            lightDot += lights[i].color.rgb*NdotL;

            float specCo = 0.0;
            if (NdotL > 0.0) specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), 16.0); // 16 refers to shine
            specular += specCo;
        }
    }

    vec4 irradiance = (fragColor*((colDiffuse + vec4(specular, 1.0))*vec4(lightDot, 1.0)));

    float irradUnit = irradiance.r * 255; //Finds the integer RGB (out of 255)
    float irradDecimal = 0.5 + (irradUnit - round(irradUnit)); //Finds what would be dropped otherwise

    finalColor = vec4(irradUnit / 255.0, irradUnit / 255.0, irradUnit / 255.0, 1.0);

    //SHADOWING
    vec3 normalOffset = normalize(fragNormal) * 0.04;
    // vec3 normalOffset = vec3(0.0, 0.0, 0.0);

    float textureDepth = texture(depthTex, ShadowCoord.xy).x; // depth from the depth texture
    //point to plane distance computation (from frag position to the oblique plane of the light)
    float x1 = fragPosition.x + normalOffset.x; //world coordinates of the fragment of interest
    float y1 = fragPosition.y + normalOffset.y;
    float z1 = fragPosition.z + normalOffset.z;

    float A = lightPosition.x; //for the equation of the light's plane as A(x-x0) + B(y-y0) + C(z-z0) = 0
    float B = lightPosition.y;
    float C = lightPosition.z;

    float x0 = lightPosition.x; //coordinates of a point we know lies in the plane
    float y0 = lightPosition.y;
    float z0 = lightPosition.z;

    float d = -dot(vec3(x1-x0, y1-y0, z1-z0), vec3(A, B, C))/40.0;

    float cosTheta = dot(normalize(lightPosition), normalize(fragNormal));

    // float bias = 0.001 * tan(acos(cosTheta));
    float bias = 0.002 * tan(acos(cosTheta));
    bias = clamp(bias, 0.001, 0.04);

    if(textureDepth < d - bias) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    // finalColor = texture(depthTex, ShadowCoord.xy);
    // finalColor = vec4(d, d, d, 1.0);
}
