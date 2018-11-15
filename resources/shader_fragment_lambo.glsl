#version 330 core
out vec3 color;
in vec3 vertex_pos;

void main()
{
    color = vec3(1.0,1.0,1.0);
    color = vertex_pos;
}
