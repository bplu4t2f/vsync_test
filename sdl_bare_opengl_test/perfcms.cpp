#include <SDL.h>
#include "perfcms.h"

static Uint64 performance_frequency = SDL_GetPerformanceFrequency();

double get_elapsed_ms(Uint64 before)
{
	Uint64 after = SDL_GetPerformanceCounter();
	return (double)(after - before) * 1000.0 / (double)performance_frequency;
}

double make_ms(Sint64 diff)
{
	return (double)(diff) * 1000.0 / (double)performance_frequency;
}
