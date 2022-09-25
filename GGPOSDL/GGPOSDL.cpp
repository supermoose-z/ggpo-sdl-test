// GGPOSDL.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <cmath>

#include <Windows.h>

#include <SDL.h>
#include <ggponet.h>
#include "GameState.h"

#define FRAME_LENGTH 16

using namespace std;

enum class GameStatus : int
{
    None = 0,
    Disconnected = 1,
    Waiting = 2,
    Running = 3,
    Ahead = 4,
    Interrupted = 5,
};

class GGPOGame
{
public:
    static GGPOGame* main;
    GameState* state;
    GGPOSession *session;
    int localPlayer, remotePlayer;
    int localInput;
    Uint64 next, now;
    GameStatus status;
    
    GGPOGame(GameState* s)
    {
        localInput = PADDLE_IDLE;
        localPlayer = 1;
        remotePlayer = 0;
        session = NULL; 
        state = s;
        status = GameStatus::None;
        
        now = SDL_GetTicks64();
        next = now + FRAME_LENGTH;
    }

    void initCallbacks(GGPOSessionCallbacks &cb)
    {
        cb.begin_game = &begin_game;
        cb.save_game_state = &save_game_state;
        cb.load_game_state = &load_game_state;
        cb.log_game_state = &log_game_state;
        cb.free_buffer = &free_buffer;
        cb.advance_frame = &advance_frame;
        cb.on_event = &on_event;
    }

    int getLocalInput()
    {
        int num = 0;
        const Uint8* state = SDL_GetKeyboardState(&num);

        if (state[SDL_SCANCODE_W])
            return PADDLE_UP;
        
        if (state[SDL_SCANCODE_S])
            return PADDLE_DOWN;

        return PADDLE_IDLE;
    }

    void advanceGame()
    {
        int inputs[2] = { 64, 64 };
        int dcflags = 0;

        int result = ggpo_synchronize_input(session, &inputs, sizeof(inputs), &dcflags);

        if (result == GGPO_OK)
        {
            for (int i = 0; i < 2; i++)
                state->paddles[i]->dir = inputs[i];

            state->updateGame();

            ggpo_advance_frame(session);
        }
    }

    void update()
    {
        now = SDL_GetTicks64();
        
        if (session != NULL && status >= GameStatus::Waiting)
        {
            int ticks = next - now;

            if (ticks < 0)
                ticks = 0;

            ggpo_idle(session, ticks);
        }
            
        if (now >= next)
        {
            next = now + FRAME_LENGTH;

            if (status == GameStatus::Running)
            {
                int input = getLocalInput();
                int dcflags = 0;

                int result = ggpo_add_local_input(session, localPlayer, &input, sizeof(input));
                if (result == GGPO_OK)
                {
                    advanceGame();
                }
            }
        }
    }

    void setMainGame()
    {
        main = this;
    }

    void startSession(GGPOPlayer local, GGPOPlayer remote, Uint16 port)
    {
        GGPOSessionCallbacks cb;
        initCallbacks(cb);

        ggpo_start_session(&session, &cb, "GGPO Test?", 2, 4, port);

        ggpo_set_disconnect_timeout(session, 3000);
        ggpo_set_disconnect_notify_start(session, 1000);

        ggpo_add_player(session, &local, &localPlayer);
        ggpo_add_player(session, &remote, &remotePlayer);

        ggpo_set_frame_delay(session, localPlayer, 1);

        status = GameStatus::Waiting;
    }

    // GGPO callbacks
    static bool begin_game(const char* game)
    {
        cout << "Begin game called?" << endl;
        //main->status = GameStatus::Running;
        return true;
    }

    static bool save_game_state(unsigned char** buffer, int* len, int* checksum, int frame)
    {
        SavedState state;
        Uint8* buf = new Uint8[sizeof(state)];

        main->state->saveState(state);

        memcpy(buf, &state, sizeof(state));

        *buffer = buf;
        *len = sizeof(state);
        *checksum = 0;

        return true;
    }

    static bool load_game_state(unsigned char* buffer, int len)
    {
        SavedState state;

        memcpy(&state, buffer, len);

        main->state->restoreState(state);

        return true;
    }

    static bool log_game_state(char* filename, unsigned char* buffer, int len)
    {
        return true;
    }
    
    static void free_buffer(void* buffer)
    {
        Uint8* buf = (Uint8*)buffer;
        delete buf;
    }
    
    static bool advance_frame(int flags)
    {
        main->advanceGame();
        return true;
    }

    static bool on_event(GGPOEvent* info)
    {
        cout << "Event received" << endl;

        switch (info->code)
        {
        case GGPO_EVENTCODE_CONNECTED_TO_PEER:
            std::cout << "Connected to peer" << std::endl;
            break;

        case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
            cout << "Synchronizing: " << info->u.synchronizing.count << " out of " << info->u.synchronizing.total << endl;
            break;

        case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
            cout << "Synchronized!" << endl;
            break;

        case GGPO_EVENTCODE_RUNNING:
            main->status = GameStatus::Running;
            cout << "Running!" << endl;
            break;

        case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
            main->status = GameStatus::Disconnected;
            cout << "Disconnected" << endl;
            break;

        case GGPO_EVENTCODE_TIMESYNC:
            main->status = GameStatus::Ahead;
            cout << "Ahead " << info->u.timesync.frames_ahead << endl;
            break;

        case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
            main->status = GameStatus::Interrupted;
            cout << "Interrupted" << endl;
            break;

        case GGPO_EVENTCODE_CONNECTION_RESUMED:
            main->status = GameStatus::Running;
            cout << "Resuming" << endl;
            break;
        }

        return true;
    }
};

GGPOGame* GGPOGame::main = NULL;

int main(int argc, char *argv[])
{
    WSADATA data;

    WSAStartup(MAKEWORD(2, 2), &data);

    int playerNum = 1;
    char ip[] = "127.0.0.1\0";
    Uint16 port;
    GameState state;
    GGPOPlayer local, remote;
    GGPOGame ggpoGame(&state);

    ggpoGame.setMainGame();

    if (argc >= 2)
    {
        std::string s(argv[1]);
        
        if (s == "1")
        {
            playerNum = 1;
        }
        else if (s == "2")
        {
            playerNum = 2;
        }
        else
        {
            std::cout << "invalid argument: " << s << std::endl;
            return 1;
        }
    }

    if (playerNum == 1)
    {
        port = 10001;

        local.player_num = 1;
        local.type = GGPO_PLAYERTYPE_LOCAL;
        local.size = sizeof(local);

        remote.player_num = 2;
        remote.type = GGPO_PLAYERTYPE_REMOTE;
        remote.u.remote.port = 10002;
        remote.size = sizeof(remote);
    }
    else if (playerNum == 2)
    {
        port = 10002;

        local.player_num = 2;
        local.type = GGPO_PLAYERTYPE_LOCAL;
        local.size = sizeof(local);

        remote.player_num = 1;
        remote.type = GGPO_PLAYERTYPE_REMOTE;
        remote.u.remote.port = 10001;
        remote.size = sizeof(remote);
    }
    
    memcpy(remote.u.remote.ip_address, ip, strlen(ip));

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Renderer* render;
    SDL_Window* win;

    SDL_CreateWindowAndRenderer(1600, 900, 0, &win, &render);

    SDL_Event evt;
    bool done = false;

    ggpoGame.startSession(local, remote, port);

    while (!done)
    {
        //ggpoGame.setLocalInput(PADDLE_IDLE);

        while (SDL_PollEvent(&evt))
        {
            switch (evt.type)
            {
            case SDL_QUIT:
                done = true;
                break;

            case SDL_KEYDOWN:
                switch (evt.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                    done = true;
                    break;
                }
            }
        }

        ggpoGame.update();

        SDL_SetRenderDrawColor(render, 0, 0, 0, 0);
        SDL_RenderClear(render);

        state.drawGame(render);

        SDL_RenderPresent(render);
    }

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(win);

    SDL_Quit();

    WSACleanup();
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
