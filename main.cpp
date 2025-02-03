// ============================= INCLUDES ============================= //
// Includes SDL2 libraries for graphics, text rendering, and audio.
// Includes standard C++ libraries for strings, vectors, math, file I/O, and random generation.
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

using namespace std;

// ============================= DEFINITIONS ============================= //
// Constants for screen size, car dimensions, and lane positions.
// These values are used for scaling and positioning elements in the game.
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

// ============================= GAME STATE ENUM ============================= //
// Represents the game's current state (menu, active play, or game over).
enum GameState
{
    MAIN_MENU,
    NORMAL_MODE,
    DEATH_SCREEN
};

// ============================= ENTITY CLASS ============================= //
// Base class for renderable objects like cars or obstacles.
// Manages texture, size, and rendering properties.
class Entity
{
public:
    SDL_Texture *texture;
    SDL_Rect srcRect, destRect;
    bool collected = false;

    // Draws the entity on the screen by copying its texture to the renderer.
    void render(SDL_Renderer *renderer)
    {
        SDL_RenderCopy(renderer, texture, NULL, &destRect);
    }
};

// ============================= CAR CLASS ============================= //
// Represents a player's car with added movement and rotation functionality.
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

    // Uses a sine wave to calculate the angle the car should rotate while moving.
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

    // Make the car go smoothly when changing lanes.
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

// ============================= GAME CLASS ============================= //
// Manages the game loop, event handling, rendering, and state transitions.
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
    vector<Entity> obstacles;
    Car blueCar, redCar;
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
    SDL_Rect restartButtonRect, homeButtonRect;
    Mix_Music *backgroundMusic;
    int highscore = 0;

    void loadAssets();
    void spawnObstacle();
    void updateObstacles();
    void checkCollision();
    void increaseDifficulty();
    void updateMenuAnimation();
    void resetCars();
    void renderMenu();
    void renderDeathScreen();
    void loadHighscore();
    void saveHighscore();
};

Game::Game() {}
Game::~Game() {}

// ============================= INIT METHOD ============================= //
// Sets up SDL systems, creates the game window, loads assets, and initializes the game state.
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
            cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << endl;
            isRunning = false;
            return;
        }

        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
        {
            cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << endl;
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
                cerr << "Failed to play background music! SDL_mixer Error: " << Mix_GetError() << endl;
            }
        }
    }
    else
    {
        isRunning = false;
    }
}

// ============================= ASSET LOADING ============================= //
// Loads textures, sounds, and fonts needed for the game.
// Also sets default positions and properties for cars and obstacles.
void Game::loadAssets()
{
    SDL_Surface *tmpSurface;

    tmpSurface = IMG_Load("assets/icon.png");
    SDL_SetWindowIcon(window, tmpSurface);
    SDL_FreeSurface(tmpSurface);

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
        cerr << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << endl;
    }
}

// ============================= SPAWNING OBSTACLES ============================= //
// Dynamically creates obstacles at random lanes with proper spacing.
void Game::spawnObstacle()
{
    static int patternTimer = 0;
    if (patternTimer == 0)
    {
        vector<int> lanes = {0, 1, 2, 3};
        shuffle(lanes.begin(), lanes.end(), default_random_engine(random_device()()));

        // Boolean values to make sure the game doesnt become impossible by spawning 2 boxes at the same time.
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

// ============================= OBSTACLE UPDATES ============================= //
// Moves obstacles downward and removes off-screen ones.
// Handles game-over logic if collectible obstacles are missed.
void Game::updateObstacles()
{
    for (auto &obstacle : obstacles)
    {
        obstacle.destRect.y += obstacleSpeed;
    }
    // Destroy any obstacle that is below the screen.
    obstacles.erase(remove_if(obstacles.begin(), obstacles.end(), [this](Entity &o)
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

// ============================= COLLISION DETECTION ============================= //
// Detects collisions between cars and obstacles.
// Updates score and transitions to game over if a collision occurs.
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
    obstacles.erase(remove_if(obstacles.begin(), obstacles.end(), [](Entity &o)
                              { return o.collected; }),
                    obstacles.end());

    if (currentState == DEATH_SCREEN && score > highscore)
    {
        highscore = score;
        saveHighscore();
    }
}

// ============================= DIFFICULTY PROGRESSION ============================= //
// Gradually increases the spawn rate and speed of obstacles over time.
void Game::increaseDifficulty()
{
    static Uint32 lastIncreaseTime = 0;
    Uint32 currentTime = SDL_GetTicks();
    Uint32 elapsedTime = (currentTime - startTime) / 1000;

    if (elapsedTime % 30 == 0 && currentTime - lastIncreaseTime >= 1000)
    {
        if (spawnRate > 20)
            spawnRate -= 20;
        if (obstacleSpeed < 15)
            obstacleSpeed += 2;
        lastIncreaseTime = currentTime;
    }
}

// ============================ HANDLING USER INTERACTION ============================ //
// Handles any type of event from the user in all states of the game.
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
            resetCars();
        }
        else if (currentState == NORMAL_MODE)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                currentState = MAIN_MENU;
                score = 0;
                obstacles.clear();
                spawnRate = 80;
                obstacleSpeed = 6;
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
                startTime = SDL_GetTicks();
                currentState = NORMAL_MODE;
                resetCars();
            }
            else if (event.key.keysym.sym == SDLK_h)
            {
                startTime = SDL_GetTicks();
                currentState = MAIN_MENU;
                resetCars();
            }
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (currentState == MAIN_MENU)
        {
            currentState = NORMAL_MODE;
            startTime = SDL_GetTicks();
            resetCars();
        }
        else if (currentState == DEATH_SCREEN)
        {
            int x, y;
            SDL_GetMouseState(&x, &y);
            if (x >= restartButtonRect.x && x <= restartButtonRect.x + restartButtonRect.w &&
                y >= restartButtonRect.y && y <= restartButtonRect.y + restartButtonRect.h)
            {
                currentState = NORMAL_MODE;
                startTime = SDL_GetTicks();
                resetCars();
            }
            else if (x >= homeButtonRect.x && x <= homeButtonRect.x + homeButtonRect.w &&
                     y >= homeButtonRect.y && y <= homeButtonRect.y + homeButtonRect.h)
            {
                currentState = MAIN_MENU;
                resetCars();
            }
        }
        break;
    }
}

// ============================== UPDATING ENTITIES ============================= //
// Updates anything in the game before rendering.
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

// ============================= MENU ANIMATION ============================= //
// Animates the "Press Any Key" text in the menu by moving it up and down.
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

// ============================== RENDERING ============================== //
// Rendersing everything on the game in its different states.
void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 37, 51, 122, 255);
    SDL_RenderClear(renderer);

    if (currentState == MAIN_MENU)
    {
        renderMenu();
    }
    else if (currentState == NORMAL_MODE || currentState == DEATH_SCREEN)
    {

        SDL_SetRenderDrawColor(renderer, 117, 138, 219, 255);

        SDL_RenderDrawLine(renderer, LANE_WIDTH, 0, LANE_WIDTH, SCREEN_HEIGHT);

        for (int i = -2; i <= 2; ++i)
        {
            SDL_RenderDrawLine(renderer, 2 * LANE_WIDTH + i, 0, 2 * LANE_WIDTH + i, SCREEN_HEIGHT);
        }
        SDL_RenderDrawLine(renderer, 3 * LANE_WIDTH, 0, 3 * LANE_WIDTH, SCREEN_HEIGHT);

        SDL_RenderCopyEx(renderer, blueCar.texture, NULL, &blueCar.destRect, blueCar.angle, NULL, SDL_FLIP_NONE);
        SDL_RenderCopyEx(renderer, redCar.texture, NULL, &redCar.destRect, redCar.angle, NULL, SDL_FLIP_NONE);

        for (auto &obstacle : obstacles)
        {
            obstacle.render(renderer);
        }

        if (currentState == DEATH_SCREEN)
        {
            renderDeathScreen();
        }
    }

    SDL_RenderPresent(renderer);
}

// Renders the main menu
void Game::renderMenu()
{
    SDL_RenderCopy(renderer, titleTextTexture, NULL, &titleTextRect);
    SDL_RenderCopy(renderer, playTextTexture1, NULL, &playTextRect1);
    SDL_RenderCopy(renderer, playTextTexture2, NULL, &playTextRect2);
}

// Renders the death screen
// Todo? Match the impelementation style with the rest of the code
void Game::renderDeathScreen()
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, NULL);

    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface *textSurface;
    SDL_Texture *textTexture;
    SDL_Rect textRect;

    textSurface = TTF_RenderText_Solid(titleFont, "Restart (R)", color);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {restartButtonRect.x + (restartButtonRect.w - textSurface->w) / 2, restartButtonRect.y, textSurface->w, textSurface->h};
    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);

    textSurface = TTF_RenderText_Solid(titleFont, "Home (H)", color);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {homeButtonRect.x + (homeButtonRect.w - textSurface->w) / 2, homeButtonRect.y, textSurface->w, textSurface->h};
    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);

    string scoreText = "Score: " + to_string(score);
    textSurface = TTF_RenderText_Solid(titleFont, scoreText.c_str(), color);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 - 100, textSurface->w, textSurface->h};
    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);

    string highscoreText = "Highscore: " + to_string(highscore);
    textSurface = TTF_RenderText_Solid(titleFont, highscoreText.c_str(), color);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = {SCREEN_WIDTH / 2 - textSurface->w / 2, SCREEN_HEIGHT / 2 - 150, textSurface->w, textSurface->h};
    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);
}

// ============================== RESETING CARS POSITION ============================== //
// Reset car positions after death or escape.
void Game::resetCars()
{
    blueCar.destRect = {LANE_1, SCREEN_HEIGHT - 100, CAR_WIDTH, CAR_HEIGHT};
    redCar.destRect = {LANE_4, SCREEN_HEIGHT - 100, CAR_WIDTH, CAR_HEIGHT};
    score = 0;
    obstacles.clear();
    spawnRate = 80;
    obstacleSpeed = 6;
}

// ============================= RESOURCE MANAGEMENT ============================= //
// Cleans up the memory when the program closes.
void Game::clean()
{
    SDL_DestroyTexture(blueCar.texture);
    SDL_DestroyTexture(redCar.texture);
    SDL_DestroyTexture(redBox);
    SDL_DestroyTexture(redCircle);
    SDL_DestroyTexture(blueBox);
    SDL_DestroyTexture(blueCircle);
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

// ============================== HIGHSCORE MANAGEMENT ============================== //
// Uses file i/o to save and load the highscore from player.dat with a simple XOR encoding step.
void Game::loadHighscore()
{
    ifstream file("player.dat", ios::binary);
    if (file.is_open())
    {
        int encodedHighscore;
        file.read(reinterpret_cast<char *>(&encodedHighscore), sizeof(encodedHighscore));
        highscore = encodedHighscore ^ 0xA5A5A5A5;
        file.close();
    }
}

void Game::saveHighscore()
{
    ofstream file("player.dat", ios::binary);
    if (file.is_open())
    {
        int encodedHighscore = highscore ^ 0xA5A5A5A5; // Simple XOR encoding
        file.write(reinterpret_cast<const char *>(&encodedHighscore), sizeof(encodedHighscore));
        file.close();
    }
}

// ============================= MAIN GAME LOOP ============================= //
// Runs the main game loop with stable frame timing, handling events, updates, and rendering.
Game *game = nullptr;

int main(int argc, char *argv[])
{
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    Uint32 frameStart;
    int frameTime;

    game = new Game();
    game->init("Two Cars Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, false);

    while (game->running())
    {
        frameStart = SDL_GetTicks();
        game->handleEvents();
        game->update();
        game->render();

        // Limits frame rate to 60 FPS.
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime)
        {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    game->clean();
    return 0;
}
