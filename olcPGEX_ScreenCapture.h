/*
	olcPGEX_ScreenCapture.h
	+-------------------------------------------------------------+
	|         OneLoneCoder Pixel Game Engine Extension            |
	|               Screen Capture - v1.1                         |
	+-------------------------------------------------------------+
	What is this?
	~~~~~~~~~~~~~
	This is an extension to the olcPixelGameEngine, which provides
	a basic example of reading the screen back into a sprite / decal
	including any draw decals or shader effects.  Set capture to true
	to capture this frame after it is drawn.  On the next OnUserUpdate
	the screen data is available in sprite and decal.

	Author
	~~~~~~
	Dandistine

	License (OLC-3)
	~~~~~~~~~~~~~~~
	Copyright 2018 - 2019 OneLoneCoder.com
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:
	1. Redistributions or derivations of source code must retain the above
	copyright notice, this list of conditions and the following disclaimer.
	2. Redistributions or derivative works in binary form must reproduce
	the above copyright notice. This list of conditions and the following
	disclaimer must be reproduced in the documentation and/or other
	materials provided with the distribution.
	3. Neither the name of the copyright holder nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once
#include "olcPixelGameEngine.h"
#include <memory>
#include <gl/GL.h>

#define CALLSTYLE __stdcall
#define OGL_LOAD(t, n) (t*)wglGetProcAddress(#n)

namespace olc::gfx::cap {

	typedef ptrdiff_t GLsizeiptr;
	typedef int GLintptr;

	typedef void CALLSTYLE locReadPixels_t(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* data);
	typedef void CALLSTYLE locBindBuffer_t(GLenum target, GLuint buffer);
	typedef void CALLSTYLE locGenBuffers_t(GLsizei n, GLuint* buffers);
	typedef void CALLSTYLE locBufferData_t(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
	typedef void CALLSTYLE locGetBufferSubData_t(GLenum target, GLintptr offset, GLsizeiptr size, void* data);

	constexpr GLenum GL_PIXEL_PACK_BUFFER = 0x88EB;
	constexpr GLenum GL_DYNAMIC_READ = 0x88E9;

	class ScreenCapture : olc::PGEX {
	private:
		locReadPixels_t* locReadPixels = nullptr;
		locBindBuffer_t* locBindBuffer = nullptr;
		locGenBuffers_t* locGenBuffers = nullptr;
		locBufferData_t* locBufferData = nullptr;
		locGetBufferSubData_t* locGetBufferSubData = nullptr;

		GLuint buffer = 0;
		bool copy_out = false;

	public:

		ScreenCapture() : olc::PGEX(true) {};

		bool capture = false;

		std::unique_ptr<olc::Sprite> sprite;
		std::unique_ptr<olc::Decal> decal;


		void OnBeforeUserCreate() override {
			locGenBuffers = OGL_LOAD(locGenBuffers_t, glGenBuffers);
			locBindBuffer = OGL_LOAD(locBindBuffer_t, glBindBuffer);
			locBufferData = OGL_LOAD(locBufferData_t, glBufferData);
			locGetBufferSubData = OGL_LOAD(locGetBufferSubData_t, glGetBufferSubData);

			//Generate a buffer identifier for where each frame will be captured
			locGenBuffers(1, &buffer);
			locBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);

			//And size it to store an entire frame
			locBufferData(GL_PIXEL_PACK_BUFFER, pge->ScreenHeight() * pge->ScreenWidth() * 4, nullptr, GL_DYNAMIC_READ);

			sprite = std::make_unique<olc::Sprite>(pge->ScreenWidth(), pge->ScreenHeight());
			decal = std::make_unique<olc::Decal>(sprite.get());
		}

		bool OnBeforeUserUpdate(float& fElapsedTime) override {
			if (capture) {
				locBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
				glReadPixels(0, 0, pge->ScreenWidth(), pge->ScreenHeight(), GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				copy_out = true;
				capture = false;
			}

			return false;
		}

		void OnAfterUserUpdate(float fElapsedTime) override {
			if (copy_out) {
				locBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
				locGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0, pge->ScreenHeight() * pge->ScreenWidth() * 4, sprite->pColData.data());
				decal->Update();
				copy_out = false;
			}
		}
	};
}
