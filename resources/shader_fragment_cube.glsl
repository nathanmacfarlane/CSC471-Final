#version 330 core
out vec3 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec3 vertex_col;

void main()
{
    color = vec3(0,0,normalize(vertex_pos).y);
}
