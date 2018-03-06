#include "VRplayer.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <openvr.h>
#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

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

const vr::TrackedDevicePose_t& VRplayer::getHeadsetPose() const{
	return(HMDPose);
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

void VRplayer::setSeatedMode(){
	trackingOrigin = vr::TrackingUniverseSeated;
}
void VRplayer::setStandingMode(){
	trackingOrigin = vr::TrackingUniverseStanding;
}

void VRplayer::playerControlsTick(GLFWwindow * window, double dt){
	mat4 view = getHeadView();
	vec3 rightDir = normalize(view[0]);
	vec3 upDir = normalize(view[1]);
	vec3 viewDir = normalize(view[2]);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		worldPosition += viewDir * static_cast<float>(scale*dt);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		worldPosition += -viewDir * static_cast<float>(scale*dt);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		worldPosition += rightDir * static_cast<float>(scale*dt);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		worldPosition += -rightDir * static_cast<float>(scale*dt);
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		worldPosition += upDir * static_cast<float>(scale*dt);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		worldPosition += -upDir * static_cast<float>(scale*dt);
	}
}

const glm::vec3 & VRplayer::getPositionOffset() const{
	return(worldPosition);
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