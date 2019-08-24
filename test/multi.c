/* multi - a SDL_haa sample with more than one actor
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

static HAA_Actor *actor1, *actor2, *actor3;

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
	
	SDL_TimerID timer = SDL_AddTimer(1, tick, NULL);
	assert(timer != NULL);

	actor1 = HAA_CreateActor(0, 200, 200, 16);
	assert(actor1);
	
	actor2 = HAA_CreateActor(0, 200, 200, 16);
	assert(actor2);
	
	actor3 = HAA_CreateActor(0, 200, 200, 16);
	assert(actor3);
	
	SDL_FillRect(actor1->surface, NULL,
		SDL_MapRGB(actor1->surface->format, 255, 0, 0));
	SDL_FillRect(actor2->surface, NULL,
		SDL_MapRGB(actor2->surface->format, 0, 255, 0));
	SDL_FillRect(actor3->surface, NULL,
		SDL_MapRGB(actor3->surface->format, 0, 0, 255));
	
	HAA_SetPosition(actor1, 100, 150);
	HAA_Show(actor1);
	
	HAA_SetPosition(actor2, 300, 150);
	HAA_Show(actor2);
	
	HAA_SetPosition(actor3, 500, 150);
	HAA_Show(actor3);
	
	res = HAA_Flip(actor1);
	assert(res == 0);
	res = HAA_Flip(actor2);
	assert(res == 0);
	res = HAA_Flip(actor3);
	assert(res == 0);

	SDL_Event event;
	while (SDL_WaitEvent(&event)) {
		if (HAA_FilterEvent(&event) == 0) continue;
		switch (event.type) {
			case SDL_QUIT:
				goto quit;
			case SDL_VIDEOEXPOSE:
				res = HAA_Flip(actor1);
				assert(res == 0);
				res = HAA_Flip(actor2);
				assert(res == 0);
				res = HAA_Flip(actor3);
				assert(res == 0);
				break;
			case SDL_USEREVENT:
				HAA_SetRotation(actor1, HAA_X_AXIS, degrees, 0, 0, 0);
				HAA_SetRotation(actor2, HAA_Y_AXIS, degrees, 0, 0, 0);
				HAA_SetRotation(actor3, HAA_Z_AXIS, degrees, 0, 0, 0);
				res = HAA_Commit(actor1);
				assert(res == 0);
				res = HAA_Commit(actor2);
				assert(res == 0);
				res = HAA_Commit(actor3);
				assert(res == 0);
				break;
		}
	}

quit:
	HAA_FreeActor(actor1);
	HAA_FreeActor(actor2);
	HAA_FreeActor(actor3);

	HAA_Quit();
	SDL_Quit();

	return 0;
}
