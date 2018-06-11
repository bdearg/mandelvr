#pragma once
#ifndef VRPLAYER_H_
#define VRPLAYER_H_

#define GLM_ENABLE_EXPERIMENTAL
#include <openvr.h>
#include <GLFW\glfw3.h>
#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

using namespace glm;
using namespace std;

class VRplayer{
public:
	VRplayer(vr::IVRSystem* playervrsystem) : playerVRSystem(playervrsystem) { initControllers(); }
	VRplayer(vr::IVRSystem* playervrsystem, vr::ETrackingUniverseOrigin eorigin, const vec3& startloc, const double startscale);

	void initControllers();

	void playerWaitGetPoses();
	
	const vr::TrackedDevicePose_t& getRawHeadsetPose() const;
	mat4 getHeadsetPose() const;
	mat4 getLeftEyeView() const;
	mat4 getRightEyeView() const;
	mat4 getEyeView(vr::Hmd_Eye eye) const;
	mat4 getHeadView() const;
	mat4 getEyeViewProj(vr::Hmd_Eye eye) const;
	mat4 getEyeProj(vr::Hmd_Eye eye) const;

	void setSeatedMode();
	void setStandingMode();
	
	void playerControlsTick(GLFWwindow* window, double dt);

	const glm::vec3& getPositionOffset() const;
	const glm::mat4& getRotationOffset() const;
	double getPlayerScale() const;
	void setPlayerScale(double newscale);
	void shrinkPlayer(double rate, double dt);
	void growPlayer(double rate, double dt);

	long double getFocusMult();
	void setFocusMult(long double focus);
	void shrinkFocus(double rate, double dt);
	void growFocus(double rate, double dt);

	static vec3 extractViewDir(const mat4& view);

	void resetView();

	enum ScaleMode{AUTOMATIC, MANUAL};

protected:
	vr::TrackedDevicePose_t HMDPose;
	int32_t VRplayer::getAxisFromController(vr::TrackedDeviceIndex_t ctrlr, vr::EVRControllerAxisType axistype);

private:
	vr::IVRSystem* playerVRSystem = nullptr;
	vr::ETrackingUniverseOrigin trackingOrigin = vr::TrackingUniverseRawAndUncalibrated;

	ScaleMode scalingMode = MANUAL; 

	vr::TrackedDeviceIndex_t vr_controllers[2];
	int lhandJoystickAxis = -1, rhandJoystickAxis = -1, lhandSqueezeAxis = -1, rhandSqueezeAxis = -1;

	vec3 worldPosition = vec3(0.0);
	quat rotationOffset = glm::quat(glm::vec3(glm::radians(60.), 0., glm::radians(-45.)));
	long double scale = 1.0;
	long double focusMult = 1.0;
};

#endif