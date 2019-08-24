/* This file is part of SDL_haa - SDL addon for Hildon Animation Actors
 * Copyright (C) 2010 Javier S. Pedro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA or see <http://www.gnu.org/licenses/>.
 */

#ifndef __SDL_HAA_H
#define __SDL_HAA_H

#include "SDL_video.h"
#include "SDL_events.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum HAA_Axis {
	HAA_X_AXIS = 0,
	HAA_Y_AXIS = 1,
	HAA_Z_AXIS = 2
} HAA_Axis;

typedef enum HAA_Gravity {
	HAA_GRAVITY_NONE	= 0,
	HAA_GRAVITY_N		= 1,
	HAA_GRAVITY_NE		= 2,
	HAA_GRAVITY_E		= 3,
	HAA_GRAVITY_SE		= 4,
	HAA_GRAVITY_S		= 5,
	HAA_GRAVITY_SW		= 6,
	HAA_GRAVITY_W		= 7,
	HAA_GRAVITY_NW		= 8,
	HAA_GRAVITY_CENTER	= 9
} HAA_Gravity;

typedef enum HAA_Actor_Pending {
	HAA_PENDING_NOTHING		= 0,
	HAA_PENDING_PARENT			= (1 << 0),
	HAA_PENDING_SHOW			= (1 << 1),
	HAA_PENDING_POSITION		= (1 << 2),
	HAA_PENDING_SCALE			= (1 << 3),
	HAA_PENDING_ANCHOR			= (1 << 4),
	HAA_PENDING_ROTATION_X		= (1 << 5),
	HAA_PENDING_ROTATION_Y		= (1 << 6),
	HAA_PENDING_ROTATION_Z		= (1 << 7),
	HAA_PENDING_EVERYTHING		= 0xFFU
} HAA_Actor_Pending;

/** A Hildon Animation Actor. */
typedef struct HAA_Actor {
	/** The associated SDL surface; you can render to it. */
	SDL_Surface * surface;
	Uint8 pending;
	Uint8 visible, opacity, gravity;
	Sint32 position_x, position_y, depth;
	Sint32 scale_x, scale_y;
	Sint32 anchor_x, anchor_y;
	Sint32 x_rotation_angle, x_rotation_y, x_rotation_z;
	Sint32 y_rotation_angle, y_rotation_x, y_rotation_z;
	Sint32 z_rotation_angle, z_rotation_x, z_rotation_y;
} HAA_Actor;

/** Invoke after SDL_Init.
	@param flags reserved for future expansion (pass 0)
	@return 0 if SDL_haa was initialized correctly.
  */
extern DECLSPEC int SDLCALL HAA_Init(Uint32 flags);

/** Invoke just before SDL_Quit.
  */
extern DECLSPEC void SDLCALL HAA_Quit();

/** 
  Call before handling any SDL_Event (or use SDL_SetEventFilter).
  @param event the SDL_Event you were about to handle.
  @return 1 if the event was filtered and should not be handled by your app.
*/
extern DECLSPEC int SDLCALL HAA_FilterEvent(const SDL_Event *event);

/** Call after calling SDL_SetVideoMode() if you have any actors created
  * to ensure they're visible in the new window.
  * If you have no actors, it does nothing.
  * @return 0 if everything went OK.
  */
extern DECLSPEC int SDLCALL HAA_SetVideoMode(void);

/** Creates both an animation actor and its associated surface.
  * @param flags reserved (pass 0)
  * @param width size of the actor surface
  * @param height
  * @param bitsPerPixel depth of the actor surface
  * 	a 32 bpp surface will have an alpha channel.
  * @return the created HAA_Actor, or NULL if an error happened.
  */
extern DECLSPEC HAA_Actor* SDLCALL HAA_CreateActor(Uint32 flags,
	int width, int height, int bitsPerPixel);

/** Frees an animation actor and associated surface. */
extern DECLSPEC void SDLCALL HAA_FreeActor(HAA_Actor* actor);

/** Flushes any pending position, scale, orientation, etc. changes. */
extern DECLSPEC int SDLCALL HAA_Commit(HAA_Actor* actor);
/** Puts contents of actor surface to screen. */
extern DECLSPEC int SDLCALL HAA_Flip(HAA_Actor* actor);

static inline void HAA_Show
(HAA_Actor* actor)
{
	actor->visible = 1;
	actor->pending |= HAA_PENDING_SHOW;
}

static inline void HAA_Hide
(HAA_Actor* actor)
{
	actor->visible = 0;
	actor->pending |= HAA_PENDING_SHOW;
}

static inline void HAA_SetOpacity
(HAA_Actor* actor, unsigned char opacity)
{
	actor->opacity = opacity;
	actor->pending |= HAA_PENDING_SHOW;
}

static inline void HAA_SetPosition
(HAA_Actor* actor, int x, int y)
{
	actor->position_x = x;
	actor->position_y = y;
	actor->pending |= HAA_PENDING_POSITION;
}

static inline void HAA_SetDepth
(HAA_Actor* actor, int depth)
{
	actor->depth = depth;
	actor->pending |= HAA_PENDING_POSITION;
}

static inline void HAA_SetScaleX
(HAA_Actor* actor, Sint32 x, Sint32 y)
{
	actor->scale_x = x;
	actor->scale_y = y;
	actor->pending |= HAA_PENDING_SCALE;
}

static inline void HAA_SetScale
(HAA_Actor* actor, double x, double y)
{
	HAA_SetScaleX(actor, x * (1 << 16), y * (1 << 16));
}

static inline void HAA_SetAnchor
(HAA_Actor* actor, int x, int y)
{
	actor->gravity = HAA_GRAVITY_NONE;
	actor->anchor_x = x;
	actor->anchor_y = y;
	actor->pending |= HAA_PENDING_ANCHOR;
}

static inline void HAA_SetGravity
(HAA_Actor* actor, HAA_Gravity gravity)
{
	actor->gravity = gravity;
	actor->anchor_x = 0;
	actor->anchor_y = 0;
	actor->pending |= HAA_PENDING_ANCHOR;
}

static inline void HAA_SetRotationX
(HAA_Actor* actor, HAA_Axis axis, Sint32 degrees, int x, int y, int z)
{
	switch (axis) {
		case HAA_X_AXIS:
			actor->x_rotation_angle = degrees;
			actor->x_rotation_y = y;
			actor->x_rotation_z = z;
			actor->pending |= HAA_PENDING_ROTATION_X;
			break;
		case HAA_Y_AXIS:
			actor->y_rotation_angle = degrees;
			actor->y_rotation_x = x;
			actor->y_rotation_z = z;
			actor->pending |= HAA_PENDING_ROTATION_Y;
			break;
		case HAA_Z_AXIS:
			actor->z_rotation_angle = degrees;
			actor->z_rotation_x = x;
			actor->z_rotation_y = y;
			actor->pending |= HAA_PENDING_ROTATION_Z;
			break;
	}
}

static inline void HAA_SetRotation
(HAA_Actor* actor, HAA_Axis axis, double degrees, int x, int y, int z)
{
	HAA_SetRotationX(actor, axis, degrees * (1 << 16), x, y, z);
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif
