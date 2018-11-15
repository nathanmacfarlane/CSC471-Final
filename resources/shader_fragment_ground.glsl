#version 330 core
out vec3 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec3 vertex_col;
in vec2 vertex_tex;

uniform sampler2D tex;

void main()
{
//    color = vertex_pos;
    color = vec3(0.2,0.2,0.2);
//    color = texture(tex, vertex_tex).rgb;
}
