#version 430 core

uniform sampler2D leftEye;
uniform sampler2D rightEye;
uniform vec2 vr_resolution;
out vec4 color;

void main()
{
	vec2 uv = gl_FragCoord.xy / (vr_resolution*.5);
	float leftMask = step(uv.x, 1.0);
	float rightMask = step(1.0, uv.x);

	vec3 col = leftMask*texture(leftEye,uv).rgb + rightMask*texture(rightEye, fract(uv)).rgb;

	color = vec4(col, 1.0);
}
