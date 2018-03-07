#include "VRplayer.hpp"

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <openvr.h>
#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/ext.hpp"

using namespace std;
using namespace glm;

static mat4 vraffine_to_glm(vr::HmdMatrix34_t vrmat);
static mat4 correct_matrix_order(vr::HmdMatrix44_t vrmat);

VRplayer::VRplayer(vr::IVRSystem* playervrsystem, vr::ETrackingUniverseOrigin eorigin, const vec3& startloc, const double startscale) : 
	playerVRSystem(playervrsystem),
	trackingOrigin(eorigin),
	worldPosition(startloc),
	scale(startscale) {

}

const vr::TrackedDevicePose_t& VRplayer::getRawHeadsetPose() const{
	return(HMDPose);
}

mat4 VRplayer::getHeadsetPose() const {
	if (!playerVRSystem) {
		return(mat4(0.0));
	}
	mat4 hmdpose = vraffine_to_glm(HMDPose.mDeviceToAbsoluteTracking);

	return(hmdpose);
}

void VRplayer::playerWaitGetPoses(){
	vr::VRCompositor()->WaitGetPoses(&HMDPose, 1, nullptr, 0);

	if(!HMDPose.bPoseIsValid){
		fprintf(stderr, "Warning! OpenVR says the headset pose is invalid!\n");
	}
}

mat4 VRplayer::getLeftEyeView() const{
	return(getEyeView(vr::Eye_Left));
}

mat4 VRplayer::getRightEyeView() const{
	return(getEyeView(vr::Eye_Right));
}

mat4 VRplayer::getEyeView(vr::Hmd_Eye eye) const{
	if(!playerVRSystem){
		return(mat4(0.0));
	}
	mat4 hmdpose = inverse(vraffine_to_glm(HMDPose.mDeviceToAbsoluteTracking));
	mat4 eyepose = inverse(vraffine_to_glm(playerVRSystem->GetEyeToHeadTransform(eye)));

	return(eyepose*hmdpose);
}

mat4 VRplayer::getHeadView() const
{
	if (!playerVRSystem) {
		return(mat4(0.0));
	}
	mat4 hmdpose = inverse(vraffine_to_glm(HMDPose.mDeviceToAbsoluteTracking));

	return(hmdpose);
}

mat4 VRplayer::getEyeViewProj(vr::Hmd_Eye eye) const{
	if(!playerVRSystem){
		return(mat4(0.0));
	}
	mat4 hmdpose = inverse(vraffine_to_glm(HMDPose.mDeviceToAbsoluteTracking));
	mat4 eyepose = inverse(vraffine_to_glm(playerVRSystem->GetEyeToHeadTransform(eye)));
	mat4 P = correct_matrix_order(playerVRSystem->GetProjectionMatrix(eye, .01, 100.0));

	return(P*eyepose*hmdpose);
}

mat4 VRplayer::getEyeProj(vr::Hmd_Eye eye) const
{
	if (!playerVRSystem) {
		return(mat4(0.0));
	}
	return(correct_matrix_order(playerVRSystem->GetProjectionMatrix(eye, .01, 100.0)));
}

void VRplayer::setSeatedMode(){
	trackingOrigin = vr::TrackingUniverseSeated;
}
void VRplayer::setStandingMode(){
	trackingOrigin = vr::TrackingUniverseStanding;
}

void VRplayer::playerControlsTick(GLFWwindow * window, double dt){
	mat4 view = getHeadView();
	vec3 rightDir = normalize(view[0]) * toMat4(rotationOffset);
	vec3 upDir = normalize(view[1]) * toMat4(rotationOffset);
	vec3 viewDir = normalize(view[2]) * toMat4(rotationOffset);

	float movementscalar = static_cast<float>(scale*dt);

	if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
		int joycount, buttoncount;
		const float* joyinput = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &joycount);
		const unsigned char* buttoninput = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttoncount);

		vec3 planar_movement = rightDir * joyinput[0] + viewDir * joyinput[1];
		vec3 vertical_movement = upDir * joyinput[3] * movementscalar;
		float rotation_factor = -joyinput[2] * .2f;
		vec3 final_planar = planar_movement == vec3(0.0) || glm::all(isnan(planar_movement)) ? vec3(0.0) : normalize(planar_movement) * movementscalar;

		worldPosition += final_planar;
		worldPosition += glm::all(isnan(vertical_movement)) ? vec3(0.0) : vertical_movement;

		if (fabs(rotation_factor) > .05 && !isnan(rotation_factor) && !isinf(rotation_factor)) {
			// rotationOffset *= angleAxis(rotation_factor * static_cast<float>(dt), upDir);
		}

		float shrinkFactor = (joyinput[4] + 1.0)*.5;
		float growFactor = (joyinput[5] + 1.0)*.5;
		float scalefactor = (shrinkFactor > growFactor) ? mix(1.0, .5, dt) : mix(1.0, 2.0, dt);
		if (shrinkFactor + growFactor > .05 && !isnan(scalefactor) && !isinf(scalefactor)) {
			scale = scale * scalefactor;
		}
		
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		worldPosition += viewDir * movementscalar;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		worldPosition += -viewDir * movementscalar;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		worldPosition += rightDir * movementscalar;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		worldPosition += -rightDir * movementscalar;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		worldPosition += upDir * movementscalar;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		worldPosition += -upDir * movementscalar;
	}
}

const glm::vec3 & VRplayer::getPositionOffset() const{
	return(worldPosition);
}

const glm::mat4 & VRplayer::getRotationOffset() const
{
	return(toMat4(rotationOffset));
}

double VRplayer::getPlayerScale() const{
	return(scale);
}
void VRplayer::setPlayerScale(double newscale){
	scale = newscale;
}
void VRplayer::shrinkPlayer(double rate, double dt){
	scale = scale*(rate/dt);
}
void VRplayer::growPlayer(double rate, double dt){
	scale = scale*(dt/rate);
}

vec3 VRplayer::extractViewDir(const mat4 & view)
{
	return(view[2]);
}

static mat4 vraffine_to_glm(vr::HmdMatrix34_t vrmat){
	return(mat4(
		vrmat.m[0][0], vrmat.m[1][0], vrmat.m[2][0], 0.0,
		vrmat.m[0][1], vrmat.m[1][1], vrmat.m[2][1], 0.0,
		vrmat.m[0][2], vrmat.m[1][2], vrmat.m[2][2], 0.0,
		vrmat.m[0][3], vrmat.m[1][3], vrmat.m[2][3], 1.0f
	));
}

static mat4 correct_matrix_order(vr::HmdMatrix44_t vrmat){
	return mat4(
		vrmat.m[0][0], vrmat.m[1][0], vrmat.m[2][0], vrmat.m[3][0],
		vrmat.m[0][1], vrmat.m[1][1], vrmat.m[2][1], vrmat.m[3][1], 
		vrmat.m[0][2], vrmat.m[1][2], vrmat.m[2][2], vrmat.m[3][2], 
		vrmat.m[0][3], vrmat.m[1][3], vrmat.m[2][3], vrmat.m[3][3]
	);
}