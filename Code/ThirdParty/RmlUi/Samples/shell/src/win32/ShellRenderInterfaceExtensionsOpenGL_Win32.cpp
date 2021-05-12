/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <ShellRenderInterfaceExtensions.h>
#include <ShellRenderInterfaceOpenGL.h>
#include <win32/IncludeWindows.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Profiling.h>
#include <RmlUi/Core/Types.h>
#include <Shell.h>
#include <string.h>


void ShellRenderInterfaceOpenGL::SetViewport(int width, int height)
{
	if(m_width != width || m_height != height) {
		Rml::Matrix4f projection, view;

		m_width = width;
		m_height = height;
		
		glViewport(0, 0, width, height);
		projection = Rml::Matrix4f::ProjectOrtho(0, (float)width, (float)height, 0, -10000, 10000);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(projection.data());
		view = Rml::Matrix4f::Identity();
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(view.data());
	}
}

bool ShellRenderInterfaceOpenGL::AttachToNative(void *nativeWindow)
{
	this->render_context = nullptr;
	this->window_handle = (HWND)nativeWindow;
	this->device_context = GetDC(this->window_handle);
	if (this->device_context == nullptr)
	{
		Shell::DisplayError("Could not get device context.");
		return false;
	}

	PIXELFORMATDESCRIPTOR pixel_format_descriptor;
	memset(&pixel_format_descriptor, 0, sizeof(pixel_format_descriptor));
	pixel_format_descriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixel_format_descriptor.nVersion = 1;
	pixel_format_descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pixel_format_descriptor.iPixelType = PFD_TYPE_RGBA;
	pixel_format_descriptor.cColorBits = 32;
	pixel_format_descriptor.cRedBits = 8;
	pixel_format_descriptor.cGreenBits = 8;
	pixel_format_descriptor.cBlueBits = 8;
	pixel_format_descriptor.cAlphaBits = 8;
	pixel_format_descriptor.cDepthBits = 24;
	pixel_format_descriptor.cStencilBits = 8;

	int pixel_format = ChoosePixelFormat(this->device_context, &pixel_format_descriptor);
	if (pixel_format == 0)
	{
		Shell::DisplayError("Could not choose 32-bit pixel format.");
		return false;
	}

	if (SetPixelFormat(this->device_context, pixel_format, &pixel_format_descriptor) == FALSE)
	{
		Shell::DisplayError("Could not set pixel format.");
		return false;
	}

	this->render_context = wglCreateContext(this->device_context);
	if (this->render_context == nullptr)
	{ 
		Shell::DisplayError("Could not create OpenGL rendering context.");
		return false;
	}

	// Activate the rendering context.
	if (wglMakeCurrent(this->device_context, this->render_context) == FALSE)
	{
		Shell::DisplayError("Unable to make rendering context current.");
		return false;
	}

	// Set up the GL state.
	glClearColor(0, 0, 0, 1);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	RECT crect;
	GetClientRect(this->window_handle, &crect);
	glOrtho(0, (crect.right - crect.left), (crect.bottom - crect.top), 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	return true;
}

void ShellRenderInterfaceOpenGL::DetachFromNative()
{
	// Shutdown OpenGL
	if (this->render_context != nullptr)
	{
		wglMakeCurrent(nullptr, nullptr); 
		wglDeleteContext(this->render_context);
		this->render_context = nullptr;
	}

	if (this->device_context != nullptr)
	{
		ReleaseDC(this->window_handle, this->device_context);
		this->device_context = nullptr;
	}
}

void ShellRenderInterfaceOpenGL::PrepareRenderBuffer()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

void ShellRenderInterfaceOpenGL::PresentRenderBuffer()
{
	// Flips the OpenGL buffers.
	SwapBuffers(this->device_context);
	RMLUI_FrameMark;
}
