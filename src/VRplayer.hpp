#pragma once
#ifndef VRPLAYER_H_
#define VRPLAYER_H_

#include <openvr.h>
#include <GLFW\glfw3.h>
#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace glm;
using namespace std;

class VRplayer{
public:
	VRplayer(vr::IVRSystem* playervrsystem) : playerVRSystem(playervrsystem) {}
	VRplayer(vr::IVRSystem* playervrsystem, vr::ETrackingUniverseOrigin eorigin, const vec3& startloc, const double startscale);

	void playerWaitGetPoses();
	
	const vr::TrackedDevicePose_t& getHeadsetPose() const;
	mat4 getLeftEyeView() const;
	mat4 getRightEyeView() const;
	mat4 getEyeView(vr::Hmd_Eye eye) const;
	mat4 getHeadView() const;
	mat4 getEyeViewProj(vr::Hmd_Eye eye) const;

	void setSeatedMode();
	void setStandingMode();
	
	void playerControlsTick(GLFWwindow* window, double dt);

	const glm::vec3& getPositionOffset() const;
	double getPlayerScale() const;
	void setPlayerScale(double newscale);
	void shrinkPlayer(double rate, double dt);
	void growPlayer(double rate, double dt);

	static vec3 extractViewDir(const mat4& view);


	enum ScaleMode{AUTOMATIC, MANUAL};

protected:
	vr::TrackedDevicePose_t HMDPose;

private:
	vr::IVRSystem* playerVRSystem = nullptr;
	vr::ETrackingUniverseOrigin trackingOrigin = vr::TrackingUniverseRawAndUncalibrated;

	ScaleMode scalingMode = MANUAL; 

	vec3 worldPosition = vec3(0.0);
	double scale = 1.0; 
};

#endif