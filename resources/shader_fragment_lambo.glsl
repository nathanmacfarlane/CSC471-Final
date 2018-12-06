#version 330 core
out vec4 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 vertex_normal;

uniform sampler2D texL;
uniform sampler2D tex2;
uniform float isBraking;
uniform vec3 campos;

void main()
{
    vec3 n = normalize(vertex_normal);
    vec3 lp=vec3(15, -20, -100);
    vec3 ld = normalize(vertex_pos - lp);
    float diffuse = dot(n,ld);

    color = texture(texL, vertex_tex);
    color *= diffuse * 2;

    color *= ((vertex_pos.z + 1.001)/2);

    if (isBraking > 0.5 && vertex_pos.z < -0.9) {
        color.r += 0.5;
    }
}
