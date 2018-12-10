#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
uniform vec3 campos;
uniform vec3 carpos;

uniform sampler2D shadowMapTex;
uniform sampler2D tex;
uniform sampler2D mask;

uniform float isRoad2WayHorizontal;
uniform float isRoad2WayVertical;
uniform float isRoad3WayNoBottom;
uniform float isRoad3WayNoLeft;
uniform float isRoad3WayNoRight;
uniform float isRoad3WayNoTop;
uniform float isRoad4Way;
uniform float isRoadCornerBottomLeft;
uniform float isRoadCornerBottomRight;
uniform float isRoadCornerTopLeft;
uniform float isRoadCornerTopRight;
uniform float isRoad1WayBottom;

uniform sampler2D road2WayHorizontal;
uniform sampler2D road2WayVertical;
uniform sampler2D Road3WayNoBottom;
uniform sampler2D Road3WayNoLeft;
uniform sampler2D road3WayNoRight;
uniform sampler2D Road3WayNoTop;
uniform sampler2D road4Way;
uniform sampler2D roadCornerBottomLeft;
uniform sampler2D roadCornerBottomRight;
uniform sampler2D roadCornerTopLeft;
uniform sampler2D roadCornerTopRight;
uniform sampler2D road1WayBottom;

in vec3 lps[10];
uniform float isGround;
uniform float isLamp;
uniform float highBeams;
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
    float highs = 0.3;
    if (isLamp > 0.5) {
        color.rgb = vec3(0.2, 0.2, 0.2) + shadowFactor * 0.9;
        color.rgb = vec3(0.1, 0.1, 0.1);
    } else if (isGround > 0.5) {

        if (isRoad2WayHorizontal > 0.5) {
            color.rgb = texture(road2WayHorizontal, vertex_tex).rgb;
        } else if (isRoad2WayVertical > 0.5) {
            color.rgb = texture(road2WayVertical, vertex_tex).rgb;
        } else if (isRoad3WayNoBottom > 0.5) {
            color.rgb = texture(Road3WayNoBottom, vertex_tex).rgb;
        } else if (isRoad3WayNoLeft > 0.5) {
            color.rgb = texture(Road3WayNoLeft, vertex_tex).rgb;
        } else if (isRoad3WayNoRight > 0.5) {
            color.rgb = texture(road3WayNoRight, vertex_tex).rgb;
        } else if (isRoad3WayNoTop > 0.5) {
            color.rgb = texture(Road3WayNoTop, vertex_tex).rgb;
        } else if (isRoad4Way > 0.5) {
            color.rgb = texture(road4Way, vertex_tex).rgb;
        } else if (isRoadCornerBottomLeft > 0.5) {
            color.rgb = texture(roadCornerBottomLeft, vertex_tex).rgb;
        } else if (isRoadCornerBottomRight > 0.5) {
            color.rgb = texture(roadCornerBottomRight, vertex_tex).rgb;
        } else if (isRoadCornerTopLeft > 0.5) {
            color.rgb = texture(roadCornerTopLeft, vertex_tex).rgb;
        } else if (isRoadCornerTopRight > 0.5) {
            color.rgb = texture(roadCornerTopRight, vertex_tex).rgb;
        } else if (isRoad1WayBottom > 0.5) {
            color.rgb = texture(road1WayBottom, vertex_tex).rgb;
        }  else {
            color.rgb = vec3(0.3, 0.3, 0.3);
        }

        if (highBeams > 0.5) {
            highs = 1.0;
        }

        color.rgb *= vec3(0.3,0.3,0.3) * 0.1 + shadowFactor * highs;

    } else {
        color.rgb = texture(tex, vertex_tex).rgb;
        if (!(color.r > 0.5 && color.g > 0.5 && color.b > 0.5)) {
            color.rgb *= 0.1 + shadowFactor * highs;
        }
    }

//    float amount = 0;
//    for (int i = 0; i < 2; i++) {
//        float dist = distance(vertex_pos, lps[i]) * 2;
//        amount += 1.0/dist;
//    }

//    float dist = distance(vertex_pos, vec3(0,-15,0.6) - campos) * 10;
//    color.r += 1.0/dist;


//    color.b += amount;
    color.a = 1;
}
