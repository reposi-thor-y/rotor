#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define SAVE 100
#define REPS 50
#define MAXANGLE 10000.0
#define DEFAULTCOUNT 3

typedef struct {
    float angle;
    float radius;
    float start_radius;
    float end_radius;
    float radius_drift_max;
    float radius_drift_now;
    float ratio;
    float start_ratio;
    float end_ratio;
    float ratio_drift_max;
    float ratio_drift_now;
} Element;

typedef struct {
    Element *elements;
    int lastx, lasty;
    int num, rotor, prev;
    int savex[SAVE], savey[SAVE];
    float angle;
    int centerx, centery;
    bool firsttime;
    bool smallscreen;
    bool forward;
} FlightStruct;

void init_rotor(FlightStruct *fs, int width, int height, int batchcount) {
    fs->centerx = width / 2;
    fs->centery = height / 2;
    fs->smallscreen = (width < 100);

    if (batchcount > 12)
        batchcount = DEFAULTCOUNT;
    fs->num = batchcount;

    fs->elements = calloc(fs->num, sizeof(Element));
    if (fs->elements == NULL) {
        fprintf(stderr, "Failed to allocate memory for elements\n");
        exit(1);
    }

    for (int x = 0; x < fs->num; x++) {
        Element *pelem = &fs->elements[x];
        pelem->radius_drift_max = 1.0f;
        pelem->radius_drift_now = 1.0f;
        pelem->end_radius = 100.0f;
        pelem->ratio_drift_max = 1.0f;
        pelem->ratio_drift_now = 1.0f;
        pelem->end_ratio = 10.0f;
    }

    fs->rotor = 0;
    fs->prev = 1;
    fs->lastx = fs->centerx;
    fs->lasty = fs->centery;
    fs->angle = (float)(rand() % (long)MAXANGLE) / 3.0f;
    fs->forward = true;
    fs->firsttime = true;
}

void draw_rotor(SDL_Renderer *renderer, FlightStruct *fs) {
    for (int rp = 0; rp < REPS; rp++) {
        int thisx = fs->centerx;
        int thisy = fs->centery;

        for (int i = 0; i < fs->num; i++) {
            Element *pelem = &fs->elements[i];

            if (pelem->radius_drift_max <= pelem->radius_drift_now) {
                pelem->start_radius = pelem->end_radius;
                pelem->end_radius = (float)(rand() % 40000) / 100.0f - 200.0f;
                pelem->radius_drift_max = (float)(rand() % 100000) + 10000.0f;
                pelem->radius_drift_now = 0.0f;
            }
            if (pelem->ratio_drift_max <= pelem->ratio_drift_now) {
                pelem->start_ratio = pelem->end_ratio;
                pelem->end_ratio = (float)(rand() % 2000) / 100.0f - 10.0f;
                pelem->ratio_drift_max = (float)(rand() % 100000) + 10000.0f;
                pelem->ratio_drift_now = 0.0f;
            }

            pelem->ratio = pelem->start_ratio + (pelem->end_ratio - pelem->start_ratio) / pelem->ratio_drift_max * pelem->ratio_drift_now;
            pelem->angle = fs->angle * pelem->ratio;
            pelem->radius = pelem->start_radius + (pelem->end_radius - pelem->start_radius) / pelem->radius_drift_max * pelem->radius_drift_now;

            thisx += (int)(cos(pelem->angle) * pelem->radius);
            thisy += (int)(sin(pelem->angle) * pelem->radius);

            pelem->ratio_drift_now += 1.0f;
            pelem->radius_drift_now += 1.0f;
        }

        if (!fs->firsttime) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawLine(renderer, 
                fs->savex[fs->rotor], fs->savey[fs->rotor],
                fs->savex[fs->prev], fs->savey[fs->prev]);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawLine(renderer, fs->lastx, fs->lasty, thisx, thisy);
        } else {
            fs->firsttime = false;
        }

        fs->savex[fs->rotor] = fs->lastx = thisx;
        fs->savey[fs->rotor] = fs->lasty = thisy;

        fs->rotor = (fs->rotor + 1) % SAVE;
        fs->prev = (fs->prev + 1) % SAVE;

        if (fs->forward) {
            fs->angle += 0.01f;
            if (fs->angle >= MAXANGLE) {
                fs->angle = MAXANGLE;
                fs->forward = false;
            }
        } else {
            fs->angle -= 0.1f;
            if (fs->angle <= 0) {
                fs->angle = 0.0f;
                fs->forward = true;
            }
        }
    }
}

int main(int argc, char* args[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Rotor Animation", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    srand((unsigned int)time(NULL));

    FlightStruct fs = {0};
    init_rotor(&fs, WINDOW_WIDTH, WINDOW_HEIGHT, DEFAULTCOUNT);

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        draw_rotor(renderer, &fs);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);  // Cap at roughly 60 fps
    }

    free(fs.elements);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
