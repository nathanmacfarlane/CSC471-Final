#version 330 core
out vec3 color;

in vec3 vPos;

void main()
{
    if (vPos.y > 0.85 && vPos.y < 0.9 && vPos.x < 0.1) {
        color = vec3(1.0, 1.0, 0.8);
    } else {
        color = vec3(0.0, 0.0, 0.0);
    }
}
