#version 430 core 
out vec4 color;
in vec2 fragTex;
layout(location = 0) uniform sampler2D tex;

void main()
{
	color.b=0;
	color.rg = fragTex;
	color.a=1;
}
