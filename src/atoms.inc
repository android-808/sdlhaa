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

/* Definitions for all the atoms the library is going to use */

enum atoms {
	ATOM_NET_WM_STATE,
	ATOM_NET_WM_STATE_FULLSCREEN,
	ATOM_NET_WM_WINDOW_TYPE,
	ATOM_HILDON_WM_WINDOW_TYPE_ANIMATION_ACTOR,
	ATOM_HILDON_ANIMATION_CLIENT_READY,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_SHOW,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_POSITION,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_ROTATION,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_SCALE,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_ANCHOR,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_PARENT,
	ATOM_COUNT
};

static const char * atom_names[] = {
	"_NET_WM_STATE",
	"_NET_WM_STATE_FULLSCREEN",
	"_NET_WM_WINDOW_TYPE",
	"_HILDON_WM_WINDOW_TYPE_ANIMATION_ACTOR",
	"_HILDON_ANIMATION_CLIENT_READY",
	"_HILDON_ANIMATION_CLIENT_MESSAGE_SHOW",	
	"_HILDON_ANIMATION_CLIENT_MESSAGE_POSITION",
	"_HILDON_ANIMATION_CLIENT_MESSAGE_ROTATION",
	"_HILDON_ANIMATION_CLIENT_MESSAGE_SCALE",
	"_HILDON_ANIMATION_CLIENT_MESSAGE_ANCHOR",
	"_HILDON_ANIMATION_CLIENT_MESSAGE_PARENT",
	""
};

static Atom atom_values[ATOM_COUNT];

#define ATOM(X) atom_values[ ATOM ## X ]

