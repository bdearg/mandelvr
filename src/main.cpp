/* Lab 6 base code - transforms using local matrix functions
	to be written by students -
	based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
	& Ian Dunn, Christian Eckhardt
*/
#include <iostream>
#include <chrono>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "camera.h"
// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;

#define FPSBUFSIZE 15


class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> pixshader;

	//camera
	camera mycam;

	GLuint VertexArrayUnitPlane, VertexBufferUnitPlane;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{


		GLSL::checkVersion();

		
		// Set background color.
		glClearColor(0.12f, 0.34f, 0.56f, 1.0f);

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		//culling:
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		//transparency
		glEnable(GL_BLEND);
		//next function defines how to mix the background color with the transparent pixel in the foreground. 
		//This is the standard:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

		pixshader = make_shared<Program>();
		pixshader->setVerbose(true);
		pixshader->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/IQ_mandelbulb_derivative.fs");
		if (!pixshader->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		pixshader->init();
		pixshader->addAttribute("vertPos");
		pixshader->addUniform("resolution");
		pixshader->addUniform("time");


	}

	void initGeom(const std::string& resourceDirectory)
	{

		glGenVertexArrays(1, &VertexArrayUnitPlane);
		glBindVertexArray(VertexArrayUnitPlane);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferUnitPlane);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferUnitPlane);

		GLfloat *ver = new GLfloat[6 * 3];
		// front
		int verc = 0;

		ver[verc++] = 0.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
		ver[verc++] = 1.0, ver[verc++] = 1.0, ver[verc++] = 0.0;
		ver[verc++] = 0.0, ver[verc++] = 1.0, ver[verc++] = 0.0;
		ver[verc++] = 0.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
		ver[verc++] = 1.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
		ver[verc++] = 1.0, ver[verc++] = 1.0, ver[verc++] = 0.0;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), ver, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	}

	void render()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);
		
		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		pixshader->bind();
		glUniform2f(pixshader->getUniform("resolution"), static_cast<float>(width), static_cast<float>(width));
		glUniform1f(pixshader->getUniform("time"), glfwGetTime());

		glBindVertexArray(VertexArrayUnitPlane);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		pixshader->unbind();
		
	}

};
//*********************************************************************************************************
int main(int argc, char **argv)
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}


	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(2160, 1200);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	chrono::microseconds fpsbuffer[FPSBUFSIZE];
	size_t fpsoff = 0;

	
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		auto start = chrono::steady_clock::now();
		// Render scene.
		application->render();
		
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		auto stop = chrono::steady_clock::now();
		// Poll for and process events.
		glfwPollEvents();

		fpsbuffer[(++fpsoff) % FPSBUFSIZE] = chrono::duration_cast<chrono::microseconds>(stop - start);
		fpsoff %= FPSBUFSIZE;
		double avgfps = 0.0; for (int i = 0; i < FPSBUFSIZE; i++) { avgfps += 1e6 / fpsbuffer[i].count(); } avgfps /= FPSBUFSIZE;
		cout << "Frame: " << fpsbuffer[fpsoff].count() / 1e3 << "ms, FPS: " << 1e6 / fpsbuffer[fpsoff].count() << ", FPS(avg): " << avgfps << endl;
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}


