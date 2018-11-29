#version 330 core

out vec4 color;

in vec2 fragTex;


uniform sampler2D tex;

void main()
{
	vec3 texturecolor = texture(tex, fragTex).rgb;
	color.rgb = texturecolor;
	color.a = 1.0;
}
