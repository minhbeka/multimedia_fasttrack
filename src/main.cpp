#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <vector>
#include <SDL_ttf.h>
#include <string>

int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;

// Layout
SDL_Rect playerArea;
SDL_Rect playlistArea;

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

// Playlist data
std::vector<std::string> playlistPaths;
std::vector<SDL_Texture *> playlistTextTex;
std::vector<SDL_Rect> playlistTextRect;

// Globals
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

// Playlist highlight
SDL_Texture *arrowTex = nullptr;
SDL_Rect arrowRect;

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
    int playerW = SCREEN_WIDTH * 0.7f;
    int playlistW = SCREEN_WIDTH - playerW;

    playerArea = {0, 0, playerW, SCREEN_HEIGHT};
    playlistArea = {playerW, 0, playlistW, SCREEN_HEIGHT};
    // -------- Content area (top 75%) --------
    contentRect = {playerArea.x, playerArea.y, playerArea.w, int(playerArea.h * 0.75f)};
    // -------- Control area (bottom 25%) --------
    controlRect = {playerArea.x, contentRect.y + contentRect.h, playerArea.w, playerArea.h - contentRect.h};
    // -------- Layout constants --------
    int marginX = 40, timeW = 80, timeH = 24, gap = 12;
    // Y center line for time + progress
    int barCenterY = controlRect.y + 45;

    // -------- Current time--------
    currentTimeRect = {playerArea.x + marginX, barCenterY - timeH / 2, timeW, timeH};

    // -------- Duration time--------
    durationTimeRect = {playerArea.x + playerArea.w - marginX - timeW, barCenterY - timeH / 2, timeW, timeH};

    // -------- Progress bar--------
    progressBg = {
        currentTimeRect.x + currentTimeRect.w + gap,
        barCenterY - 6,
        durationTimeRect.x - (currentTimeRect.x + currentTimeRect.w) - gap * 2,
        12};

    // Filled progress (demo 40%)
    progressFill = progressBg;
    progressFill.w = progressBg.w * 0.4f;

    // -------- Control buttons (bottom area) --------
    int btnSize = 64, spacing = 80, centerX = playerArea.x + playerArea.w / 2;
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
    playlistPaths = {
        "../assets/music1.png",
        "../assets/music2.png",
        "../assets/music3.png",
        "../assets/music4.png",
        "../assets/music5.png"};

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

    arrowTex = loadTexture("../assets/arrow.png");
    if (!arrowTex)
        return false;

    return true;
}

void buildPlaylistUI()
{
    // Clear old playlist textures
    for (auto tex : playlistTextTex)
        SDL_DestroyTexture(tex);

    playlistTextTex.clear();
    playlistTextRect.clear();

    SDL_Color textColor = {220, 220, 220, 255};

    // ---- Layout config ----
    int startY = playlistArea.y + 20; // padding top
    int itemH = 36;                   // khoảng cách giữa các dòng
    int paddingX = 36;                // padding left

    for (size_t i = 0; i < playlistPaths.size(); ++i)
    {
        // Add index number before path
        std::string text =
            std::to_string(i + 1) + ". " + playlistPaths[i];

        // Create text texture
        SDL_Texture *tex = createTextTexture(text, textColor);
        if (!tex)
            continue;

        int w, h;
        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
        int maxTextWidth = playlistArea.w - paddingX - 12;

        SDL_Rect r = {
            playlistArea.x + paddingX,
            startY + int(i) * itemH,
            std::min(w, maxTextWidth),
            h};

        playlistTextTex.push_back(tex);
        playlistTextRect.push_back(r);
    }
}

void renderUI()
{
    // ===== Clear screen =====
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    // ===== Left: Content area =====
    SDL_RenderCopy(renderer, mediaList[currentMediaIndex], nullptr, &contentRect);

    // ===== Control bar =====
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &controlRect);

    // ===== Progress bar =====
    SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
    SDL_RenderFillRect(renderer, &progressBg);

    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &progressFill);

    // ===== Buttons =====
    SDL_RenderCopy(renderer, prevTex, nullptr, &prevBtn);
    SDL_Texture *currentPlayTex = isPlaying ? pauseTex : playTex;
    SDL_RenderCopy(renderer, currentPlayTex, nullptr, &playBtn);
    SDL_RenderCopy(renderer, nextTex, nullptr, &nextBtn);

    // ===== Right: Playlist area =====
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &playlistArea);

    // Border chia 2 khu
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderDrawLine(renderer,
                       playlistArea.x,
                       0,
                       playlistArea.x,
                       SCREEN_HEIGHT);

    // ===== Highlight current item =====
    if (currentMediaIndex >= 0 &&
        currentMediaIndex < (int)playlistTextRect.size())
    {
        SDL_Rect bg = playlistTextRect[currentMediaIndex];

        bg.x = playlistArea.x + 6;
        bg.w = playlistArea.w - 12;
        bg.y -= 4;
        bg.h += 8;

        SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
        SDL_RenderFillRect(renderer, &bg);
    }

    // ===== Arrow indicator =====
    if (arrowTex &&
        currentMediaIndex >= 0 &&
        currentMediaIndex < (int)playlistTextRect.size())
    {
        SDL_Rect textRect = playlistTextRect[currentMediaIndex];

        SDL_Rect arrowRect = {
            playlistArea.x + 8,
            textRect.y + textRect.h / 2 - 8,
            16,
            16};

        SDL_RenderCopy(renderer, arrowTex, nullptr, &arrowRect);
    }

    // ===== Playlist items =====
    for (size_t i = 0; i < playlistTextTex.size(); ++i)
    {
        SDL_RenderCopy(renderer,
                       playlistTextTex[i],
                       nullptr,
                       &playlistTextRect[i]);
    }

    // ===== Time text =====
    SDL_Color white = {220, 220, 220, 255};

    // Current time
    if (currentTimeTex)
        SDL_DestroyTexture(currentTimeTex);

    currentTimeTex = createTextTexture(formatTime(currentTimeSec), white);
    SDL_QueryTexture(currentTimeTex, nullptr, nullptr,
                     &currentTimeRect.w, &currentTimeRect.h);
    SDL_RenderCopy(renderer, currentTimeTex, nullptr, &currentTimeRect);

    // Duration
    if (durationTimeTex)
        SDL_DestroyTexture(durationTimeTex);

    durationTimeTex = createTextTexture(formatTime(durationSec), white);
    SDL_QueryTexture(durationTimeTex, nullptr, nullptr,
                     &durationTimeRect.w, &durationTimeRect.h);
    SDL_RenderCopy(renderer, durationTimeTex, nullptr, &durationTimeRect);

    // ===== Present =====
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

    for (auto tex : playlistTextTex)
        SDL_DestroyTexture(tex);

    playlistTextTex.clear();
    playlistTextRect.clear();
    playlistPaths.clear();

    if (arrowTex)
    {
        SDL_DestroyTexture(arrowTex);
        arrowTex = nullptr;
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
    buildPlaylistUI();

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
            // auto rescale
            if (e.type == SDL_WINDOWEVENT &&
                e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                SCREEN_WIDTH = e.window.data1;  // width mới
                SCREEN_HEIGHT = e.window.data2; // height mới

                initLayout(); // tính lại toàn bộ UI
                buildPlaylistUI();
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
