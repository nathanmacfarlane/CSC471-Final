#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec3 lamp_pos;
uniform vec3 campos;

//TODO
//layout(location = 1) uniform sampler2D shadowMapTex;
/*
// Evaluates how shadowed a point is using PCF with 5 samples
// Credit: Sam Freed - https://github.com/sfreed141/vct/blob/master/shaders/phong.frag
float calcShadowFactor(vec4 lightSpacePosition) {
    vec3 shifted = (lightSpacePosition.xyz / lightSpacePosition.w + 1.0) * 0.5;

    float shadowFactor = 0;
    float bias = 0.01;
    float fragDepth = shifted.z - bias;

    if (fragDepth > 1.0) {
        return 0.0;
    }

    const int numSamples = 5;
    const ivec2 offsets[numSamples] = ivec2[](
        ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(-1, 0), ivec2(0, -1)
    );

    for (int i = 0; i < numSamples; i++) {
        if (fragDepth > textureOffset(shadowMapTex, shifted.xy, offsets[i]).r) {
            shadowFactor += 1;
        }
    }
    shadowFactor /= numSamples;

    
*/

void main()
{

//float shadowFactor = 1.0 - calcShadowFactor(fragLightSpacePos);
    vec3 n = normalize(vertex_normal);
    vec3 lp = vec3(lamp_pos.x-2.0, 0.3, lamp_pos.z-0.9);
    vec3 ld = normalize(lp - vertex_pos);
    float diff = max(dot(n, ld), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 0.7);
    color = vec4(diffuse, 1.0) * 0.2;

	//color.rgb *=shadowFactor;

}
