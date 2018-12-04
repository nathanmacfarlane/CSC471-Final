#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 lightSpace;
uniform mat4 M;

out vec3 vertex_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;
out vec4 fragLightSpacePos;

void main()
{
    vertex_tex = vertTex;
	vertex_pos = (M * vec4(vertPos, 1.0)).xyz;
	vertex_normal = (M * vec4(vertNor, 0.0)).xyz;
	fragLightSpacePos = lightSpace * M * vec4(vertPos, 1.0);

	gl_Position = P * V * M * vec4(vertPos, 1.0);
}
