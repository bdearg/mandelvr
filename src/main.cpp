/* Lab 6 base code - transforms using local matrix functions
	to be written by students -
	based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
	& Ian Dunn, Christian Eckhardt
*/
#include <iostream>
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






class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog,prog2;

	// Shape to be used (from obj file)
	shared_ptr<Shape> shape;
	
	//camera
	camera mycam;

	//texture for sim
	GLuint Texture;
	GLuint Texture2,Texture3,Texture4,FBOtex,fb,depth_rb;

	GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;
	
	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID;

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


		prog2 = make_shared<Program>();
		prog2->setVerbose(true);
		prog2->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_nolight.glsl");
		if (!prog2->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog2->init();
		prog2->addUniform("P");
		prog2->addUniform("V");
		prog2->addUniform("M");
		prog2->addAttribute("vertPos");
		prog2->addAttribute("vertTex");

	
	


	}

	void initGeom(const std::string& resourceDirectory)
	{

		glGenVertexArrays(1, &VertexArrayIDBox);
		glBindVertexArray(VertexArrayIDBox);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);

		GLfloat *ver = new GLfloat[10000 * 18];
		// front
		int verc = 0;

		ver[verc++] = 0.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
		ver[verc++] = 1.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
		ver[verc++] = 0.0, ver[verc++] = 1.0, ver[verc++] = 0.0;
		ver[verc++] = 1.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
		ver[verc++] = 1.0, ver[verc++] = 1.0, ver[verc++] = 0.0;
		ver[verc++] = 0.0, ver[verc++] = 1.0, ver[verc++] = 0.0;


		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), ver, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferTex);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);

		float t = 1. / 100.;
		GLfloat *cube_tex = new GLfloat[10000 * 12];
		int texc = 0;

		cube_tex[texc++] = 0, cube_tex[texc++] = 0;
		cube_tex[texc++] = 1, cube_tex[texc++] = 0;
		cube_tex[texc++] = 0, cube_tex[texc++] = 1;
		cube_tex[texc++] = 1, cube_tex[texc++] = 0;
		cube_tex[texc++] = 1, cube_tex[texc++] = 1;
		cube_tex[texc++] = 0, cube_tex[texc++] = 1;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), cube_tex, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(2);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);


	}

	void render()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		auto P = std::make_shared<MatrixStack>();
		P->pushMatrix();	
		P->perspective(70., width, height, 0.1, 100.0f);
		glm::mat4 M,V,S,T;		
		
		V = mycam.process();
		V = glm::mat4(1);
		
		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		prog2->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FBOtex);
		M = glm::scale(glm::mat4(1),glm::vec3(1.2,1,1)) * glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5, -1));
		glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog2->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		prog2->unbind();
		
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
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.

		application->render();
		
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}


