#version 430 core

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;
layout(location = 2) in vec3 vertNor;

out vec2 uv;
flat out int textureLayer;

uniform mat4 MVP;

void main()
{
  gl_Position = MVP * vec4(vertPos, 1.);
  
  // this gives us interpolation
  uv = vertTex;
  textureLayer = int(floor(vertNor.z + 0.5));
}

