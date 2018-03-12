#version 430 core 

// Dead simple fragment shader to show 2D pos by color

uniform vec2 resolution;

out vec4 color;

void main(){

	vec2 uv = gl_FragCoord.xy / resolution;

	color = vec4(uv.x, uv.y, length(uv.xy), 1.0);
}
