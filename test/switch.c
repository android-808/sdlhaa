/* fullscreen - a SDL_haa sample able to enter fullscreen mode
 *
 * This file is in the public domain, furnished "as is", without technical
 * support, and with no warranty, express or implied, as to its usefulness for
 * any purpose.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL.h>
#include <SDL_haa.h>

static bool fullscreen = false;

static SDL_Surface *screen;

static HAA_Actor *actor;

static int degrees = 0;

static Uint32 tick(Uint32 interval, void* param)
{
	SDL_Event e;
	e.type = SDL_VIDEOEXPOSE;

	degrees = (degrees+2) % 360;
	SDL_PushEvent(&e);

	return interval;
}

static void setup_video_mode()
{
	unsigned flags = SDL_SWSURFACE | (fullscreen ? SDL_FULLSCREEN : 0);
	printf("video mode ...\n");
	screen = SDL_SetVideoMode(0, 0, 16, flags);
	assert(screen);
	printf("updating haa ...\n");
	int res = HAA_SetVideoMode();
	assert(res == 0);
	printf("video mode set\n");
}

int main()
{
	int res;
	res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	assert(res == 0);

	res = HAA_Init(0);
	assert(res == 0);

	setup_video_mode();

	SDL_TimerID timer = SDL_AddTimer(10, tick, NULL);
	assert(timer != NULL);

	actor = HAA_CreateActor(SDL_SWSURFACE, 200, 200, 16);
	assert(actor);

	SDL_FillRect(actor->surface, NULL,
		SDL_MapRGB(actor->surface->format, 255, 255, 0));

	HAA_SetPosition(actor, 600, 120);
	HAA_Show(actor);

	SDL_Event event;
	while (SDL_WaitEvent(&event)) {
		if (HAA_FilterEvent(&event) == 0) continue;
		switch (event.type) {
			case SDL_QUIT:
				goto quit;
			case SDL_VIDEOEXPOSE:
				//HAA_SetRotation(actor, HAA_Y_AXIS, degrees, 0, 0, 0);
				res = HAA_Flip(actor);
				assert(res == 0);
				break;
			case SDL_MOUSEBUTTONUP:
				printf("Switching fullscreen\n");
				fullscreen = !fullscreen;
				setup_video_mode();
				break;
		}
	}

quit:
	HAA_FreeActor(actor);

	HAA_Quit();
	SDL_Quit();

	return 0;
}

