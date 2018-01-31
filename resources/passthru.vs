#version 430 core
layout(location = 0) in vec4 vertPos;

void main()
{
	gl_Position = vec4(vertPos.xy*2.0 - 1.0, 0.0, 1.0);
}
