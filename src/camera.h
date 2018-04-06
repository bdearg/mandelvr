#pragma once


#ifndef LAB471_CAMERA_H_INCLUDED
#define LAB471_CAMERA_H_INCLUDED

#include <stack>
#include <memory>

#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"





class camera
{
public:
	glm::vec3 pos;
	glm::vec3 rot;
	
	float zoomLevel = 1.0;
	
	const float scaling_rate = 0.98;
	int w, a, s, d;
	camera()
	{
		w = a = s = d = 0;
		pos = rot = glm::vec3(0, 0, 0);
	}
	
	void rotate(double yaw, double pitch)
	{
	  rot.z += pitch;
	  rot.y += yaw;
	}
	
	void translate(glm::vec3 offset)
	{
	  pos += offset;
	}
	
	glm::mat4 process()
	{
	  // zoom-based camera
		float distance_scalar = 1.0;
		if (w == 1)
			zoomLevel *= scaling_rate;
		if (s == 1)
			zoomLevel *= 1./scaling_rate;
			
		if (a == 1)
			rot.y += 0.01;
		if (d == 1)
			rot.y -= 0.01;
		glm::mat4 R = glm::mat4(1);
		R = glm::rotate(R, rot.z, glm::vec3(1, 0, 0)); // yaw
		R = glm::rotate(R, rot.y, glm::vec3(0, 1, 0)); // pitch

		//glm::vec4 rpos = glm::vec4(0, 0, going_forward, 1);
//		rpos = R *rpos;
//		pos.x += -rpos.x;
//		pos.z += rpos.z;

		glm::mat4 T = glm::translate(glm::mat4(1), zoomLevel*glm::vec3(pos.x, pos.y, pos.z));
		return R*T;
	}

};








#endif // LAB471_CAMERA_H_INCLUDED
