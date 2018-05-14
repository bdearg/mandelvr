#version 430 core

uniform sampler2DArray sphereMap;

in vec2 uv;
flat in int textureLayer;

out vec4 color;

void main()
{
  color = texture(sphereMap, vec3(uv, textureLayer));
}
