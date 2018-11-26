#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec3 lamp_pos;
uniform vec3 campos;

void main()
{
    vec3 n = normalize(vertex_normal);
    vec3 lp = vec3(lamp_pos.x-2.0, 0.3, lamp_pos.z-0.9);
    vec3 ld = normalize(lp - vertex_pos);
    float diff = max(dot(n, ld), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 0.7);
    color = vec4(diffuse, 1.0) * 0.2;

}
