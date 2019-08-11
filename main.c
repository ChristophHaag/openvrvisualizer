/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

/* OpenGL Test - Main Implementation */

#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "gl.h"
#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

#define degreesToRadians(angleDegrees) ((angleDegrees) * M_PI / 180.0)
#define radiansToDegrees(angleRadians) ((angleRadians) * 180.0 / M_PI)


#include <openvr/openvr_capi.h>
extern const char * VR_GetVRInitErrorAsSymbol(EVRInitError error);
extern intptr_t VR_GetGenericInterface(const char *pchInterfaceVersion, EVRInitError *peError);
extern intptr_t VR_InitInternal(EVRInitError *peError, EVRApplicationType eType);
struct VR_IVRSystem_FnTable* systemfn;
void check_error(int line, EVRInitError error) { if (error != 0) printf("%d: error %s\n", line, VR_GetVRInitErrorAsSymbol(error)); }


void GLAPIENTRY
gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
				  GLsizei length, const GLchar* message, const void* userParam)
{
	fprintf(stderr, "GL DEBUG CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
			(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
			type, severity, message);
}

float randf()
{
	return (float)rand() / (float)RAND_MAX;
}

static inline void
print_matrix(float m[])
{
	printf("[[%0.4f, %0.4f, %0.4f, %0.4f],\n"
	"[%0.4f, %0.4f, %0.4f, %0.4f],\n"
	"[%0.4f, %0.4f, %0.4f, %0.4f],\n"
	"[%0.4f, %0.4f, %0.4f, %0.4f]]\n",
	m[0], m[4], m[8], m[12],
	m[1], m[5], m[9], m[13],
	m[2], m[6], m[10], m[14],
	m[3], m[7], m[11], m[15]);
}

// cubes arranged in a circle around the user, 20° between them. 360°/20° = 18 cubes
mat4_t cube_modelmatrix[18];
vec3_t cube_colors[18];
float cube_alpha[18];

void gen__cubes()
{
	for(int i = 0; i < 18; i ++) {
		cube_modelmatrix[i] = m4_identity();
		cube_modelmatrix[i] = m4_mul(cube_modelmatrix[i], m4_rotation(degreesToRadians(i * 20), vec3(0, 1, 0)));
		cube_modelmatrix[i] = m4_mul(cube_modelmatrix[i], m4_translation(vec3(0, 1.8, -5)));
		cube_modelmatrix[i] = m4_mul(cube_modelmatrix[i], m4_rotation(degreesToRadians(randf() * 360), vec3(randf(), randf(), randf())));

		cube_colors[i] = vec3(randf(), randf(), randf());
		cube_alpha[i] = randf();
	}
}

void draw_cubes(GLuint shader)
{
	int modelLoc = glGetUniformLocation(shader, "model");
	int colorLoc = glGetUniformLocation(shader, "uniformColor");
	for(int i = 0; i < 18; i ++) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) cube_modelmatrix[i].m);
		glUniform4f(colorLoc, cube_colors[i].x, cube_colors[i].y, cube_colors[i].z, cube_alpha[i]);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// floor is 10x10m, 0.1m thick
	mat4_t floor = m4_identity();
	floor = m4_mul(floor, m4_scaling(vec3(10, 0.1, 10)));
	// we could move the floor to -1.8m height if the HMD tracker sits at zero
	// floor = m4_mul(floor, m4_translation(vec3(0, -1.8, 0)));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) floor.m);
	glUniform4f(colorLoc, 0, .4f, .25f, .9f);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void draw_hmd(GLuint shader, mat4_t *model_matrix)
{
	int modelLoc = glGetUniformLocation(shader, "model");
	int colorLoc = glGetUniformLocation(shader, "uniformColor");


	mat4_t scaled = m4_mul(*model_matrix, m4_scaling(vec3(1.0, 0.5, 0.25)));

	vec3_t hmd_color = vec3(1, 1, 1);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) scaled.m);
	glUniform4f(colorLoc, hmd_color.x, hmd_color.y, hmd_color.z, 1.0);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

/*
void draw_controllers(GLuint shader, ohmd_device *lc, ohmd_device *rc)
{
	int modelLoc = glGetUniformLocation(shader, "model");
	int colorLoc = glGetUniformLocation(shader, "uniformColor");

	mat4_t lcmodel;
	ohmd_device_getf(lc, OHMD_GL_MODEL_MATRIX, (float*) lcmodel.m);
	lcmodel = m4_mul(lcmodel, m4_scaling(vec3(0.03, 0.03, 0.1)));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) lcmodel.m);
	glUniform4f(colorLoc, 1.0, 0.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	mat4_t rcmodel;
	ohmd_device_getf(rc, OHMD_GL_MODEL_MATRIX, (float*) rcmodel.m);
	rcmodel = m4_mul(rcmodel, m4_scaling(vec3(0.03, 0.03, 0.1)));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) rcmodel.m);
	glUniform4f(colorLoc, 0.0, 1.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}
*/

mat4_t matrix34_to_mat4 (HmdMatrix34_t *mat34)
{
	/*
	return mat4(
		mat34->m[0][0], mat34->m[1][0], mat34->m[2][0], 0,
		mat34->m[0][1], mat34->m[1][1], mat34->m[2][1], 0,
		mat34->m[0][2], mat34->m[1][2], mat34->m[2][2], 0,
		mat34->m[0][3], mat34->m[1][3], mat34->m[2][3], 1
	);
	*/
	return mat4(
		mat34->m[0][0], mat34->m[0][1], mat34->m[0][2], mat34->m[0][3],
		mat34->m[1][0], mat34->m[1][1], mat34->m[1][2], mat34->m[1][3],
		mat34->m[2][0], mat34->m[2][1], mat34->m[2][2], mat34->m[2][3],
		0, 0, 0, 1
	);
}

int main(int argc, char** argv)
{
	int hmd_w = 1600;
	int hmd_h = 900;

	EVRInitError error;
	VR_InitInternal (&error, EVRApplicationType_VRApplication_Background);
	check_error(__LINE__, error);

	char fn_table_name[128];
	sprintf (fn_table_name, "FnTable:%s", IVRSystem_Version);
	systemfn = (struct VR_IVRSystem_FnTable*) VR_GetGenericInterface(fn_table_name, &error);
	check_error(__LINE__, error);

	gl_ctx gl;
	GLuint VAOs[2];
	GLuint appshader;
	init_gl(&gl, hmd_w, hmd_h, VAOs, &appshader);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_debug_callback, 0);

	GLuint texture;
	GLuint framebuffer;
	GLuint depthbuffer;
	for (int i = 0; i < 2; i++)
		create_fbo(hmd_w, hmd_h, &framebuffer, &texture, &depthbuffer);

	gen__cubes();

	SDL_ShowCursor(SDL_DISABLE);

	TrackedDevicePose_t poses[k_unMaxTrackedDeviceCount];

	bool done = false;
	while(!done){
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type == SDL_KEYDOWN){
				switch(event.key.keysym.sym){
					case SDLK_ESCAPE:
						done = true;
						break;
					default:
						break;
				}
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		glViewport(0, 0, hmd_w, hmd_h);
		//glScissor(0, 0, hmd_w, hmd_h);

		mat4_t projectionmatrix = m4_perspective(90, (float)hmd_w / (float)hmd_h, 0.001, 100);

		glUseProgram(appshader);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthbuffer, 0);

		glClearColor(0.0, 0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(VAOs[0]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);

		glUniformMatrix4fv(glGetUniformLocation(appshader, "proj"), 1, GL_FALSE, (GLfloat*) projectionmatrix.m);

		vec3_t from = vec3(0, 7.0, 0);
		vec3_t to = vec3(0, 0.0, -0.0001); // can't look at 0,0,0
		vec3_t up = vec3(0, 1, 0);
		mat4_t viewmatrix = m4_look_at(from, to, up);

		glUniformMatrix4fv(glGetUniformLocation(appshader, "view"), 1, GL_FALSE, (GLfloat*)viewmatrix.m);

		draw_cubes(appshader);
		//draw_controllers(appshader, lc, rc);

		systemfn->GetDeviceToAbsoluteTrackingPose(ETrackingUniverseOrigin_TrackingUniverseStanding, 0, poses, k_unMaxTrackedDeviceCount);
		TrackedDevicePose_t* openvr_hmd_pose = &poses[0];
		mat4_t hmd_modelmatrix = matrix34_to_mat4(&openvr_hmd_pose->mDeviceToAbsoluteTracking);
		draw_hmd(appshader, &hmd_modelmatrix);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBlitNamedFramebuffer(
			(GLuint)framebuffer, // readFramebuffer
			(GLuint)0,    // backbuffer     // drawFramebuffer
			(GLint)0,     // srcX0
			(GLint)0,     // srcY0
			(GLint)hmd_w,     // srcX1
			(GLint)hmd_h,     // srcY1
			(GLint)0,     // dstX0
			(GLint)0,     // dstY0
			(GLint)hmd_w, // dstX1
			(GLint)hmd_h, // dstY1
			(GLbitfield)GL_COLOR_BUFFER_BIT, // mask
			(GLenum)GL_LINEAR);              // filter

		// Da swap-dawup!
		SDL_GL_SwapWindow(gl.window);
		SDL_Delay(10);
	}


	return 0;
}
