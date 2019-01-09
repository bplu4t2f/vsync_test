// Need this to bypass the retarded main redirect logic
#define SDL_MAIN_HANDLED

#include <stdio.h>
//#include <GL/glew.h>
#include <SDL.h>
//#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
//#include <gl/GL.h>
//#include <gl/GLU.h>
#include <SDL_opengl.h>

#include <dwmapi.h>
#include <limits>

#define PERFCMS_IMPLEMENTATION
#include "perfcms.h"



extern "C"
{
#include "d3dkmt_wait_for_veritcal_blank.h"
}

extern int main_d3d11();

static int main_opengl();
// Should use D3D9 by default
static int main_sdl();
static int main_vbos();
static int sdl_rotate();

#include <sstream>

#include "sformat.h"



int main(int argc, char **argv)
{
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	main_d3d11();
}


//
// This version uses OpenGL directly to do all the drawing.
//


//#include <d3d9.h>

static_assert(sizeof(int) == 4, "use a sane compiler");
static_assert(sizeof(float) == 4, "use a sane compiler");
static_assert(sizeof(double) == 8, "use a sane compiler");
static_assert(sizeof(short) == 2, "use a sane compiler");

static HANDLE vsync_mre;
static Uint64 last_vsync;

template<class T>
void logi(_Printf_format_string_ const char *format, T arg)
{
	to_string(std::stringstream(), arg);
	sprintf_s(ctx.log_buf, format, arg);
}


static DWORD WINAPI vsync_thread_proc_sdl(LPVOID param)
{
	SDL_Window *dummy_window = SDL_CreateWindow("vsync_dummy", 0, 0, 0, 0, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
	if (dummy_window == NULL)
	{
		printf("vsync window failed %s\r\n", SDL_GetError());
		return 1;
	}

	SDL_GLContext context = SDL_GL_CreateContext(dummy_window);
	if (context == NULL)
	{
		printf("vsync context failed %s\n", SDL_GetError());
		return 1;
	}

	if (SDL_GL_SetSwapInterval(-1) != 0)
	{
		printf("vsync set swap interval failed %s\n", SDL_GetError());
		return 1;
	}

	while (1)
	{
		SDL_GL_SwapWindow(dummy_window);
		last_vsync = SDL_GetPerformanceCounter();
		if (!PulseEvent(vsync_mre))
		{
			printf("PulseEvent failure %x\r\n", GetLastError());
		}
	}
}

static DWORD WINAPI vsync_thread_proc_existing_window(LPVOID param)
{
	SDL_Window *dummy_window = (SDL_Window *)param;

	if (dummy_window == NULL)
	{
		printf("vsync window failed %s\r\n", SDL_GetError());
		return 1;
	}

	SDL_GLContext context = SDL_GL_CreateContext(dummy_window);
	if (context == NULL)
	{
		printf("vsync context failed %s\n", SDL_GetError());
		return 1;
	}

	if (SDL_GL_SetSwapInterval(-1) != 0)
	{
		printf("vsync set swap interval failed %s\n", SDL_GetError());
		return 1;
	}

	while (1)
	{
		SDL_GL_SwapWindow(dummy_window);
		if (!PulseEvent(vsync_mre))
		{
			printf("PulseEvent failure %x\r\n", GetLastError());
		}
	}
}

static void make_dummy_window(SDL_Window *main_window)
{
	vsync_mre = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE thread = CreateThread(NULL, 0, vsync_thread_proc_sdl, main_window, 0, NULL);
	if (thread == INVALID_HANDLE_VALUE)
	{
		printf("vsync thread creation failed\r\n");
	}
}






static SDL_GLContext context;
static SDL_Window *window;


int main_opengl()
{
	Uint64 sw;

	sw = SDL_GetPerformanceCounter();
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("sdl init failed %s\n", SDL_GetError());
		//return 1;
	}
	printf("SDL_Init OK: %lf\n", get_elapsed_ms(sw));

	sw = SDL_GetPerformanceCounter();
#if 0
	// This block seems to have no effect on the problem
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

#if 1
	// Enabling multisampling seems to improve the issue, but it's not good enough yet.
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1); // @TODO doesn't seem to be necessary
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
#endif

#if 1
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
	printf("gl attribute garbage OK: %lf\n", get_elapsed_ms(sw));

	sw = SDL_GetPerformanceCounter();
	window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		printf("window failed %s\n", SDL_GetError());
		return 1;
	}
	printf("SDL_CreateWindow OK: %lf\n", get_elapsed_ms(sw));

	//SDL_SetWindowFullscreen(window, 1);

	sw = SDL_GetPerformanceCounter();
	context = SDL_GL_CreateContext(window);
	if (context == NULL)
	{
		printf("context failed %s\n", SDL_GetError());
		return 1;
	}
	printf("SDL_GL_CreateContext OK: %lf\n", get_elapsed_ms(sw));

#if 0
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
		return 1;
	}

	if (SDL_GL_MakeCurrent(window, context) != 0)
	{
		printf("make current failed %s \n", SDL_GetError());
		//return 1;
	}
#endif

	

#if 1
	// "Enable vsync"
	sw = SDL_GetPerformanceCounter();
	if (SDL_GL_SetSwapInterval(1) != 0)
	{
		printf("set swap interval failed %s\n", SDL_GetError());
		return 1;
	}
	printf("SDL_GL_SetSwapInterval OK: %lf\n", get_elapsed_ms(sw));
#endif

#if 0
	if (SDL_GL_MakeCurrent(window, NULL) != 0)
	{
		printf("make current failed %s \n", SDL_GetError());
		//return 1;
	}
#endif

	//SDL_CreateThread(ThreadProc, "thethread", NULL);

	int frame_counter = 0;

	Uint64 performance_frequency = SDL_GetPerformanceFrequency();
	Uint64 last_tick = SDL_GetPerformanceCounter();
	Uint64 performance_start = last_tick;



	//
	// create a test framebuffer
	//

	//GLuint fbo;
	//glGenFramebuffers(1, &fbo);
	//glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	/*unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/

	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	/*D3DKMTWFVB_STATE d3dkmtwfvb_state;
	if (d3dkmt_wait_for_vertical_blank_init(&d3dkmtwfvb_state) != S_OK)
	{
		printf("d3dkmt_wait_for_vertical_blank_init failed\r\n");
		return 1;
	}*/


	//make_dummy_window(window);
	
	sw = SDL_GetPerformanceCounter();
	GLsync (APIENTRY *glFenceSync)(int, int) = (GLsync(APIENTRY *)(int, int)) wglGetProcAddress("glFenceSync");
	if (glFenceSync == nullptr)
	{
		printf("glFenceSync doesn't exist.");
	}

	PFNGLDELETESYNCPROC glDeleteSync = (PFNGLDELETESYNCPROC)wglGetProcAddress("glDeleteSync");
	if (glDeleteSync == nullptr)
	{
		printf("glDeleteSync doesn't exist.");
	}

	void (APIENTRY *glClientWaitSync)(GLsync, GLbitfield, GLuint64) = (void (APIENTRY *)(GLsync, GLbitfield, GLuint64)) wglGetProcAddress("glClientWaitSync");
	if (glClientWaitSync == nullptr)
	{
		printf("glClientWaitSync doesn't exist.");
	}

	void (APIENTRY *glWaitSync)(GLsync, GLbitfield, GLuint64) = (void (APIENTRY *)(GLsync, GLbitfield, GLuint64)) wglGetProcAddress("glWaitSync");
	if (glClientWaitSync == nullptr)
	{
		printf("glWaitSync doesn't exist.");
	}
	printf("fence shit OK: %lf\n", get_elapsed_ms(sw));

	

	
	bool held_down = false;

	int mouse_x = 0, mouse_y = 0;

	bool enable_dwm_flush = false;
	bool enable_glFinish = false;
	bool enableGlClientWaitSync = false;

	while (1)
	{
		bool glFinishOnce = false;
		bool extraWait = false;

		mouse_x = (frame_counter * 8) % 800;
		if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)
		{
			SDL_WarpMouseInWindow(window, mouse_x, 300);
		}

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				goto _quit;
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				held_down = true;
				SDL_CaptureMouse(SDL_TRUE);
			}
			else if (event.type == SDL_MOUSEBUTTONUP)
			{
				held_down = false;
				SDL_CaptureMouse(SDL_FALSE);
			}
			else if (event.type == SDL_MOUSEMOTION)
			{
				if (held_down)
				{
					//printf("motion %d %d\r\n", event.motion.x, event.motion.y);
				}
				int mouse_x_tmp = event.motion.x;
				mouse_y = event.motion.y;
			}
			else if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.scancode == SDL_SCANCODE_Q)
				{
					goto _quit;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F)
				{
					glFinishOnce = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F1)
				{
					enable_dwm_flush = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F2)
				{
					enable_dwm_flush = false;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F3)
				{
					enable_glFinish = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F4)
				{
					enable_glFinish = false;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F5)
				{
					if (SDL_GL_SetSwapInterval(1) != 0)
					{
						printf("set swap interval 1 failed %s\n", SDL_GetError());
					}
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F6)
				{
					if (SDL_GL_SetSwapInterval(0) != 0)
					{
						printf("set swap interval 0 failed %s\n", SDL_GetError());
					}
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F7)
				{
					enableGlClientWaitSync = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F8)
				{
					enableGlClientWaitSync = false;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_W)
				{
					extraWait = true;
				}
			}
		}
		//continue;



		++frame_counter;
		//printf("frame: %d\r\n", frame_counter);

		double frame_counter_float;
		Uint64 milli_frame_counter;

		{
			Uint64 tmp = last_tick;
			last_tick = SDL_GetPerformanceCounter();
			Uint64 elapsed = last_tick - tmp;
			double elapsed_ms = (double)elapsed * 1000.0 / (double)performance_frequency;
			if (elapsed_ms < 15.0 || elapsed_ms > 17.0)
			{
				//printf("A %d : %.3lf\r\n", frame_counter, elapsed_ms);
			}

			Uint64 total_elapsed_ms = ((last_tick - performance_start) * 1000ULL + performance_frequency / 2) / performance_frequency;
#define MONITOR_REFRESH_RATE_HZ 60
			//frame_counter = (total_elapsed_ms * MONITOR_REFRESH_RATE_HZ + 500) / 1000;

			Uint64 total_elapsed_us = ((last_tick - performance_start) * 1000000ULL + performance_frequency / 2) / performance_frequency;
			milli_frame_counter = (total_elapsed_us * MONITOR_REFRESH_RATE_HZ + 500) / 1000;
			frame_counter_float = (double)milli_frame_counter / 1000.0;
		}


		int fc_mod4 = frame_counter % 4;
		// "Divide-remainder" by the number of colored rects you want
		int fc_mod8 = frame_counter % 8;


		//glBindFramebuffer(GL_FRAMEBUFFER, fbo);

#if 1
		//if (frame_counter & 1)
		{
			glClearColor(60.0f / 255.0f, 50.0f / 255.0f, 75.0f / 255.0f, 0.0f);
		}
		//else
		{
			//	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		}
#else
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
#endif
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 1



		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);


		/*
		//if (frame_counter & 1)
		{
		glColor3f(60.0f / 255.0f, 50.0f / 255.0f, 75.0f / 255.0f);
		}
		//else
		{
		//glColor3f(0.0f, 0.0f, 0.0f);
		}

		glBegin(GL_QUADS);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 600, 0);
		glVertex3f(800, 600, 0);
		glVertex3f(800, 0, 0);
		glEnd();
		*/



		// Draw 8 square columns:
		float r = fc_mod4 == 0 || fc_mod4 == 3 ? 1.0f : 0.0f;
		float g = fc_mod4 != 3 ? 1.0f : 0.0f;
		float b = fc_mod4 == 1 ? 1.0f : 0.0f;

		glColor3f(r, g, b);

		// This is ridiculous - if we render 20000 rects, everything is OK, if we render
		// only 20, everything is horrible.
		int num_rects = ((frame_counter / 120) & 1) ? 20000 : 20;

		glBegin(GL_QUADS);
		for (int i = 0; i < num_rects; ++i)
		{
			glVertex3f(10.0f + fc_mod8 * 20.0f, 10.0f + i * 20.0f, 0.0f);
			glVertex3f(10.0f + fc_mod8 * 20.0f, 20.0f + i * 20.0f, 0.0f);
			glVertex3f(20.0f + fc_mod8 * 20.0f, 20.0f + i * 20.0f, 0.0f);
			glVertex3f(20.0f + fc_mod8 * 20.0f, 10.0f + i * 20.0f, 0.0f);
		}
		glEnd();


		glPushMatrix();
		glTranslatef(400.0f, 300.0f, 0.0f);
		double angle = (double)(milli_frame_counter % 360000) / 1000.0;
		glRotatef((float)angle, 0.0f, 0.0f, 1.0f);



		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glColor4f(1.0f, 1.0f, 0.0f, 0.8f);


		glBegin(GL_QUADS);
		for (int i = 0; i < 1; ++i)
		{
			glVertex3f(100.0f, 100.0f, 0.0f);
			glVertex3f(120.0f, 200.0f, 0.0f);
			glVertex3f(220.0f, 200.0f, 0.0f);
			glVertex3f(200.0f, 110.0f, 0.0f);
		}
		glEnd();
		glPopMatrix();
#endif

#if 0
		// Alternative way to fix it (instead of drawing many rects): Call this often
		for (int i = 0; i < 10000; ++i)
		{
			glFlush();
		}
#endif

		// mouse "cursor"
		glBegin(GL_TRIANGLES);
		glVertex3f(mouse_x, mouse_y, 0.0f);
		glVertex3f(mouse_x, mouse_y + 20, 0.0f);
		glVertex3f(mouse_x + 20, mouse_y, 0.0f);
		glEnd();

		glFlush();

		

		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glLoadIdentity();
		//glBindTexture(GL_TEXTURE_2D, texture);
		//glBegin(GL_QUADS);
		//glVertex3f(0, 0, 0);
		//glVertex3f(0, 600, 0);
		//glVertex3f(800, 600, 0);
		//glVertex3f(800, 0, 0);
		//glEnd();

		//glFlush();


		{
			Uint64 sw = SDL_GetPerformanceCounter();
			SDL_GL_SwapWindow(window);
			Uint64 after = SDL_GetPerformanceCounter();
			double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
			double delay_to_vsync = (double)(after - last_vsync) * 1000.0 / (double)performance_frequency;
			//if (frame_counter < 15)
			{
				printf("swap call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
			}
			if (frame_counter < 60)
			{
				//printf("delay to vsync: %d, %.4lf\r\n", frame_counter, delay_to_vsync);
			}
		}
		//GLsync gl_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		//if (gl_sync == 0)
		//{
		//	printf("glFenceSync failed: %d\n", glGetError());
		//}
		if (enable_glFinish || glFinishOnce)
		{
			Uint64 sw = SDL_GetPerformanceCounter();
			glFinish();
			glFinish();
			glFinish();
			glFinish();
			glFinish();
			Uint64 after = SDL_GetPerformanceCounter();
			double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
			double delay_to_vsync = (double)(after - last_vsync) * 1000.0 / (double)performance_frequency;
			//if (frame_counter < 15)
			{
				printf("glFinish call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
			}
		}
		//if (enableGlClientWaitSync)
		//{
		//	Uint64 sw = SDL_GetPerformanceCounter();
		//	glClientWaitSync(gl_sync, 0, GL_TIMEOUT_IGNORED);
		//	Uint64 after = SDL_GetPerformanceCounter();
		//	double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
		//	double delay_to_vsync = (double)(after - last_vsync) * 1000.0 / (double)performance_frequency;
		//	//if (frame_counter < 15)
		//	{
		//		printf("glClientWaitSync call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
		//	}
		//}
		//glDeleteSync(gl_sync);
		if (false)
		{
			Uint64 sw = SDL_GetPerformanceCounter();
			WaitForSingleObject(vsync_mre, 1000);
			Uint64 after = SDL_GetPerformanceCounter();
			double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
			double delay_to_vsync = (double)(after - last_vsync) * 1000.0 / (double)performance_frequency;
			if (frame_counter < 60)
			{
				printf("WaitForSingleObject call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
				printf("delay to vsync: %d, %.4lf\r\n", frame_counter, delay_to_vsync);
			}
		}
		if (enable_dwm_flush && !glFinishOnce)
		{
			Uint64 sw = SDL_GetPerformanceCounter();
			DwmFlush();
			if (extraWait)
			{
				DwmFlush();
			}
			//vsyncThread(NULL);
			Uint64 after = SDL_GetPerformanceCounter();
			double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
			double delay_to_vsync = (double)(after - last_vsync) * 1000.0 / (double)performance_frequency;
			//if (frame_counter < 6000)
			{
				printf("DwmFlush call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
				//printf("delay to vsync: %d, %.4lf\r\n", frame_counter, delay_to_vsync);
			}
		}
#if 0
		if (false)
		{
			Uint64 sw = SDL_GetPerformanceCounter();
			//DwmFlush();
			if (d3dkmt_wait_for_vertical_blank(&d3dkmtwfvb_state) != S_OK)
			{
				printf("d3dkmt_wait_for_vertical_blank failed\r\n");
			}
			Uint64 after = SDL_GetPerformanceCounter();
			double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
			if (frame_counter < 60)
			{
				printf("vsyncThread call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
			}
		}
#endif
		if (false)
		{
			Uint64 tmp = last_tick;
			last_tick = SDL_GetPerformanceCounter();
			Uint64 elapsed = last_tick - tmp;
			double elapsed_ms = (double)elapsed * 1000.0 / (double)performance_frequency;
			if (elapsed_ms < 15.0 || elapsed_ms > 17.0)
			{
				printf("B %d : %.3lf\r\n", frame_counter, elapsed_ms);
			}

			Uint64 total_elapsed_ms = ((last_tick - performance_start) * 1000ULL + performance_frequency / 2) / performance_frequency;
#define MONITOR_REFRESH_RATE_HZ 60
			//frame_counter = (total_elapsed_ms * MONITOR_REFRESH_RATE_HZ + 500) / 1000;

			Uint64 total_elapsed_us = ((last_tick - performance_start) * 1000000ULL + performance_frequency / 2) / performance_frequency;
			milli_frame_counter = (total_elapsed_us * MONITOR_REFRESH_RATE_HZ + 500) / 1000;
			frame_counter_float = (double)milli_frame_counter / 1000.0;
		}

		//SDL_GL_SwapWindow(window);

		if (false)
		{
			Uint64 tmp = last_tick;
			last_tick = SDL_GetPerformanceCounter();
			Uint64 elapsed = last_tick - tmp;
			double elapsed_ms = (double)elapsed * 1000.0 / (double)performance_frequency;
			if (elapsed_ms < 15.0 || elapsed_ms > 17.0)
			{
				printf("B %d : %.3lf\r\n", frame_counter, elapsed_ms);
			}

			Uint64 total_elapsed_ms = ((last_tick - performance_start) * 1000ULL + performance_frequency / 2) / performance_frequency;
#define MONITOR_REFRESH_RATE_HZ 60
			//frame_counter = (total_elapsed_ms * MONITOR_REFRESH_RATE_HZ + 500) / 1000;

			Uint64 total_elapsed_us = ((last_tick - performance_start) * 1000000ULL + performance_frequency / 2) / performance_frequency;
			milli_frame_counter = (total_elapsed_us * MONITOR_REFRESH_RATE_HZ + 500) / 1000;
			frame_counter_float = (double)milli_frame_counter / 1000.0;
		}

		//SDL_GL_SwapWindow(window);

		if (false)
		{
			Uint64 tmp = last_tick;
			last_tick = SDL_GetPerformanceCounter();
			Uint64 elapsed = last_tick - tmp;
			double elapsed_ms = (double)elapsed * 1000.0 / (double)performance_frequency;
			if (elapsed_ms < 15.0 || elapsed_ms > 17.0)
			{
				printf("B %d : %.3lf\r\n", frame_counter, elapsed_ms);
			}

			Uint64 total_elapsed_ms = ((last_tick - performance_start) * 1000ULL + performance_frequency / 2) / performance_frequency;
#define MONITOR_REFRESH_RATE_HZ 60
			//frame_counter = (total_elapsed_ms * MONITOR_REFRESH_RATE_HZ + 500) / 1000;

			Uint64 total_elapsed_us = ((last_tick - performance_start) * 1000000ULL + performance_frequency / 2) / performance_frequency;
			milli_frame_counter = (total_elapsed_us * MONITOR_REFRESH_RATE_HZ + 500) / 1000;
			frame_counter_float = (double)milli_frame_counter / 1000.0;
		}

		//SDL_GL_SwapWindow(window);

		//GL_NO_ERROR

#if 0
		auto error = glGetError();
		if (error != 0)
		{
			printf("error: %x\r\n", error);
		}
#endif


		//SDL_Delay(7);






#if 0
		auto errors = glGetError();
		if (errors != 0)
		{
			printf("ERROR %x\r\n", (int)errors);
		}
#endif

	}

_quit:

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	getc(stdin);

	return 0;
}


#if 1
//
// This version uses SDL to do all the drawing, and doesn't use any OpenGL API directly
//

int main_sdl()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		return 1;
	}

	int selected_renderer_index = -1;
	int num_render_drivers = SDL_GetNumRenderDrivers();
	for (int i = 0; i < num_render_drivers; ++i)
	{
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		printf("Available renderer %d: %s\r\n", i, info.name);
#if 0
		// Use #if 0 to use the default, which should be d3d, and #if 1 to use Open GL with SDL abstraction.
		if (!strcmp("opengl", info.name))
		{
			printf("^ using this renderer\r\n");
			selected_renderer_index = i;
		}
#endif
	}

	Uint32 sdl_renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
#if 1
	// enable vsync
	sdl_renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
#endif
	SDL_Renderer *renderer = SDL_CreateRenderer(window, selected_renderer_index, sdl_renderer_flags);
	if (renderer == NULL)
	{
		return 1;
	}

	printf("Init OK\r\n");

	int frame_counter = 0;

	Uint64 performance_frequency = SDL_GetPerformanceFrequency();
	Uint64 last_tick = SDL_GetPerformanceCounter();

	SDL_Texture *texTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 800, 600);

	while (1)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				goto _quit;
			}
		}

		++frame_counter;

		//frame_counter = 0;

		{
			Uint64 tmp = last_tick;
			last_tick = SDL_GetPerformanceCounter();
			Uint64 elapsed = last_tick - tmp;
			double elapsed_ms = (double)elapsed * 1000.0 / (double)performance_frequency;
			//if (elapsed_ms < 15.0 || elapsed_ms > 17.0)
			{
				printf("%.3lf\r\n", elapsed_ms);
			}
		}

		int fc_mod4 = frame_counter % 4;
		int fc_mod8 = frame_counter % 8;

		if (frame_counter & 1)
		{
			SDL_SetRenderDrawColor(renderer, 60, 50, 75, 255);
		}
		else
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		}
		SDL_RenderClear(renderer);

#if 1
		// Draw 8 square columns:
#define RECT_COUNT 20
		SDL_Rect rects[RECT_COUNT];
		Uint8 r = fc_mod4 == 0 || fc_mod4 == 3 ? 255 : 0;
		Uint8 g = fc_mod4 != 3 ? 255 : 0;
		Uint8 b = fc_mod4 == 1 ? 255 : 0;
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
		for (int i = 0; i < RECT_COUNT; ++i)
		{
			rects[i].x = 10 + fc_mod8 * 20;
			rects[i].y = 10 + i * 20;
			rects[i].w = 10;
			rects[i].h = 10;
		}
		SDL_RenderFillRects(renderer, rects, RECT_COUNT);
#endif

		SDL_SetRenderTarget(renderer, texTarget);

		SDL_RenderClear(renderer);
		SDL_Rect rect1;
		rect1.x = 500;
		rect1.y = 100;
		rect1.w = 40;
		rect1.h = 30;
		SDL_RenderFillRect(renderer, &rect1);

		SDL_SetRenderTarget(renderer, NULL);

		SDL_RenderCopy(renderer, texTarget, NULL, &rect1);

		SDL_RenderPresent(renderer);
	}

_quit:

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

#endif

#if 0
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
const char *fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\n\0";

int main_vbos()
{
	// settings

	// glfw: initialize and configure
	// ------------------------------
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	SDL_Window *window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_WIDTH, SCR_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if (window == NULL)
	{
		return -1;
	}

	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (context == NULL)
	{
		return 1;
	}

#if 0
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
		return 1;
	}
#endif

	if (SDL_GL_MakeCurrent(window, context) != 0)
	{
		return 1;
	}

#if 0
	// "Enable vsync"
	if (SDL_GL_SetSwapInterval(1) != 0)
	{
		return 1;
	}
#endif

	// build and compile our shader program
	// ------------------------------------
	// vertex shader
	int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		return 1;
		//std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// fragment shader
	int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// check for shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		return 1;
		//std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// link shaders
	int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		return 1;
		//std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
#define NUM_TRIANGLES 100
#define NUM_VERTICES_PER_TRIANGLE 3
#define NUM_FLOATS_PER_VERTEX 3
	float vertices[NUM_TRIANGLES * NUM_VERTICES_PER_TRIANGLE * NUM_FLOATS_PER_VERTEX];
	for (int i = 0; i < NUM_TRIANGLES; ++i)
	{
		int cursor = 0;
		vertices[i * 9 + cursor++] = 0.5f;
		vertices[i * 9 + cursor++] = 0.5f;
		vertices[i * 9 + cursor++] = 0.0f;

		vertices[i * 9 + cursor++] = 0.5f;
		vertices[i * 9 + cursor++] = -0.5f;
		vertices[i * 9 + cursor++] = 0.0f;

		vertices[i * 9 + cursor++] = -0.5f;
		vertices[i * 9 + cursor++] = -0.5f;
		vertices[i * 9 + cursor++] = 0.0f;
	}

#if 0
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,  // first Triangle
		1, 2, 3   // second Triangle
	};
#endif
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

#if 0
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
#endif

	glVertexAttribPointer(0, NUM_FLOATS_PER_VERTEX, GL_FLOAT, GL_FALSE, NUM_FLOATS_PER_VERTEX * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);


	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	int frame_counter = 0;

	// render loop
	// -----------
	while (1)
	{
		// input
		// -----
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				goto _end;
			}
		}

		++frame_counter;

		// render
		// ------
		if (frame_counter & 1)
		{
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		}
		else
		{
			glClearColor(0, 0, 0, 1.0f);
		}
		glClear(GL_COLOR_BUFFER_BIT);

		// draw our first triangle
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

#if 1
		glDrawArrays(GL_TRIANGLES, 0, NUM_TRIANGLES * NUM_VERTICES_PER_TRIANGLE);
#else
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
#endif
		// glBindVertexArray(0); // no need to unbind it every time 

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------

		SDL_GL_SwapWindow(window);

#if 0
		auto errors = glGetError();
		if (errors != 0)
		{
			printf("ERROR %x\r\n", (int)errors);
		}
#endif

		SDL_Delay(10);
	}

_end:

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	return 0;
}




static int sdl_rotate()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		return 1;
	}

	int selected_renderer_index = -1;
	int num_render_drivers = SDL_GetNumRenderDrivers();
	for (int i = 0; i < num_render_drivers; ++i)
	{
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		printf("Available renderer %d: %s\r\n", i, info.name);
#if 0
		// Use #if 0 to use the default, which should be d3d, and #if 1 to use Open GL with SDL abstraction.
		if (!strcmp("opengl", info.name))
		{
			printf("^ using this renderer\r\n");
			selected_renderer_index = i;
		}
#endif
	}

	Uint32 sdl_renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
#if 0
	// enable vsync
	sdl_renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
#endif
	SDL_Renderer *renderer = SDL_CreateRenderer(window, selected_renderer_index, sdl_renderer_flags);
	if (renderer == NULL)
	{
		return 1;
	}

	printf("Init OK\r\n");

	int frame_counter = 0;

	Uint64 performance_frequency = SDL_GetPerformanceFrequency();
	Uint64 last_tick = SDL_GetPerformanceCounter();
	Uint64 performance_start = last_tick;

	SDL_Texture *texTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 800, 600);

	while (1)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				goto _quit;
			}
		}

		++frame_counter;

		Uint64 total_elapsed_ms = ((last_tick - performance_start) * 1000ULL + performance_frequency / 2) / performance_frequency;
#define MONITOR_REFRESH_RATE_HZ 60
		frame_counter = (total_elapsed_ms * MONITOR_REFRESH_RATE_HZ + 500) / 1000;

		{
			Uint64 tmp = last_tick;
			last_tick = SDL_GetPerformanceCounter();
			Uint64 elapsed = last_tick - tmp;
			double elapsed_ms = (double)elapsed * 1000.0 / (double)performance_frequency;
			if (elapsed_ms < 15.0 || elapsed_ms > 17.0)
			{
				//printf("%.3lf\r\n", elapsed_ms);
			}
		}

		int fc_mod4 = frame_counter % 4;
		int fc_mod8 = frame_counter % 8;

		if (frame_counter & 1)
		{
			SDL_SetRenderDrawColor(renderer, 60, 50, 75, 255);
		}
		else
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		}
		SDL_RenderClear(renderer);

#if 1
		// Draw 8 square columns:
#define RECT_COUNT 20
		SDL_Rect rects[RECT_COUNT];
		Uint8 r = fc_mod4 == 0 || fc_mod4 == 3 ? 255 : 0;
		Uint8 g = fc_mod4 != 3 ? 255 : 0;
		Uint8 b = fc_mod4 == 1 ? 255 : 0;
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
		for (int i = 0; i < RECT_COUNT; ++i)
		{
			rects[i].x = 10 + fc_mod8 * 20;
			rects[i].y = 10 + i * 20;
			rects[i].w = 10;
			rects[i].h = 10;
		}
		SDL_RenderFillRects(renderer, rects, RECT_COUNT);
#endif

		SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
		SDL_Rect rect2;
		rect2.w = 60;
		rect2.h = 40;
		rect2.x = 400 + (int)(cos(frame_counter / 180.0 * 3.14) * 200);
		rect2.y = 300 + (int)(sin(frame_counter / 180.0 * 3.14) * 200);
		SDL_RenderFillRect(renderer, &rect2);

#if 0
		SDL_SetRenderTarget(renderer, texTarget);

		SDL_RenderClear(renderer);
		SDL_Rect rect1;
		rect1.x = 500;
		rect1.y = 100;
		rect1.w = 40;
		rect1.h = 30;
		SDL_RenderFillRect(renderer, &rect1);

		SDL_SetRenderTarget(renderer, NULL);

		SDL_RenderCopy(renderer, texTarget, NULL, &rect1);
#endif

		SDL_RenderPresent(renderer);

		SDL_Delay(4);
	}

_quit:

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
#endif
