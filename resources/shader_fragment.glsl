#version 330 core
out vec3 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
uniform vec3 campos;
void main()
{
vec3 n = normalize(vertex_normal);
vec3 lp=vec3(10, 10, 100);
vec3 ld = normalize(lp - vertex_pos);
float diff = max(dot(n, ld), 0.0);
vec3 diffuse = diff * vec3(0.6, 0.6, 0.6);
color = diffuse;

//color = vertex_pos;
}
