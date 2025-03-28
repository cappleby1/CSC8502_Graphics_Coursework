#version 330 core

uniform sampler2D diffuseTex; // Diffuse texture map
uniform sampler2D bumpTex; // Bump map

in Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 worldPos;
} IN;

out vec4 fragColour[2]; // Our final output colours

void main(void) {
    mat3 TBN = mat3(normalize(IN.tangent),
                    normalize(IN.binormal),
                    normalize(IN.normal));

    vec3 normal = texture(bumpTex, IN.texCoord).rgb * 2.0 - 1.0;
    normal = normalize(TBN * normalize(normal));

    fragColour[0] = texture(diffuseTex, IN.texCoord);
    fragColour[1] = vec4(normal * 0.5 + 0.5, 1.0); // Normal in [0,1] range
}