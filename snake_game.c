#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define GRID_SIZE 20
#define SNAKE_MAX_LENGTH 100
#define BASE_SPEED 100  // Base speed in milliseconds
#define TIME_ATTACK_DURATION 60  // Duration in seconds for Time Attack mode
#define PHANTOM_WALL_INTERVAL 5.0  // Seconds between phantom wall appearances
#define PHANTOM_WALL_DURATION 3.0  // Seconds the phantom wall stays active
#define NAV_BAR_HEIGHT 80  // Height of the navigation/status bar
#define COUNTDOWN_DURATION 3.0f  // Duration of the countdown animation (3 seconds)

void SpawnFood();

typedef enum {
    FRONT_PAGE,
    MENU,
    COUNTDOWN,
    GAME,
    GAME_OVER
} GameState;

typedef enum {
    CLASSIC,
    TIME_ATTACK,
    CHALLENGE,
    INFINITE  // New mode
} GameMode;

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position body[SNAKE_MAX_LENGTH];
    int length;
    int dx, dy;
} Snake;

typedef struct {
    Position start;
    Position end;
    float timer;
    int active;
    int countdown; 
} PhantomWall;

typedef struct {
    Rectangle rect;
    Color baseColor;
    Color hoverColor;
    Color textColor;
    const char* text;
    bool hover;
} Button;

Position food, goldenFruit;
Snake snake;
PhantomWall phantomWall;
int running = 1;
int goldenActive = 0;
int paused = 0;
int speed = BASE_SPEED;
double lastUpdateTime = 0;
float phantomWallTimer = 0;
float countdownTimer = COUNTDOWN_DURATION;

Color bgColor = { 0, 0, 0, 255 };              // Pure black background
Color navBarColor = { 15, 15, 20, 255 };       // Very dark gray for nav bar
Color menuBgColor = { 10, 10, 15, 255 };       // Very dark background for menu
Color accentColor1 = { 193, 255, 114, 255 };   // Bright lime green (#c1ff72) - main accent
Color accentColor2 = { 92, 225, 230, 255 };    // Bright cyan (#5ce1e6) - secondary accent
Color accentColor3 = { 255, 215, 0, 255 };     // Gold for highlights
Color phantomWallColor = {255, 100, 100, 255}; // Red for danger (phantom walls)

// Menu buttons
Button menuButtons[4];
Button startButton;
Button exitButton;

// Game state variables
GameState currentState = FRONT_PAGE;
GameMode currentMode = CLASSIC;
GameMode selectedMode = CLASSIC;
float timeAttackTimer = TIME_ATTACK_DURATION;
int selectedOption = 0;
const int numOptions = 4;
char* menuOptions[] = {"Classic Mode", "Time Attack Mode", "Challenge Mode", "Infinite Play"};

void InitButtons() {
    // Initialize start button for front page
    startButton.rect = (Rectangle){
        SCREEN_WIDTH/2 - 150,
        SCREEN_HEIGHT/2 - 30,
        300,
        60
    };
    startButton.baseColor = (Color){ 193, 255, 114, 255 }; // Bright lime green
    startButton.hoverColor = (Color){ 213, 255, 144, 255 }; // Even brighter
    startButton.textColor = BLACK;
    startButton.text = "START GAME";
    startButton.hover = false;
   
    // Initialize exit button for front page
    exitButton.rect = (Rectangle){
        SCREEN_WIDTH/2 - 150,
        SCREEN_HEIGHT/2 + 60,
        300,
        60
    };
    exitButton.baseColor = (Color){ 255, 100, 100, 255 }; // Bright red
    exitButton.hoverColor = (Color){ 255, 130, 130, 255 };
    exitButton.textColor = WHITE;
    exitButton.text = "EXIT GAME";
    exitButton.hover = false;
   
    // Initialize menu buttons with bright theme
    Color buttonColors[4] = {
        (Color){193, 255, 114, 255},  // Classic - lime green
        (Color){92, 225, 230, 255},   // Time Attack - cyan
        (Color){150, 240, 180, 255},  // Challenge - mint green
        (Color){255, 220, 100, 255}   // Infinite Play - bright yellow
    };
   
    for (int i = 0; i < numOptions; i++) {
        menuButtons[i].rect = (Rectangle){
            SCREEN_WIDTH/2 - 150,
            250 + i * 70,
            300,
            50
        };
        menuButtons[i].baseColor = buttonColors[i];
        menuButtons[i].hoverColor = (Color){
            buttonColors[i].r + 20,
            buttonColors[i].g + 20,
            buttonColors[i].b + 20,
            255
        };
        menuButtons[i].textColor = (Color){20, 20, 30, 255};  // Dark text for contrast on bright buttons
        menuButtons[i].text = menuOptions[i];
        menuButtons[i].hover = false;
    }
}

void InitSnake() {
    snake.length = 2;
    for (int i = 0; i < snake.length; i++) {
        snake.body[i].x = (SCREEN_WIDTH / GRID_SIZE / 2) * GRID_SIZE - i * GRID_SIZE;
        snake.body[i].y = ((SCREEN_HEIGHT - NAV_BAR_HEIGHT) / GRID_SIZE / 2 + NAV_BAR_HEIGHT / GRID_SIZE) * GRID_SIZE;
    }
    snake.dx = GRID_SIZE;
    snake.dy = 0;
}

void ResetGame() {
    running = 1;
    paused = 0;
    speed = BASE_SPEED;
    timeAttackTimer = TIME_ATTACK_DURATION;
    phantomWallTimer = 0;
    phantomWall.active = 0;
    phantomWall.countdown = 0;
    goldenActive = 0;
    currentMode = selectedMode;
    InitSnake();
    SpawnFood();
}

void SpawnFood() {
    do {
        food.x = (GetRandomValue(0, (SCREEN_WIDTH / GRID_SIZE) - 1)) * GRID_SIZE;
        food.y = (GetRandomValue(0, (SCREEN_HEIGHT / GRID_SIZE) - 1)) * GRID_SIZE;
    } while (food.y < NAV_BAR_HEIGHT);

    if (currentMode == CHALLENGE) {
        if (GetRandomValue(0, 4) == 0) {
            do {
                goldenFruit.x = (GetRandomValue(0, (SCREEN_WIDTH / GRID_SIZE) - 1)) * GRID_SIZE;
                goldenFruit.y = (GetRandomValue(0, (SCREEN_HEIGHT / GRID_SIZE) - 1)) * GRID_SIZE;
            } while (goldenFruit.y < NAV_BAR_HEIGHT);
            goldenActive = 1;
        } else {
            goldenActive = 0;
        }
    } else {
        goldenActive = 0;
    }
}

void GeneratePhantomWall() {
    if (currentMode != CHALLENGE) return;
   
    int dx = food.x - snake.body[0].x;
    int dy = food.y - snake.body[0].y;
   
    float distance = sqrtf(dx*dx + dy*dy);
    if (distance < GRID_SIZE) {
        phantomWall.active = 0;
        return;
    }
   
    float ndx = dx / distance;
    float ndy = dy / distance;
   
    float midPointX = snake.body[0].x + (dx * 0.33f);
    float midPointY = snake.body[0].y + (dy * 0.33f);
   
    midPointX = roundf(midPointX / GRID_SIZE) * GRID_SIZE;
    midPointY = roundf(midPointY / GRID_SIZE) * GRID_SIZE;
   
    float perpX = -ndy;
    float perpY = ndx;
   
    int wallLength = 4 + snake.length / 3;
    if (wallLength > 12) wallLength = 12;
   
    phantomWall.start.x = midPointX + perpX * wallLength * GRID_SIZE / 2;
    phantomWall.start.y = midPointY + perpY * wallLength * GRID_SIZE / 2;
    phantomWall.end.x = midPointX - perpX * wallLength * GRID_SIZE / 2;
    phantomWall.end.y = midPointY - perpY * wallLength * GRID_SIZE / 2;
   
    if (phantomWall.start.x < 0) phantomWall.start.x = 0;
    if (phantomWall.start.x >= SCREEN_WIDTH) phantomWall.start.x = SCREEN_WIDTH - GRID_SIZE;
    if (phantomWall.start.y < NAV_BAR_HEIGHT) phantomWall.start.y = NAV_BAR_HEIGHT;
    if (phantomWall.start.y >= SCREEN_HEIGHT) phantomWall.start.y = SCREEN_HEIGHT - GRID_SIZE;
   
    if (phantomWall.end.x < 0) phantomWall.end.x = 0;
    if (phantomWall.end.x >= SCREEN_WIDTH) phantomWall.end.x = SCREEN_WIDTH - GRID_SIZE;
    if (phantomWall.end.y < NAV_BAR_HEIGHT) phantomWall.end.y = NAV_BAR_HEIGHT;
    if (phantomWall.end.y >= SCREEN_HEIGHT) phantomWall.end.y = SCREEN_HEIGHT - GRID_SIZE;
   
    phantomWall.active = 1;
    phantomWall.timer = PHANTOM_WALL_DURATION;
    phantomWall.countdown = 3;  
}

int IsPointOnPhantomWall(int x, int y) {
    if (!phantomWall.active) return 0;
   
    int minX = phantomWall.start.x < phantomWall.end.x ? phantomWall.start.x : phantomWall.end.x;
    int maxX = phantomWall.start.x > phantomWall.end.x ? phantomWall.start.x : phantomWall.end.x;
    int minY = phantomWall.start.y < phantomWall.end.y ? phantomWall.start.y : phantomWall.end.y;
    int maxY = phantomWall.start.y > phantomWall.end.y ? phantomWall.start.y : phantomWall.end.y;
   
    if (maxX - minX < maxY - minY) {
        if (x >= minX && x <= maxX + GRID_SIZE && y >= minY && y <= maxY) {
            float t = (float)(x - phantomWall.start.x) / (phantomWall.end.x - phantomWall.start.x);
            if (isnan(t)) t = 0;
            float interpolatedY = phantomWall.start.y + t * (phantomWall.end.y - phantomWall.start.y);
            return (y >= interpolatedY - GRID_SIZE && y <= interpolatedY + GRID_SIZE);
        }
    } else {
        if (y >= minY && y <= maxY + GRID_SIZE && x >= minX && x <= maxX) {
            float t = (float)(y - phantomWall.start.y) / (phantomWall.end.y - phantomWall.start.y);
            if (isnan(t)) t = 0;
            float interpolatedX = phantomWall.start.x + t * (phantomWall.end.x - phantomWall.start.x);
            return (x >= interpolatedX - GRID_SIZE && x <= interpolatedX + GRID_SIZE);
        }
    }
   
    return 0;
}

void UpdateSnake() {
    if (paused) return;
    Position newHead = { snake.body[0].x + snake.dx, snake.body[0].y + snake.dy };
   
    // In infinite mode, wrap around screen instead of dying
    if (currentMode == INFINITE) {
        if (newHead.x < 0) newHead.x = SCREEN_WIDTH - GRID_SIZE;
        if (newHead.x >= SCREEN_WIDTH) newHead.x = 0;
        if (newHead.y < NAV_BAR_HEIGHT) newHead.y = SCREEN_HEIGHT - GRID_SIZE;
        if (newHead.y >= SCREEN_HEIGHT) newHead.y = NAV_BAR_HEIGHT;
    } else {
        // Regular wall collision for other modes
        if (newHead.x < 0 || newHead.x >= SCREEN_WIDTH || newHead.y < NAV_BAR_HEIGHT || newHead.y >= SCREEN_HEIGHT) {
            running = 0;
        }
    }
   
    // Phantom wall collision (only in Challenge mode)
    if (currentMode == CHALLENGE && IsPointOnPhantomWall(newHead.x, newHead.y)) {
        running = 0;
    }
   
    // Self collision (disabled in infinite mode when length is below 20)
    if (currentMode != INFINITE || (currentMode == INFINITE && snake.length >= 20)) {
        for (int i = 1; i < snake.length; i++) {
            if (newHead.x == snake.body[i].x && newHead.y == snake.body[i].y) {
                running = 0;
            }
        }
    }
   
    // Rest of the function remains the same...
    // Move snake
    for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
    }
    snake.body[0] = newHead;
   
    // Eat food
    if (newHead.x == food.x && newHead.y == food.y) {
        if (snake.length < SNAKE_MAX_LENGTH) {
            snake.length++;
        }
        SpawnFood();
        if (speed > 40) {
            if (currentMode == CLASSIC || currentMode == INFINITE) {
                speed -= 2;
            } else {
                speed -= 5;
            }
        }
    }
   
    // Eat golden fruit
    if (currentMode == CHALLENGE && goldenActive && newHead.x == goldenFruit.x && newHead.y == goldenFruit.y) {
        if (snake.length < SNAKE_MAX_LENGTH - 3) {
            snake.length += 3;
        }
        goldenActive = 0;
        if (speed > 30) speed -= 10;
    }
}

void HandleFrontPageInput() {
    Vector2 mousePoint = GetMousePosition();
   
    // Check if start button is hovered
    if (CheckCollisionPointRec(mousePoint, startButton.rect)) {
        startButton.hover = true;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            currentState = MENU;
        }
    } else {
        startButton.hover = false;
    }
   
    // Check if exit button is hovered
    if (CheckCollisionPointRec(mousePoint, exitButton.rect)) {
        exitButton.hover = true;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            CloseWindow();
        }
    } else {
        exitButton.hover = false;
    }
   
    // Start game with Enter or Space key
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        currentState = MENU;
    }
   
    // Exit with Escape key - quit the game
    if (IsKeyPressed(KEY_ESCAPE)) {
        CloseWindow();
    }
}

void HandleMenuInput() {
    Vector2 mousePoint = GetMousePosition();
   
    for (int i = 0; i < numOptions; i++) {
        if (CheckCollisionPointRec(mousePoint, menuButtons[i].rect)) {
            menuButtons[i].hover = true;
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                selectedOption = i;
                selectedMode = (GameMode)i;
                currentState = COUNTDOWN;
                countdownTimer = COUNTDOWN_DURATION;
            }
        } else {
            menuButtons[i].hover = false;
        }
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        selectedOption = (selectedOption + 1) % numOptions;
    }
   
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        selectedOption = (selectedOption - 1 + numOptions) % numOptions;
    }
   
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        selectedMode = (GameMode)selectedOption;
        currentState = COUNTDOWN;
        countdownTimer = COUNTDOWN_DURATION;
    }
   
    // ESC goes back to front page
    if (IsKeyPressed(KEY_ESCAPE)) {
        currentState = FRONT_PAGE;
    }
}

void HandleGameInput() {
    if (IsKeyPressed(KEY_P)) {
        paused = !paused;
    }
    if (!paused) {
        if ((IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) && snake.dy == 0) {
            snake.dx = 0;
            snake.dy = -GRID_SIZE;
        }
        if ((IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) && snake.dy == 0) {
            snake.dx = 0;
            snake.dy = GRID_SIZE;
        }
        if ((IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) && snake.dx == 0) {
            snake.dx = -GRID_SIZE;
            snake.dy = 0;
        }
        if ((IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) && snake.dx == 0) {
            snake.dx = GRID_SIZE;
            snake.dy = 0;
        }
    }
   
    // Q key - go back to menu
    if (IsKeyPressed(KEY_Q)) {
        currentState = MENU;
    }
    
    // ESC key - quit the game completely
    if (IsKeyPressed(KEY_ESCAPE)) {
        CloseWindow();
    }
}

void HandleGameOverInput() {
    // ENTER or SPACE - return to menu
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        currentState = MENU;
    }
    
    // Q key - also go back to menu
    if (IsKeyPressed(KEY_Q)) {
        currentState = MENU;
    }
    
    // ESC key - quit the game completely
    if (IsKeyPressed(KEY_ESCAPE)) {
        CloseWindow();
    }
}

void UpdateCountdown(float deltaTime) {
    countdownTimer -= deltaTime;
    if (countdownTimer <= 0) {
        ResetGame();
        currentState = GAME;
    }
}

void UpdateGame(float deltaTime) {
    if (currentMode == TIME_ATTACK && !paused) {
        timeAttackTimer -= deltaTime;
        if (timeAttackTimer <= 0) {
            running = 0;
        }
    }
   
    if (currentMode == CHALLENGE && !paused) {
        if (phantomWall.active) {
            phantomWall.timer -= deltaTime;
           
            // Update countdown display (3, 2, 1)
            int currentCountdown = (int)ceil(phantomWall.timer);
            if (currentCountdown != phantomWall.countdown && currentCountdown >= 0 && currentCountdown <= 3) {
                phantomWall.countdown = currentCountdown;
            }
           
            if (phantomWall.timer <= 0) {
                phantomWall.active = 0;
            }
        } else {
            phantomWallTimer += deltaTime;
            if (phantomWallTimer >= PHANTOM_WALL_INTERVAL) {
                GeneratePhantomWall();
                phantomWallTimer = 0;
            }
        }
    }
   
    double currentTime = GetTime();
    if (currentTime - lastUpdateTime > speed / 1000.0) {
        UpdateSnake();
        lastUpdateTime = currentTime;
    }
   
    if (!running && currentMode != INFINITE) {
        currentState = GAME_OVER;
    }
}

void RenderFrontPage() {
    BeginDrawing();
    ClearBackground(bgColor);

    // SNAKESCAPE title with glow effect
    const char* gameTitle = "SNAKESCAPE";
    int titleSize = 70;
    int titleY = 120;
    
    // Glow effect for title
    for (int i = 5; i > 0; i--) {
        Color glowColor = accentColor1;
        glowColor.a = 30 + 10 * i;
        DrawText(gameTitle, 
                 SCREEN_WIDTH / 2 - MeasureText(gameTitle, titleSize) / 2 + i/2, 
                 titleY - i/2, 
                 titleSize, 
                 glowColor);
        DrawText(gameTitle, 
                 SCREEN_WIDTH / 2 - MeasureText(gameTitle, titleSize) / 2 - i/2, 
                 titleY - i/2, 
                 titleSize, 
                 glowColor);
    }
    
    // Main title
    DrawText(gameTitle,
             SCREEN_WIDTH / 2 - MeasureText(gameTitle, titleSize) / 2,
             titleY,
             titleSize,
             accentColor1);

    // Subtitle "Hunt. Grow. Survive." with color gradient
    const char* subtitle = "Hunt. Grow. Survive.";
    int subtitleSize = 30;
    int subtitleY = titleY + 80;
    
    // Glow effect for subtitle
    for (int i = 3; i > 0; i--) {
        Color glowColor = accentColor2;
        glowColor.a = 40;
        DrawText(subtitle,
                 SCREEN_WIDTH / 2 - MeasureText(subtitle, subtitleSize) / 2 + i/2,
                 subtitleY - i/2,
                 subtitleSize,
                 glowColor);
    }
    
    // Main subtitle
    DrawText(subtitle,
             SCREEN_WIDTH / 2 - MeasureText(subtitle, subtitleSize) / 2,
             subtitleY,
             subtitleSize,
             accentColor2);

    // Update button positions to be at bottom, side by side
    int buttonWidth = 180;
    int buttonHeight = 45;
    int buttonSpacing = 20; // Space between buttons
    
    startButton.rect.x = SCREEN_WIDTH/2 - buttonWidth - buttonSpacing/2;
    startButton.rect.y = SCREEN_HEIGHT - 120;
    startButton.rect.width = buttonWidth;
    startButton.rect.height = buttonHeight;
    
    exitButton.rect.x = SCREEN_WIDTH/2 + buttonSpacing/2;
    exitButton.rect.y = SCREEN_HEIGHT - 120;
    exitButton.rect.width = buttonWidth;
    exitButton.rect.height = buttonHeight;

    // Draw START button with shadow
    DrawRectangleRec(
        (Rectangle){
            startButton.rect.x + 4,
            startButton.rect.y + 4,
            startButton.rect.width,
            startButton.rect.height
        },
        (Color){ 0, 0, 0, 100 }
    );
    
    DrawRectangleRec(
        startButton.rect,
        startButton.hover ? startButton.hoverColor : startButton.baseColor
    );
    
    DrawRectangleLinesEx(
        startButton.rect,
        3,
        startButton.hover ? WHITE : (Color){ 200, 200, 200, 255 }
    );
    
    DrawText(
        startButton.text,
        startButton.rect.x + startButton.rect.width/2 - MeasureText(startButton.text, 22)/2,
        startButton.rect.y + startButton.rect.height/2 - 11,
        22,
        startButton.textColor
    );

    // Draw EXIT button with shadow
    DrawRectangleRec(
        (Rectangle){
            exitButton.rect.x + 4,
            exitButton.rect.y + 4,
            exitButton.rect.width,
            exitButton.rect.height
        },
        (Color){ 0, 0, 0, 100 }
    );
    
    DrawRectangleRec(
        exitButton.rect,
        exitButton.hover ? exitButton.hoverColor : exitButton.baseColor
    );
    
    DrawRectangleLinesEx(
        exitButton.rect,
        3,
        exitButton.hover ? WHITE : (Color){ 200, 200, 200, 255 }
    );
    
    DrawText(
        exitButton.text,
        exitButton.rect.x + exitButton.rect.width/2 - MeasureText(exitButton.text, 22)/2,
        exitButton.rect.y + exitButton.rect.height/2 - 11,
        22,
        exitButton.textColor
    );

    // Display keyboard shortcuts at bottom (below buttons)
    const char* msg = "or use ENTER / ESC keys";
    DrawText(msg,
             SCREEN_WIDTH / 2 - MeasureText(msg, 16) / 2,
             SCREEN_HEIGHT - 55,
             16,
             (Color){150, 150, 150, 255});

    EndDrawing();
}

void RenderMenu() {
    BeginDrawing();

    // Solid background with subtle gradient
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float factor = (float)y / SCREEN_HEIGHT;
        Color lineColor = {
            (unsigned char)(menuBgColor.r * (1.0 - factor * 0.2)),
            (unsigned char)(menuBgColor.g * (1.0 - factor * 0.2)),
            (unsigned char)(menuBgColor.b * (1.0 - factor * 0.1)),
            255
        };
        DrawLine(0, y, SCREEN_WIDTH, y, lineColor);
    }
   
    // Title with glow effect
    int titleSize = 50;
    const char* title = "SELECT GAME MODE";
   
    // Glow effect
    for (int i = 3; i > 0; i--) {
        Color glowColor = accentColor1;
        glowColor.a = 20 + 10 * i;
        DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, titleSize)/2 + i/2, 100 - i/2, titleSize, glowColor);
        DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, titleSize)/2 - i/2, 100 - i/2, titleSize, glowColor);
    }
   
    // Main title
    DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, titleSize)/2, 100, titleSize, accentColor1);
   
    // Menu buttons
    for (int i = 0; i < numOptions; i++) {
        // Button shadow
        DrawRectangleRec((Rectangle){
            menuButtons[i].rect.x + 4,
            menuButtons[i].rect.y + 4,
            menuButtons[i].rect.width,
            menuButtons[i].rect.height
        }, (Color){ 0, 0, 0, 70 });
       
        // Button with hover effect
        DrawRectangleRec(
            menuButtons[i].rect,
            menuButtons[i].hover ? menuButtons[i].hoverColor : menuButtons[i].baseColor
        );
       
        // Button border
        DrawRectangleLinesEx(
            menuButtons[i].rect,
            2,
            menuButtons[i].hover ? WHITE : (Color){ 200, 200, 200, 255 }
        );
       
        // Button text
        DrawText(
            menuButtons[i].text,
            menuButtons[i].rect.x + menuButtons[i].rect.width/2 - MeasureText(menuButtons[i].text, 25)/2,
            menuButtons[i].rect.y + menuButtons[i].rect.height/2 - 12,
            25,
            menuButtons[i].textColor
        );
       
        // Selection indicator
        if (i == selectedOption) {
            DrawRectangleLinesEx(
                (Rectangle){
                    menuButtons[i].rect.x - 5,
                    menuButtons[i].rect.y - 5,
                    menuButtons[i].rect.width + 10,
                    menuButtons[i].rect.height + 10
                },
                2,
                WHITE
            );
        }
    }
 
    DrawText("Click on a mode to start playing",
        SCREEN_WIDTH/2 - MeasureText("Click on a mode to start playing", 20)/2,
        SCREEN_HEIGHT - 70, 20, accentColor3);
   
    // Game mode descriptions
    int descY = SCREEN_HEIGHT - 40;
    if (selectedOption == 0) {
        DrawText("Classic: The traditional snake game experience",
            SCREEN_WIDTH/2 - MeasureText("Classic: The traditional snake game experience", 20)/2,
            descY, 20, accentColor1);
    } else if (selectedOption == 1) {
        DrawText("Time Attack: Score as much as possible before time runs out",
            SCREEN_WIDTH/2 - MeasureText("Time Attack: Score as much as possible before time runs out", 20)/2,
            descY, 20, accentColor2);
    } else if (selectedOption == 2) {
        DrawText("Challenge: Dodge phantom walls and collect golden fruit!",
            SCREEN_WIDTH/2 - MeasureText("Challenge: Dodge phantom walls and collect golden fruit!", 20)/2,
            descY, 20, accentColor3);
    } else if (selectedOption == 3) {
        DrawText("Infinite: Snake can't die - play as long as you want!",
            SCREEN_WIDTH/2 - MeasureText("Infinite: Snake can't die - play as long as you want!", 20)/2,
            descY, 20, (Color){200, 200, 0, 255});
    }
    
    EndDrawing();
}

void RenderCountdown() {
    BeginDrawing();
   
    // Solid background
    ClearBackground(bgColor);
   
    // Draw the countdown number or "GO!"
    char countdownText[10];
    int currentCount = (int)ceil(countdownTimer);
   
    if (currentCount > 0) {
        sprintf(countdownText, "%d", currentCount);
    } else {
        strcpy(countdownText, "GO!");
    }
   
    // Pulse animation
    float pulseScale = 1.0f + 0.3f * sin((COUNTDOWN_DURATION - countdownTimer) * 10);
    int fontSize = (int)(100 * pulseScale);
   
    // Color based on countdown progress
    Color countdownColor;
    if (currentCount == 3) {
        countdownColor = accentColor1;
    } else if (currentCount == 2) {
        countdownColor = accentColor2;
    } else if (currentCount == 1) {
        countdownColor = accentColor3;
    } else {
        countdownColor = WHITE;
    }
   
    // Glow effect
    for (int i = 3; i > 0; i--) {
        Color glowColor = countdownColor;
        glowColor.a = 50;
        DrawText(
            countdownText,
            SCREEN_WIDTH/2 - MeasureText(countdownText, fontSize)/2 + i,
            SCREEN_HEIGHT/2 - fontSize/2,
            fontSize,
            glowColor
        );
    }
   
    // Main text
    DrawText(
        countdownText,
        SCREEN_WIDTH/2 - MeasureText(countdownText, fontSize)/2,
        SCREEN_HEIGHT/2 - fontSize/2,
        fontSize,
        countdownColor
    );
   
    // Selected mode
    const char* modeText;
    Color modeColor;
   
    switch (selectedMode) {
        case CLASSIC:
            modeText = "CLASSIC MODE";
            modeColor = accentColor1;
            break;
        case TIME_ATTACK:
            modeText = "TIME ATTACK MODE";
            modeColor = accentColor2;
            break;
        case CHALLENGE:
            modeText = "CHALLENGE MODE";
            modeColor = accentColor3;
            break;
        case INFINITE:
            modeText = "INFINITE PLAY";
            modeColor = (Color){200, 200, 0, 255};
            break;
    }
   
    DrawText(
        modeText,
        SCREEN_WIDTH/2 - MeasureText(modeText, 30)/2,
        SCREEN_HEIGHT/2 + 100,
        30,
        modeColor
    );
   
    // Get ready message
    DrawText(
        "Get Ready!",
        SCREEN_WIDTH/2 - MeasureText("Get Ready!", 40)/2,
        SCREEN_HEIGHT/2 - 150,
        40,
        WHITE
    );
   
    EndDrawing();
}

void RenderGame() {
    BeginDrawing();
    ClearBackground(bgColor);

    // Draw the navigation bar
    DrawRectangle(0, 0, SCREEN_WIDTH, NAV_BAR_HEIGHT, navBarColor);
   
    // Draw score
    char scoreText[30];
    sprintf(scoreText, "Score: %d", snake.length - 2);
    DrawText(scoreText, 20, 20, 24, WHITE);
   
    // Draw timer for Time Attack mode
    if (currentMode == TIME_ATTACK) {
        char timerText[30];
        sprintf(timerText, "Time: %.1f", timeAttackTimer);
        DrawText(timerText, SCREEN_WIDTH - MeasureText(timerText, 24) - 20, 20, 24, accentColor2);
    }
    
    // Challenge mode - display countdown until next phantom wall
    if (currentMode == CHALLENGE && !phantomWall.active) {
        float timeRemaining = PHANTOM_WALL_INTERVAL - phantomWallTimer;
        if (timeRemaining < 0) timeRemaining = 0;

        char wallTimerText[50];
        sprintf(wallTimerText, "Next Wall: %.1fs", timeRemaining);
        DrawText(wallTimerText,
                 SCREEN_WIDTH - MeasureText(wallTimerText, 20) - 20,
                 20,
                 20,
                 phantomWallColor);
    }

    // Draw game mode
    const char* modeText;
    Color modeColor;
   
    switch (currentMode) {
        case CLASSIC:
            modeText = "CLASSIC MODE";
            modeColor = accentColor1;
            break;
        case TIME_ATTACK:
            modeText = "TIME ATTACK";
            modeColor = accentColor2;
            break;
        case CHALLENGE:
            modeText = "CHALLENGE MODE";
            modeColor = accentColor3;
            break;
        case INFINITE:
            modeText = "INFINITE PLAY";
            modeColor = (Color){200, 200, 0, 255};
            break;
    }
   
    DrawText(modeText, SCREEN_WIDTH/2 - MeasureText(modeText, 24)/2, 20, 24, modeColor);
   
    // Draw pause indicator
    if (paused) {
        DrawText("PAUSED", SCREEN_WIDTH/2 - MeasureText("PAUSED", 40)/2,
                SCREEN_HEIGHT/2 - 20, 40, WHITE);
        DrawText("Press P to resume", SCREEN_WIDTH/2 - MeasureText("Press P to resume", 20)/2,
                SCREEN_HEIGHT/2 + 30, 20, accentColor3);
    }
   
    // Draw grid lines (subtle grey - more visible)
    for (int i = 0; i < SCREEN_WIDTH/GRID_SIZE; i++) {
        DrawLine(i * GRID_SIZE, NAV_BAR_HEIGHT, i * GRID_SIZE, SCREEN_HEIGHT,
                 (Color){70, 70, 70, 120});
    }
   
    for (int i = NAV_BAR_HEIGHT/GRID_SIZE; i < SCREEN_HEIGHT/GRID_SIZE; i++) {
        DrawLine(0, i * GRID_SIZE, SCREEN_WIDTH, i * GRID_SIZE,
                 (Color){70, 70, 70, 120});
    }
   
    // Draw apple-shaped food with natural colors
    const int padding = 2;
    const int appleSize = GRID_SIZE - 2 * padding;
    const int stemHeight = 4;
    const int stemWidth = 2;
    const int leafWidth = 4;
    const int leafHeight = 2;

    // Define natural apple colors
    Color appleRed = (Color){ 200, 0, 0, 255 };
    Color stemBrown = (Color){ 101, 67, 33, 255 };
    Color leafGreen = (Color){ 34, 139, 34, 255 };

    // Apple body
    DrawCircle(
        food.x + GRID_SIZE / 2,
        food.y + GRID_SIZE / 2,
        appleSize / 2,
        appleRed
    );

    // Stem
    DrawRectangle(
        food.x + GRID_SIZE / 2 - stemWidth / 2,
        food.y + padding,
        stemWidth,
        stemHeight,
        stemBrown
    );

    // Leaf
    DrawEllipse(
        food.x + GRID_SIZE / 2 + leafWidth / 2,
        food.y + padding,
        leafWidth,
        leafHeight,
        leafGreen
    );
   
    // Draw golden fruit if active
    if (goldenActive) {
        Color goldenColor = (Color){255, 165, 0, 255};
       
        float fruitX = goldenFruit.x + 2;
        float fruitY = goldenFruit.y + 2;
        float fruitSize = GRID_SIZE - 4;
   
        // Draw the orange fruit with more natural shape
        DrawRectangleRounded(
            (Rectangle){ fruitX, fruitY, fruitSize, fruitSize },
            0.9f,
            16,
            (Color){ 255, 165, 0, 255 }
        );
   
        // Draw the brown stem with curve
        float stemWidth = fruitSize * 0.2f;
        float stemHeight = fruitSize * 0.3f;
        DrawRectangleRounded(
            (Rectangle){
                fruitX + (fruitSize - stemWidth)/2,
                fruitY - stemHeight + 2,
                stemWidth,
                stemHeight
            },
            0.8f,
            6,
            (Color){ 139, 69, 19, 255 }
        );
    }
   
    // Draw snake body
    for (int i = 0; i < snake.length; i++) {
        // Calculate color gradient from head to tail
        float colorFactor = (float)i / snake.length;
        Color snakeColor;
       
        if (currentMode == CLASSIC) {
            snakeColor = (Color){
                (unsigned char)(accentColor1.r * (1.0f - colorFactor * 0.5f)),
                (unsigned char)(accentColor1.g * (1.0f - colorFactor * 0.3f)),
                (unsigned char)(accentColor1.b * (1.0f - colorFactor * 0.1f)),
                255
            };
        } else if (currentMode == TIME_ATTACK) {
            snakeColor = (Color){
                (unsigned char)(accentColor2.r * (1.0f - colorFactor * 0.5f)),
                (unsigned char)(accentColor2.g * (1.0f - colorFactor * 0.3f)),
                (unsigned char)(accentColor2.b * (1.0f - colorFactor * 0.1f)),
                255
            };
        } else {
            snakeColor = (Color){
                (unsigned char)(accentColor3.r * (1.0f - colorFactor * 0.5f)),
                (unsigned char)(accentColor3.g * (1.0f - colorFactor * 0.3f)),
                (unsigned char)(accentColor3.b * (1.0f - colorFactor * 0.1f)),
                255
            };
        }
       
        // Draw segment
        float segmentRadius = i == 0 ? 0.5f : 0.3f;
        int segmentCorners = i == 0 ? 8 : 6;
       
        DrawRectangleRounded(
            (Rectangle){snake.body[i].x, snake.body[i].y, GRID_SIZE, GRID_SIZE},
            segmentRadius,
            segmentCorners,
            snakeColor
        );
       
        // Draw eyes on the head
        if (i == 0) {
            float eyeSize = GRID_SIZE * 0.2f;
            float eyeOffset = GRID_SIZE * 0.25f;
           
            float eyeX1, eyeX2, eyeY1, eyeY2;
           
            if (snake.dx > 0) {
                eyeX1 = eyeX2 = snake.body[0].x + GRID_SIZE - eyeSize - 2;
                eyeY1 = snake.body[0].y + eyeOffset;
                eyeY2 = snake.body[0].y + GRID_SIZE - eyeSize - eyeOffset;
            } else if (snake.dx < 0) {
                eyeX1 = eyeX2 = snake.body[0].x + 2;
                eyeY1 = snake.body[0].y + eyeOffset;
                eyeY2 = snake.body[0].y + GRID_SIZE - eyeSize - eyeOffset;
            } else if (snake.dy > 0) {
                eyeY1 = eyeY2 = snake.body[0].y + GRID_SIZE - eyeSize - 2;
                eyeX1 = snake.body[0].x + eyeOffset;
                eyeX2 = snake.body[0].x + GRID_SIZE - eyeSize - eyeOffset;
            } else {
                eyeY1 = eyeY2 = snake.body[0].y + 2;
                eyeX1 = snake.body[0].x + eyeOffset;
                eyeX2 = snake.body[0].x + GRID_SIZE - eyeSize - eyeOffset;
            }
           
            DrawRectangleRounded(
                (Rectangle){eyeX1, eyeY1, eyeSize, eyeSize},
                0.8f,
                4,
                WHITE
            );
           
            DrawRectangleRounded(
                (Rectangle){eyeX2, eyeY2, eyeSize, eyeSize},
                0.8f,
                4,
                WHITE
            );
        }
    }
   
    // Draw phantom wall if active
    if (phantomWall.active) {
        DrawLineEx(
            (Vector2){phantomWall.start.x + GRID_SIZE/2, phantomWall.start.y + GRID_SIZE/2},
            (Vector2){phantomWall.end.x + GRID_SIZE/2, phantomWall.end.y + GRID_SIZE/2},
            GRID_SIZE,
            phantomWallColor
        );
       
        if (phantomWall.countdown > 0) {
            char countdownText[10];
            sprintf(countdownText, "%d", phantomWall.countdown);
           
            float midX = (phantomWall.start.x + phantomWall.end.x) / 2;
            float midY = (phantomWall.start.y + phantomWall.end.y) / 2;
           
            float pulseScale = 1.0f + 0.3f * sinf(GetTime() * 5.0f);
            int fontSize = (int)(40 * pulseScale);
           
            DrawText(
                countdownText,
                midX - MeasureText(countdownText, fontSize)/2 + GRID_SIZE/2,
                midY - fontSize/2 + GRID_SIZE/2,
                fontSize,
                WHITE
            );
        }
    }
   
    // Draw controls reminder
    DrawText("P: Pause | Q: Menu | ESC: Quit Game", 20, 50, 16, (Color){180, 180, 180, 200});
   
    EndDrawing();
}

void RenderGameOver() {
    BeginDrawing();
    ClearBackground(bgColor);
   
    // Title with glow effect
    int titleSize = 60;
    const char* title = "GAME OVER";
   
    // Glow effect
    for (int i = 4; i > 0; i--) {
        Color glowColor = (Color){200, 30, 30, 40 + 10 * i};
        DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, titleSize)/2 + i/2, 150 - i/2, titleSize, glowColor);
        DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, titleSize)/2 - i/2, 150 - i/2, titleSize, glowColor);
    }
   
    // Main title
    DrawText(title, SCREEN_WIDTH/2 - MeasureText(title, titleSize)/2, 150, titleSize, RED);
   
    // Score
    char scoreText[50];
    sprintf(scoreText, "Final Score: %d", snake.length - 2);
    DrawText(
        scoreText,
        SCREEN_WIDTH/2 - MeasureText(scoreText, 40)/2,
        SCREEN_HEIGHT/2 - 20,
        40,
        WHITE
    );
   
    // Additional stats based on game mode
    if (currentMode == TIME_ATTACK) {
        char timeText[50];
        sprintf(timeText, "Time: %.1f seconds", TIME_ATTACK_DURATION - timeAttackTimer);
        DrawText(
            timeText,
            SCREEN_WIDTH/2 - MeasureText(timeText, 30)/2,
            SCREEN_HEIGHT/2 + 40,
            30,
            accentColor2
        );
    } else if (currentMode == CHALLENGE) {
        char challengeText[50];
        sprintf(challengeText, "Challenge Mode Completed!");
        DrawText(
            challengeText,
            SCREEN_WIDTH/2 - MeasureText(challengeText, 30)/2,
            SCREEN_HEIGHT/2 + 40,
            30,
            accentColor3
        );
    }
   
    // Return to menu instructions
    DrawText(
        "Press ENTER or SPACE to return to menu",
        SCREEN_WIDTH/2 - MeasureText("Press ENTER or SPACE to return to menu", 20)/2,
        SCREEN_HEIGHT - 100,
        20,
        accentColor1
    );
   
    EndDrawing();
}

int main() {
    // Initialize window and game
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake Game");

    SetTargetFPS(60);
   
    // Initialize random seed
    srand(time(NULL));
   
    // Initialize game components
    InitButtons();
    InitSnake();
    SpawnFood();
   
    // Main game loop
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
       
        // Handle input based on game state
        switch (currentState) {
            case FRONT_PAGE:
                HandleFrontPageInput();
                break;
            case MENU:
                HandleMenuInput();
                break;
            case COUNTDOWN:
                UpdateCountdown(deltaTime);
                break;
            case GAME:
                HandleGameInput();
                UpdateGame(deltaTime);
                break;
            case GAME_OVER:
                HandleGameOverInput();
                break;
        }
       
        // Render current game state
        switch (currentState) {
            case FRONT_PAGE:
                RenderFrontPage();
                break;
            case MENU:
                RenderMenu();
                break;
            case COUNTDOWN:
                RenderCountdown();
                break;
            case GAME:
                RenderGame();
                break;
            case GAME_OVER:
                RenderGameOver();
                break;
        }
    }
   
    // Close window and clean up
    CloseWindow();
   
    return 0;
}
