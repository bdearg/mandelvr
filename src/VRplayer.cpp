#include "VRplayer.hpp"

#include <iostream>
#include <sstream>
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

#define CONTROLLER_LHAND 0
#define CONTROLLER_RHAND 1

#define VRBUTTON_OCULUS_A static_cast<vr::EVRButtonId>(7)
#define VRBUTTON_OCULUS_B static_cast<vr::EVRButtonId>(1)
#define VRBUTTON_OCULUS_X static_cast<vr::EVRButtonId>(7)
#define VRBUTTON_OCULUS_Y static_cast<vr::EVRButtonId>(1)

static mat4 vraffine_to_glm(vr::HmdMatrix34_t vrmat);
static mat4 correct_matrix_order(vr::HmdMatrix44_t vrmat);

static string propErrorToString(vr::TrackedPropertyError err)
{
	switch (err)
	{
	case vr::TrackedProp_Success:
		return "No error";
	case vr::TrackedProp_BufferTooSmall:
		return "Buffer too small";
	case vr::TrackedProp_WrongDataType:
		return "Wrong data type";
	case vr::TrackedProp_InvalidDevice:
		return "Invalid device";
	case vr::TrackedProp_InvalidOperation:
		return "Invalid operation";
	case vr::TrackedProp_NotYetAvailable:
		return "Not yet available";
	case vr::TrackedProp_ValueNotProvidedByDevice:
		return "Device does not provide that value";
	case vr::TrackedProp_WrongDeviceClass:
		return "Wrong device class";
	case vr::TrackedProp_UnknownProperty:
		return "Unknown property";
	}
	stringstream errstring;
	errstring << "unknown or NYI error(" << static_cast<int>(err) << ")";

	return errstring.str();
}

VRplayer::VRplayer(vr::IVRSystem* playervrsystem, vr::ETrackingUniverseOrigin eorigin, const vec3& startloc, const double startscale) : 
	playerVRSystem(playervrsystem),
	trackingOrigin(eorigin),
	worldPosition(startloc),
	scale(startscale) {
	initControllers();
}

const vr::TrackedDevicePose_t& VRplayer::getRawHeadsetPose() const{
	return(HMDPose);
}

int32_t VRplayer::getAxisFromController(vr::TrackedDeviceIndex_t ctrlr, vr::EVRControllerAxisType axistype)
{
	int32_t result;
	for (int32_t i = 0; i < vr::Prop_Axis3Type_Int32 - vr::Prop_Axis0Type_Int32; i++)
	{
		vr::TrackedPropertyError err;
		int32_t axisType = playerVRSystem->GetInt32TrackedDeviceProperty(ctrlr,
			static_cast<vr::ETrackedDeviceProperty>(vr::Prop_Axis0Type_Int32 + i), &err);
		if (err != vr::TrackedProp_Success)
		{
			cout << "Property error: " << propErrorToString(err) << endl;
			abort();
		}
		if (axisType == axistype)
		{
			result = i;
		}
	}
	return result;
}

void VRplayer::initControllers()
{
	bool ok = true;
	vr_controllers[CONTROLLER_LHAND] = playerVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
	vr_controllers[CONTROLLER_RHAND] = playerVRSystem->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
	if (vr_controllers[CONTROLLER_LHAND] == vr::k_unTrackedDeviceIndexInvalid)
	{
		cout << "Left hand controller not found." << endl;
		ok = false;
	}
	if (vr_controllers[CONTROLLER_RHAND] == vr::k_unTrackedDeviceIndexInvalid)
	{
		cout << "Right hand controller not found." << endl;
		ok = false;
	}
	if(!ok)
	{
		return;
	}
	//return; // need code to make this more compatible with other VR setups
	useVRcontrollers = true;

	lhandJoystickAxis = getAxisFromController(vr_controllers[CONTROLLER_LHAND], vr::k_eControllerAxis_Joystick);
	rhandJoystickAxis = getAxisFromController(vr_controllers[CONTROLLER_RHAND], vr::k_eControllerAxis_Joystick);
	lhandSqueezeAxis = getAxisFromController(vr_controllers[CONTROLLER_LHAND], vr::k_eControllerAxis_Trigger);
	rhandSqueezeAxis = getAxisFromController(vr_controllers[CONTROLLER_RHAND], vr::k_eControllerAxis_Trigger);

	if (lhandJoystickAxis == -1 || rhandJoystickAxis == -1
		|| lhandSqueezeAxis == -1 || rhandSqueezeAxis == -1)
	{
		cout << "Could not find the needed joystick axes!!" << endl;
		assert(false);
		exit(1);
	}
	else
	{
		cout << "lhand: " << vr_controllers[CONTROLLER_LHAND] << ", rhand: " << vr_controllers[CONTROLLER_RHAND] << endl;
	}
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
	mat4 rot = toMat4(rotationOffset);
	mat4 rviewTp = glm::inverse(glm::transpose(view * rot));

	// cam: eyeview
	// rotationoffset

	vec3 rightDir = normalize(rviewTp[0]);
	vec3 upDir = normalize(rviewTp[1]);
	vec3 viewDir = normalize(rviewTp[2]);

	const float movementInhibitor = 0.25;
	float movementscalar = static_cast<float>(movementInhibitor*scale*dt);

	vr::VRControllerState_t lhand, rhand;
	float jsAxis1X = 0., jsAxis1Y = 0., jsAxis2X = 0., jsAxis2Y = 0., boostAxis1 = 0., boostAxis2 = 0.;
	bool b1Held = false, b2Held = false;
	bool b3Held = false, b4Held = false;

	if (!playerVRSystem->IsTrackedDeviceConnected(vr_controllers[CONTROLLER_LHAND]) ||
		!playerVRSystem->IsTrackedDeviceConnected(vr_controllers[CONTROLLER_RHAND]))
	{
		initControllers();
	}

	if (useVRcontrollers)
	{
		if (playerVRSystem->IsTrackedDeviceConnected(vr_controllers[CONTROLLER_LHAND]))
		{
			playerVRSystem->GetControllerState(vr_controllers[CONTROLLER_LHAND], &lhand, sizeof(lhand));
			jsAxis1X = max(min(lhand.rAxis[lhandJoystickAxis].x, 1.f), -1.f);
			jsAxis1Y = max(min(lhand.rAxis[lhandJoystickAxis].y, 1.f), -1.f);
			boostAxis1 = max(min(lhand.rAxis[lhandSqueezeAxis].x, 1.f), 0.f);
			b1Held = lhand.ulButtonPressed & vr::ButtonMaskFromId(VRBUTTON_OCULUS_A);
			b3Held = lhand.ulButtonPressed & vr::ButtonMaskFromId(VRBUTTON_OCULUS_B);
		}
		if (playerVRSystem->IsTrackedDeviceConnected(vr_controllers[CONTROLLER_RHAND]))
		{
			playerVRSystem->GetControllerState(vr_controllers[CONTROLLER_RHAND], &rhand, sizeof(rhand));
			jsAxis2X = max(min(rhand.rAxis[lhandJoystickAxis].x, 1.f), -1.f);
			jsAxis2Y = max(min(rhand.rAxis[lhandJoystickAxis].y, 1.f), -1.f);
			boostAxis2 = max(min(rhand.rAxis[rhandSqueezeAxis].x, 1.f), 0.f);
			b2Held = rhand.ulButtonPressed & vr::ButtonMaskFromId(VRBUTTON_OCULUS_X);
			b4Held = rhand.ulButtonPressed & vr::ButtonMaskFromId(VRBUTTON_OCULUS_Y);
		}
	}
	else if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
		int joycount, buttoncount;
		const float* joyinput = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &joycount);
		const unsigned char* buttoninput = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttoncount);

		jsAxis1X = joyinput[0];
		jsAxis1Y = joyinput[1]; 
		jsAxis2X = joyinput[2];
		jsAxis2Y = joyinput[3];

		b1Held = buttoninput[0];
		b2Held = buttoninput[1];
		b3Held = buttoninput[2];
		b4Held = buttoninput[3];
	}

	movementscalar *= glm::mix(1., 100., boostAxis1) * glm::mix(1., 100., boostAxis2);

	vec3 planar_movement = rightDir * jsAxis1X + viewDir * jsAxis1Y;
	if (glm::any(glm::isnan(planar_movement)) || glm::any(glm::isinf(planar_movement)))
	{
		planar_movement = vec3(0.);
	}
	vec3 vertical_movement =  upDir * jsAxis2Y * movementscalar;
	if (glm::any(glm::isnan(vertical_movement)) || glm::any(glm::isinf(vertical_movement)))
	{
		vertical_movement = vec3(0.);
	}
	vec3 final_planar = (planar_movement == vec3(0.0)) ? vec3(0.0) : normalize(planar_movement) * movementscalar;

	worldPosition += glm::any(isnan(final_planar)) ? vec3(0.0) : final_planar;
	worldPosition += glm::any(isnan(vertical_movement)) ? vec3(0.0) : vertical_movement;

	float rotation_factor = -jsAxis2X * .6f;
	rotation_factor *= glm::mix(1., 100., boostAxis1) * glm::mix(1., 100., boostAxis2);

	if (fabs(rotation_factor) > .05 && !isnan(rotation_factor) && !isinf(rotation_factor)) {
		 rotationOffset *= angleAxis(rotation_factor * static_cast<float>(dt), upDir);
	}

	float shrinkFactor = b1Held ? 1. : 0.;
	float growFactor = b2Held ? 1. : 0.;
	float scalefactor = (shrinkFactor > growFactor) ? mix(1.0, .5, dt) : mix(1.0, 2.0, dt);
	if (shrinkFactor + growFactor > .05 && !isnan(scalefactor) && !isinf(scalefactor)) {
		scale = scale * scalefactor;
		scale = glm::clamp(scale, 1e-10L, 20.L);
	}

	float fshrinkFactor = b3Held ? 1. : 0.;
	float fgrowFactor = b4Held ? 1. : 0.;
	float fscalefactor = (fshrinkFactor > fgrowFactor) ? mix(1.0, .5, dt) : mix(1.0, 2.0, dt);
	if (fshrinkFactor + fgrowFactor > .05 && !isnan(fscalefactor) && !isinf(fscalefactor)) {
		focusMult *= fscalefactor;
		focusMult = glm::clamp(focusMult, 1e-10L, 20.L);
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
	return(glm::toMat4(rotationOffset));
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

long double VRplayer::getFocusMult()
{
	return focusMult;
}

void VRplayer::setFocusMult(long double focus)
{
	focusMult = focus;
}

void VRplayer::shrinkFocus(double rate, double dt)
{
	focusMult *= (rate / dt);
}

void VRplayer::growFocus(double rate, double dt)
{
	focusMult *= (dt / rate);
}

vec3 VRplayer::extractViewDir(const mat4 & view)
{
	return(view[2]);
}


void VRplayer::resetView()
{
	rotationOffset = glm::quat(glm::vec3(glm::radians(0.), 0., glm::radians(0.)));
	worldPosition = vec3(0.f, 0., -2.);
	scale = 1.;
	focusMult = 1.;
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