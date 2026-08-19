#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 u_ObjectPos;
uniform float u_Alpha;

out vec4 finalColor;

const mat4 bayerMatrix = mat4(
    vec4( 1.0,  9.0,  3.0, 11.0) / 17.0,
    vec4(13.0,  5.0, 15.0,  7.0) / 17.0,
    vec4( 4.0, 12.0,  2.0, 10.0) / 17.0,
    vec4(16.0,  8.0, 14.0,  6.0) / 17.0
);

float hash3D(vec3 p)
{
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 color = texelColor * colDiffuse * fragColor;

    // offset for avoiding hash collision
    float h = hash3D(u_ObjectPos);
    int offsetX = int(mod(h * 1000.0, 4.0));
    int offsetY = int(mod(h * 10000.0, 4.0));

    int x = int(mod(floor(gl_FragCoord.x) + offsetX, 4.0));
    int y = int(mod(floor(gl_FragCoord.y) + offsetY, 4.0));
    
    float threshold = bayerMatrix[x][y];

    float alphaLimit = (u_Alpha > 0.0) ? u_Alpha : fragColor.a;

    // if the alpha is below the threshold, discard the fragment
    if (alphaLimit < threshold)
    {
        discard;
    }

    finalColor = color;
}