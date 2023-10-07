#include <SDL.h>
#include <blend2d.h>

struct ball {
    BLCircle c;
    double vX;
    double vY;
    int dirX;
    int dirY;

    void move()
    {
        c = { c.cx + vX * dirX, c.cy + vY * dirY, c.r };
    }

    void reverseX()
    {
        dirX = -1 * dirX;
    }

    void reverseY()
    {
        dirY = -1 * dirY;
    }
};

struct paddle {
public:
    BLRectI rect;

    void setX(int x)
    {
        rect.x = x;
    }
};

class pongGame {
public:
    BLFont font;

    paddle paddle {};
    ball ball {};
    int score;

    Uint32 lastUpdateTime {};

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* displayTexture;
    SDL_Texture* scoreTexture;

    BLImage displaySurface;
    BLImage scoreSurface;

    bool quitting = false;

    int frameCounter = 0;
    Uint32 frameTicks = 0;
    double fps = 0;

    int mouseX = 0;

    int displayWidth = 600;
    int displayHeight = 500;

    int scoreWidth = 200;
    int scoreHeight = displayHeight;

    SDL_Rect displayRect = { 0, 0, displayWidth, displayHeight };
    SDL_Rect scoreRect = { displayWidth, 0, scoreWidth, scoreHeight };
    pongGame() noexcept
        : score(0)
        , window(nullptr)
        , renderer(nullptr)
        , displayTexture(nullptr)
        , scoreTexture(nullptr)
        , displaySurface()
        , scoreSurface()
    {

        BLFontFace face;

        face.createFromFile("IntelOneMono.ttf");
        font.createFromFace(face, 20.f);

        paddle = { .rect = BLRectI(mouseX, displayHeight - 40, 200, 20) };
        ball = { .c = BLCircle(100.0, 100.0, 20.0),
            .vX = 1,
            .vY = 1,
            .dirX = 1,
            .dirY = 1 };
    }

    ~pongGame() noexcept { destroyWindow(); }

    bool createWindow() noexcept
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
            printf("Failed to initialize SDL: %s", SDL_GetError());
            return false;
        }

        SDL_ShowCursor(SDL_DISABLE);

        window = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, displayWidth + scoreWidth, displayHeight, SDL_WINDOW_SHOWN);
        if (!window)
            return false;

        renderer = SDL_CreateRenderer(window, -1, 0);
        if (!renderer) {
            printf("FAILED to create SDL_Renderer: %s\n", SDL_GetError());
            destroyWindow();
            return false;
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        if (!createDisplaySurface()) {
            printf("Failed to create displaySurface");
            return false;
        }

        if (!createScoreSurface()) {
            printf("Failed to create scoreSurface");
            return false;
        }

        return true;
    }

    void destroySurfaces() noexcept
    {
        if (displayTexture) {
            SDL_DestroyTexture(displayTexture);
            displayTexture = nullptr;
        }

        if (scoreTexture) {
            SDL_DestroyTexture(scoreTexture);
            scoreTexture = nullptr;
        }

        displaySurface.reset();
        scoreSurface.reset();
    }

    bool createDisplaySurface() noexcept
    {
        destroySurfaces();

        displayTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, displayWidth, displayHeight);
        if (!displayTexture)
            return false;
        SDL_SetTextureBlendMode(displayTexture, SDL_BLENDMODE_NONE);

        if (displaySurface.create(displayWidth, displayHeight, BL_FORMAT_PRGB32) != BL_SUCCESS)
            return false;

        return true;
    }

    bool createScoreSurface() noexcept
    {
        scoreTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, scoreWidth, scoreHeight);
        if (!scoreTexture)
            return false;
        SDL_SetTextureBlendMode(scoreTexture, SDL_BLENDMODE_NONE);

        if (scoreSurface.create(scoreWidth, scoreHeight, BL_FORMAT_PRGB32) != BL_SUCCESS)
            return false;

        return true;
    };

    void destroyWindow() noexcept
    {
        destroySurfaces();

        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }

        if (!window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }

    int run() noexcept
    {
        SDL_Event event;

        for (;;) {
            while (!quitting && SDL_PollEvent(&event)) {
                onEvent(event);
            }

            if (quitting)
                break;

            if (updateStates()) {
                render();
                blit();
                updateFrameCounter();
            }
        }

        return 0;
    }

    void updateFrameCounter() noexcept
    {
        Uint32 ticks = SDL_GetTicks();
        if (++frameCounter >= 100) {
            fps = (1000.0 / double(ticks - frameTicks)) * double(frameCounter);
            frameCounter = 0;
            frameTicks = ticks;
        }
    }

    void onEvent(SDL_Event& event) noexcept
    {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN and event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
            quitting = true;
        }

        SDL_GetMouseState(&mouseX, nullptr);
    }

    bool updateStates() noexcept
    {
        if (SDL_TICKS_PASSED(SDL_GetTicks(), lastUpdateTime + 1) && score > -1) {
            ball.move();
            paddle.setX(mouseX);

            if (ball.c.cx > displayWidth or ball.c.cx <= 0) {
                ball.reverseX();
            } else if (ball.c.cy < 0) {
                ball.reverseY();
            }

            if (ball.c.cy + ball.c.r > paddle.rect.y
                && ball.c.cx > paddle.rect.x
                && ball.c.cx < paddle.rect.x + paddle.rect.w) {

                ball.c.cy = paddle.rect.y - ball.c.r;
                ball.reverseY();
                score += 1;
            } else if (ball.c.cy + ball.c.r > displayHeight) {
                score = -1;
            }

            lastUpdateTime = SDL_GetTicks();
            return true;
        } else {
            return false;
        }
    }

    void render() noexcept
    {
        BLContext ctx(displaySurface);

        ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
        ctx.fillAll();

        ctx.setFillStyle(BLRgba32(0xFF000000u));
        ctx.setFillRule(BL_FILL_RULE_EVEN_ODD);

        ctx.fillRect(paddle.rect);

        ctx.setFillStyle(BLRgba32(0xFF8d17adu));
        ctx.fillCircle(ball.c);

        BLContext scoreCtx(scoreSurface);

        char fpsBuf[128];
        snprintf(fpsBuf, 128, "FPS: %.2f", fps);

        char scoreBuf[128];
        if (score < 0) {
            snprintf(scoreBuf, 128, "You failed.");
        } else {
            snprintf(scoreBuf, 128, "Score: %.2d", score);
        }

        scoreCtx.setFillStyle(BLRgba32(0xFF00FF00u));
        scoreCtx.fillAll();

        scoreCtx.setFillStyle(BLRgba32(0xFFFF0000u));
        scoreCtx.setFillRule(BL_FILL_RULE_EVEN_ODD);

        scoreCtx.fillUtf8Text(BLPoint(2, 20), font, fpsBuf);
        scoreCtx.fillUtf8Text(BLPoint(2, 50), font, scoreBuf);
    }

    void blit() const noexcept
    {
        BLImageData displayData {};
        BLImageData scoreData {};

        displaySurface.getData(&displayData);
        scoreSurface.getData(&scoreData);

        SDL_UpdateTexture(displayTexture, nullptr, displayData.pixelData, int(displayData.stride));
        SDL_UpdateTexture(scoreTexture, nullptr, scoreData.pixelData, int(scoreData.stride));
        SDL_RenderCopy(renderer, displayTexture, nullptr, &displayRect);
        SDL_RenderCopy(renderer, scoreTexture, nullptr, &scoreRect);

        SDL_RenderPresent(renderer);
    }
};

int main()
{
    pongGame app;

    if (!app.createWindow()) {
        return 1;
    }

    return app.run();
}
