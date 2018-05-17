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
	double pitch;
	double yaw;
	
	float zoomLevel = 1.0;
	
	const float scaling_rate = 0.98;
	int w, a, s, d, q, e;
	camera()
	{
		w = a = s = d = q = e = 0;
		pos = glm::vec3(0, 0, 0);
		pitch = yaw = 0.;
	}
	
	void rotate(double dyaw, double dpitch)
	{
	  pitch = glm::clamp(pitch + dpitch, -glm::pi<double>()/4., glm::pi<double>()/4.);
	  yaw = glm::mod(yaw + dyaw,2*glm::pi<double>());
	}
	
	void translate(glm::vec3 offset)
	{
	  pos += offset;
	}
	
	glm::vec3 getForward()
	{
	  return glm::normalize(glm::vec3(
	    cos(pitch)*sin(yaw),
	    sin(pitch),
	    cos(pitch)*cos(yaw)
	  ));
	}
	
	glm::vec3 getUp()
	{
	  return glm::normalize(glm::cross(getRight(), getForward()));
	}
  
  glm::vec3 getRight()
  {
    // cross forward with global up to get the vector to the right
    return glm::normalize(glm::cross(getForward(), glm::vec3(0, 1, 0)));
  }
	
	glm::mat4 getView()
	{
		return glm::lookAt(pos, pos + getForward(), getUp());
  }
  
  glm::vec3 xMovement()
  {
    return getRight();
  }
  
  glm::vec3 zMovement()
  {
    return getForward();
  }
	
	glm::mat4 process()
	{
	  // zoom-based camera
		float distance_scalar = 1.0;
		float zVel = 0;
		float xVel = 0;
		
		const float moveConst = .01;
		if (q == 1)
			zoomLevel *= scaling_rate;
		if (e == 1)
			zoomLevel *= 1./scaling_rate;
		
		if (w == 1)
		  zVel -= moveConst;
		if (s == 1)
		  zVel += moveConst;
		
		if (a == 1)
		  xVel -= moveConst;
		if (d == 1)
			xVel += moveConst;
		/*
		if (a == 1)
			rot.y += 0.01;
		if (d == 1)
			rot.y -= 0.01;
	  */

		pos += zoomLevel*xVel*xMovement();
		pos += zoomLevel*zVel*zMovement();

		return getView();
	}

};

#endif // LAB471_CAMERA_H_INCLUDED
