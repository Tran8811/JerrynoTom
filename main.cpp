#include <iostream>
#include <fstream>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include "graphics.h"
#include "defs.h"
#include "game.h"
#include "score.h"
#include "window.h"
#include "menu.h"

using namespace std;

int main(int argc, char *argv[]) {
    bool quit = false;
    SDL_Event event;

    EngineWindow appWindow;
    appWindow.createWindow("Menu Demonstration", 800, 600);

    EngineMenu engineMenu(appWindow.renderer, appWindow.mainWindow);
    engineMenu.initSplashScreen("Press Enter to start", "Jerry_no_Tom", "PixelGosub-ZaRz.ttf", "background.png");

    Graphics graphics;

    bool enterPressed = false; // Variable to check if Enter key is pressed

    while (!quit && !enterPressed) {
        SDL_PollEvent(&event);
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
            quit = true;
        }
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                enterPressed = true; // Mark that Enter key has been pressed
            }
        }

        SDL_RenderClear(appWindow.renderer);
        engineMenu.displaySplashScreen();
        SDL_RenderPresent(appWindow.renderer);
        SDL_Delay(20);
    }

    engineMenu.quitSplashScreen();
    appWindow.destroyWindow();

    // Initialize SDL and SDL_ttf
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    if (!graphics.init()) {
        cerr << "Failed to initialize graphics." << endl;
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("PixelGosub-ZaRz.ttf", 28);
    if (font == nullptr) {
        cerr << "Failed to load font. SDL_ttf Error: " << TTF_GetError() << endl;
        graphics.quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* cheeseTexture = graphics.loadTexture("cheese.png");
    if (cheeseTexture == nullptr) {
        cerr << "Failed to load textures." << endl;
        graphics.quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    int currentDirection = 0; // 0: right, 1: left, 2: up, 3: down

    Mouse mouse(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    Cheese cheese(100, 100);
    SDL_Texture* backgroundTexture = graphics.loadTexture("background.png");
    if (backgroundTexture == nullptr) {
        cerr << "Failed to load background texture." << endl;
        graphics.quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    int score = 0; // Variable to store the score
    int highestScore = 0; // Variable to store the highest score
    ifstream inFile("highestscore.txt");
    if (inFile.is_open()) {
        inFile >> highestScore;
        inFile.close();
    }

    Mix_Chunk* eatSound = graphics.loadSound("eat.wav");

    while (!quit && !gameOver(mouse)) {
        graphics.prepareScene();
        graphics.renderTexture(backgroundTexture, 0, 0);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;
        }

        const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

        if (currentKeyStates[SDL_SCANCODE_UP]) {
            mouse.turnNorth();
            currentDirection = 2; // Change direction to up
        }
        if (currentKeyStates[SDL_SCANCODE_DOWN]) {
            mouse.turnSouth();
            currentDirection = 3; // Change direction to down
        }
        if (currentKeyStates[SDL_SCANCODE_LEFT]) {
            mouse.turnWest();
            currentDirection = 1; // Change direction to left
        }
        if (currentKeyStates[SDL_SCANCODE_RIGHT]) {
            mouse.turnEast();
            currentDirection = 0; // Change direction to right
        }

        std::string mouseTexturePath;
        switch (currentDirection) {
            case 0:
                mouseTexturePath = "mouserightframe1.png";
                break;
            case 1:
                mouseTexturePath = "mouseleftframe1.png";
                break;
            case 2:
                mouseTexturePath = "mouseupframe1.png";
                break;
            case 3:
                mouseTexturePath = "mousedownframe1.png";
                break;
        }
        SDL_Texture* mouseTexture = graphics.loadTexture(mouseTexturePath.c_str());

        mouse.move();
        if (mouse.canEat(cheese)) {
            cheese.respawn();
            mouse.grow();
            graphics.play(eatSound);
            score++; // Increase score when cheese is eaten
            if (score > highestScore) {
                highestScore = score; // Update highest score if current score exceeds highest score
                // Save highest score to file
                ofstream outFile("highestscore.txt");
                if (outFile.is_open()) {
                    outFile << highestScore;
                    outFile.close();
                }
            }
        }

        graphics.renderTexture(mouseTexture, mouse.rect.x, mouse.rect.y);
        graphics.renderTexture(cheeseTexture, cheese.rect.x, cheese.rect.y);
        renderScore(graphics, font, score, highestScore);
        SDL_DestroyTexture(mouseTexture);
        graphics.presentScene();
        SDL_Delay(10);
    }

    // Free resources before creating the game over window
    SDL_DestroyTexture(cheeseTexture);
    SDL_DestroyTexture(backgroundTexture);
    TTF_CloseFont(font);

    // Create Game Over Window
    EngineWindow gameOverWindow;
    gameOverWindow.createWindow("Game Over", 800, 600);
    SDL_RenderClear(gameOverWindow.renderer);

    TTF_Font* gameOverFont = TTF_OpenFont("PixelGosub-ZaRz.ttf", 72);
    if (gameOverFont == nullptr) {
        cerr << "Failed to load font. SDL_ttf Error: " << TTF_GetError() << endl;
        gameOverWindow.destroyWindow();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Render Game Over message
    Mix_Chunk* deadSound = graphics.loadSound("dead.wav");
    graphics.play(deadSound);
    SDL_Color textColor = {255, 255, 255, 255}; // White color
    SDL_Surface* gameOverSurface = TTF_RenderText_Solid(gameOverFont, "Game Over", textColor);
    SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(gameOverWindow.renderer, gameOverSurface);

    int textWidth = gameOverSurface->w;
    int textHeight = gameOverSurface->h;
    SDL_FreeSurface(gameOverSurface);

    SDL_Rect textRect = {400 - textWidth / 2, 300 - textHeight / 2, textWidth, textHeight}; // Centered in the window

    SDL_RenderCopy(gameOverWindow.renderer, gameOverTexture, NULL, &textRect);
    SDL_RenderPresent(gameOverWindow.renderer);

    bool gameOverQuit = false;
    while (!gameOverQuit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                gameOverQuit = true;
            }
            if (event.type == SDL_KEYDOWN) {
                gameOverQuit = true;
            }
        }
        SDL_Delay(10);
    }

    Mix_FreeChunk(eatSound);
    Mix_FreeChunk(deadSound);
    SDL_DestroyTexture(gameOverTexture);
    TTF_CloseFont(gameOverFont);
    gameOverWindow.destroyWindow();

    TTF_Quit();
    graphics.quit();
    SDL_Quit();
    return 0;
}
