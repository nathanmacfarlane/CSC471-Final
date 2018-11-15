#version 330 core
out vec3 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 vertex_normal;

uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{
//    color = vertex_pos;
    color = texture(tex, vertex_tex).rgb;
//    vec3 n = normalize(vertex_normal);
//    vec3 lp=vec3(10,0,-100);
//    vec3 ld = normalize(vertex_pos - lp);
//    float diffuse = dot(n,ld);
//
//    color = texture(tex, vertex_tex).rgb;
//
//    vec3 cd = normalize(vertex_pos - campos);
//    vec3 h = normalize(cd+ld);
//    float spec = dot(n,h);
//    spec = clamp(spec,0,1);
//    spec = pow(spec,25);
//    color += color*spec*1;
//
//    color *= diffuse*0.7;
//
//    if (color.r < 0.1 && color.g < 0.1 && color.b < 0.1) {
//        color += texture(tex2, vertex_tex).rgb;
//    }
//    else {
//        color = texture(tex2, vertex_tex).rgb + color*spec*1;
//    }
}
