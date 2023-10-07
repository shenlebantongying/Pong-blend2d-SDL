#include <SDL.h>
#include <blend2d.h>

const int width = 600;
const int height = 500;

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
    SDL_Texture* texture;
    BLImage blSurface;

    bool quitting = false;

    int frameCounter = 0;
    Uint32 frameTicks = 0;
    double fps = 0;

    int mouseX = 0;

    pongGame() noexcept
        : score(0)
        , window(nullptr)
        , renderer(nullptr)
        , texture(nullptr)
        , blSurface()
    {

        BLFontFace face;

        face.createFromFile("IntelOneMono.ttf");
        font.createFromFace(face, 20.f);

        paddle = { .rect = BLRectI(mouseX, height - 40, 200, 20) };
        ball = { .c = BLCircle(100.0, 100.0, 20.0),
            .vX = 1,
            .vY = 1,
            .dirX = 1,
            .dirY = 1 };
    }

    ~pongGame() noexcept { destroyWindow(); }

    bool createWindow(int w, int h) noexcept
    {

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
            printf("Failed to initialize SDL: %s", SDL_GetError());
            return false;
        }

        SDL_ShowCursor(SDL_DISABLE);

        window = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
        if (!window)
            return false;

        renderer = SDL_CreateRenderer(window, -1, 0);
        if (!renderer) {
            printf("FAILED to create SDL_Renderer: %s\n", SDL_GetError());
            destroyWindow();
            return false;
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        if (!createSurface(w, h)) {
            destroyWindow();
            return false;
        }

        return true;
    }

    bool createSurface(int w, int h) noexcept
    {
        destroySurface();

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!texture)
            return false;
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

        if (blSurface.create(w, h, BL_FORMAT_PRGB32) != BL_SUCCESS)
            return false;

        return true;
    }

    void destroyWindow() noexcept
    {
        destroySurface();

        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }

        if (!window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }

    void destroySurface() noexcept
    {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        blSurface.reset();
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

            if (ball.c.cx > width or ball.c.cx <= 0) {
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
            } else if (ball.c.cy + ball.c.r > height) {
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
        BLContext ctx(blSurface);

        ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
        ctx.fillAll();

        ctx.setFillStyle(BLRgba32(0xFF000000u));
        ctx.setFillRule(BL_FILL_RULE_EVEN_ODD);

        ctx.fillRect(paddle.rect);

        ctx.setFillStyle(BLRgba32(0xFF8d17adu));
        ctx.fillCircle(ball.c);

        char fpsBuf[128];
        snprintf(fpsBuf, 128, "FPS: %.2f", fps);

        char scoreBuf[128];
        if (score < 0) {
            snprintf(scoreBuf, 128, "You failed.");

        } else {
            snprintf(scoreBuf, 128, "Score: %.2d", score);
        }

        ctx.setFillStyle(BLRgba32(0xFFFF0000u));
        ctx.fillUtf8Text(BLPoint(2, 20), font, fpsBuf);
        ctx.fillUtf8Text(BLPoint(2, 50), font, scoreBuf);
    }

    void blit() const noexcept
    {
        BLImageData data {};
        blSurface.getData(&data);

        SDL_UpdateTexture(texture, nullptr, data.pixelData, int(data.stride));
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
};

int main()
{
    pongGame app;

    if (!app.createWindow(width, height)) {
        return 1;
    }

    return app.run();
}
