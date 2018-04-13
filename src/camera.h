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
	int w, a, s, d, q, e;
	camera()
	{
		w = a = s = d = q = e = 0;
		pos = rot = glm::vec3(0, 0, 0);
	}
	
	glm::mat4 getDirection()
	{
		glm::mat4 R = glm::mat4(1);
		R = glm::rotate(R, rot.z, glm::vec3(1, 0, 0)); // yaw
		R = glm::rotate(R, rot.y, glm::vec3(0, 1, 0)); // pitch
		return R;
	}
	
	void rotate(double yaw, double pitch)
	{
	  rot.z -= pitch;
	  rot.y += yaw;
	}
	
	void translate(glm::vec3 offset)
	{
	  pos += offset;
	}
	
	glm::vec3 viewVec()
	{
	  return glm::vec3(cos(rot.y)*cos(rot.z),
	    sin(rot.z),
	    sin(rot.y)*cos(rot.z));
	}
	
	glm::mat4 process()
	{
	  // zoom-based camera
		float distance_scalar = 1.0;
		float zVel = 0;
		float xVel = 0;
		
		const float moveConst = .1;
		if (q == 1)
			zoomLevel *= scaling_rate;
		if (e == 1)
			zoomLevel *= 1./scaling_rate;
		
		if (w == 1)
		  zVel += moveConst*zoomLevel;
		if (s == 1)
		  zVel -= moveConst*zoomLevel;
		
		if (a == 1)
		  xVel += moveConst*zoomLevel;
		if (d == 1)
			xVel -= moveConst*zoomLevel;
		/*
		if (a == 1)
			rot.y += 0.01;
		if (d == 1)
			rot.y -= 0.01;
	  */
			

		glm::mat4 view = glm::lookAt(pos, pos + viewVec(), glm::vec3(0, 1, 0));
		glm::vec4 movement = glm::transpose(view)*glm::vec4(zVel, 0, xVel, 0);
		pos+= zoomLevel*glm::vec3(movement);

		return view;
	}

};








#endif // LAB471_CAMERA_H_INCLUDED
