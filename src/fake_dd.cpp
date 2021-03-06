/* 
 * FFNx - Complete OpenGL replacement of the Direct3D renderer used in 
 * the original ports of Final Fantasy VII and Final Fantasy VIII for the PC.
 *
 * me program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * me program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with me program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * fake_dd.c - fake DirectDraw interface used to intercept FF8 movie player output
 */

#include <ddraw.h>
#include "bgfx.h"

#include "fake_dd.h"
#include "types.h"
#include "gl.h"
#include "cfg.h"
#include "log.h"
#include "common.h"
#include "globals.h"

uint __stdcall fake_dd_blit_fast(struct ddsurface **me, uint unknown1, uint unknown2, struct ddsurface **target, LPRECT source, uint unknown3);
uint __stdcall fake_ddsurface_get_pixelformat(struct ddsurface **me, LPDDPIXELFORMAT pf);
uint __stdcall fake_ddsurface_get_surface_desc(struct ddsurface **me, LPDDSURFACEDESC2 sd);
uint __stdcall fake_ddsurface_get_dd_interface(struct ddsurface **me, struct dddevice ***dd);
uint __stdcall fake_ddsurface_get_palette(struct ddsurface **me, void **palette);
uint __stdcall fake_ddsurface_lock(struct ddsurface **me, LPRECT dest, LPDDSURFACEDESC sd, uint flags, uint unused);
uint __stdcall fake_ddsurface_unlock(struct ddsurface **me, LPRECT dest);
uint __stdcall fake_ddsurface_islost(struct ddsurface **me);
uint __stdcall fake_ddsurface_restore(struct ddsurface **me);
uint __stdcall fake_dd_query_interface(struct dddevice **me, uint *iid, void **ppvobj);
uint __stdcall fake_dd_addref(struct dddevice **me);
uint __stdcall fake_dd_release(struct dddevice **me);
uint __stdcall fake_dd_create_clipper(struct dddevice **me, DWORD flags, LPDIRECTDRAWCLIPPER *clipper);
uint __stdcall fake_dd_create_palette(struct dddevice **me, LPPALETTEENTRY palette_entry, LPDIRECTDRAWPALETTE *palette, IUnknown *unused);
uint __stdcall fake_dd_create_surface(struct dddevice **me, LPDDSURFACEDESC sd, LPDIRECTDRAWSURFACE *surface, IUnknown *unused);
uint __stdcall fake_dd_get_caps(struct dddevice **me, LPDDCAPS halcaps, LPDDCAPS helcaps);
uint __stdcall fake_dd_get_display_mode(struct dddevice **me, LPDDSURFACEDESC sd);
uint __stdcall fake_dd_set_coop_level(struct dddevice **me, uint coop_level);
uint __stdcall fake_d3d_get_caps(struct d3d2device **me, void *a, void *b);

struct ddsurface fake_dd_surface = {
	fake_dd_query_interface,
	fake_dd_addref,
	fake_dd_release,
	0,
	0,
	0,
	0,
	fake_dd_blit_fast,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	fake_ddsurface_get_palette,
	fake_ddsurface_get_pixelformat,
	fake_ddsurface_get_surface_desc,
	0,
	fake_ddsurface_islost,
	fake_ddsurface_lock,
	0,
	fake_ddsurface_restore,
	0,
	0,
	0,
	0,
	fake_ddsurface_unlock,
	0,
	0,
	0,
	fake_ddsurface_get_dd_interface,
};
struct ddsurface *_fake_dd_back_surface = &fake_dd_surface;

struct ddsurface fake_dd_front_surface = {
	fake_dd_query_interface,
	fake_dd_addref,
	fake_dd_release,
	0,
	0,
	0,
	0,
	fake_dd_blit_fast,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	fake_ddsurface_get_palette,
	fake_ddsurface_get_pixelformat,
	fake_ddsurface_get_surface_desc,
	0,
	fake_ddsurface_islost,
	fake_ddsurface_lock,
	0,
	fake_ddsurface_restore,
	0,
	0,
	0,
	0,
	fake_ddsurface_unlock,
	0,
	0,
	0,
	fake_ddsurface_get_dd_interface,
};
struct ddsurface *_fake_dd_front_surface = &fake_dd_front_surface;

struct ddsurface fake_dd_temp_surface = {
	fake_dd_query_interface,
	fake_dd_addref,
	fake_dd_release,
	0,
	0,
	0,
	0,
	fake_dd_blit_fast,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	fake_ddsurface_get_palette,
	fake_ddsurface_get_pixelformat,
	fake_ddsurface_get_surface_desc,
	0,
	fake_ddsurface_islost,
	fake_ddsurface_lock,
	0,
	fake_ddsurface_restore,
	0,
	0,
	0,
	0,
	fake_ddsurface_unlock,
	0,
	0,
	0,
	fake_ddsurface_get_dd_interface,
};
struct ddsurface *_fake_dd_temp_surface = &fake_dd_temp_surface;

struct dddevice fake_dddevice = {
	fake_dd_query_interface,
	fake_dd_addref,
	fake_dd_release,
	0,
	fake_dd_create_clipper,
	fake_dd_create_palette,
	fake_dd_create_surface,
	0,
	0,
	0,
	0,
	fake_dd_get_caps,
	fake_dd_get_display_mode,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	fake_dd_set_coop_level,
	0,
	0,
	0,
};
struct dddevice *_fake_dddevice = &fake_dddevice;

struct d3d2device fake_d3d2device = {
	fake_dd_query_interface,
	fake_dd_addref,
	fake_dd_release,
	fake_d3d_get_caps,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};
struct d3d2device *_fake_d3d2device = &fake_d3d2device;

uint __stdcall fake_dd_blit_fast(struct ddsurface **me, uint unknown1, uint unknown2, struct ddsurface **source, LPRECT src_rect, uint unknown3)
{
	if(trace_all || trace_fake_dx) trace("blit_fast\n");

	return DD_OK;
}

uint __stdcall fake_ddsurface_get_pixelformat(struct ddsurface **me, LPDDPIXELFORMAT pf)
{
	if(trace_all || trace_fake_dx) trace("get_pixelformat\n");

	pf->dwFlags = DDPF_RGB;
	pf->dwRGBBitCount = 24;
	pf->dwRBitMask = 0xFF0000;
	pf->dwGBitMask = 0xFF00;
	pf->dwBBitMask = 0xFF;

	return 0;
}

uint __stdcall fake_ddsurface_get_surface_desc(struct ddsurface **me, LPDDSURFACEDESC2 sd)
{
	if(trace_all || trace_fake_dx) trace("get_surface_desc\n");

	return 0;
}

uint __stdcall fake_ddsurface_get_dd_interface(struct ddsurface **me, struct dddevice ***dd)
{
	if(trace_all || trace_fake_dx) trace("get_dd_interface\n");

	*dd = &_fake_dddevice;
	return 0;
}

uint __stdcall fake_ddsurface_get_palette(struct ddsurface **me, void **palette)
{
	if(trace_all || trace_fake_dx) trace("get_palette\n");

	//*palette = 0;

	return DDERR_UNSUPPORTED;
}

char *fake_dd_surface_buffer;

uint __stdcall fake_ddsurface_lock(struct ddsurface **me, LPRECT dest, LPDDSURFACEDESC sd, uint flags, uint unused)
{
	if(trace_all || trace_fake_dx) trace("lock\n");

	if(!fake_dd_surface_buffer) fake_dd_surface_buffer = (char*)driver_malloc(640 * 480 * 3);

	sd->lpSurface = fake_dd_surface_buffer;
	sd->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_LPSURFACE;

	sd->dwWidth = 640;
	sd->dwHeight = 480;
	sd->lPitch = 640 * 3;

	sd->ddpfPixelFormat.dwFlags = DDPF_RGB;
	sd->ddpfPixelFormat.dwRGBBitCount = 24;
	sd->ddpfPixelFormat.dwRBitMask = 0xFF0000;
	sd->ddpfPixelFormat.dwGBitMask = 0xFF00;
	sd->ddpfPixelFormat.dwBBitMask = 0xFF;

	return DD_OK;
}

uint movie_texture;

uint __stdcall fake_ddsurface_unlock(struct ddsurface **me, LPRECT dest)
{
	if(trace_all || trace_fake_dx) trace("unlock\n");

	if (movie_texture) newRenderer.deleteTexture(movie_texture);

	movie_texture = newRenderer.createTexture(
		(uint8_t*)fake_dd_surface_buffer,
		640,
		480,
		0
	);

	newRenderer.useTexture(movie_texture);

	gl_draw_movie_quad(640, 480);

	return DD_OK;
}

uint __stdcall fake_ddsurface_islost(struct ddsurface **me)
{
	if(trace_all || trace_fake_dx) trace("islost\n");

	return DD_OK;
}

uint __stdcall fake_ddsurface_restore(struct ddsurface **me)
{
	if(trace_all || trace_fake_dx) trace("restore\n");

	return DD_OK;
}

uint __stdcall fake_dd_query_interface(struct dddevice **me, uint *iid, void **ppvobj)
{
	if(trace_all || trace_fake_dx) trace("query_interface: 0x%p 0x%p 0x%p 0x%p\n", iid[0], iid[1], iid[2], iid[3]);

	if(iid[0] == 0x6C14DB80)
	{
		*ppvobj = me;
		return S_OK;
	}

	if(iid[0] == 0x57805885)
	{
		*ppvobj = &_fake_dd_temp_surface;
		return S_OK;
	}
	
	return E_NOINTERFACE;
}

uint __stdcall fake_dd_addref(struct dddevice **me)
{
	if(trace_all || trace_fake_dx) trace("addref\n");

	return 1;
}

uint __stdcall fake_dd_release(struct dddevice **me)
{
	if(trace_all || trace_fake_dx) trace("release\n");

	return 0;
}

uint __stdcall fake_dd_create_clipper(struct dddevice **me, DWORD flags, LPDIRECTDRAWCLIPPER *clipper)
{
	if(trace_all || trace_fake_dx) trace("create_clipper\n");

	if(clipper == 0) return DDERR_INVALIDPARAMS;

	*clipper = 0;

	return DD_OK;
}

uint __stdcall fake_dd_create_palette(struct dddevice **me, LPPALETTEENTRY palette_entry, LPDIRECTDRAWPALETTE *palette, IUnknown *unused)
{
	if(trace_all || trace_fake_dx) trace("create_palette\n");

	if(palette == 0) return DDERR_INVALIDPARAMS;

	*palette = 0;

	return DD_OK;
}

uint __stdcall fake_dd_create_surface(struct dddevice **me, LPDDSURFACEDESC sd, LPDIRECTDRAWSURFACE *surface, IUnknown *unused)
{
	if(trace_all || trace_fake_dx) trace("create_surface %ix%i\n", sd->dwWidth, sd->dwHeight);

	*surface = (LPDIRECTDRAWSURFACE)&_fake_dd_temp_surface;

	return 0;
}

uint __stdcall fake_dd_get_caps(struct dddevice **me, LPDDCAPS halcaps, LPDDCAPS helcaps)
{
	if(trace_all || trace_fake_dx) trace("get_caps\n");

	halcaps->dwCaps = DDCAPS_BLTSTRETCH;
	return 0;
}

uint __stdcall fake_dd_get_display_mode(struct dddevice **me, LPDDSURFACEDESC sd)
{
	if(trace_all || trace_fake_dx) trace("get_display_mode\n");

	sd->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;

	sd->dwWidth = 1280;
	sd->dwHeight = 960;
	sd->lPitch = 1280 * 4;

	sd->ddpfPixelFormat.dwFlags = DDPF_RGB;
	sd->ddpfPixelFormat.dwRGBBitCount = 32;
	sd->ddpfPixelFormat.dwRBitMask = 0xFF;
	sd->ddpfPixelFormat.dwGBitMask = 0xFF00;
	sd->ddpfPixelFormat.dwBBitMask = 0xFF0000;
	sd->ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;

	return 0;
}

uint __stdcall fake_dd_set_coop_level(struct dddevice **me, uint coop_level)
{
	if(trace_all || trace_fake_dx) trace("set_coop_level\n");

	return 0;
}

uint __stdcall fake_d3d_get_caps(struct d3d2device **me, void *a, void *b)
{
	if(trace_all || trace_fake_dx) trace("d3d_get_caps\n");

	memset(a, -1, 0xFC);
	memset(b, -1, 0xFC);

	return DD_OK;
}
