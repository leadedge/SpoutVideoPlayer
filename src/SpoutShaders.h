/*

				SpoutShaders.h

		Functions to manage compute shaders

	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	Copyright (c) 2016-2026, Lynn Jarvis. All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, 
	are permitted provided that the following conditions are met:

		1. Redistributions of source code must retain the above copyright notice, 
		   this list of conditions and the following disclaimer.

		2. Redistributions in binary form must reproduce the above copyright notice, 
		   this list of conditions and the following disclaimer in the documentation 
		   and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"	AND ANY 
	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE	ARE DISCLAIMED. 
	IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#pragma once
#ifndef __spoutShaders__
#define __spoutShaders__

#include <windows.h>
#include <algorithm> // for std::replace

// Change path as necessary
#include "..\apps\SpoutGL\SpoutGLextensions.h"

using namespace spoututils;

class spoutShaders {

	public:

		spoutShaders();
		~spoutShaders();

		// Texture copy
		bool Copy(GLuint SourceID, GLuint DestID,
			unsigned int width, unsigned int height,
			bool bInvert = false, bool swap = false);

		// Flip image in place
		bool Flip(GLuint SourceID, unsigned int width, unsigned int height, bool bSwap = false);

		// Mirror image in place
		bool Mirror(GLuint SourceID, unsigned int width, unsigned int height, bool bSwap = false);

		// Swap RGBA <> BGRA
		bool Swap(GLuint SourceID, unsigned int width, unsigned int height);

		// Temperature
		bool Temperature(GLuint SourceID, GLuint DestID,
			unsigned int width, unsigned int height, float temp);

		// Image adjust - brightness, contrast, saturation, gamma
		bool Adjust(GLuint SourceID, GLuint DestID, 
			unsigned int width, unsigned int height,
			float brightness, float contrast, 
			float saturation, float gamma);

		// Gaussian blur
		bool Blur(GLuint SourceID, GLuint DestID,
			unsigned int width, unsigned int height, float amount);

		// Two pass blur
		bool Blur2(GLuint SourceID, GLuint DestID,
			unsigned int width, unsigned int height, float amount);

		// Bloom effect
		bool Bloom(GLuint SourceID,
			unsigned int width, unsigned int height, float amount);

		// Unsharp mask sharpen
		bool Sharpen(GLuint SourceID, GLuint DestID, 
			unsigned int width, unsigned int height,
			float sharpenWidth, float sharpenStrength);

		// Contrast adaptive sharpen
		bool AdaptiveSharpen(GLuint SourceID,
			unsigned int width, unsigned int height, float caswidth, float caslevel);

		// Kuwahara
		bool Kuwahara(GLuint SourceID, GLuint DestID, 
			unsigned int width, unsigned int height, float amount);

		// Rgba2yuv
		bool Rgba2yuv(GLuint SourceID, GLuint DestID,
			unsigned int width, unsigned int height);

		// Motion blur
		bool Motion(GLuint SourceID, GLuint DestID,
			unsigned int width, unsigned int height, float amount);

		// Shader format
		void SetGLformat(GLint glformat);

	protected :

		void CheckShaderFormat(std::string& shaderstr);
		bool ComputeShader(std::string &shader, GLuint &program, 
			GLuint SourceID, GLuint DestID, 
			unsigned int width, unsigned int height,
			float uniform0 = -1.0, float uniform1 = -1.0,
			float uniform2 = -1.0, float uniform3 = -1.0);
		GLuint CreateComputeShader(std::string shader, unsigned int nWgX, unsigned int nWgY);
		std::string GetFileString(const char* filepath); // Get shader string from file

		// Global program identifiers
		GLuint m_copyProgram     = 0;
		GLuint m_flipProgram     = 0;
		GLuint m_mirrorProgram   = 0;
		GLuint m_swapProgram     = 0;
		GLuint m_tempProgram     = 0;
		GLuint m_brcosaProgram   = 0;
		GLuint m_blurProgram     = 0;
		GLuint m_hBlurProgram    = 0;
		GLuint m_vBlurProgram    = 0;
		GLuint m_sharpenProgram  = 0;
		GLuint m_casProgram      = 0;
		GLuint m_bloomProgram    = 0;
		GLuint m_kuwaharaProgram = 0;
		GLuint m_rgba2yuvProgram = 0;
		GLuint m_motionProgram   = 0;

		// Formats
		GLint m_GLformat = GL_RGBA8;
		std::string m_GLformatName = "rgba8";

		//
		// Shader source
		//

		//
		// Texture copy
		//
		std::string m_copystr = R"(

		layout(rgba8, binding=0) uniform readonly image2D src;
		layout(rgba8, binding=1) uniform writeonly image2D dst;
		layout (location = 0) uniform bool flip;
		layout (location = 1) uniform bool swap;
		void main() {
			// Copy
			vec4 c = imageLoad(src, ivec2(gl_GlobalInvocationID.xy));
			uint ypos = gl_GlobalInvocationID.y;
			if(flip) ypos = imageSize(src).y-ypos; // Flip image option
			// Texture copy with output alpha = 1
			if(swap) { // Swap RGBA<>BGRA option
			    imageStore(dst, ivec2(gl_GlobalInvocationID.x, ypos), vec4(c.b,c.g,c.r,c.a));
			}
			else {
			    imageStore(dst, ivec2(gl_GlobalInvocationID.x, ypos), vec4(c.r,c.g,c.b,c.a));
			}
		}

		)";

		//
		// Flip in place
		//
		std::string m_flipstr = R"(

		layout(rgba8, binding=0) uniform image2D src;
		layout (location = 0) uniform bool swap;
		void main() {
			// Flip
			if(gl_GlobalInvocationID.y > imageSize(src).y/2) // Half image
			    return;
			uint ypos = imageSize(src).y-gl_GlobalInvocationID.y; // Flip y position
			vec4 c0 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy)); // This pixel
			vec4 c1 = imageLoad(src, ivec2(gl_GlobalInvocationID.x, ypos)); // Flip pixel
			if (swap) { // Swap RGBA<>BGRA option
				c0 = vec4(c0.b, c0.g, c0.r, c0.a);
				c1 = vec4(c1.b, c1.g, c1.r, c1.a);
			}
			imageStore(src, ivec2(gl_GlobalInvocationID.x, ypos), c0); // Move this pixel to flip position
			imageStore(src, ivec2(gl_GlobalInvocationID.xy), c1);  // Move flip pixel to this position
		}

		)";

		//
		// Mirror in place
		//
		std::string m_mirrorstr = R"(

		layout(rgba8, binding=0) uniform image2D src;
		layout (location = 0) uniform bool swap;
		void main() {
			// Mirror
			if(gl_GlobalInvocationID.x > imageSize(src).x/2)
			    return;
			uint xpos = imageSize(src).x-gl_GlobalInvocationID.x;
			vec4 c0 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy));
			vec4 c1 = imageLoad(src, ivec2(xpos, gl_GlobalInvocationID.y));
			if (swap) {
				c0 = vec4(c0.b, c0.g, c0.r, c0.a);
				c1 = vec4(c1.b, c1.g, c1.r, c1.a);
			}
			imageStore(src, ivec2(xpos, gl_GlobalInvocationID.y), c0);
			imageStore(src, ivec2(gl_GlobalInvocationID.xy), c1);
		}

		)";

		//
		// Swap RGBA <> BGRA
		//
		std::string m_swapstr = R"(

		layout(rgba8, binding=0) uniform image2D src;
		void main() {
			// Swap
			vec4 c0 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy));
			imageStore(src, ivec2(gl_GlobalInvocationID.xy), vec4(c0.b, c0.g, c0.r, c0.a));
		}

		)";


		//
		// Adjust - brightness, contrast, saturation, gamma
		//
		std::string m_brcosastr = R"(

			layout(rgba8, binding=0) uniform image2D src; // Read/Write
			layout(rgba8, binding=1) uniform writeonly image2D dst; // Write only
			layout(location = 0) uniform float brightness;
			layout(location = 1) uniform float contrast;
			layout(location = 2) uniform float saturation;
			layout(location = 3) uniform float gamma;
			
		void main() {
			// Adjust
			vec4 c1 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy));
			// Gamma (0 > 10) default 1
			vec3 c2 = pow(c1.rgb, vec3(1.0 / gamma)); // rgb
			// Saturation (0 > 3) default 1
			float luminance = dot(c2, vec3(0.2125, 0.7154, 0.0721)); // weights sum to 1
			c2 = mix(vec3(luminance), c2, vec3(saturation));
			// Contrast (0 > 2) default
			c2 = (c2 - 0.5) * contrast + 0.5;
			// Brightness (-1 > 1) default 0
			c2 += brightness;
			// Output with original alpha
			imageStore(dst, ivec2(gl_GlobalInvocationID.xy), vec4(c2, c1.a));
		}

		)";

		
		//
		// Sharpen - unsharp mask
		//
		std::string m_sharpenstr = R"(

		layout(rgba8, binding=0) uniform image2D src;
		layout(rgba8, binding=1) uniform writeonly image2D dst;
		layout(location = 0) uniform float sharpenwidth;
		layout(location = 1) uniform float sharpenstrength;
			
		void main() {
			// Sharpen
			// Original pixel
			vec4 orig = imageLoad(src, ivec2(gl_GlobalInvocationID.xy));
			
			// Get the blur neighbourhood 3x3 or 5x5
			float dx = sharpenwidth;
			float dy = sharpenwidth;
			
			vec4 c1 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(-dx, -dy));
			vec4 c2 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, -dy));
			vec4 c3 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(dx, -dy));
			vec4 c4 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(-dx, 0.0));
			vec4 c5 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(dx, 0.0));
			vec4 c6 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(-dx, dy));
			vec4 c7 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, dy));
			vec4 c8 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(dx, dy));
			
			// Gaussian blur filter
			// [ 1, 2, 1 ]
			// [ 2, 4, 2 ]
			// [ 1, 2, 1 ]
			//  c1 c2 c3
			//  c4    c5
			//  c6 c7 c8
			vec4 blur = ((c1 + c3 + c6 + c8) + 2.0 * (c2 + c4 + c5 + c7) + 4.0 * orig) / 16.0;
			// Subtract the blurred image from the original image
			vec4 coeff_blur = vec4(sharpenstrength);
			vec4 coeff_orig = vec4(1.0) + coeff_blur;
			vec4 c9 = coeff_orig * orig - coeff_blur * blur;
			
			// Output
			imageStore(dst, ivec2(gl_GlobalInvocationID.xy), c9);
		}

		)";
		
		//
		// Contrast Adaptive sharpening
		//
		// AMD FidelityFX https://gpuopen.com/fidelityfx-cas/
		// Adapted from  https://www.shadertoy.com/view/ftsXzM
		//
		std::string m_casstr = R"(
			layout(rgba8, binding=0) uniform image2D src;
			layout (location = 0) uniform float caswidth;
			layout (location = 1) uniform float caslevel;
			
			float luminance(in vec3 col)
			{
				return dot(vec3(0.2126, 0.7152, 0.0722), col);
			}

		void main() {
			// Centre pixel (rgba)
			vec4 c0 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy));
			// Offsets 1, 2, 3
			float dx = caswidth;
			float dy = caswidth;
			//
			// Neighbourhood
			//
			//     b
			//  a  x  c
			//     d
			//
			// Centre pixel (rgb)
			vec3 col = imageLoad(src, ivec2(gl_GlobalInvocationID.xy)).rgb; // x
			float max_g = luminance(col);
			float min_g = luminance(col);
			//
			vec3 col1;
			col1 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(-dx, 0.0)).rgb; // a
			max_g = max(max_g, luminance(col1));
			min_g = min(min_g, luminance(col1));
			vec3 colw = col1;
			//
			col1 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, dy)).rgb; // b
			max_g = max(max_g, luminance(col1));
			min_g = min(min_g, luminance(col1));
			colw += col1;
			//
			col1 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(dx, 0.0)).rgb; // c
			max_g = max(max_g, luminance(col1));
			min_g = min(min_g, luminance(col1));
			colw += col1;
			//
			col1 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, dy)).rgb; // d
			max_g = max(max_g, luminance(col1));
			min_g = min(min_g, luminance(col1));
			colw += col1;
			// 
			// CAS algorithm
			//
			float d_min_g = min_g;
			float d_max_g = 1.0-max_g;
			float A;
			if (d_max_g < d_min_g) {
			    A = d_max_g / max_g;
			} else {
			    A = d_min_g / max_g;
			}
			A = sqrt(A);
			A *= mix(-0.125, -0.2, caslevel); // level - CAS level 0-1
			// Sharpened result
			vec3 col_out = (col+colw*A)/(1.0+4.0*A);
			// Output
			imageStore(src, ivec2(gl_GlobalInvocationID.xy), vec4(col_out, c0.a));
		}

		)";

		//
		// Temperature 
		// https://www.shadertoy.com/view/ltlcWN
		//
		std::string m_tempstr = R"(

			layout(rgba8, binding=0) uniform image2D src;
			layout(rgba8, binding=1) uniform writeonly image2D dst;
			layout(location = 0) uniform float temp;
			
			vec3 rgb2hsv(in vec3 c)
			{
				vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
				vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
				vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);
				float d = q.x - min(q.w, q.y);
				float e = 1.0e-10;
				return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
			}
		
			vec3 hsv2rgb(in vec3 c)
			{
				vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
				vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
				return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
			}
		
			vec3 kelvin2rgb(in float K)
			{
				float t = K / 100.0;
				vec3 o1, o2;
				float tg1 = t - 2.;
				float tb1 = t - 10.;
				float tr2 = t - 55.0;
				float tg2 = t - 50.0;
				o1.r = 1.;
				o1.g = (-155.25485562709179 - 0.44596950469579133 * tg1 + 104.49216199393888 * log(tg1)) / 255.;
				o1.b = (-254.76935184120902 + 0.8274096064007395 * tb1 + 115.67994401066147 * log(tb1)) / 255.;
				o1.b = mix(0., o1.b, step(2001., K));
				o2.r = (351.97690566805693 + 0.114206453784165 * tr2 - 40.25366309332127 * log(tr2)) / 255.;
				o2.g = (325.4494125711974 + 0.07943456536662342 * tg2 - 28.0852963507957 * log(tg2)) / 255.;
				o2.b = 1.;
				o1 = clamp(o1, 0., 1.);
				o2 = clamp(o2, 0., 1.);
				return mix(o1, o2, step(66., t));
			}
		
			vec3 temperature(in vec3 c_in, in float K)
			{
				vec3 chsv_in = rgb2hsv(c_in);
				vec3 c_temp = kelvin2rgb(K);
				vec3 c_mult = c_temp * c_in;
				vec3 chsv_mult = rgb2hsv(c_mult);
				return hsv2rgb(vec3(chsv_mult.x, chsv_mult.y, chsv_in.z));
			}
		
			void main() {
				vec4 c1 = imageLoad(src, ivec2(gl_GlobalInvocationID.xy));
				vec4 c2 = vec4(temperature(c1.rgb, temp), c1.a);
				// Output
				imageStore(dst, ivec2(gl_GlobalInvocationID.xy), c2);
			}

		)";

		//
		//     RGBA to YUV422 (UYVY)
		//
		// Y sampled at every pixel
		// U and V sampled at every second pixel 
		//
		std::string m_rgba2yuvstr = R"(

		layout(rgba8, binding=0) uniform readonly image2D src;
		layout(rgba8, binding=1) uniform writeonly image2D dst;

		void main() {

			float r = 0.0;
			float g = 0.0;
			float b = 0.0;
			float a = 1.0;
	
			// Destination position
			uint xpos = gl_GlobalInvocationID.x;
			uint ypos = gl_GlobalInvocationID.y;
	
			// Get the pixel color from the rgba texture
			// U and V from the first of every second pixel
			// Y0 and Y1 luminance from each of the pair
			vec4 rgba0 = imageLoad(src, ivec2(xpos*2, ypos));
			vec4 rgba1 = imageLoad(src, ivec2(xpos*2+1, ypos));
	
			// Calculate Y0 Y1 U V
			// NDI uses Rec.719 for 720p and 1920p
			//
			// BT.709
			// https://gist.github.com/yohhoy/dafa5a47dade85d8b40625261af3776a
			// https://www.itu.int/rec/R-REC-BT.709
			// Y  = 0.2126*R + 0.7152*G + 0.0722*B
			// Cb = (B-Y) / 1.8556
			// Cr = (R-Y) / 1.5748
			//
			float y0 =  0.2126*rgba0.r + 0.7152*rgba0.g + 0.0722*rgba0.b;
			float y1 =  0.2226*rgba1.r + 0.7152*rgba1.g + 0.0722*rgba1.b;
			float u  =  (rgba0.b-y0) / 1.8556;
			float v  =  (rgba0.r-y0) / 1.5748;
	
			// Convert Y from 0-255 to 16-235
			// (0-1 to 0.06274-0.92156)
			//  y = (y/1.16438)+0.06274
			y0 = y0/1.16438 + 0.06274;
			y1 = y1/1.16438 + 0.06274;
		
			// Adjust u and v to 0-1 range
			u += 0.5;
			v += 0.5;

			// u y0 v y1
			vec4 yuv422 = vec4(0.0);
			yuv422.x = u;
			yuv422.y = y0;
			yuv422.z = v;
			yuv422.w = y1;
	
		    imageStore(dst, ivec2(gl_GlobalInvocationID.x, ypos), yuv422);

		}

		)";


		//
		// Gaussian blur
		// Adapted from Openframeworks "09_gaussianBlurFilter" example
		// https://openframeworks.cc/
		//

		//
		// Horizontal Gaussian blur
		//
		std::string m_hblurstr = R"(
			layout(rgba8, binding=0) uniform image2D src;
			layout(rgba8, binding=1) uniform writeonly image2D dst;
			layout(location = 0) uniform float amount;
			
		void main() {
			// H blur
			vec4 c1 = 0.000229 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(amount*-4.0, 0.0));
			vec4 c2 = 0.005977 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(amount*-3.0, 0.0));
			vec4 c3 = 0.060598 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(amount*-2.0, 0.0));
			vec4 c4 = 0.241732 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(amount*-1.0, 0.0));
			vec4 c5 = 0.382928 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, 0.0));
			vec4 c6 = 0.241732 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(amount*1.0, 0.0));
			vec4 c7 = 0.060598 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(amount*2.0, 0.0));
			vec4 c8 = 0.005977 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(amount*3.0, 0.0));
			vec4 c9 = 0.000229 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(amount*4.0, 0.0));
			
			// Output
			imageStore(dst, ivec2(gl_GlobalInvocationID.xy), (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9));
			
		}

		)";

		//
		// Vertical Gaussian blur
		//
		std::string m_vblurstr = R"(
			layout(rgba8, binding=0) uniform image2D src;
			layout(rgba8, binding=1) uniform writeonly image2D dst;
			layout(location = 0) uniform float amount;
			
		void main() {
			// V blur
			vec4 c1 = 0.000229 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, amount*-4.0));
			vec4 c2 = 0.005977 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, amount*-3.0));
			vec4 c3 = 0.060598 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, amount*-2.0));
			vec4 c4 = 0.241732 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, amount*-1.0));
			vec4 c5 = 0.382928 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, 0.0));
			vec4 c6 = 0.241732 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, amount*1.0));
			vec4 c7 = 0.060598 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, amount*2.0));
			vec4 c8 = 0.005977 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, amount*3.0));
			vec4 c9 = 0.000229 * imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(0.0, amount*4.0));
			
			// Output
			imageStore(dst, ivec2(gl_GlobalInvocationID.xy), (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9));
			
		}

		)";


		//
		// Blur
		//
		// Single pass Gaussian blur
		//
		std::string m_blurstr = R"(

		#version 430

		layout (local_size_x = 16, local_size_y = 16) in;
		layout(rgba8, binding = 0) uniform readonly image2D src;
		layout(rgba8, binding = 1) uniform writeonly image2D dst;
		layout(location = 0) uniform float sigma;
		layout(location = 1) uniform int imageWidth;
		layout(location = 2) uniform int imageHeight;

		const float PI = 3.14159265359;

		// Shared memory for horizontally blurred values
		shared vec4 sharedH[16][16 + 32]; // +32 to handle radius up to 16 on both sides

		float gaussian(float x, float sigma) {
			return exp(-(x * x) / (2.0 * sigma * sigma)) / (sqrt(2.0 * PI) * sigma);
		}

		void main() {
			ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
			ivec2 lid = ivec2(gl_LocalInvocationID.xy);

		    if (gid.x >= imageWidth || gid.y >= imageHeight)
				return;

		    int radius = int(ceil(3.0 * sigma));
			float weightSum = 0.0;
			vec4 hBlur = vec4(0.0);

		    // Horizontal blur
			for (int i = -radius; i <= radius; ++i) {
				int x = clamp(gid.x + i, 0, imageWidth - 1);
				float w = gaussian(float(i), sigma);
				hBlur += imageLoad(src, ivec2(x, gid.y)) * w;
				weightSum += w;
			}

			hBlur /= weightSum;

		    // Store to shared memory
			sharedH[lid.y][lid.x + radius] = hBlur;

		    // Load additional pixels into shared memory for vertical blur (overlapping edges)
			for (int i = 1; i <= radius; ++i) {
				if (lid.x < i) {
					ivec2 srcGid = ivec2(gid.x - i, gid.y);
		            srcGid.x = clamp(srcGid.x, 0, imageWidth - 1);
				    float w = gaussian(float(-i), sigma);
					sharedH[lid.y][lid.x + radius - i] = imageLoad(src, ivec2(srcGid.x, gid.y)) * w / weightSum;

		            srcGid = ivec2(gid.x + i, gid.y);
		            srcGid.x = clamp(srcGid.x, 0, imageWidth - 1);
		            w = gaussian(float(i), sigma);
				    sharedH[lid.y][lid.x + radius + i] = imageLoad(src, ivec2(srcGid.x, gid.y)) * w / weightSum;
				}
			}
			barrier(); // Synchronize all threads before vertical blur

			// Vertical blur
			vec4 result = vec4(0.0);
			weightSum = 0.0;

		    for (int j = -radius; j <= radius; ++j) {
				int y = clamp(gid.y + j, 0, imageHeight - 1);
		        float w = gaussian(float(j), sigma);
		        result += sharedH[clamp(lid.y + j, 0, 15)][lid.x + radius] * w;
		        weightSum += w;
			}

			result /= weightSum;

		    imageStore(dst, gid, result);
		}

		)";

		//
		//    Bloom
		//
		// Adapted from (Proper gaussian) : https://www.shadertoy.com/view/MtfSDH
		// Improved version of https://www.shadertoy.com/view/lsXGWn
		//
		// Chosen due to smooth change with amount and effectiveness
		//
		std::string m_bloomstr = R"(

		layout(rgba8, binding=0) uniform image2D src;
		layout(location = 0) uniform float amount;
		void main()
		{
			float blurSize = 1.0/512.0;
		    ivec2 texcoord = ivec2(gl_GlobalInvocationID.xy);

		    vec4 sum = vec4(0);
		    sum += imageLoad(src, ivec2(texcoord.x, texcoord.y)) * 0.20;

			sum += imageLoad(src, ivec2(texcoord.x + blurSize, texcoord.y)) * 0.11;
			sum += imageLoad(src, ivec2(texcoord.x - blurSize, texcoord.y)) * 0.11;
			sum += imageLoad(src, ivec2(texcoord.x, texcoord.y + blurSize)) * 0.11;
			sum += imageLoad(src, ivec2(texcoord.x, texcoord.y - blurSize)) * 0.11;

		    sum += imageLoad(src, ivec2(texcoord.x + blurSize, texcoord.y + blurSize)) * 0.07;
			sum += imageLoad(src, ivec2(texcoord.x + blurSize, texcoord.y - blurSize)) * 0.07;
			sum += imageLoad(src, ivec2(texcoord.x - blurSize, texcoord.y + blurSize)) * 0.07;
			sum += imageLoad(src, ivec2(texcoord.x - blurSize, texcoord.y - blurSize)) * 0.07;

		    sum += imageLoad(src, ivec2(texcoord.x + 2.0*blurSize, texcoord.y)) * 0.02;
			sum += imageLoad(src, ivec2(texcoord.x - 2.0*blurSize, texcoord.y)) * 0.02;
			sum += imageLoad(src, ivec2(texcoord.x, texcoord.y + 2.0*blurSize)) * 0.02;
			sum += imageLoad(src, ivec2(texcoord.x, texcoord.y - 2.0*blurSize)) * 0.02;
	
		    // increase blur with intensity and add to original
			sum = sum*amount + imageLoad(src, texcoord); 
   
		    imageStore(src, texcoord, sum);
   
		}

		)";

		//
		// Kuwahara effect
		// Adapted from : Jan Eric Kyprianidis (http://www.kyprianidis.com/)
		//
		std::string m_kuwaharastr = R"(

		layout(rgba8, binding=0) uniform image2D src;
		layout(rgba8, binding=1) uniform writeonly image2D dst;
		layout(location = 0) uniform float radius;
		void main() {
			vec3 m[4];
			vec3 s[4];
			for (int j = 0; j < 4; ++j) {
				m[j] = vec3(0.0);
				s[j] = vec3(0.0);
			}
	
			vec3 c;
			int ir = int(floor(radius));
			for (int j = -ir; j <= 0; ++j) {
				for (int i = -ir; i <= 0; ++i) {
					c = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(i, j)).rgb;
					m[0] += c;
					s[0] += c * c;
				}
			}
	
			for (int j = -ir; j <= 0; ++j) {
				for (int i = 0; i <= ir; ++i) {
					c = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(i, j)).rgb;
					m[1] += c;
					s[1] += c * c;
				}
			}

			for (int j = 0; j <= ir; ++j) {
				for (int i = 0; i <= ir; ++i) {
					c = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(i, j)).rgb;
					m[2] += c;
					s[2] += c * c;
				}
			}

			for (int j = 0; j <= ir; ++j) {
				for (int i = -ir; i <= 0; ++i) {
					c = imageLoad(src, ivec2(gl_GlobalInvocationID.xy) + ivec2(i, j)).rgb;
					m[3] += c;
					s[3] += c * c;
				}
			}

			float min_sigma2 = 1e+2;
			float n = float((radius+1)*(radius+1));
			for (int k = 0; k < 4; ++k) {
				m[k] /= n;
				s[k] = abs(s[k] / n - m[k] * m[k]);
				float sigma2 = s[k].r + s[k].g + s[k].b;
				if (sigma2 < min_sigma2) {
					min_sigma2 = sigma2;
					imageStore(dst, ivec2(gl_GlobalInvocationID.xy), vec4(m[k], 1.0));
				}
			}
		}

		)";

		std::string m_motionstr = R"(
		//
		// Motion blur
		//
		// src - last frame
		// dst - this frame
		//
		layout(rgba8, binding=0) uniform image2D src;
		layout(rgba8, binding=1) uniform image2D dst;
		layout(location = 0) uniform float amount;
		void main() {

			vec4 lastframe = imageLoad(src, ivec2(gl_GlobalInvocationID.xy));
			vec4 thisframe = imageLoad(dst, ivec2(gl_GlobalInvocationID.xy));
	
			// 0.0 - 1.0
			// Limit maximum to 0.98
			// 1.0 is always the last frame and freezes the image
			float amt = amount;
			if(amt > 0.98) amt = 0.98;
	
			vec4 newframe = lastframe*amt + thisframe*(1.0-amt);
	
			// Save the accumulated result for the next frame
			imageStore(src, ivec2(gl_GlobalInvocationID.xy), newframe);
	
			// Display the accumulated result for this frame
			imageStore(dst, ivec2(gl_GlobalInvocationID.xy), newframe);
	
		}

		)";




}; // end SpoutShaders.h

#endif
