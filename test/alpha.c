/* alpha - a simple SDL_haa sample with a RGBA transparent actor
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

static SDL_Surface *screen;

static HAA_Actor *actor;

static int degrees = 0;

static Uint32 tick(Uint32 interval, void* param)
{
	SDL_UserEvent e;
	e.type = SDL_USEREVENT;

	degrees = (degrees+2) % 360;
	SDL_PushEvent((SDL_Event*)&e);

	return interval;
}

int main()
{
	int res;
	res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	assert(res == 0);

	res = HAA_Init(0);
	assert(res == 0);

	screen = SDL_SetVideoMode(0, 0, 16, SDL_SWSURFACE);
	assert(screen);

	SDL_FillRect(screen, NULL,
		SDL_MapRGBA(screen->format, 150, 0, 0, 255));
		
	SDL_Rect dark = {350, 180, 160, 50};
	SDL_FillRect(screen, &dark,
		SDL_MapRGBA(screen->format, 255, 0, 0, 255));

	actor = HAA_CreateActor(SDL_SWSURFACE, 200, 200, 32);
	assert(actor);

	SDL_FillRect(actor->surface, NULL,
		SDL_MapRGBA(actor->surface->format, 0, 255, 0, 250));

	SDL_Rect hole = {63, 63, 74, 74};
	SDL_FillRect(actor->surface, &hole,
		SDL_MapRGBA(actor->surface->format, 0, 0, 255, 80));

	HAA_SetPosition(actor, 400, 160);
	HAA_Show(actor);

	SDL_TimerID timer = SDL_AddTimer(10, tick, NULL);
	assert(timer != NULL);

	SDL_Event event;
	while (SDL_WaitEvent(&event)) {
		if (HAA_FilterEvent(&event) == 0) continue;
		switch (event.type) {
			case SDL_QUIT:
				goto quit;
			case SDL_VIDEOEXPOSE:
				res = HAA_Flip(actor);
				assert(res == 0);
				res = SDL_Flip(screen);
				assert(res == 0);
				break;
			case SDL_USEREVENT:
				HAA_SetRotation(actor, HAA_Y_AXIS, degrees, 0, 0, 0);
				res = HAA_Commit(actor);
				assert(res == 0);
				break;
		}
	}

quit:
	HAA_FreeActor(actor);

	HAA_Quit();
	SDL_Quit();

	return 0;
}
