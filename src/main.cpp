#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <vector>
#include <SDL_ttf.h>
#include <string>

int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;

// Layout
SDL_Rect contentRect;
SDL_Rect controlRect;

SDL_Rect prevBtn;
SDL_Rect playBtn;
SDL_Rect nextBtn;

SDL_Rect progressBg;
SDL_Rect progressFill;

// Textures
std::vector<SDL_Texture *> mediaList;
int currentMediaIndex = 0;

SDL_Texture *prevTex = nullptr;
SDL_Texture *playTex = nullptr;
SDL_Texture *pauseTex = nullptr;
bool isPlaying = false;
SDL_Texture *nextTex = nullptr;

// timestamp
TTF_Font *font = nullptr;
SDL_Texture *currentTimeTex = nullptr;
SDL_Texture *durationTimeTex = nullptr;

SDL_Rect currentTimeRect;
SDL_Rect durationTimeRect;

// fake data
int currentTimeSec = 83; // 01:23
int durationSec = 296;   // 04:56

// Globals
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

SDL_Texture *loadTexture(const char *path)
{
    SDL_Surface *surface = IMG_Load(path);
    if (!surface)
    {
        printf("IMG_Load error: %s\n", IMG_GetError());
        return nullptr;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return tex;
}

void initLayout()
{
    // -------- Content area (top 75%) --------
    contentRect = {0, 0, SCREEN_WIDTH, int(SCREEN_HEIGHT * 0.75f)};

    // -------- Control area (bottom 25%) --------
    controlRect = {0, contentRect.h, SCREEN_WIDTH, SCREEN_HEIGHT - contentRect.h};

    // -------- Layout constants --------
    int marginX = 100, timeW = 80, timeH = 24, gap = 12;
    // Y center line for time + progress
    int barCenterY = controlRect.y + 45;

    // -------- Current time--------
    currentTimeRect = {marginX, barCenterY - timeH / 2, timeW, timeH};

    // -------- Duration time--------
    durationTimeRect = {SCREEN_WIDTH - marginX - timeW, barCenterY - timeH / 2, timeW, timeH};

    // -------- Progress bar--------
    progressBg = {
        currentTimeRect.x + currentTimeRect.w,
        barCenterY - 6,
        durationTimeRect.x - (currentTimeRect.x + currentTimeRect.w + gap) - gap,
        12};

    // Filled progress (demo 40%)
    progressFill = progressBg;
    progressFill.w = progressBg.w * 0.4f;

    // -------- Control buttons (bottom area) --------
    int btnSize = 64, spacing = 80, centerX = SCREEN_WIDTH / 2;
    int btnY = controlRect.y + 85;

    prevBtn = {centerX - btnSize - spacing, btnY, btnSize, btnSize};
    playBtn = {centerX - btnSize / 2, btnY, btnSize, btnSize};
    nextBtn = {centerX + spacing, btnY, btnSize, btnSize};
}

// format time
std::string formatTime(int sec)
{
    int m = sec / 60;
    int s = sec % 60;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return std::string(buf);
}

// create texture from time
SDL_Texture *createTextTexture(const std::string &text, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface)
        return nullptr;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return tex;
}

bool init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return false;

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
        return false;
    // time
    if (TTF_Init() == -1)
        return false;

    font = TTF_OpenFont("../assets/Roboto-Regular.ttf", 20);
    if (!font)
    {
        printf("Font load error: %s\n", TTF_GetError());
        return false;
    }
    window = SDL_CreateWindow(
        "Multimedia",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    return renderer != nullptr;
}

bool loadMedia()
{
    mediaList.push_back(loadTexture("../assets/music1.png"));
    mediaList.push_back(loadTexture("../assets/music2.png"));
    mediaList.push_back(loadTexture("../assets/music3.png"));
    mediaList.push_back(loadTexture("../assets/music4.png"));
    mediaList.push_back(loadTexture("../assets/music5.png"));

    prevTex = loadTexture("../assets/prev-button.png");
    playTex = loadTexture("../assets/play-button.png");
    pauseTex = loadTexture("../assets/pause-button.png");
    nextTex = loadTexture("../assets/next-button.png");

    if (!prevTex || !playTex || !pauseTex || !nextTex)
        return false;

    for (auto tex : mediaList)
        if (!tex)
            return false;

    return true;
}

void renderUI()
{
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    // Content
    SDL_RenderCopy(renderer, mediaList[currentMediaIndex], NULL, &contentRect);

    // Control bar background
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &controlRect);

    // Progress bar
    SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
    SDL_RenderFillRect(renderer, &progressBg);

    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &progressFill);

    // Buttons
    SDL_RenderCopy(renderer, prevTex, NULL, &prevBtn);
    SDL_Texture *currentPlayTex = isPlaying ? pauseTex : playTex;
    SDL_RenderCopy(renderer, currentPlayTex, NULL, &playBtn);
    SDL_RenderCopy(renderer, nextTex, NULL, &nextBtn);

    // ---- Time text ----

    SDL_Color white = {220, 220, 220, 255};

    // ---- current time ----
    if (currentTimeTex)
        SDL_DestroyTexture(currentTimeTex);

    currentTimeTex = createTextTexture(
        formatTime(currentTimeSec), white);

    SDL_QueryTexture(currentTimeTex, NULL, NULL,
                     &currentTimeRect.w, &currentTimeRect.h);

    SDL_RenderCopy(renderer, currentTimeTex, NULL, &currentTimeRect);

    // ---- duration ----
    if (durationTimeTex)
        SDL_DestroyTexture(durationTimeTex);

    durationTimeTex = createTextTexture(
        formatTime(durationSec), white);

    SDL_QueryTexture(durationTimeTex, NULL, NULL,
                     &durationTimeRect.w, &durationTimeRect.h);

    SDL_RenderCopy(renderer, durationTimeTex, NULL, &durationTimeRect);

    // push frame
    SDL_RenderPresent(renderer);
}

void cleanup()
{
    // Media textures
    for (auto tex : mediaList)
        SDL_DestroyTexture(tex);
    mediaList.clear();

    // Time textures
    if (currentTimeTex)
    {
        SDL_DestroyTexture(currentTimeTex);
        currentTimeTex = nullptr;
    }

    if (durationTimeTex)
    {
        SDL_DestroyTexture(durationTimeTex);
        durationTimeTex = nullptr;
    }

    // Button textures
    SDL_DestroyTexture(prevTex);
    SDL_DestroyTexture(playTex);
    SDL_DestroyTexture(pauseTex);
    SDL_DestroyTexture(nextTex);

    prevTex = playTex = pauseTex = nextTex = nullptr;

    // Font
    if (font)
    {
        TTF_CloseFont(font);
        font = nullptr;
    }

    // Renderer & window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    renderer = nullptr;
    window = nullptr;

    // Quit subsystems
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

// check click
bool isInside(const SDL_Rect &r, int x, int y)
{
    return x >= r.x && x <= r.x + r.w &&
           y >= r.y && y <= r.y + r.h;
}

int main(int argc, char *argv[])
{
    if (!init() || !loadMedia())
        return -1;

    initLayout();

    bool quit = false;
    SDL_Event e;

    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            //auto rescale
            if (e.type == SDL_WINDOWEVENT &&
                e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                SCREEN_WIDTH = e.window.data1;  // width mới
                SCREEN_HEIGHT = e.window.data2; // height mới

                initLayout(); //tính lại toàn bộ UI
            }
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                int x = e.button.x;
                int y = e.button.y;

                // NEXT
                if (isInside(nextBtn, x, y))
                {
                    currentMediaIndex++;
                    if (currentMediaIndex >= mediaList.size())
                        currentMediaIndex = 0; // quay vòng
                }
                // PLAY / PAUSE
                if (isInside(playBtn, x, y))
                {
                    isPlaying = !isPlaying;
                    // controller.togglePlayPause();
                }
                // PREVIOUS
                if (isInside(prevBtn, x, y))
                {
                    currentMediaIndex--;
                    if (currentMediaIndex < 0)
                        currentMediaIndex = mediaList.size() - 1;
                }
            }
        }

        renderUI();
    }

    cleanup();
    return 0;
}
