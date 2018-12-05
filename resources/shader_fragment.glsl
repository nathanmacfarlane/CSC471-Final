#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
uniform vec3 campos;

uniform sampler2D shadowMapTex;
uniform sampler2D tex;
uniform sampler2D mask;
uniform sampler2D streetTex;

in vec3 lps[10];
uniform float isGround;
uniform float isLamp;
in vec4 fragLightSpacePos;

// Evaluates how shadowed a point is using PCF with 5 samples
// Credit: Sam Freed - https://github.com/sfreed141/vct/blob/master/shaders/phong.frag
float calcShadowFactor(vec4 lightSpacePosition) {
    vec3 shifted = (lightSpacePosition.xyz / lightSpacePosition.w + 1.0) * 0.5;

    float shadowFactor = 0;
    float bias = 0.0001;
    float fragDepth = shifted.z - bias;

    if (fragDepth > 1.0) {
        return 0.0;
    }

	if(shifted.x>1.) return 0;
	if(shifted.x<0) return 0;
	if(shifted.y>1.) return 0;
	if(shifted.y<0) return 0;


    const int numSamples = 5;
    const ivec2 offsets[numSamples] = ivec2[](
        ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(-1, 0), ivec2(0, -1)
    );

	vec3 maskcolor = texture(mask, shifted.xy).rgb;

    if (fragDepth > textureOffset(shadowMapTex, shifted.xy, offsets[0]).r) {
        shadowFactor += 1;
    }
    shadowFactor = 1-shadowFactor;
    return shadowFactor * maskcolor.r;
}



void main()
{
    float shadowFactor = calcShadowFactor(fragLightSpacePos);
    if (isLamp > 0.5) {
        color.rgb = vec3(0.7, 0.7, 0.7) * 0.1 + shadowFactor * 0.9;;
    } else if (isGround > 0.5) {
        color.rgb = texture(streetTex, vertex_tex).rgb;
        color.rgb *= vec3(0.3,0.3,0.3) * 0.1 + shadowFactor * 0.9;
    } else {
        color.rgb = texture(tex, vertex_tex).rgb;
        if (!(color.r > 0.5 && color.g > 0.5 && color.b > 0.5)) {
            color.rgb *= 0.1 + shadowFactor * 0.9;
        }
    }
    color.a = 1;

    float amount = 0;
    for (int i = 0; i < 2; i++) {
        float dist = distance(vertex_pos, lps[i]) * 4;
        amount += 1.0/dist;
    }
    color.b += amount;
}
