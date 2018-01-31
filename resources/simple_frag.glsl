#version 430 core 

out vec4 color;

in vec3 pos;
in vec2 fragTex;
in vec3 fragNor;
in vec3 fragBinorm;
in vec3 fragTan;

uniform vec3 campos;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex2;


void main()
{
	vec3 normal = normalize(fragNor);

	vec3 texturecolor = texture(tex, fragTex).rgb;
	vec3 texturenormal = texture(tex2, fragTex).rgb;
	texturenormal = (texturenormal - vec3(0.5,0.5,0.5))*2.0;

	// Calculate the normal from the data in the bump map.
    vec3 bumpNormal = (texturenormal.x * fragTan) + (texturenormal.y * fragBinorm) + (texturenormal.z * normal);
	bumpNormal = normalize(bumpNormal);

	//diffuse light
	vec3 lp = vec3(100,100,100);
	vec3 ld = normalize(lp - pos);
	float light = dot(ld,bumpNormal);	
	light = clamp(light,0,1);

	//specular light
	vec3 camvec = normalize(campos - pos);
	vec3 h = normalize(camvec+ld);
	float spec = pow(dot(h,bumpNormal),5);
	spec = clamp(spec,0,1);
	
	color.rgb = texturecolor *light + vec3(1,1,1)*spec;

	color.a=1;
}
