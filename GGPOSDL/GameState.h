
#pragma once

#include <SDL.h>

#define PADDLE_IDLE 1
#define PADDLE_UP 2
#define PADDLE_DOWN 3

struct SavedState
{
	struct
	{
		int x, y, dir;
	} p1;

	struct
	{
		int x, y, dir;
	} p2;
};

class Paddle
{
public:
	int x, y, dir;

	Paddle();

	void updatePaddle();
	void drawPaddle(SDL_Renderer* render);
};

class GameState
{
public:
	Paddle p1, p2;
	Paddle* paddles[2];

	GameState();

	void updateGame();
	void drawGame(SDL_Renderer* render);

	void saveState(SavedState& s);
	void restoreState(SavedState& s);
};

