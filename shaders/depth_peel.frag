#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D u_OpaqueDepth;
uniform sampler2D u_PrevDepth;
uniform vec2 u_ScreenSize;
uniform bool u_Peeled;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main()
{
    vec2 screenUV = vec2(gl_FragCoord.x, gl_FragCoord.y) / u_ScreenSize;

    float opaqueDepth = texture(u_OpaqueDepth, screenUV).r;
    if (gl_FragCoord.z >= opaqueDepth)
        discard;

    if (u_Peeled)
    {
        float prevDepth = texture(u_PrevDepth, screenUV).r;
        if (gl_FragCoord.z <= prevDepth)
            discard;
    }

    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 baseColor = texelColor * colDiffuse * fragColor;

    finalColor = vec4(baseColor.rgb * baseColor.a, baseColor.a);
}