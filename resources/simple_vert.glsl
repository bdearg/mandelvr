#version 430 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
layout(location = 3) in vec3 vertTan;
layout(location = 4) in vec3 vertBinorm;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec3 fragNor;
out vec2 fragTex;
out vec3 fragTan;
out vec3 fragBinorm;
out vec3 pos;

void main()
{
pos=vertPos.xyz;
gl_Position = P * V * M * vertPos;

fragNor = (M * vec4(vertNor, 0.0)).xyz;
fragTan = (M * vec4(vertTan, 0.0)).xyz;
fragBinorm = (M * vec4(vertBinorm, 0.0)).xyz;

fragTex = vertTex;
fragTex.y*=-1;
}
