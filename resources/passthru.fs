#version 430 core

uniform sampler2D pixtex;
uniform vec2 resolution;
out vec4 color;

void main()
{

	vec2 uv = gl_FragCoord.xy / resolution;

	color = texture(pixtex,uv);
}
