/*
 *  Copyright (C) 2002-2021  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef DOSBOX_RENDER_H
#define DOSBOX_RENDER_H

#include <cstring>
#include <deque>
#include <string>

#include "../src/gui/render_scalers.h"
#include "fraction.h"
#include "vga.h"

struct RenderPal_t {
	struct {
		uint8_t red    = 0;
		uint8_t green  = 0;
		uint8_t blue   = 0;
		uint8_t unused = 0;
	} rgb[256] = {};

	union {
		uint16_t b16[256];
		uint32_t b32[256] = {};
	} lut = {};

	bool changed          = false;
	uint8_t modified[256] = {};
	uint32_t first        = 0;
	uint32_t last         = 0;
};

struct Render_t {
	ImageInfo src = {};
	uint32_t src_start   = 0;

	// Frames per second
	double fps = 0;

	struct {
		uint32_t size = 0;

		ScalerMode inMode  = {};
		ScalerMode outMode = {};

		bool clearCache = false;

		ScalerLineHandler_t lineHandler    = nullptr;
		ScalerLineHandler_t linePalHandler = nullptr;

		uint32_t blocks     = 0;
		uint32_t lastBlock  = 0;
		int outPitch        = 0;
		uint8_t* outWrite   = nullptr;
		uint32_t cachePitch = 0;
		uint8_t* cacheRead  = nullptr;
		uint32_t inHeight   = 0;
		uint32_t inLine     = 0;
		uint32_t outLine    = 0;
	} scale = {};

	RenderPal_t pal = {};

	bool updating  = false;
	bool active    = false;
	bool fullFrame = true;

	std::string current_shader_name = {};
	bool force_reload_shader        = false;
};

// A frame of the emulated video output that's passed to the rendering backend
// or to the image and video capturers.
//
// Also used for passing the post-shader output read back from the frame buffer
// to the image capturer.
//
struct RenderedImage {
	ImageInfo params = {};

	// If true, the image is stored flipped vertically, starting from the
	// bottom row
	bool is_flipped_vertically = false;

	// Bytes per row
	uint16_t pitch = 0;

	// (width * height) number of pixels stored in the pixel format defined
	// by pixel_format
	uint8_t* image_data = nullptr;

	// Pointer to a (256 * 4) byte long palette data, stored as 8-bit RGB
	// values with 1 extra padding byte per entry (R0, G0, B0, X0, R1, G1,
	// B1, X1, etc.)
	uint8_t* palette_data = nullptr;

	inline bool is_paletted() const
	{
		return (params.pixel_format == PixelFormat::Indexed8);
	}

	RenderedImage deep_copy() const
	{
		RenderedImage copy = *this;

		// Deep-copy image and palette data
		const auto image_data_num_bytes = static_cast<uint32_t>(
		        params.height * pitch);

		copy.image_data = new uint8_t[image_data_num_bytes];

		assert(image_data);
		std::memcpy(copy.image_data, image_data, image_data_num_bytes);

		// TODO it's bad that we need to make this assumption downstream
		// on the size and alignment of the palette...
		if (palette_data) {
			constexpr uint16_t PaletteNumBytes = 256 * 4;
			copy.palette_data = new uint8_t[PaletteNumBytes];

			std::memcpy(copy.palette_data, palette_data, PaletteNumBytes);
		}
		return copy;
	}

	void free()
	{
		delete[] image_data;
		image_data = nullptr;

		delete[] palette_data;
		palette_data = nullptr;
	}
};

extern Render_t render;
extern ScalerLineHandler_t RENDER_DrawLine;

void RENDER_AddConfigSection(const config_ptr_t& conf);

bool RENDER_IsAspectRatioCorrectionEnabled();
const std::string RENDER_GetCgaColorsSetting();

void RENDER_SyncMonochromePaletteSetting(const enum MonochromePalette palette);

std::deque<std::string> RENDER_GenerateShaderInventoryMessage();

void RENDER_SetSize(const uint16_t width, const uint16_t height,
                    const bool double_width, const bool double_height,
                    const Fraction& render_pixel_aspect_ratio,
                    const PixelFormat pixel_format,
                    const double frames_per_second, const VideoMode& video_mode);

bool RENDER_StartUpdate(void);
void RENDER_EndUpdate(bool abort);

void RENDER_SetPalette(const uint8_t entry, const uint8_t red,
                       const uint8_t green, const uint8_t blue);

bool RENDER_MaybeAutoSwitchShader([[maybe_unused]] const uint16_t canvas_width,
                                  [[maybe_unused]] const uint16_t canvas_height,
                                  [[maybe_unused]] const VideoMode& video_mode,
                                  [[maybe_unused]] const bool reinit_render);

void RENDER_NotifyEgaModeWithVgaPalette();

#endif // DOSBOX_RENDER_H
