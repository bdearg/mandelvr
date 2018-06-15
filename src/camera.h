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
	
	float velocityFactor = 1.0;
	
	const float scaling_rate = 1.02;
	int w, a, s, d, q, e;
	camera()
	{
		w = a = s = d = q = e = 0;
		pos = glm::vec3(0, 0, 0);
		pitch = yaw = 0.;
	}
	
	bool anyButtonPressed()
	{
	  return w || a || s || d || q || e;
	}
	
	bool noButtonsPressed()
	{
	  return !w && !a && !s && !d && !q && !e;
	}
	
	void rotate(double dyaw, double dpitch)
	{
	  pitch = glm::clamp(pitch - dpitch, -glm::pi<double>()/2. + 1e-3, glm::pi<double>()/2. - 1e-3);
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
	    -cos(pitch)*cos(yaw)
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
		return glm::lookAt(pos, (pos) + getForward(), getUp());
  }
  
  glm::vec3 xMovement()
  {
    return getRight();
  }
  
  glm::vec3 zMovement()
  {
    return getForward();
  }
	
	glm::mat4 process(float dt)
	{
	  // tune movement to 1/60 of a second
	  // dt is in seconds
	  float time_scale = dt/(1./60.);
	  
	  // zoom-based camera
		float zVel = 0;
		float xVel = 0;
		
		const float moveConst = .01;
		if (q == 1)
	  {
			zoomLevel *= scaling_rate;
		}
		if (e == 1)
		{
			zoomLevel = glm::max(1.f, zoomLevel/scaling_rate);
		}
		
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

		pos += velocityFactor*xVel*xMovement();
		pos += velocityFactor*zVel*zMovement();

		return getView();
	}

};

#endif // LAB471_CAMERA_H_INCLUDED
