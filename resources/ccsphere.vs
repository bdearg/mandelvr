#version 430 core

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;

out vec2 uv

uniform mat4 VP;

void main()
{
  gl_Position = VP * vec4(vertPos, 1.);
  
  // this gives us interpolation
  uv = vertTex;
}

