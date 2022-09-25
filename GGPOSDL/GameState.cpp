#include "GameState.h"

Paddle::Paddle()
{
	x = 0;
	y = 0;
	dir = 0;
}

void Paddle::updatePaddle()
{
	int speed = 5;
	int dy = 0;

	if (dir == PADDLE_UP)
		dy = -5;
	else if (dir == PADDLE_DOWN)
		dy = 5;
	else
		dy = 0;

	y += dy;
}

void Paddle::drawPaddle(SDL_Renderer* render)
{
	int width = 30;
	int height = 150;
	SDL_Rect area;

	area.x = x - (width / 2);
	area.y = y - (height / 2);
	area.w = width;
	area.h = height;

	SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
	SDL_RenderFillRect(render, &area);
}

GameState::GameState() : p1(), p2()
{
	paddles[0] = &p1;
	paddles[1] = &p2;

	p1.x = 60;
	p1.y = 900 / 2;

	p2.x = 1600 - 60;
	p2.y = 900 / 2;
}

void GameState::updateGame()
{
	paddles[0]->updatePaddle();
	paddles[1]->updatePaddle();
}

void GameState::drawGame(SDL_Renderer* r)
{
	paddles[0]->drawPaddle(r);
	paddles[1]->drawPaddle(r);
}

void GameState::saveState(SavedState& s)
{
	s.p1.x = p1.x;
	s.p1.y = p1.y;
	s.p1.dir = p1.dir;

	s.p2.x = p2.x;
	s.p2.y = p2.y;
	s.p2.dir = p2.dir;
}

void GameState::restoreState(SavedState& s)
{
	p1.x = s.p1.x;
	p1.y = s.p1.y;
	p1.dir = s.p1.dir;

	p2.x = s.p2.x;
	p2.y = s.p2.y;
	p2.dir = s.p2.dir;
}