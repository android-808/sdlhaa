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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <SDL.h>
#include <SDL_syswm.h>

#ifdef HAVE_XSHM
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include "SDL_haa.h"
#include "atoms.inc"

typedef struct HAA_ActorPriv {
	HAA_Actor p;

	Window window, parent;
	Visual *visual;
	Colormap colormap;
	XImage *image;
	GC gc;
#ifdef HAVE_XSHM
	XShmSegmentInfo shminfo;
#endif
	unsigned char ready;
	struct HAA_ActorPriv *prev, *next;
} HAA_ActorPriv;

static Display *display;
static Window parent_window;
static HAA_ActorPriv *first = NULL, *last = NULL;

/* Queued reparents. */
static Uint32 queued_reparent_time;
static Bool queued_reparent_fs;

#ifdef HAVE_XSHM
static int shm_major, shm_minor;
static Bool shm_pixmaps;
static Bool have_shm;
#else
static const Bool have_shm = False;
#endif

int HAA_Init(Uint32 flags)
{
	SDL_SysWMinfo info;

	SDL_VERSION(&info.version);
	if (SDL_GetWMInfo(&info) != 1) {
		SDL_SetError("SDL_haa is incompatible with this SDL version");
		return -1;
	}

	display = info.info.x11.display;
	parent_window = 0;
	queued_reparent_time = 0;
	first = last = NULL;

	XInternAtoms(display, (char**)atom_names, ATOM_COUNT, True, atom_values);

#ifdef HAVE_XSHM
	have_shm = XShmQueryVersion(display, &shm_major, &shm_minor, &shm_pixmaps);
#endif

	/* This might add some noise to your event queue, but we need them. */
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	return 0;
}

void HAA_Quit()
{
	/* Nothing to do for now */
}

static HAA_ActorPriv* find_actor_for_window(Window w)
{
	HAA_ActorPriv* a;

	for (a = first; a; a = a->next) {
		if (a->window == w) {
			return a;
		}
	}

	return NULL;
}

static void actor_send_message(HAA_ActorPriv* actor, Atom message_type,
		Uint32 l0, Uint32 l1, Uint32 l2, Uint32 l3, Uint32 l4)
{
	Window window = actor->window;
	XEvent event = { 0 };

	event.xclient.type = ClientMessage;
	event.xclient.window = window;
	event.xclient.message_type = message_type;
	event.xclient.format = 32;
	event.xclient.data.l[0] = l0;
	event.xclient.data.l[1] = l1;
	event.xclient.data.l[2] = l2;
	event.xclient.data.l[3] = l3;
	event.xclient.data.l[4] = l4;

	XSendEvent(display, window, True,
		StructureNotifyMask,
		(XEvent *)&event);
}

static void reparent_all_to(Window new_parent)
{
	HAA_ActorPriv* a;
	/* video mode has changed */
	parent_window = new_parent;

	/* if we don't have any actors, no need to reparent them */
	if (first == NULL) {
		assert(last == NULL);
		return;
	}

	/* unmap all actors that were already ready */
	for (a = first; a; a = a->next) {
		a->parent = parent_window;
		if (a->ready) {
			XUnmapWindow(display, a->window);
		}
	}

	XSync(display, False);

	/* now remap and reconfigure all actors */
	for (a = first; a; a = a->next) {
		if (a->ready) {
			XMapWindow(display, a->window);
			a->ready = 0;
			a->p.pending |= HAA_PENDING_EVERYTHING;
		} else {
			a->p.pending |= HAA_PENDING_PARENT | HAA_PENDING_SHOW;
		}
	}

	XFlush(display);
}

static int auto_reparent_all_to(Bool fullscreen)
{
	SDL_SysWMinfo info;
	Window new_parent;
	XWindowAttributes attr;
	Bool is_mapped;
	int res;

	SDL_VERSION(&info.version);
	res = SDL_GetWMInfo(&info);
	assert(res == 1);

	/* Delete any pending reparent */
	queued_reparent_time = 0;

	if (fullscreen) {
		new_parent = info.info.x11.fswindow;
	} else {
		new_parent = info.info.x11.wmwindow;
	}

	XGetWindowAttributes(display, new_parent, &attr);
	is_mapped = attr.map_state == IsViewable;

	/* Do we really need to reparent? */
	if (new_parent != parent_window) {
		/* Yes, we do. */
		if (is_mapped) {
			reparent_all_to(new_parent);
			return 0;
		} else {
			return 1;
		}
	} else if (!is_mapped) {
		/* We don't actually need to reparent,
		 * but we found that our current parent has been unmapped! */
		parent_window = 0; // Make current one it invalid
		return 1; // Signal failure
	} else {
		/* We don't need to reparent and everything is OK. */
		return 0;
	}
}

static void handle_queued_reparent()
{
	if (queued_reparent_time) {
		/* A reparent is pending. */
		Uint32 now = SDL_GetTicks();
		if (now > queued_reparent_time) {
			/* Try to do the queued reparent now. */
			int res = auto_reparent_all_to(queued_reparent_fs);
			if (res != 0) {
				/* Failed to reparent? Try again in 200 ms. */
				queued_reparent_time = now + 200;
			}
		}
	}
}

static void queue_auto_reparent_to(Bool fullscreen, Uint32 delay)
{
	queued_reparent_time = SDL_GetTicks() + delay;
	queued_reparent_fs = fullscreen;
}

int HAA_SetVideoMode()
{
	SDL_Surface *screen = SDL_GetVideoSurface();

	if (!screen) {
		SDL_SetError("Failed to get current video surface");
		return 1;
	}

	auto_reparent_all_to(screen->flags & SDL_FULLSCREEN ? True : False);

	return 0;
}

static void HAA_Pending(HAA_ActorPriv* actor)
{
	const Uint16 pending = actor->p.pending;

	if (!actor->ready) return; //Enqueue and wait

	if (pending & HAA_PENDING_ANCHOR) {
		 actor_send_message(actor,
		 	ATOM(_HILDON_ANIMATION_CLIENT_MESSAGE_ANCHOR),
			actor->p.gravity, actor->p.anchor_x, actor->p.anchor_y, 0, 0);
	}
	if (pending & HAA_PENDING_POSITION) {
		 actor_send_message(actor,
		 	ATOM(_HILDON_ANIMATION_CLIENT_MESSAGE_POSITION),
			actor->p.position_x, actor->p.position_y, actor->p.depth, 0, 0);
	}

	if (pending & HAA_PENDING_ROTATION_X) {
		 actor_send_message(actor,
		 	ATOM(_HILDON_ANIMATION_CLIENT_MESSAGE_ROTATION),
			HAA_X_AXIS,
			actor->p.x_rotation_angle,
			0, actor->p.x_rotation_y, actor->p.x_rotation_z);
	}
	if (pending & HAA_PENDING_ROTATION_Y) {
		 actor_send_message(actor,
		 	ATOM(_HILDON_ANIMATION_CLIENT_MESSAGE_ROTATION),
			HAA_Y_AXIS,
			actor->p.y_rotation_angle,
			actor->p.y_rotation_x, 0, actor->p.y_rotation_z);
	}
	if (pending & HAA_PENDING_ROTATION_Z) {
		 actor_send_message(actor,
		 	ATOM(_HILDON_ANIMATION_CLIENT_MESSAGE_ROTATION),
			HAA_Z_AXIS,
			actor->p.z_rotation_angle,
			actor->p.z_rotation_x, actor->p.z_rotation_y, 0);
	}

	if (pending & HAA_PENDING_SCALE) {
		 actor_send_message(actor,
		 	ATOM(_HILDON_ANIMATION_CLIENT_MESSAGE_SCALE),
			actor->p.scale_x, actor->p.scale_y, 0, 0, 0);
	}

	if (pending & HAA_PENDING_PARENT) {
		 actor_send_message(actor,
			ATOM(_HILDON_ANIMATION_CLIENT_MESSAGE_PARENT),
			actor->parent, 0, 0, 0, 0);
	}
	if (pending & HAA_PENDING_SHOW) {
		 actor_send_message(actor,
		 	ATOM(_HILDON_ANIMATION_CLIENT_MESSAGE_SHOW),
			actor->p.visible, actor->p.opacity, 0, 0, 0);
	}

	actor->p.pending = HAA_PENDING_NOTHING;
}

/** Push a SDL_VIDEOEXPOSE event to the queue */
static void sdl_expose()
{
	SDL_Event events[32];

	/* Pull out all old refresh events */
	SDL_PeepEvents(events, sizeof(events)/sizeof(events[0]),
		SDL_GETEVENT, SDL_VIDEOEXPOSEMASK);

	/* Post the event, if desired */
	if ( SDL_EventState(SDL_VIDEOEXPOSE, SDL_QUERY) == SDL_ENABLE ) {
		SDL_Event event;
		event.type = SDL_VIDEOEXPOSE;
		SDL_PushEvent(&event);
	}
}

/** Called when the client ready notification is received. */
static void actor_update_ready(HAA_ActorPriv* actor)
{
	Window window = actor->window;
	int status;
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop = NULL;

	status = XGetWindowProperty(display, window,
		ATOM(_HILDON_ANIMATION_CLIENT_READY), 0, 32,
		False, XA_ATOM,
		&actual_type, &actual_format,
		&nitems, &bytes_after, &prop);

	if (prop) {
		XFree(prop);
	}

	if (status != Success || actual_type != XA_ATOM ||
			actual_format != 32 || nitems != 1)  {
		actor->ready = 0;
		return;
	}

	if (actor->ready) {
		/* Ready flag already set, which means hildon-desktop just restarted.
		  Reset this actor. */
		XUnmapWindow(display, window);
		XSync(display, False);
		XMapWindow(display, window);
		XSync(display, False);

		/* Hildon-desktop restarting means all the system will be under
		  extreme load. Relax. */
		// TODO: Delay once per crash, not per actor.
		SDL_Delay(500);

		/* Next Flip will resend every setting */
		actor->p.pending = HAA_PENDING_EVERYTHING;
		return;
	}

	actor->ready = 1;

	/* Send all pending messages now */
	HAA_Pending(actor);

	/* Force a redraw */
	sdl_expose();
}

int HAA_FilterEvent(const SDL_Event *event)
{
	handle_queued_reparent();

	if (event->type == SDL_SYSWMEVENT) {
		const XEvent *e = &event->syswm.msg->event.xevent;
		if (e->type == PropertyNotify) {
			if (e->xproperty.atom == ATOM(_HILDON_ANIMATION_CLIENT_READY)) {
				HAA_ActorPriv* actor =
					find_actor_for_window(e->xproperty.window);
				if (actor) {
					actor_update_ready(actor);
					return 0; // Handled
				}
			}
		}
	} else if (event->type == SDL_ACTIVEEVENT) {
		/* We know that after an input focus loss, the fullscreen window
		 * will be unampped. We get no warnings about when this happens.
		 * So we take a preventive approach and automatically reparent to the
		 * windowed window when any out of focus event happens.
		 * Of course, we have then to reparent to the fullscreen window when
		 * the focus comes back. But we have a problem: there's a 1.5 second
		 * delay between getting focus and SDL actually mapping the fullscreen
		 * window, and we cannot reparent back to the fullscreen window while
		 * it is unmapped.
		 */
		if (event->active.state == SDL_APPINPUTFOCUS) {
			SDL_Surface *screen = SDL_GetVideoSurface();
			if (screen && screen->flags & SDL_FULLSCREEN) {
				if (event->active.gain) {
					/* Gaining fullscreen focus:
					 * Wait for 1.5 seconds before reparenting to fullscreen.
					 */
					queue_auto_reparent_to(True, 1500);
				} else {
					/* Losing fullscreen focus:
					 * Windowed mode window is always mapped; can reparent now.
					 */
					auto_reparent_all_to(False);
				}
			}
		}
	}

	return 1; // Unhandled event
}

HAA_Actor* HAA_CreateActor(Uint32 flags,
	int width, int height, int bitsPerPixel)
{
	HAA_ActorPriv *actor = malloc(sizeof(HAA_ActorPriv));
	if (!actor) {
		SDL_Error(SDL_ENOMEM);
		return NULL;
	}

	/* Refresh the parent_window if needed. */
	int res = HAA_SetVideoMode();
	if (res != 0) {
		goto cleanup_actor;
		return NULL;
	}

	/* Default actor settings */
	actor->p.position_x = 0;
	actor->p.position_y = 0;
	actor->p.depth = 0;
	actor->p.visible = 0;
	actor->p.opacity = 255;
	actor->parent = parent_window;
	actor->p.scale_x = 1 << 16;
	actor->p.scale_y = 1 << 16;
	actor->p.gravity = HAA_GRAVITY_NONE;
	actor->p.anchor_x = 0;
	actor->p.anchor_y = 0;
	actor->p.x_rotation_angle = 0;
	actor->p.x_rotation_y = 0;
	actor->p.x_rotation_z = 0;
	actor->p.y_rotation_angle = 0;
	actor->p.y_rotation_x = 0;
	actor->p.y_rotation_z = 0;
	actor->p.z_rotation_angle = 0;
	actor->p.z_rotation_x = 0;
	actor->p.z_rotation_y = 0;
	actor->p.pending =
		HAA_PENDING_POSITION | HAA_PENDING_SCALE | HAA_PENDING_PARENT;
	actor->ready = 0;

	/* Select the X11 visual */
	int screen = DefaultScreen(display);
	Window root = RootWindow(display, screen);
	XVisualInfo vinfo;
	XImage *image;
	void* pixels = NULL;
	if (!XMatchVisualInfo(display, screen, bitsPerPixel, TrueColor, &vinfo)) {
		/* Not matched; Use the default visual instead */
		int numVisuals;
		XVisualInfo *xvi;

		vinfo.screen = screen;
		xvi = XGetVisualInfo(display, VisualScreenMask, &vinfo, &numVisuals);
		assert(xvi);

		vinfo = *xvi;
		XFree(xvi);
	}
	actor->visual = vinfo.visual;
	
	if (vinfo.visual != DefaultVisual(display, screen)) {
		/* Allocate a private color map. */
		actor->colormap = XCreateColormap(display, root, 
			vinfo.visual, AllocNone);
	} else {
		actor->colormap = 0;
	}

	/* Create X11 window for actor */
	XSetWindowAttributes attr;
	unsigned long attrmask = CWBorderPixel | CWBackPixel | CWBitGravity;
	attr.background_pixel = BlackPixel(display, screen);
	attr.border_pixel = attr.background_pixel;
	attr.bit_gravity = ForgetGravity;
	if (actor->colormap) {
		attr.colormap = actor->colormap;
		attrmask |= CWColormap;
	}

	Window window = actor->window = XCreateWindow(display, root,
		0, 0, width, height, 0, vinfo.depth,
		InputOutput, vinfo.visual,
		attrmask, &attr);

	XStoreName(display, window, "sdl_haa window");

	Atom atom = ATOM(_HILDON_WM_WINDOW_TYPE_ANIMATION_ACTOR);
	XChangeProperty(display, window, ATOM(_NET_WM_WINDOW_TYPE),
		XA_ATOM, 32, PropModeReplace,
		(unsigned char *) &atom, 1);

	/* Setup the X Image */
	if (have_shm) {
		image = actor->image = XShmCreateImage(display, vinfo.visual,
			vinfo.depth, ZPixmap, NULL, &actor->shminfo, width, height);
		if (!image) {
			SDL_SetError("Cannot create XSHM image");
			goto cleanup_window;
		}

		actor->shminfo.shmid = shmget(IPC_PRIVATE,
			image->bytes_per_line * image->height, IPC_CREAT|0777);
		if (actor->shminfo.shmid < 0) {
			SDL_SetError("Failed to get shared memory");
			XDestroyImage(image);
			goto cleanup_window;
		}

		actor->shminfo.shmaddr = shmat(actor->shminfo.shmid, NULL, 0);
		if (!actor->shminfo.shmaddr) {
			SDL_SetError("Failed to attach shared memory");
			XDestroyImage(image);
			shmctl(actor->shminfo.shmid, IPC_RMID, 0);
			goto cleanup_window;
		}

		actor->shminfo.readOnly = True;
		if (!XShmAttach(display, &actor->shminfo)) {
			SDL_SetError("Failed to attach shared memory image");
			XDestroyImage(image);
			shmdt(actor->shminfo.shmaddr);
			shmctl(actor->shminfo.shmid, IPC_RMID, 0);
			goto cleanup_window;
		}

		/* Ensure attachment is done */
		XSync(display, False);

		/* Nobody else needs it now */
		shmctl(actor->shminfo.shmid, IPC_RMID, 0);

		pixels = actor->shminfo.shmaddr;
		image->data = (char*) pixels;
	} else {
		pixels = malloc(width * height * (vinfo.depth / 8));
		if (!pixels) {
			SDL_SetError("Cannot allocate image");
			goto cleanup_window;
		}
		image = actor->image = XCreateImage(display, vinfo.visual,
			vinfo.depth, ZPixmap, 0, (char*) pixels, width, height, 8, 0);
		if (!image) {
			SDL_SetError("Cannot create X image");
			goto cleanup_window;
		}
	}

	/* Guess alpha mask */
	Uint32 Amask = 0;
	if (image->depth == 32) {
		Amask = ~(vinfo.red_mask | vinfo.green_mask | vinfo.blue_mask);
	}

	/* Create GC */
	GC gc = actor->gc = XCreateGC(display, window, 0, NULL);
	XSetForeground(display, gc, 0xFFFFFFFFU);

	/** Create SDL texture for actor */
	actor->p.surface = SDL_CreateRGBSurfaceFrom(pixels,
		image->width, image->height, image->depth, image->bytes_per_line,
		vinfo.red_mask, vinfo.green_mask, vinfo.blue_mask, Amask);

	if (!actor->p.surface) {
		/* SDL Error already set */
		goto cleanup_gc;
	}

	/* Map X11 window */
	XSelectInput(display, window, PropertyChangeMask);
	XMapWindow(display, window);

	/* Add to actor linked list */
	if (first == NULL) {
		assert(last == NULL);
		actor->next = actor->prev = NULL;
		first = last = actor;
	} else {
		last->next = actor;
		actor->prev = last;
		actor->next = NULL;
		last = actor;
	}

	XSync(display, False);
	return (HAA_Actor*) actor;

cleanup_gc:
	XFreeGC(display, gc);
cleanup_image:
	if (have_shm) {
		XShmDetach(display, &actor->shminfo);
		XDestroyImage(image);
		shmdt(actor->shminfo.shmaddr);
	} else {
		XDestroyImage(image);
	}
cleanup_window:
	XDestroyWindow(display, window);
	if (actor->colormap) XFreeColormap(display, actor->colormap);
cleanup_actor:
	free(actor);

	XSync(display, True);
	return NULL;
}
	
void HAA_FreeActor(HAA_Actor* a)
{
	HAA_ActorPriv* actor = (HAA_ActorPriv*)a;
	if (!a) return;

	XFreeGC(display, actor->gc);
	if (have_shm) {
		XShmDetach(display, &actor->shminfo);
		XDestroyImage(actor->image);
		shmdt(actor->shminfo.shmaddr);
		XDestroyWindow(display, actor->window);
	} else {
		XDestroyImage(actor->image);
	}
	if (actor->colormap)
		XFreeColormap(display, actor->colormap);
	SDL_FreeSurface(actor->p.surface);

	/* Remove actor from global linked list */
	if (first == actor && last == actor) {
		assert(!actor->next && !actor->prev);
		first = NULL;
		last = NULL;
	} else if (last == actor) {
		assert(!actor->next && actor->prev);
		last = actor->prev;
		last->next = NULL;
	} else if (first == actor) {
		assert(actor->next && !actor->prev);
		first = actor->next;
		first->prev = NULL;
	} else {
		assert(actor->next && actor->prev);
		actor->prev->next = actor->next;
		actor->next->prev = actor->prev;
	}

	free(actor);

	XFlush(display);
}

int HAA_Commit(HAA_Actor* a)
{
	HAA_ActorPriv* actor = (HAA_ActorPriv*)a;

	HAA_Pending(actor);
	XSync(display, False);

	return 0;
}

int HAA_Flip(HAA_Actor* a)
{
	HAA_ActorPriv* actor = (HAA_ActorPriv*)a;
	Window window = actor->window;
	GC gc = actor->gc;
	XImage *image = actor->image;

	if (have_shm) {
		XShmPutImage(display, window, gc, image, 
						0, 0, 0, 0, image->width, image->height, False);
	} else {
		XPutImage(display, window, gc, image, 
					0, 0, 0, 0, image->width, image->height);
	}

	HAA_Pending(actor);
	XSync(display, False);

	return 0;
}

