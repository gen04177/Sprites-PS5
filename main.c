#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

//#define NUM_SPRITES 60000 // out of memory (PS5 4.50)
//#define NUM_SPRITES 50000 // FPS=6.6 (PS5 4.50)
#define NUM_SPRITES 100 // FPS=30
#define MAX_SPEED   3

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture **sprites;
static int num_sprites;
static SDL_Rect *positions;
static SDL_Rect *velocities;
static int sprite_w, sprite_h;

static TTF_Font *font;
static SDL_Texture *fpsText;
static SDL_Rect fpsRect;
static SDL_Color textColor = {255, 0, 0};

static Uint32 next_fps_check, frames, last_frame_time;
static const Uint32 fps_check_delay = 1000; //delay

int LoadSprite(const char *file);
void renderFPS();
void MoveSprites();

int LoadSprite(const char *file) {
    SDL_Surface *surface = SDL_LoadBMP(file);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load surface from %s: %s", file, SDL_GetError());
        return -1;
    }

    sprites = (SDL_Texture **)malloc(sizeof(SDL_Texture *) * NUM_SPRITES);
    if (!sprites) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for sprite textures");
        SDL_FreeSurface(surface);
        return -1;
    }

    for (int i = 0; i < NUM_SPRITES; ++i) {
        sprites[i] = SDL_CreateTextureFromSurface(renderer, surface);
        if (!sprites[i]) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create texture from surface: %s", SDL_GetError());
            SDL_FreeSurface(surface);
            for (int j = 0; j < i; ++j) {
                SDL_DestroyTexture(sprites[j]);
            }
            free(sprites);
            return -1;
        }
    }

    SDL_FreeSurface(surface);

    SDL_QueryTexture(sprites[0], NULL, NULL, &sprite_w, &sprite_h);

    return 0;
}

void renderFPS() {
    Uint32 current_time = SDL_GetTicks();
    frames++;

    if (current_time > next_fps_check) {
        float fps = frames * 1000.0 / (current_time - next_fps_check + fps_check_delay);
        next_fps_check = current_time + fps_check_delay;

        char fpsTextString[16];
        snprintf(fpsTextString, sizeof(fpsTextString), "FPS: %.1f", fps);

        SDL_Surface *fpsSurface = TTF_RenderText_Solid(font, fpsTextString, textColor);
        if (!fpsSurface) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create surface from font: %s\n", TTF_GetError());
            return;
        }

        SDL_DestroyTexture(fpsText);
        fpsText = SDL_CreateTextureFromSurface(renderer, fpsSurface);
        if (!fpsText) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s\n", SDL_GetError());
            SDL_FreeSurface(fpsSurface);
            return;
        }
        SDL_FreeSurface(fpsSurface);

        SDL_QueryTexture(fpsText, NULL, NULL, &fpsRect.w, &fpsRect.h);
        fpsRect.x = SDL_GetWindowSurface(window)->w - fpsRect.w - 10;
        fpsRect.y = 10;

        frames = 0;
    }
}

void MoveSprites() {
    SDL_Rect viewport;
    SDL_GetRendererOutputSize(renderer, &viewport.w, &viewport.h);

    SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
    SDL_RenderClear(renderer);

    for (int i = 0; i < num_sprites; ++i) {
        positions[i].x += velocities[i].x;
        positions[i].y += velocities[i].y;

        if (positions[i].x < 0 || positions[i].x > viewport.w - sprite_w) {
            velocities[i].x = -velocities[i].x;
            positions[i].x += velocities[i].x;
        }
        if (positions[i].y < 0 || positions[i].y > viewport.h - sprite_h) {
            velocities[i].y = -velocities[i].y;
            positions[i].y += velocities[i].y;
        }

        SDL_RenderCopy(renderer, sprites[i], NULL, &positions[i]);
    }

    SDL_RenderCopy(renderer, fpsText, NULL, &fpsRect);

    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv) {

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Sprites and FPS Counter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    font = TTF_OpenFont("/data/arial.ttf", 20);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_OpenFont failed: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (LoadSprite("/data/icon.bmp") < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load sprite");
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    num_sprites = NUM_SPRITES;
    positions = (SDL_Rect *)malloc(num_sprites * sizeof(SDL_Rect));
    velocities = (SDL_Rect *)malloc(num_sprites * sizeof(SDL_Rect));
    if (!positions || !velocities) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for sprite positions and velocities");
        for (int i = 0; i < num_sprites; ++i) {
            SDL_DestroyTexture(sprites[i]);
        }
        free(sprites);
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    srand((unsigned int)time(NULL));
    for (int i = 0; i < num_sprites; ++i) {
        positions[i].x = rand() % 800;
        positions[i].y = rand() % 600;
        positions[i].w = sprite_w;
        positions[i].h = sprite_h;
        velocities[i].x = (rand() % (MAX_SPEED * 2)) - MAX_SPEED;
        velocities[i].y = (rand() % (MAX_SPEED * 2)) - MAX_SPEED;

        if (velocities[i].x == 0) velocities[i].x = 1;
        if (velocities[i].y == 0) velocities[i].y = 1;
    }

    SDL_Event event;
    int quit = 0;
    next_fps_check = SDL_GetTicks() + fps_check_delay;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }

        MoveSprites();

        renderFPS();

        SDL_Delay(16);
    }

    SDL_DestroyTexture(sprites[0]);
    free(positions);
    free(velocities);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
