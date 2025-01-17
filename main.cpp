#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_mixer.h"
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <random>

#define SCREEN_WIDTH 405
#define SCREEN_HEIGHT 720
#define CAR_WIDTH SCREEN_WIDTH / 10
#define CAR_HEIGHT SCREEN_HEIGHT / 11
#define LANE_WIDTH (SCREEN_WIDTH / 4)
#define LANE_1 (LANE_WIDTH / 2 - CAR_WIDTH / 2)
#define LANE_2 (LANE_WIDTH + LANE_WIDTH / 2 - CAR_WIDTH / 2)
#define LANE_3 (2 * LANE_WIDTH + LANE_WIDTH / 2 - CAR_WIDTH / 2)
#define LANE_4 (3 * LANE_WIDTH + LANE_WIDTH / 2 - CAR_WIDTH / 2)
#define OBSTACLE_SIZE SCREEN_WIDTH / 10

enum GameState
{
    MAIN_MENU,
    NORMAL_MODE,
    DEATH_SCREEN
};

class Entity
{
public:
    SDL_Texture *texture;
    SDL_Rect srcRect, destRect;
    bool collected = false;

    void render(SDL_Renderer *renderer)
    {
        SDL_RenderCopy(renderer, texture, NULL, &destRect);
    }
};

class Car : public Entity
{
public:
    double angle = 0;
    bool rotating = false;
    Uint32 rotationStartTime;
    const Uint32 rotationDuration = 200;
    bool moving = false;
    int targetX;
    Uint32 moveStartTime;
    const Uint32 moveDuration = 200;

    void updateRotation()
    {
        Uint32 currentTime = SDL_GetTicks();
        if (rotating)
        {
            Uint32 elapsedTime = currentTime - rotationStartTime;
            if (elapsedTime < rotationDuration)
            {
                angle = 15 * sin((M_PI / rotationDuration) * elapsedTime);
            }
            else
            {
                angle = 0;
                rotating = false;
            }
        }
    }

    void updateMovement()
    {
        Uint32 currentTime = SDL_GetTicks();
        if (moving)
        {
            Uint32 elapsedTime = currentTime - moveStartTime;
            if (elapsedTime < moveDuration)
            {
                double t = (double)elapsedTime / moveDuration;
                destRect.x = destRect.x + t * (targetX - destRect.x);
            }
            else
            {
                destRect.x = targetX;
                moving = false;
            }
        }
    }
};

class Game
{
public:
    Game();
    ~Game();

    void init(const char *title, int xpos, int ypos, int width, int height, bool fullscreen);
    void handleEvents();
    void update();
    void render();
    void clean();
    bool running() { return isRunning; };

private:
    bool isRunning;
    GameState currentState;
    SDL_Window *window;
    SDL_Renderer *renderer;
    std::vector<Entity> obstacles;
    Car blueCar, redCar;
    int timer;
    Uint32 startTime;
    int score = 0;
    int spawnRate = 80;
    int obstacleSpeed = 6;
    int playTextYPosition;
    int playTextDirection;
    const int textSpeed = 1;
    const int animationRange = 10;
    TTF_Font *titleFont;
    TTF_Font *menuFont;
    SDL_Texture *titleTextTexture;
    SDL_Texture *playTextTexture1;
    SDL_Texture *playTextTexture2;
    SDL_Rect titleTextRect;
    SDL_Rect playTextRect1;
    SDL_Rect playTextRect2;
    int initialPlayTextYPosition;
    Mix_Chunk *circlePickupSound;
    Mix_Chunk *deathSound;
    Mix_Chunk *circleMissSound;
    SDL_Texture *redCircle;
    SDL_Texture *redBox;
    SDL_Texture *blueCircle;
    SDL_Texture *blueBox;
    SDL_Texture *endScreenOverlay;
    SDL_Rect restartButtonRect, homeButtonRect;
    Mix_Music *backgroundMusic;
    int highscore = 0;

    void loadAssets();
    void spawnObstacle();
    void updateObstacles();
    void checkCollision();
    void increaseDifficulty();
    void updateMenuAnimation();
    void renderMenu();
    void loadHighscore();
    void saveHighscore();
};

Game::Game() {}
Game::~Game() {}

void Game::init(const char *title, int xpos, int ypos, int width, int height, bool fullscreen)
{
    int flags = 0;
    if (fullscreen)
    {
        flags = SDL_WINDOW_FULLSCREEN;
    }
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
    {
        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        renderer = SDL_CreateRenderer(window, -1, 0);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        if (TTF_Init() == -1)
        {
            std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
            isRunning = false;
            return;
        }

        isRunning = true;
        currentState = MAIN_MENU;
        loadAssets();
        startTime = SDL_GetTicks();
        loadHighscore();

        if (isRunning)
        {
            if (Mix_PlayMusic(backgroundMusic, -1) == -1)
            {
                std::cerr << "Failed to play background music! SDL_mixer Error: " << Mix_GetError() << std::endl;
            }
        }
    }
    else
    {
        isRunning = false;
    }
}

void Game::loadAssets()
{
    SDL_Surface *tmpSurface;

    tmpSurface = IMG_Load("assets/car-red.png");
    redCar.texture = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);

    tmpSurface = IMG_Load("assets/car-blue.png");
    blueCar.texture = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);

    tmpSurface = IMG_Load("assets/circle-red.png");
    redCircle = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);

    tmpSurface = IMG_Load("assets/box-red.png");
    redBox = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);

    tmpSurface = IMG_Load("assets/circle-blue.png");
    blueCircle = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);

    tmpSurface = IMG_Load("assets/box-blue.png");
    blueBox = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);

    tmpSurface = IMG_Load("assets/end-screen-overlay.png");
    endScreenOverlay = SDL_CreateTextureFromSurface(renderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);

    circlePickupSound = Mix_LoadWAV("assets/sfx/circle_pickup.wav");
    deathSound = Mix_LoadWAV("assets/sfx/death-car.wav");
    circleMissSound = Mix_LoadWAV("assets/sfx/circle_miss.wav");

    blueCar.destRect = {LANE_1, SCREEN_HEIGHT - 100, CAR_WIDTH, CAR_HEIGHT};
    redCar.destRect = {LANE_4, SCREEN_HEIGHT - 100, CAR_WIDTH, CAR_HEIGHT};

    restartButtonRect = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 20, 100, 40};
    homeButtonRect = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 30, 100, 40};

    titleFont = TTF_OpenFont("assets/Silkscreen-Regular.ttf", 24);
    menuFont = TTF_OpenFont("assets/Silkscreen-Regular.ttf", 20);
    if (titleFont && menuFont)
    {
        SDL_Color color = {255, 255, 255, 255};

        SDL_Surface *titleSurface = TTF_RenderText_Solid(titleFont, "Two Cars Game", color);
        titleTextTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        titleTextRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, SCREEN_HEIGHT / 3 - titleSurface->h / 2, titleSurface->w, titleSurface->h};
        SDL_FreeSurface(titleSurface);

        SDL_Surface *playSurface1 = TTF_RenderText_Solid(menuFont, "Press Any Key", color);
        playTextTexture1 = SDL_CreateTextureFromSurface(renderer, playSurface1);
        playTextRect1 = {SCREEN_WIDTH / 2 - playSurface1->w / 2, SCREEN_HEIGHT / 2 - playSurface1->h, playSurface1->w, playSurface1->h};
        SDL_FreeSurface(playSurface1);

        SDL_Surface *playSurface2 = TTF_RenderText_Solid(menuFont, "to Play", color);
        playTextTexture2 = SDL_CreateTextureFromSurface(renderer, playSurface2);
        playTextRect2 = {SCREEN_WIDTH / 2 - playSurface2->w / 2, SCREEN_HEIGHT / 2, playSurface2->w, playSurface2->h};
        SDL_FreeSurface(playSurface2);
    }
    playTextYPosition = playTextRect1.y;
    initialPlayTextYPosition = playTextRect1.y;
    playTextDirection = 1;

    backgroundMusic = Mix_LoadMUS("assets/sfx/background-music.mp3");
    if (!backgroundMusic)
    {
        std::cerr << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << std::endl;
    }
}

void Game::spawnObstacle()
{
    static int patternTimer = 0;
    if (patternTimer == 0)
    {
        std::vector<int> lanes = {0, 1, 2, 3};
        std::shuffle(lanes.begin(), lanes.end(), std::default_random_engine(std::random_device()()));

        bool redBoxSpawned = false;
        bool blueBoxSpawned = false;
        bool redCircleSpawned = false;
        bool blueCircleSpawned = false;

        for (int i = 0; i < 2; ++i)
        {
            Entity obstacle;
            if (lanes[i] == 0 || lanes[i] == 1)
            {
                if (!redBoxSpawned && rand() % 2 == 0)
                {
                    obstacle.texture = redBox;
                    redBoxSpawned = true;
                }
                else if (!redCircleSpawned)
                {
                    obstacle.texture = redCircle;
                    redCircleSpawned = true;
                }
                obstacle.destRect.x = (lanes[i] == 0) ? LANE_3 : LANE_4;
            }
            else
            {
                if (!blueBoxSpawned && rand() % 2 == 0)
                {
                    obstacle.texture = blueBox;
                    blueBoxSpawned = true;
                }
                else if (!blueCircleSpawned)
                {
                    obstacle.texture = blueCircle;
                    blueCircleSpawned = true;
                }
                obstacle.destRect.x = (lanes[i] == 2) ? LANE_1 : LANE_2;
            }
            obstacle.srcRect = {0, 0, OBSTACLE_SIZE, OBSTACLE_SIZE};
            obstacle.destRect.w = OBSTACLE_SIZE;
            obstacle.destRect.h = OBSTACLE_SIZE;
            obstacle.destRect.y = -obstacle.destRect.h;

            if ((obstacle.texture == redBox || obstacle.texture == blueBox) && !obstacles.empty())
            {
                auto lastObstacle = obstacles.back();
                if (lastObstacle.destRect.y < CAR_HEIGHT + 10)
                {
                    obstacle.destRect.y = lastObstacle.destRect.y - (CAR_HEIGHT + 10);
                }
            }
            if ((obstacle.texture == redCircle || obstacle.texture == blueCircle) && !obstacles.empty())
            {
                auto lastObstacle = obstacles.back();
                if (lastObstacle.destRect.y < CAR_HEIGHT + 10)
                {
                    obstacle.destRect.y = lastObstacle.destRect.y - (CAR_HEIGHT + 10);
                }
            }
            obstacles.push_back(obstacle);
        }
        patternTimer = spawnRate;
    }
    else
    {
        patternTimer--;
    }
}

void Game::updateObstacles()
{
    for (auto &obstacle : obstacles)
    {
        obstacle.destRect.y += obstacleSpeed;
    }
    obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(), [this](Entity &o)
                                   {
                                       if (o.destRect.y > SCREEN_HEIGHT)
                                       {
                                           if ((o.texture == redCircle || o.texture == blueCircle) && !o.collected)
                                           {
                                               Mix_PlayChannel(-1, circleMissSound, 0);
                                               currentState = DEATH_SCREEN;
                                           }
                                           return true;
                                       }
                                       return false; }),
                    obstacles.end());
}

void Game::checkCollision()
{
    for (auto &obstacle : obstacles)
    {
        if (SDL_HasIntersection(&redCar.destRect, &obstacle.destRect))
        {
            if (obstacle.texture == redBox)
            {
                Mix_PlayChannel(-1, deathSound, 0);
                currentState = DEATH_SCREEN;
            }
            else if (obstacle.texture == redCircle && !obstacle.collected)
            {
                score++;
                obstacle.collected = true;
                Mix_PlayChannel(-1, circlePickupSound, 0);
            }
        }
        if (SDL_HasIntersection(&blueCar.destRect, &obstacle.destRect))
        {
            if (obstacle.texture == blueBox)
            {
                Mix_PlayChannel(-1, deathSound, 0);
                currentState = DEATH_SCREEN;
            }
            else if (obstacle.texture == blueCircle && !obstacle.collected)
            {
                score++;
                obstacle.collected = true;
                Mix_PlayChannel(-1, circlePickupSound, 0);
            }
        }
    }
    obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(), [](Entity &o)
                                   { return o.collected; }),
                    obstacles.end());

    if (currentState == DEATH_SCREEN && score > highscore)
    {
        highscore = score;
        saveHighscore();
    }
}

void Game::increaseDifficulty()
{
    static Uint32 lastIncreaseTime = 0;
    Uint32 currentTime = SDL_GetTicks();
    Uint32 elapsedTime = (currentTime - startTime) / 1000;

    if (elapsedTime % 15 == 0 && currentTime - lastIncreaseTime >= 1000)
    {
        if (spawnRate > 10)
            spawnRate -= 20;
        if (obstacleSpeed < 15)
            obstacleSpeed += 2;
        lastIncreaseTime = currentTime;
    }
}

void Game::handleEvents()
{
    SDL_Event event;
    SDL_PollEvent(&event);
    switch (event.type)
    {
    case SDL_QUIT:
        isRunning = false;
        break;
    case SDL_KEYDOWN:
        if (currentState == MAIN_MENU)
        {
            currentState = NORMAL_MODE;
            startTime = SDL_GetTicks();
        }
        else if (currentState == NORMAL_MODE)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                currentState = MAIN_MENU;
            }
            else if (event.key.keysym.sym == SDLK_a)
            {
                if (blueCar.destRect.x == LANE_1)
                {
                    blueCar.targetX = LANE_2;
                }
                else
                {
                    blueCar.targetX = LANE_1;
                }
                blueCar.moving = true;
                blueCar.moveStartTime = SDL_GetTicks();
                blueCar.rotating = true;
                blueCar.rotationStartTime = SDL_GetTicks();
            }
            else if (event.key.keysym.sym == SDLK_d)
            {
                if (redCar.destRect.x == LANE_4)
                {
                    redCar.targetX = LANE_3;
                }
                else
                {
                    redCar.targetX = LANE_4;
                }
                redCar.moving = true;
                redCar.moveStartTime = SDL_GetTicks();
                redCar.rotating = true;
                redCar.rotationStartTime = SDL_GetTicks();
            }
        }
        else if (currentState == DEATH_SCREEN)
        {
            if (event.key.keysym.sym == SDLK_r)
            {
                currentState = NORMAL_MODE;
                score = 0;
                obstacles.clear();
                startTime = SDL_GetTicks();
                spawnRate = 80;
                obstacleSpeed = 6;
            }
            else if (event.key.keysym.sym == SDLK_h)
            {
                currentState = MAIN_MENU;
                score = 0;
                obstacles.clear();
                spawnRate = 80;
                obstacleSpeed = 6;
            }
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (currentState == DEATH_SCREEN)
        {
            int x, y;
            SDL_GetMouseState(&x, &y);
            if (x >= restartButtonRect.x && x <= restartButtonRect.x + restartButtonRect.w &&
                y >= restartButtonRect.y && y <= restartButtonRect.y + restartButtonRect.h)
            {
                currentState = NORMAL_MODE;
                score = 0;
                obstacles.clear();
                startTime = SDL_GetTicks();
                spawnRate = 80;
                obstacleSpeed = 6;
            }
            else if (x >= homeButtonRect.x && x <= homeButtonRect.x + homeButtonRect.w &&
                     y >= homeButtonRect.y && y <= homeButtonRect.y + homeButtonRect.h)
            {
                currentState = MAIN_MENU;
                score = 0;
                obstacles.clear();
                spawnRate = 80;
                obstacleSpeed = 6;
            }
        }
        break;
    }
}

void Game::update()
{
    if (currentState == NORMAL_MODE)
    {
        updateObstacles();
        checkCollision();
        increaseDifficulty();
        spawnObstacle();
        blueCar.updateRotation();
        blueCar.updateMovement();
        redCar.updateRotation();
        redCar.updateMovement();
    }
    else if (currentState == MAIN_MENU)
    {
        updateMenuAnimation();
    }
}

void Game::updateMenuAnimation()
{
    playTextYPosition += textSpeed * playTextDirection;
    if (playTextYPosition <= initialPlayTextYPosition - animationRange || playTextYPosition >= initialPlayTextYPosition + animationRange)
    {
        playTextDirection *= -1;
    }
    playTextRect1.y = playTextYPosition;
    playTextRect2.y = playTextYPosition + playTextRect1.h;
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 37, 51, 122, 255); // Background color
    SDL_RenderClear(renderer);

    if (currentState == MAIN_MENU)
    {
        renderMenu();
    }
    else if (currentState == NORMAL_MODE || currentState == DEATH_SCREEN)
    {
        // Draw lanes
        SDL_SetRenderDrawColor(renderer, 117, 138, 219, 255); // Line color

        // Left lane line
        SDL_RenderDrawLine(renderer, LANE_WIDTH, 0, LANE_WIDTH, SCREEN_HEIGHT);

        // Middle lane line (thicker)
        for (int i = -2; i <= 2; ++i)
        {
            SDL_RenderDrawLine(renderer, 2 * LANE_WIDTH + i, 0, 2 * LANE_WIDTH + i, SCREEN_HEIGHT);
        }

        // Right lane line
        SDL_RenderDrawLine(renderer, 3 * LANE_WIDTH, 0, 3 * LANE_WIDTH, SCREEN_HEIGHT);

        // Render cars and obstacles
        SDL_RenderCopyEx(renderer, blueCar.texture, NULL, &blueCar.destRect, blueCar.angle, NULL, SDL_FLIP_NONE);
        SDL_RenderCopyEx(renderer, redCar.texture, NULL, &redCar.destRect, redCar.angle, NULL, SDL_FLIP_NONE);

        for (auto &obstacle : obstacles)
        {
            obstacle.render(renderer);
        }

        if (currentState == DEATH_SCREEN)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_RenderFillRect(renderer, NULL);
            SDL_RenderCopy(renderer, endScreenOverlay, NULL, NULL);

            TTF_Font *font = TTF_OpenFont("assets/Silkscreen-Regular.ttf", 24);
            if (font)
            {
                SDL_Color color = {255, 255, 255, 255};
                SDL_Surface *textSurface;
                SDL_Texture *textTexture;
                SDL_Rect textRect;

                textSurface = TTF_RenderText_Solid(font, "Restart (R)", color);
                textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                textRect = {restartButtonRect.x + (restartButtonRect.w - textSurface->w) / 2, restartButtonRect.y, textSurface->w, textSurface->h};
                SDL_FreeSurface(textSurface);
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);

                textSurface = TTF_RenderText_Solid(font, "Home (H)", color);
                textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                textRect = {homeButtonRect.x + (homeButtonRect.w - textSurface->w) / 2, homeButtonRect.y, textSurface->w, textSurface->h};
                SDL_FreeSurface(textSurface);
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);

                std::string scoreText = "Score: " + std::to_string(score);
                textSurface = TTF_RenderText_Solid(font, scoreText.c_str(), color);
                textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 - 100, textSurface->w, textSurface->h};
                SDL_FreeSurface(textSurface);
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);

                std::string highscoreText = "Highscore: " + std::to_string(highscore);
                textSurface = TTF_RenderText_Solid(font, highscoreText.c_str(), color);
                textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 - 150, textSurface->w, textSurface->h};
                SDL_FreeSurface(textSurface);
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);

                TTF_CloseFont(font);
            }
            else
            {
                std::cout << "TTF_OpenFont failed: " << TTF_GetError() << std::endl;
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void Game::renderMenu()
{
    SDL_RenderCopy(renderer, titleTextTexture, NULL, &titleTextRect);
    SDL_RenderCopy(renderer, playTextTexture1, NULL, &playTextRect1);
    SDL_RenderCopy(renderer, playTextTexture2, NULL, &playTextRect2);
}

void Game::clean()
{
    SDL_DestroyTexture(blueCar.texture);
    SDL_DestroyTexture(redCar.texture);
    SDL_DestroyTexture(redBox);
    SDL_DestroyTexture(redCircle);
    SDL_DestroyTexture(blueBox);
    SDL_DestroyTexture(blueCircle);
    SDL_DestroyTexture(endScreenOverlay);
    SDL_DestroyTexture(titleTextTexture);
    SDL_DestroyTexture(playTextTexture1);
    SDL_DestroyTexture(playTextTexture2);
    TTF_CloseFont(titleFont);
    TTF_CloseFont(menuFont);
    Mix_FreeChunk(circlePickupSound);
    Mix_FreeChunk(deathSound);
    Mix_FreeChunk(circleMissSound);
    Mix_FreeMusic(backgroundMusic);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    saveHighscore();
}

void Game::loadHighscore()
{
    std::ifstream file("highscore.txt");
    if (file.is_open())
    {
        file >> highscore;
        file.close();
    }
}

void Game::saveHighscore()
{
    std::ofstream file("highscore.txt");
    if (file.is_open())
    {
        file << highscore;
        file.close();
    }
}

Game *game = nullptr;

int main(int argc, char *argv[])
{
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    Uint32 frameStart;
    int frameTime;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return -1;
    }

    game = new Game();
    game->init("Two Cars Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, false);

    while (game->running())
    {
        frameStart = SDL_GetTicks();
        game->handleEvents();
        game->update();
        game->render();

        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime)
        {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    game->clean();
    return 0;
}