#include <stdio.h>
#include <stdbool.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>  
#include <string.h>

#include "food_img.h"
#include "2x_booster_img.h"
#include "length_shortener_img.h"
#include "point_doubler_img.h"
#include "slow_downer_img.h"

#include "eat_snd.h"
#include "wall_snd.h"
#include "double_snd.h"
#include "high_speed_snd.h"
#include "length_shortener_snd.h"
#include "slow_downer_snd.h"

#include "classic_score.h"
#include "timeattack_score.h"

#define CELL_SIZE 30
#define CELL_COUNT 25
#define OFFSET 75
#define MAX_SNAKE_LENGTH 625  
#define MAX_LEN_STR 100
#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 600
#define FRUIT_COUNT 3
#define MAX_HIGHSCORES 5
#define MAX_NAME_LENGTH 100

char playerName[MAX_LEN_STR+1]="";
char playerNames[MAX_HIGHSCORES][50];
int playerScores[MAX_HIGHSCORES];
int highscoreCount = 0;
int namelength = 0;

typedef enum {
    ENTER_NAME,
    MENU,
    GAME, 
    MULTIPLAYER_GAME,
    VS_COMPUTER_GAME,
    TIME_ATTACK_GAME
} GameState;

GameState state = ENTER_NAME;

typedef struct {
    int x, y;
} Vector2i;

typedef struct {
    Vector2i body[MAX_SNAKE_LENGTH];
    int length;
    Vector2i direction;
    bool addSegment;
} Snake;

typedef struct {
    Vector2i position;
    Texture2D texture;
} Food;

typedef struct {
    Vector2i position;
    Texture2D texture;
    bool exists;
} PowerUp;

double lastUpdateTime = 0;
bool allowMove = false;
// Speed booster variables
bool speed_booster_active = false;
double speed_boost_start = 0;
double speed_booster_spawn_time = 0;  
double speed_booster_despawn_time = 0; 
// Double booster variables
bool double_booster_active = false;
double double_boost_start = 0.0;
double double_booster_despawn_time = 0.0;
double double_booster_spawn_time = 0.0;
// Length shortener variables
double length_shortener_spawn_time = 0.0;
double length_shortener_despawn_time = 0.0;
// Slow downer variables
bool slow_downer_active = false;
double slow_downer_start = 0.0;
double slow_downer_spawn_time = 0.0;
double slow_downer_despawn_time = 0.0;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int score;
} HighScoreEntry;

// Arrays to store highscores for each mode
HighScoreEntry classicHighScores[MAX_HIGHSCORES];
HighScoreEntry timeAttackHighScores[MAX_HIGHSCORES];

// Function to convert binary data to string
char* binary_to_string(unsigned char* data, unsigned int len) {
    char* result = malloc(len + 1);
    memcpy(result, data, len);
    result[len] = '\0';
    return result;
}

// Function to parse embedded binary data and load into highscore arrays
void parse_embedded_data(unsigned char* data, unsigned int len, HighScoreEntry* highScores) {
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        highScores[i].name[0] = '\0';
        highScores[i].score = 0;
    }
    char* dataString = binary_to_string(data, len);
    char* cleanData = malloc(len + 1);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (dataString[i] == '\r') {
            if (i + 1 < len && dataString[i + 1] == '\n') {
                cleanData[j++] = '\n';
                i++;
            } 
            else {
                cleanData[j++] = '\n';
            }
        } 
        else {
            cleanData[j++] = dataString[i];
        }
    }
    cleanData[j] = '\0';
    char* line = strtok(cleanData, "\n");
    int index = 0;
    while (line != NULL && index < MAX_HIGHSCORES) {
        char name[MAX_NAME_LENGTH];
        int score;
        if (sscanf(line, "%s %d", name, &score) == 2) {
            strncpy(highScores[index].name, name, MAX_NAME_LENGTH - 1);
            highScores[index].name[MAX_NAME_LENGTH - 1] = '\0';
            highScores[index].score = score;
            printf("Loaded: %s - %d\n", highScores[index].name, highScores[index].score);
            index++;
        }
        line = strtok(NULL, "\n");
    }
    free(dataString);
    free(cleanData);
}

// Function to load highscores from embedded data
void load_highscores() {
    printf("Loading highscores from embedded header files...\n");
    printf("Loading Classic highscores:\n");
    parse_embedded_data(classic_highscores_txt, classic_highscores_txt_len, classicHighScores);
    printf("Loading TimeAttack highscores:\n");
    parse_embedded_data(timeattack_highscores_txt, timeattack_highscores_txt_len, timeAttackHighScores);
    printf("Highscores loaded successfully from embedded data.\n");
}

// Function to convert highscore array back to binary format string
char* highscores_to_string(HighScoreEntry* highScores) {
    int bufferSize = 1;
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (highScores[i].name[0] != '\0') {
            bufferSize += strlen(highScores[i].name) + 20;
        }
    }
    char* result = malloc(bufferSize);
    result[0] = '\0';
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (highScores[i].name[0] != '\0') {
            char entry[100];
            sprintf(entry, "%s %d\r\n", highScores[i].name, highScores[i].score);
            strcat(result, entry);
        }
    }
    return result;
}

// Function to save highscores (for debugging - shows what the new header would look like)
void save_highscores() {
    printf("Current highscores in memory:\n");
    char* classicData = highscores_to_string(classicHighScores);
    char* timeattackData = highscores_to_string(timeAttackHighScores);
    printf("\nClassic Mode Updated Data:\n");
    printf("String format: %s\n", classicData);
    printf("Binary format for header file:\n");
    printf("unsigned char classic_highscores_txt[] = {\n  ");
    for (int i = 0; i < strlen(classicData); i++) {
        printf("0x%02x", (unsigned char)classicData[i]);
        if (i < strlen(classicData) - 1) printf(", ");
        if ((i + 1) % 12 == 0 && i < strlen(classicData) - 1) printf("\n  ");
    }
    printf("\n};\n");
    printf("unsigned int classic_highscores_txt_len = %d;\n\n", (int)strlen(classicData));
    printf("TimeAttack Mode Updated Data:\n");
    printf("String format: %s\n", timeattackData);
    printf("Binary format for header file:\n");
    printf("unsigned char timeattack_highscores_txt[] = {\n  ");
    for (int i = 0; i < strlen(timeattackData); i++) {
        printf("0x%02x", (unsigned char)timeattackData[i]);
        if (i < strlen(timeattackData) - 1) printf(", ");
        if ((i + 1) % 12 == 0 && i < strlen(timeattackData) - 1) printf("\n  ");
    }
    printf("\n};\n");
    printf("unsigned int timeattack_highscores_txt_len = %d;\n", (int)strlen(timeattackData));
    free(classicData);
    free(timeattackData);
}

// Function to insert a new score if it qualifies for top 5
bool insert_score(const char *name, int score, const char *mode) {
    HighScoreEntry *highScores;
    printf("Attempting to insert score: %s - %d in mode: %s\n", name, score, mode);
    if (strcmp(mode, "Classic") == 0) {
        highScores = classicHighScores;
    } else if (strcmp(mode, "TimeAttack") == 0) {
        highScores = timeAttackHighScores;
    } else {
        printf("Invalid mode: %s\n", mode);
        return false;
    }
    printf("Current highscores before insertion:\n");
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (highScores[i].name[0] != '\0') {
            printf("%d. %s: %d\n", i+1, highScores[i].name, highScores[i].score);
        } else {
            printf("%d. [Empty]\n", i+1);
        }
    }
    int insertPos = MAX_HIGHSCORES; 
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (highScores[i].name[0] == '\0' || score > highScores[i].score) {
            insertPos = i;
            break;
        }
    }
    if (insertPos >= MAX_HIGHSCORES) {
        printf("Score %d did not qualify for top %d\n", score, MAX_HIGHSCORES);
        return false;
    }
    printf("Score qualifies! Inserting at position %d\n", insertPos+1);
    for (int i = MAX_HIGHSCORES - 1; i > insertPos; i--) {
        strcpy(highScores[i].name, highScores[i-1].name);
        highScores[i].score = highScores[i-1].score;
    }
    strncpy(highScores[insertPos].name, name, MAX_NAME_LENGTH - 1);
    highScores[insertPos].name[MAX_NAME_LENGTH - 1] = '\0'; 
    highScores[insertPos].score = score;
    save_highscores();
    printf("Updated highscores after insertion:\n");
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (highScores[i].name[0] != '\0') {
            printf("%d. %s: %d\n", i+1, highScores[i].name, highScores[i].score);
        } else {
            printf("%d. [Empty]\n", i+1);
        }
    }
    
    return true;
}

// Function to fetch highscores into the display buffer
void fetch_highscores(const char *mode) {
    HighScoreEntry *highScores;
    highscoreCount = 0;
    printf("Fetching highscores for mode: %s\n", mode);
    if (strcmp(mode, "Classic") == 0) {
        highScores = classicHighScores;
    } else if (strcmp(mode, "TimeAttack") == 0) {
        highScores = timeAttackHighScores;
    } else {
        printf("Invalid mode in fetch_highscores: %s\n", mode);
        return; 
    }
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (highScores[i].name[0] != '\0') {
            strcpy(playerNames[highscoreCount], highScores[i].name);
            playerScores[highscoreCount] = highScores[i].score;
            printf("Loaded score for display: %s - %d\n", playerNames[highscoreCount], playerScores[highscoreCount]);
            highscoreCount++;
        }
    }
    printf("Total scores loaded for display: %d\n", highscoreCount);
}

// Display function remains the same
bool display_scores(const char *mode) {
    bool shouldReturnToMenu = false;
    while (!WindowShouldClose() && !shouldReturnToMenu) {
        BeginDrawing();
        ClearBackground((Color){ 240, 240, 245, 255 });
        DrawRectangle(0, 0, SCREEN_WIDTH, 80, (Color){ 70, 130, 180, 255 });
        char titleText[50];
        sprintf(titleText, "%s HIGH SCORES", mode);
        DrawText(titleText, SCREEN_WIDTH/2 - MeasureText(titleText, 40)/2, 30, 40, WHITE);
        for (int i = 0; i < highscoreCount; i++) {
            char scoreText[50];
            sprintf(scoreText, "%d. %s: %d", i+1, playerNames[i], playerScores[i]);
            DrawText(scoreText, SCREEN_WIDTH/2 - MeasureText(scoreText, 25)/2, 120 + i*40, 25, (Color){ 60, 60, 60, 255 });
        }
        DrawText("Press ENTER to continue", SCREEN_WIDTH/2 - MeasureText("Press ENTER to continue", 20)/2, SCREEN_HEIGHT - 50, 20, RED);
        EndDrawing();
        if (IsKeyPressed(KEY_ENTER)) {
            shouldReturnToMenu = true;
        }
    }
    return shouldReturnToMenu;
}

bool IsSpeedPowerUpActive(double currentTime, double speed_start, double slow_start) {
    bool speed_active = (currentTime - speed_start < 15.0) && speed_start > 0;
    bool slow_active = (currentTime - slow_start < 15.0) && slow_start > 0;
    return speed_active || slow_active;
}

bool ElementInSnake(Vector2i element, Snake* snake) {
    for (int i = 0; i < snake->length; i++) {
        if (snake->body[i].x == element.x && snake->body[i].y == element.y) {
            return true;
        }
    }
    return false;
}

bool EventTriggered(double interval) {
    double currentTime = GetTime();
    if (currentTime - lastUpdateTime >= interval) {
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

void InitSnake(Snake* snake) {
    snake->length = 3;
    snake->body[0] = (Vector2i){6, 9};
    snake->body[1] = (Vector2i){5, 9};
    snake->body[2] = (Vector2i){4, 9};
    snake->direction = (Vector2i){1, 0};
    snake->addSegment = false; 
}

void DrawSnake(Snake* snake,Color colour) {
    for (int i = 0; i < snake->length; i++) {
        Rectangle segment = {
            OFFSET + snake->body[i].x * CELL_SIZE,
            OFFSET + snake->body[i].y * CELL_SIZE,
            (float)CELL_SIZE, (float)CELL_SIZE
        };
        DrawRectangleRounded(segment, 0.5, 6, colour);
    }
}

// update the snake movement
void UpdateSnake(Snake* snake) {
    for (int i = snake->length; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }
    snake->body[0].x += snake->direction.x;
    snake->body[0].y += snake->direction.y;
    if (snake->addSegment) {
        snake->length++;
        snake->addSegment = false;
    }
}

// reset the snake and all power-ups
void ResetSnake(Snake* snake, PowerUp* speed_booster, PowerUp* double_booster, PowerUp* length_shortener, PowerUp* slow_downer) {
    InitSnake(snake);
    speed_booster_active = false;
    speed_boost_start = 0;
    speed_booster->exists = false;
    speed_booster_spawn_time = GetTime() + GetRandomValue(20, 30);
    double_booster_active = false;
    double_boost_start = 0;
    double_booster->exists = false;
    double_booster_spawn_time = GetTime() + GetRandomValue(20, 30);
    slow_downer_active = false;
    slow_downer_start = 0;
    slow_downer->exists = false;
    slow_downer_spawn_time = GetTime() + GetRandomValue(20, 30);
    length_shortener->exists = false;
    length_shortener_spawn_time = GetTime() + GetRandomValue(20, 30);
}

bool PositionOverlaps(Vector2i pos, Snake* snake, Food* food, PowerUp* speed_booster, PowerUp* double_booster, PowerUp* length_shortener, PowerUp* slow_downer) {
    if (food != NULL && food->position.x == pos.x && food->position.y == pos.y) {
        return true;
    }
    if (snake != NULL) {
        for(int i = 0; i < snake->length; i++){
            if(snake->body[i].x == pos.x && snake->body[i].y == pos.y) return true;
        }
    }
    if (speed_booster != NULL && speed_booster->exists && speed_booster->position.x == pos.x && speed_booster->position.y == pos.y) return true;
    if (double_booster != NULL && double_booster->exists && double_booster->position.x == pos.x && double_booster->position.y == pos.y) return true;
    if (length_shortener != NULL && length_shortener->exists && length_shortener->position.x == pos.x && length_shortener->position.y == pos.y) return true;
    if (slow_downer != NULL && slow_downer->exists && slow_downer->position.x == pos.x && slow_downer->position.y == pos.y) return true;
    return false;
}

Vector2i GenerateRandomPosition(Snake* snake, Food* food, PowerUp* speed_booster, PowerUp* double_booster, PowerUp* length_shortener, PowerUp* slow_downer) {
    Vector2i pos;
    do {
        pos.x = GetRandomValue(0, CELL_COUNT - 1);
        pos.y = GetRandomValue(0, CELL_COUNT - 1);
    } while ((snake != NULL && ElementInSnake(pos, snake)) || PositionOverlaps(pos, snake, food, speed_booster, double_booster, length_shortener, slow_downer));
    return pos;
}

void InitFood(Food* food, Snake* snake, PowerUp* speed_booster, PowerUp* double_booster, PowerUp* length_shortener, PowerUp* slow_downer) {
    Image image = LoadImageFromMemory(".png", food_png, food_png_len);
    food->texture = LoadTextureFromImage(image);
    UnloadImage(image);
    food->position = GenerateRandomPosition(snake, NULL, speed_booster, double_booster, length_shortener, slow_downer);
}

void DrawFood(Food* food) {
    DrawTexture(food->texture, OFFSET + food->position.x * CELL_SIZE, OFFSET + food->position.y * CELL_SIZE, WHITE);
}

void DrawPowerUp(PowerUp* item) {
    if (item->exists) {
        DrawTexture(item->texture, OFFSET + item->position.x * CELL_SIZE, OFFSET + item->position.y * CELL_SIZE, WHITE);
    }
}

void UnloadFood(Food* food) {
    UnloadTexture(food->texture);
}

void UnloadPowerUp(PowerUp* item) {
    UnloadTexture(item->texture);
}

void CheckCollisionWithFood(Snake* snake, Food* food, Sound eatSound, int* score, PowerUp* speed_booster, PowerUp* double_booster, PowerUp* length_shortener, PowerUp* slow_downer) {
    if (snake->body[0].x == food->position.x && snake->body[0].y == food->position.y) {
        food->position = GenerateRandomPosition(snake, NULL, speed_booster, double_booster, length_shortener, slow_downer);
        snake->addSegment = true;
        if (double_booster_active) {
            (*score) += 2;
        } else {
            (*score)++;
        }
        SetMasterVolume(1.0f);
        PlaySound(eatSound);
    }
}

void CheckCollisionWithSpeedBooster(Snake* snake, PowerUp* speed_booster, Sound speed_booster_sound) {
    if (speed_booster->exists && snake->body[0].x == speed_booster->position.x && snake->body[0].y == speed_booster->position.y) {
        SetMasterVolume(1.0f);
        PlaySound(speed_booster_sound);
        speed_booster_active = true;
        speed_booster->exists = false;
        speed_boost_start = GetTime();
        speed_booster_spawn_time = GetTime() + GetRandomValue(15, 25);
        if (slow_downer_active) {
            slow_downer_active = false;
            slow_downer_start = 0;
        }
    }
}

void CheckCollisionWithDoubleBooster(Snake* snake, PowerUp* double_booster, Sound double_booster_sound) {
    if (double_booster->exists && snake->body[0].x == double_booster->position.x && snake->body[0].y == double_booster->position.y) {
        SetMasterVolume(1.0f);
        PlaySound(double_booster_sound);
        double_booster_active = true;
        double_booster->exists = false;
        double_boost_start = GetTime();
        double_booster_spawn_time = GetTime() + GetRandomValue(15, 25);
    }
}

void CheckCollisionWithLengthShortener(Snake* snake, PowerUp* length_shortener, Sound length_shortener_sound) {
    if (length_shortener->exists && snake->body[0].x == length_shortener->position.x && snake->body[0].y == length_shortener->position.y) {
        if (snake->length > 4) {
            SetMasterVolume(1.0f);
            PlaySound(length_shortener_sound);
            snake->length = snake->length > 6 ? snake->length - 3 : 3; 
            length_shortener->exists = false;
            length_shortener_spawn_time = GetTime() + GetRandomValue(15, 25);
        } else {
            length_shortener->exists = false;
            length_shortener_spawn_time = GetTime() + GetRandomValue(15, 25);
        }
    }
}

void CheckCollisionWithSlowDowner(Snake* snake, PowerUp* slow_downer, Sound slow_downer_sound) {
    if (slow_downer->exists && snake->body[0].x == slow_downer->position.x && snake->body[0].y == slow_downer->position.y) {
        SetMasterVolume(1.0f);
        PlaySound(slow_downer_sound);
        slow_downer_active = true;
        slow_downer->exists = false;
        slow_downer_start = GetTime();
        slow_downer_spawn_time = GetTime() + GetRandomValue(15, 25);
        if (speed_booster_active) {
            speed_booster_active = false;
            speed_boost_start = 0;
        }
    }
}

bool CheckCollisionWithEdges(Snake* snake) {
    if (snake->body[0].x < 0 || snake->body[0].x >= CELL_COUNT || snake->body[0].y < 0 || snake->body[0].y >= CELL_COUNT) {
        return true;
    }
    return false;
}

bool CheckCollisionWithTail(Snake* snake) {
    for (int i = 1; i < snake->length; i++) {
        if (snake->body[0].x == snake->body[i].x && snake->body[0].y == snake->body[i].y) {
            return true;
        }
    }
    return false;
}

void DrawPowerUpStatus(bool speed_active, bool double_active, bool slow_active, double currentTime, double speed_start, double double_start, double slow_start) {
    int y_pos = OFFSET - 50;
    int bar_width = 150;
    int bar_height = 15;
    int label_offset = 20;
    if (speed_active) {
        double time_left = 15.0 - (currentTime - speed_start);
        if (time_left > 0) {
            DrawText("Speed Boost", OFFSET, y_pos - label_offset, 15, RED);
            DrawRectangle(OFFSET, y_pos, bar_width, bar_height, LIGHTGRAY);
            float fill_width = (float)(time_left / 15.0) * bar_width;
            DrawRectangle(OFFSET, y_pos, (int)fill_width, bar_height, RED);
            DrawRectangleLines(OFFSET, y_pos, bar_width, bar_height, DARKGRAY);
        }
    }
    if (double_active) {
        double time_left = 15.0 - (currentTime - double_start);
        if (time_left > 0) {
            DrawText("Double Points", OFFSET + bar_width + 30, y_pos - label_offset, 15, BLUE);
            DrawRectangle(OFFSET + bar_width + 30, y_pos, bar_width, bar_height, LIGHTGRAY);
            float fill_width = (float)(time_left / 15.0) * bar_width;
            DrawRectangle(OFFSET + bar_width + 30, y_pos, (int)fill_width, bar_height, BLUE);
            DrawRectangleLines(OFFSET + bar_width + 30, y_pos, bar_width, bar_height, DARKGRAY);
        }
    }
    if (slow_active) {
        double time_left = 15.0 - (currentTime - slow_start);
        if (time_left > 0) {
            DrawText("Slow Mode", OFFSET + (bar_width + 30) * 2, y_pos - label_offset, 15, PURPLE);
            DrawRectangle(OFFSET + (bar_width + 30) * 2, y_pos, bar_width, bar_height, LIGHTGRAY);
            float fill_width = (float)(time_left / 15.0) * bar_width;
            DrawRectangle(OFFSET + (bar_width + 30) * 2, y_pos, (int)fill_width, bar_height, PURPLE);
            DrawRectangleLines(OFFSET + (bar_width + 30) * 2, y_pos, bar_width, bar_height, DARKGRAY);
        }
    }
}

bool SnakesCollide(Snake *s1, Snake *s2) {
    for (int i = 0; i < s2->length; i++) {
        if (s1->body[0].x == s2->body[i].x &&
            s1->body[0].y == s2->body[i].y) {
            return true;
        }
    }
    for (int i = 0; i < s1->length; i++) {
        if (s2->body[0].x == s1->body[i].x &&
            s2->body[0].y == s1->body[i].y) {
            return true;
        }
    }
    return false;
}

Vector2i GenerateRandomPositionMulti(Snake* snake1, Snake* snake2, Food* f1,Food* f2) {
    Vector2i pos;
    do {
        pos.x = GetRandomValue(0, CELL_COUNT - 1);
        pos.y = GetRandomValue(0, CELL_COUNT - 1);
    } while ((snake1 != NULL && ElementInSnake(pos, snake1) && (snake2 != NULL && ElementInSnake(pos,snake2))) || (f1 != NULL && pos.x == f1->position.x && pos.y == f1->position.y) || (f2 != NULL && pos.x == f2->position.x && pos.y == f2->position.y));
    return pos;
}

void InitFoodMulti(Food* food, Snake* snake1, Snake* snake2, Food* f1, Food* f2) {
    Image image = LoadImageFromMemory(".png", food_png, food_png_len);
    food->texture = LoadTextureFromImage(image);
    UnloadImage(image);
    food->position = GenerateRandomPositionMulti(snake1, snake2, f1, f2);
}

void CheckCollisionWithFoodMulti(Snake* snake, Food* food, Sound eatSound, int* score, Food* f1, Food* f2, Snake* snakedum) {
    if (snake->body[0].x == food->position.x && snake->body[0].y == food->position.y) {
        food->position = GenerateRandomPositionMulti(snake, snakedum, f1, f2);
        snake->addSegment = true;
        (*score)++; 
        SetMasterVolume(1.0f);
        PlaySound(eatSound);
    }
}

void ClassicMode() 
{
    SetTargetFPS(60);
    InitAudioDevice();
    Snake snake;
    Food food;
    PowerUp speed_booster;
    PowerUp double_booster;
    PowerUp length_shortener;
    PowerUp slow_downer;
    Wave eatWave = LoadWaveFromMemory(".wav", eat_wav, eat_wav_len);
    Sound eatSound = LoadSoundFromWave(eatWave);
    Wave wallWave = LoadWaveFromMemory(".wav", wall_wav, wall_wav_len);
    Sound wallSound = LoadSoundFromWave(wallWave);
    Wave speedBoosterWave = LoadWaveFromMemory(".wav", high_speed_wav, high_speed_wav_len);
    Sound speed_booster_sound = LoadSoundFromWave(speedBoosterWave);
    Wave doubleBoosterWave = LoadWaveFromMemory(".wav", double_wav, double_wav_len);
    Sound double_booster_sound = LoadSoundFromWave(doubleBoosterWave);
    Wave slowDownerWave = LoadWaveFromMemory(".wav", slow_downer_wav, slow_downer_wav_len);
    Sound slow_downer_sound = LoadSoundFromWave(slowDownerWave);
    Wave lengthShortenerWave = LoadWaveFromMemory(".wav", length_shortener_wav, length_shortener_wav_len);
    Sound length_shortener_sound = LoadSoundFromWave(lengthShortenerWave);
    InitSnake(&snake);
    speed_booster.exists = false;
    double_booster.exists = false;
    length_shortener.exists = false;
    slow_downer.exists = false;
    Image speed_image = LoadImageFromMemory(".png", __2x_booster_png, __2x_booster_png_len);
    speed_booster.texture = LoadTextureFromImage(speed_image);
    UnloadImage(speed_image);
    Image double_image = LoadImageFromMemory(".png", point_doubler_png, point_doubler_png_len);
    double_booster.texture = LoadTextureFromImage(double_image);
    UnloadImage(double_image);
    Image length_image = LoadImageFromMemory(".png", length_shortener_png, length_shortener_png_len);
    length_shortener.texture = LoadTextureFromImage(length_image);
    UnloadImage(length_image);
    Image slow_image = LoadImageFromMemory(".png", slow_downer_png, slow_downer_png_len);
    slow_downer.texture = LoadTextureFromImage(slow_image);
    UnloadImage(slow_image);
    InitFood(&food, &snake, &speed_booster, &double_booster, &length_shortener, &slow_downer);
    speed_booster.position = GenerateRandomPosition(&snake, &food, NULL, &double_booster, &length_shortener, &slow_downer);
    double_booster.position = GenerateRandomPosition(&snake, &food, &speed_booster, NULL, &length_shortener, &slow_downer);
    length_shortener.position = GenerateRandomPosition(&snake, &food, &speed_booster, &double_booster, NULL, &slow_downer);
    slow_downer.position = GenerateRandomPosition(&snake, &food, &speed_booster, &double_booster, &length_shortener, NULL);
    int score = 0;
    bool running = true;
    bool gameover = false;
    double currentTime = GetTime();
    speed_booster_spawn_time = currentTime + GetRandomValue(15, 25);
    double_booster_spawn_time = currentTime + GetRandomValue(15, 25);
    length_shortener_spawn_time = currentTime + GetRandomValue(15, 25);
    slow_downer_spawn_time = currentTime + GetRandomValue(15, 25);
    while (!WindowShouldClose() && running) {
        BeginDrawing();
        ClearBackground(GREEN);
        currentTime = GetTime();
        double moveInterval = 0.2; 
        if (speed_booster_active && (currentTime - speed_boost_start < 15.0)) {
            moveInterval = 0.1; 
        }
        if (slow_downer_active && (currentTime - slow_downer_start < 15.0)) {
            moveInterval = 0.3; 
        }
        if (EventTriggered(moveInterval) && !gameover) {
            allowMove = true;
            if (running) {
                UpdateSnake(&snake);
                CheckCollisionWithFood(&snake, &food, eatSound, &score, &speed_booster, &double_booster, &length_shortener, &slow_downer);
                CheckCollisionWithSpeedBooster(&snake, &speed_booster, speed_booster_sound);
                CheckCollisionWithDoubleBooster(&snake, &double_booster, double_booster_sound);
                CheckCollisionWithLengthShortener(&snake, &length_shortener, length_shortener_sound);
                CheckCollisionWithSlowDowner(&snake, &slow_downer, slow_downer_sound);
                if (CheckCollisionWithEdges(&snake) || CheckCollisionWithTail(&snake)) {
                    SetMasterVolume(1.0f);
                    PlaySound(wallSound);
                    ResetSnake(&snake, &speed_booster, &double_booster, &length_shortener, &slow_downer);
                    food.position = GenerateRandomPosition(&snake, NULL, &speed_booster, &double_booster, &length_shortener, &slow_downer);
                    gameover = true;
                }
            }
        }
        if (!speed_booster.exists && !speed_booster_active && currentTime >= speed_booster_spawn_time) {
            if (!(slow_downer_active && (currentTime - slow_downer_start < 15.0))) {
                speed_booster.position = GenerateRandomPosition(&snake, &food, NULL, &double_booster, &length_shortener, &slow_downer);
                speed_booster.exists = true;
                speed_booster_despawn_time = currentTime + 10.0;  
            } else {
                speed_booster_spawn_time = currentTime + 15.0; 
            }
        }
        if (speed_booster.exists && ((currentTime >= speed_booster_despawn_time) || (slow_downer_active && (currentTime - slow_downer_start < 15.0)))) {
            speed_booster.exists = false;
            speed_booster_spawn_time = currentTime + GetRandomValue(15, 25);
        }
        if (speed_booster_active && (currentTime - speed_boost_start >= 15.0)) {
            speed_booster_active = false;  
        }
        if (!double_booster.exists && !double_booster_active && currentTime >= double_booster_spawn_time) {
            double_booster.position = GenerateRandomPosition(&snake, &food, &speed_booster, NULL, &length_shortener, &slow_downer);
            double_booster.exists = true;
            double_booster_despawn_time = currentTime + 10.0;  
        }
        if (double_booster.exists && currentTime >= double_booster_despawn_time) {
            double_booster.exists = false;
            double_booster_spawn_time = currentTime + GetRandomValue(15, 25);
        }
        if (double_booster_active && (currentTime - double_boost_start >= 15.0)) {
            double_booster_active = false; 
        }
        if (!length_shortener.exists && snake.length > 4 && currentTime >= length_shortener_spawn_time) {
            length_shortener.position = GenerateRandomPosition(&snake, &food, &speed_booster, &double_booster, NULL, &slow_downer);
            length_shortener.exists = true;
            length_shortener_despawn_time = currentTime + 10.0;  
        }
        if (length_shortener.exists && currentTime >= length_shortener_despawn_time) {
            length_shortener.exists = false;
            length_shortener_spawn_time = currentTime + GetRandomValue(15, 25);
        }
        if (!slow_downer.exists && !slow_downer_active && currentTime >= slow_downer_spawn_time) {
            if (!(speed_booster_active && (currentTime - speed_boost_start < 15.0))) {
                slow_downer.position = GenerateRandomPosition(&snake, &food, &speed_booster, &double_booster, &length_shortener, NULL);
                slow_downer.exists = true;
                slow_downer_despawn_time = currentTime + 10.0;  
            } else {
                slow_downer_spawn_time = currentTime + 15.0; 
            }
        }
        if (slow_downer.exists && ((currentTime >= slow_downer_despawn_time) || (speed_booster_active && (currentTime - speed_boost_start < 15.0)))) {
            slow_downer.exists = false;
            slow_downer_spawn_time = currentTime + GetRandomValue(15, 25);
        }
        if (slow_downer_active && (currentTime - slow_downer_start >= 15.0)) {
            slow_downer_active = false;  
        }
        if (IsKeyPressed(KEY_W) && snake.direction.y != 1 && allowMove) {
            snake.direction = (Vector2i){0, -1};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_S) && snake.direction.y != -1 && allowMove) {
            snake.direction = (Vector2i){0, 1};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_A) && snake.direction.x != 1 && allowMove) {
            snake.direction = (Vector2i){-1, 0};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_D) && snake.direction.x != -1 && allowMove) {
            snake.direction = (Vector2i){1, 0};
            running = true;
            allowMove = false;
        }
        DrawRectangleLinesEx((Rectangle){OFFSET - 5, OFFSET - 5, CELL_SIZE * CELL_COUNT + 10, CELL_SIZE * CELL_COUNT + 10}, 5, DARKGREEN);
        DrawText(TextFormat("Score: %i", score), OFFSET - 5, OFFSET + CELL_SIZE * CELL_COUNT + 10, 40, DARKGREEN);
        DrawPowerUpStatus(speed_booster_active, double_booster_active, slow_downer_active, currentTime, speed_boost_start, double_boost_start, slow_downer_start);
        DrawFood(&food);
        DrawSnake(&snake,DARKGREEN);
        if (speed_booster.exists) DrawPowerUp(&speed_booster);
        if (double_booster.exists) DrawPowerUp(&double_booster);
        if (length_shortener.exists) DrawPowerUp(&length_shortener);
        if (slow_downer.exists) DrawPowerUp(&slow_downer);
        // When game is over
        if(gameover){
            if (strlen(playerName) == 0) {
                strcpy(playerName, "Player"); 
            }
            printf("Game over! Player: %s, Score: %d, Mode: Classic\n", playerName, score);
            load_highscores();
            bool scoreInserted = insert_score(playerName, score, "Classic");
            printf("Score insertion result: %s\n", scoreInserted ? "SUCCESS" : "FAILED");
            fetch_highscores("Classic");
            if (display_scores("Classic")) {
                state = MENU;
                running = false;
            }
        }
        EndDrawing();
    }
    UnloadSound(eatSound);
    UnloadSound(wallSound);
    UnloadSound(speed_booster_sound);
    UnloadSound(double_booster_sound);
    UnloadSound(slow_downer_sound);
    UnloadSound(length_shortener_sound);
    UnloadFood(&food);
    UnloadPowerUp(&speed_booster);
    UnloadPowerUp(&double_booster);
    UnloadPowerUp(&length_shortener);
    UnloadPowerUp(&slow_downer);
    CloseAudioDevice();
}

void TimeStreakMode() 
{
    SetTargetFPS(60);
    InitAudioDevice();
    Snake snake;
    Food food;
    PowerUp speed_booster;
    PowerUp double_booster;
    PowerUp length_shortener;
    PowerUp slow_downer;
    Wave eatWave = LoadWaveFromMemory(".wav", eat_wav, eat_wav_len);
    Sound eatSound = LoadSoundFromWave(eatWave);
    Wave wallWave = LoadWaveFromMemory(".wav", wall_wav, wall_wav_len);
    Sound wallSound = LoadSoundFromWave(wallWave);
    Wave speedBoosterWave = LoadWaveFromMemory(".wav", high_speed_wav, high_speed_wav_len);
    Sound speed_booster_sound = LoadSoundFromWave(speedBoosterWave);
    Wave doubleBoosterWave = LoadWaveFromMemory(".wav", double_wav, double_wav_len);
    Sound double_booster_sound = LoadSoundFromWave(doubleBoosterWave);
    Wave slowDownerWave = LoadWaveFromMemory(".wav", slow_downer_wav, slow_downer_wav_len);
    Sound slow_downer_sound = LoadSoundFromWave(slowDownerWave);
    Wave lengthShortenerWave = LoadWaveFromMemory(".wav", length_shortener_wav, length_shortener_wav_len);
    Sound length_shortener_sound = LoadSoundFromWave(lengthShortenerWave);
    InitSnake(&snake);
    speed_booster.exists = false;
    double_booster.exists = false;
    length_shortener.exists = false;
    slow_downer.exists = false;
    float time_remaining = 60.0f; // time limit
    Image speed_image = LoadImageFromMemory(".png", __2x_booster_png, __2x_booster_png_len);
    speed_booster.texture = LoadTextureFromImage(speed_image);
    UnloadImage(speed_image);
    Image double_image = LoadImageFromMemory(".png", point_doubler_png, point_doubler_png_len);
    double_booster.texture = LoadTextureFromImage(double_image);
    UnloadImage(double_image);
    Image length_image = LoadImageFromMemory(".png", length_shortener_png, length_shortener_png_len);
    length_shortener.texture = LoadTextureFromImage(length_image);
    UnloadImage(length_image);
    Image slow_image = LoadImageFromMemory(".png", slow_downer_png, slow_downer_png_len);
    slow_downer.texture = LoadTextureFromImage(slow_image);
    UnloadImage(slow_image);
    InitFood(&food, &snake, &speed_booster, &double_booster, &length_shortener, &slow_downer);
    speed_booster.position = GenerateRandomPosition(&snake, &food, NULL, &double_booster, &length_shortener, &slow_downer);
    double_booster.position = GenerateRandomPosition(&snake, &food, &speed_booster, NULL, &length_shortener, &slow_downer);
    length_shortener.position = GenerateRandomPosition(&snake, &food, &speed_booster, &double_booster, NULL, &slow_downer);
    slow_downer.position = GenerateRandomPosition(&snake, &food, &speed_booster, &double_booster, &length_shortener, NULL);
    int score = 0;
    bool running = true;
    bool gameover = false;
    double currentTime = GetTime();
    speed_booster_spawn_time = currentTime + GetRandomValue(5, 15);
    double_booster_spawn_time = currentTime + GetRandomValue(5, 15);
    length_shortener_spawn_time = currentTime + GetRandomValue(5, 15);
    slow_downer_spawn_time = currentTime + GetRandomValue(5, 15);
    while (!WindowShouldClose() && running) {
        float delta_time = GetFrameTime();
        time_remaining -= delta_time;
        if (time_remaining <= 0) {
            running = false;
        }
        BeginDrawing();
        ClearBackground(GREEN);
        currentTime = GetTime();
        double moveInterval = 0.2;
        if (speed_booster_active && (currentTime - speed_boost_start < 15.0)) {
            moveInterval = 0.1; 
        }
        if (slow_downer_active && (currentTime - slow_downer_start < 15.0)) {
            moveInterval = 0.3; 
        }
        if (EventTriggered(moveInterval) && !gameover) {
            allowMove = true;
            if (running) {
                UpdateSnake(&snake);
                CheckCollisionWithFood(&snake, &food, eatSound, &score, &speed_booster, &double_booster, &length_shortener, &slow_downer);
                CheckCollisionWithSpeedBooster(&snake, &speed_booster, speed_booster_sound);
                CheckCollisionWithDoubleBooster(&snake, &double_booster, double_booster_sound);
                CheckCollisionWithLengthShortener(&snake, &length_shortener, length_shortener_sound);
                CheckCollisionWithSlowDowner(&snake, &slow_downer, slow_downer_sound);
                if (CheckCollisionWithEdges(&snake) || CheckCollisionWithTail(&snake)) {
                    SetMasterVolume(1.0f);
                    PlaySound(wallSound);
                    ResetSnake(&snake, &speed_booster, &double_booster, &length_shortener, &slow_downer);
                    food.position = GenerateRandomPosition(&snake, NULL, &speed_booster, &double_booster, &length_shortener, &slow_downer);
                    gameover = true;
                }
            }
        }
        if (!speed_booster.exists && !speed_booster_active && currentTime >= speed_booster_spawn_time) {
            if (!(slow_downer_active && (currentTime - slow_downer_start < 15.0))) {
                speed_booster.position = GenerateRandomPosition(&snake, &food, NULL, &double_booster, &length_shortener, &slow_downer);
                speed_booster.exists = true;
                speed_booster_despawn_time = currentTime + 10.0; 
            } 
            else speed_booster_spawn_time = currentTime + 15.0;
        }
        if (speed_booster.exists && ((currentTime >= speed_booster_despawn_time) || (slow_downer_active && (currentTime - slow_downer_start < 15.0)))) {
            speed_booster.exists = false;
            speed_booster_spawn_time = currentTime + GetRandomValue(5, 15);
        }
        if (speed_booster_active && (currentTime - speed_boost_start >= 15.0)) {
            speed_booster_active = false;
        }
        if (!double_booster.exists && !double_booster_active && currentTime >= double_booster_spawn_time) {
            double_booster.position = GenerateRandomPosition(&snake, &food, &speed_booster, NULL, &length_shortener, &slow_downer);
            double_booster.exists = true;
            double_booster_despawn_time = currentTime + 10.0; 
        }
        if (double_booster.exists && currentTime >= double_booster_despawn_time) {
            double_booster.exists = false;
            double_booster_spawn_time = currentTime + GetRandomValue(5, 15);
        }
        if (double_booster_active && (currentTime - double_boost_start >= 15.0)) {
            double_booster_active = false;  
        }
        if (!length_shortener.exists && snake.length > 4 && currentTime >= length_shortener_spawn_time) {
            length_shortener.position = GenerateRandomPosition(&snake, &food, &speed_booster, &double_booster, NULL, &slow_downer);
            length_shortener.exists = true;
            length_shortener_despawn_time = currentTime + 10.0; 
        }
        if (length_shortener.exists && currentTime >= length_shortener_despawn_time) {
            length_shortener.exists = false;
            length_shortener_spawn_time = currentTime + GetRandomValue(5, 15);
        }
        if (!slow_downer.exists && !slow_downer_active && currentTime >= slow_downer_spawn_time) {
            if (!(speed_booster_active && (currentTime - speed_boost_start < 15.0))) {
                slow_downer.position = GenerateRandomPosition(&snake, &food, &speed_booster, &double_booster, &length_shortener, NULL);
                slow_downer.exists = true;
                slow_downer_despawn_time = currentTime + 10.0;
            } 
            else slow_downer_spawn_time = currentTime + 15.0;
        }
        if (slow_downer.exists && ((currentTime >= slow_downer_despawn_time) || (speed_booster_active && (currentTime - speed_boost_start < 15.0)))) {
            slow_downer.exists = false;
            slow_downer_spawn_time = currentTime + GetRandomValue(5, 15);
        }
        if (slow_downer_active && (currentTime - slow_downer_start >= 15.0)) {
            slow_downer_active = false; 
        }
        if (IsKeyPressed(KEY_W) && snake.direction.y != 1 && allowMove) {
            snake.direction = (Vector2i){0, -1};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_S) && snake.direction.y != -1 && allowMove) {
            snake.direction = (Vector2i){0, 1};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_A) && snake.direction.x != 1 && allowMove) {
            snake.direction = (Vector2i){-1, 0};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_D) && snake.direction.x != -1 && allowMove) {
            snake.direction = (Vector2i){1, 0};
            running = true;
            allowMove = false;
        }
        DrawRectangleLinesEx((Rectangle){OFFSET - 5, OFFSET - 5, CELL_SIZE * CELL_COUNT + 10, CELL_SIZE * CELL_COUNT + 10}, 5, DARKGREEN);
        DrawText(TextFormat("Score: %i", score), OFFSET - 5, OFFSET + CELL_SIZE * CELL_COUNT + 10, 40, DARKGREEN);
        DrawText(TextFormat("Time: %.1f", time_remaining), OFFSET + CELL_SIZE * CELL_COUNT - 90,OFFSET - 40, 30, RED);
        DrawPowerUpStatus(speed_booster_active, double_booster_active, slow_downer_active, currentTime, speed_boost_start, double_boost_start, slow_downer_start);
        DrawFood(&food);
        DrawSnake(&snake,DARKGREEN);
        if (speed_booster.exists) DrawPowerUp(&speed_booster);
        if (double_booster.exists) DrawPowerUp(&double_booster);
        if (length_shortener.exists) DrawPowerUp(&length_shortener);
        if (slow_downer.exists) DrawPowerUp(&slow_downer);
        if (gameover) {
            if (strlen(playerName) == 0) {
                strcpy(playerName, "Player"); 
            }
            printf("Game over! Player: %s, Score: %d, Mode: Classic\n", playerName, score);
            load_highscores();
            bool scoreInserted = insert_score(playerName, score, "TimeAttack");
            printf("Score insertion result: %s\n", scoreInserted ? "SUCCESS" : "FAILED");
            fetch_highscores("TimeAttack");
            if (display_scores("TimeAttack")) {
                state = MENU;
                running = false;
            }
        }
        EndDrawing();
    }
    UnloadSound(eatSound);
    UnloadSound(wallSound);
    UnloadSound(speed_booster_sound);
    UnloadSound(double_booster_sound);
    UnloadSound(slow_downer_sound);
    UnloadSound(length_shortener_sound);
    UnloadFood(&food);
    UnloadPowerUp(&speed_booster);
    UnloadPowerUp(&double_booster);
    UnloadPowerUp(&length_shortener);
    UnloadPowerUp(&slow_downer);
    CloseAudioDevice();
}

void MultiplayerMode() 
{
    SetTargetFPS(60);
    InitAudioDevice();
    Snake snake1, snake2;
    Food foods[FRUIT_COUNT];
    Wave eatWave = LoadWaveFromMemory(".wav", eat_wav, eat_wav_len);
    Sound eatSound = LoadSoundFromWave(eatWave);
    Wave wallWave = LoadWaveFromMemory(".wav", wall_wav, wall_wav_len);
    Sound wallSound = LoadSoundFromWave(wallWave);
    InitSnake(&snake1);
    snake2.body[0] = (Vector2i){20, 15};
    snake2.body[1] = (Vector2i){19, 15};
    snake2.body[2] = (Vector2i){18, 15};
    snake2.length = 3;
    snake2.direction = (Vector2i){0,-1};
    snake2.addSegment = false;
    for(int i=0;i<FRUIT_COUNT;i++) {
        if(i==0) InitFoodMulti(&foods[i], &snake1, &snake2, NULL, NULL);
        else if(i==1) InitFoodMulti(&foods[i], &snake1, &snake2, &foods[0], NULL);
        else InitFoodMulti(&foods[i], &snake1, &snake2, &foods[0], &foods[1]);
    }
    int score1 = 0, score2 = 0;
    bool running = true;
    int winner = 0;
    bool gameover = false;
    bool allowMove = false;
    double currentTime = GetTime();
    while (!WindowShouldClose() && running) 
    {
        BeginDrawing();
        ClearBackground(GREEN);
        currentTime = GetTime();
        double moveInterval = 0.2;
        if (EventTriggered(moveInterval) && !gameover) {
            allowMove = true;
            if (running) 
            {
                if (SnakesCollide(&snake1, &snake2)) {
                    PlaySound(wallSound);
                    allowMove = false;
                    gameover = true;
                    if (score1 > score2) winner = 1;
                    else if (score2 > score1) winner = 2;
                    else winner = 0;
                }
                else if (CheckCollisionWithEdges(&snake1) || CheckCollisionWithTail(&snake1)) {
                    PlaySound(wallSound);
                    gameover = true;
                    allowMove = false;
                    winner = 2;
                } 
                else if (CheckCollisionWithEdges(&snake2) || CheckCollisionWithTail(&snake2)) {
                    PlaySound(wallSound);
                    gameover = true;
                    allowMove = false;
                    winner = 1;
                }
                if(!gameover) UpdateSnake(&snake1);
                if(!gameover) UpdateSnake(&snake2);
                for (int i=0;i<FRUIT_COUNT;i++){
                    if(i==0){
                        CheckCollisionWithFoodMulti(&snake1, &foods[i], eatSound, &score1, &foods[1], &foods[2], &snake2);
                        CheckCollisionWithFoodMulti(&snake2, &foods[i], eatSound, &score2, &foods[1], &foods[2], &snake1);
                    }
                    else if(i==1){
                        CheckCollisionWithFoodMulti(&snake1, &foods[i], eatSound, &score1, &foods[0], &foods[2], &snake2);
                        CheckCollisionWithFoodMulti(&snake2, &foods[i], eatSound, &score2, &foods[0], &foods[2], &snake1);
                    }
                    else {
                        CheckCollisionWithFoodMulti(&snake1, &foods[i], eatSound, &score1, &foods[1], &foods[0], &snake2);
                        CheckCollisionWithFoodMulti(&snake2, &foods[i], eatSound, &score2, &foods[1], &foods[0], &snake1);
                    }
                }
                
            }
        }
        if (IsKeyPressed(KEY_W) && snake1.direction.y != 1 && allowMove) {
            snake1.direction = (Vector2i){0, -1};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_S) && snake1.direction.y != -1 && allowMove) {
            snake1.direction = (Vector2i){0, 1};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_A) && snake1.direction.x != 1 && allowMove) {
            snake1.direction = (Vector2i){-1, 0};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_D) && snake1.direction.x != -1 && allowMove) {
            snake1.direction = (Vector2i){1, 0};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_UP) && snake2.direction.y != 1 && allowMove) {
            snake2.direction = (Vector2i){0, -1}; 
            running = true; 
            allowMove = false;
        }
        if (IsKeyPressed(KEY_DOWN) && snake2.direction.y != -1 && allowMove) {
            snake2.direction = (Vector2i){0, 1}; 
            running = true; 
            allowMove = false;
        }
        if (IsKeyPressed(KEY_LEFT) && snake2.direction.x != 1 && allowMove) {
            snake2.direction = (Vector2i){-1, 0}; 
            running = true; 
            allowMove = false;
        }
        if (IsKeyPressed(KEY_RIGHT) && snake2.direction.x != -1 && allowMove) {
            snake2.direction = (Vector2i){1, 0}; 
            running = true; 
            allowMove = false;
        }
        DrawRectangleLinesEx((Rectangle){OFFSET - 5, OFFSET - 5, CELL_SIZE * CELL_COUNT + 10, CELL_SIZE * CELL_COUNT + 10}, 5, DARKGREEN);
        DrawText(TextFormat("P1: %i", score1), OFFSET - 5, OFFSET - 30, 25, DARKGREEN);
        DrawText(TextFormat("P2: %i", score2), OFFSET + CELL_SIZE * CELL_COUNT - 150, OFFSET - 30, 25, ORANGE);
        for (int i=0;i<FRUIT_COUNT;i++){
            DrawFood(&foods[i]);
        }
        DrawSnake(&snake1,DARKGREEN);
        DrawSnake(&snake2,ORANGE);
        
        if (gameover) {
            if (winner == 1) {
                DrawText("Player 1 Wins!", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 30, DARKGREEN);
                DrawText("Press ENTER to continue", SCREEN_WIDTH/2-120, 320, 20, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {
                    state = MENU;
                    running = false;
                }
            } else if (winner == 2) {
                DrawText("Player 2 Wins!", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 30, ORANGE);
                DrawText("Press ENTER to continue", SCREEN_WIDTH/2-120, 320, 20, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {
                    state = MENU;
                    running = false;
                }
            } else {
                DrawText("It's a Tie!", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 30, YELLOW);
                DrawText("Press ENTER to continue", SCREEN_WIDTH/2-120, 320, 20, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {
                    state = MENU;
                    running = false;
                }
            }
        }
        EndDrawing();
    }
    UnloadSound(eatSound);
    UnloadSound(wallSound);
    for (int i=0;i<FRUIT_COUNT;i++){
        UnloadFood(&foods[i]);
    }
    CloseAudioDevice();
}

// simple AI algorithm to navigate towards food while avoiding walls and itself
void AIThink(Snake* ai_snake, Food* food, Snake* player_snake) 
{
    Vector2i head = ai_snake->body[0];
    Vector2i directions[4] = {
        {0, -1}, 
        {1, 0},   
        {0, 1},   
        {-1, 0}   
    };
    Vector2i opposite = {-ai_snake->direction.x, -ai_snake->direction.y};
    Vector2i possible_moves[3];
    int num_moves = 0;
    for (int i = 0; i < 4; i++) {
        if (directions[i].x == opposite.x && directions[i].y == opposite.y) continue;  
        Vector2i next = {head.x + directions[i].x, head.y + directions[i].y};
        if (next.x < 0) next.x = CELL_COUNT - 1;
        if (next.x >= CELL_COUNT) next.x = 0;
        if (next.y < 0) next.y = CELL_COUNT - 1;
        if (next.y >= CELL_COUNT) next.y = 0;
        bool self_collision = false;
        for (int j = 0; j < ai_snake->length; j++) {
            if (next.x == ai_snake->body[j].x && next.y == ai_snake->body[j].y) {
                self_collision = true;
                break;
            }
        }
        if (self_collision) continue;
        bool player_collision = false;
        for (int j = 0; j < player_snake->length; j++) {
            if (next.x == player_snake->body[j].x && next.y == player_snake->body[j].y) {
                player_collision = true;
                break;
            }
        }
        if (player_collision) continue;
        possible_moves[num_moves++] = directions[i];
    }
    if (num_moves == 0) {
        return;
    }
    int best_move = 0;
    int best_score = 9999;
    for (int i = 0; i < num_moves; i++) {
        Vector2i next = {head.x + possible_moves[i].x, head.y + possible_moves[i].y};
        if (next.x < 0) next.x = CELL_COUNT - 1;
        if (next.x >= CELL_COUNT) next.x = 0;
        if (next.y < 0) next.y = CELL_COUNT - 1;
        if (next.y >= CELL_COUNT) next.y = 0;
        int distance = abs(next.x - food->position.x) + abs(next.y - food->position.y);
        if (distance < best_score) {
            best_score = distance;
            best_move = i;
        }
    }
    ai_snake->direction = possible_moves[best_move];
}

// Update the AI snake movement with boundary check
void UpdateAISnake(Snake* ai_snake) 
{
    for (int i = ai_snake->length; i > 0; i--) {
        ai_snake->body[i] = ai_snake->body[i - 1];
    }
    Vector2i newHead = ai_snake->body[0];
    newHead.x += ai_snake->direction.x;
    newHead.y += ai_snake->direction.y;
    if (newHead.x < 0) newHead.x = CELL_COUNT - 1;
    if (newHead.x >= CELL_COUNT) newHead.x = 0;
    if (newHead.y < 0) newHead.y = CELL_COUNT - 1;
    if (newHead.y >= CELL_COUNT) newHead.y = 0;
    ai_snake->body[0] = newHead;
    if (ai_snake->addSegment) {
        ai_snake->length++;
        ai_snake->addSegment = false;
    }
}

// Update the snake movement with boundary check
void UpdateSnakeVs(Snake* snake) 
{
    for (int i = snake->length; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }
    Vector2i newHead = snake->body[0];
    newHead.x += snake->direction.x;
    newHead.y += snake->direction.y;
    if (newHead.x < 0) newHead.x = CELL_COUNT - 1;
    if (newHead.x >= CELL_COUNT) newHead.x = 0;
    if (newHead.y < 0) newHead.y = CELL_COUNT - 1;
    if (newHead.y >= CELL_COUNT) newHead.y = 0;
    snake->body[0] = newHead;
    if (snake->addSegment) {
        snake->length++;
        snake->addSegment = false;
    }
}

void DrawPVCGameOver(int winner, Snake* player_snake, Snake* ai_snake) {
    const char* message;
    Color color;
    
    if (winner == 0) {
        message = "DRAW!";
        color = YELLOW;
    } else if (winner == 2) {
        message = "COMPUTER WINS!";
        color = ORANGE;
    } else if (winner == 1) {
        message = "PLAYER WINS!";
        color = DARKGREEN;
    } else {
        return; 
    }
    
    int text_width = MeasureText(message, 60);
    DrawText(message, OFFSET + CELL_SIZE * CELL_COUNT / 2 - text_width / 2, OFFSET + CELL_SIZE * CELL_COUNT / 2 - 30, 60, color);
    DrawText("Press ENTER to continue", SCREEN_WIDTH/2-120, 320, 20, WHITE);

}

void VsComputerMode()
{
    SetTargetFPS(60);
    InitAudioDevice();

    Snake player_snake;
    Snake ai_snake;
    Food food;

    InitSnake(&player_snake);
    ai_snake.length = 3;
    ai_snake.body[0] = (Vector2i){CELL_COUNT - 7, CELL_COUNT - 10};
    ai_snake.body[1] = (Vector2i){CELL_COUNT - 6, CELL_COUNT - 10};
    ai_snake.body[2] = (Vector2i){CELL_COUNT - 5, CELL_COUNT - 10};
    ai_snake.direction = (Vector2i){-1, 0};
    ai_snake.addSegment = false;

    Wave eatWave = LoadWaveFromMemory(".wav", eat_wav, eat_wav_len);
    Sound eatSound = LoadSoundFromWave(eatWave);
    Wave wallWave = LoadWaveFromMemory(".wav", wall_wav, wall_wav_len);
    Sound wallSound = LoadSoundFromWave(wallWave);

    InitFoodMulti(&food, &player_snake, &ai_snake, NULL, NULL);

    bool running = true;
    int winner = 0;
    bool gameover = false;
    int player_score = 0;
    int ai_score = 0;

    double currentTime = GetTime();
    bool allowMove = false;

    while(!WindowShouldClose() && running)
    {
        ClearBackground(GREEN);
        BeginDrawing();
        currentTime = GetTime();
        double moveInterval = 0.2; // Default move interval

        if (EventTriggered(moveInterval) && !gameover) {
            allowMove = true;
            if (running && !gameover) {
                AIThink(&ai_snake, &food, &player_snake);
                UpdateSnakeVs(&player_snake);
                UpdateAISnake(&ai_snake);
                CheckCollisionWithFoodMulti(&player_snake, &food, eatSound, &player_score, NULL, NULL, &ai_snake);
                CheckCollisionWithFoodMulti(&ai_snake, &food, eatSound, &ai_score, NULL, NULL, &player_snake);
                if(CheckCollisionWithTail(&player_snake) && !gameover) {
                    gameover = true;
                    winner = 2;
                }
                if(CheckCollisionWithTail(&ai_snake) && !gameover) {
                    gameover = true;
                    winner = 1;
                }
                if (SnakesCollide(&player_snake, &ai_snake)) {
                    allowMove = false;
                    gameover = true;
                    SetMasterVolume(1.0f);
                    PlaySound(wallSound);
                    if (player_score > ai_score) winner = 1;
                    else if (ai_score > player_score) winner = 2;
                    else winner = 0;
                }
                
            }
        }
        if (IsKeyPressed(KEY_W) && player_snake.direction.y != 1 && allowMove) {
            player_snake.direction = (Vector2i){0, -1};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_S) && player_snake.direction.y != -1 && allowMove) {
            player_snake.direction = (Vector2i){0, 1};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_A) && player_snake.direction.x != 1 && allowMove) {
            player_snake.direction = (Vector2i){-1, 0};
            running = true;
            allowMove = false;
        }
        if (IsKeyPressed(KEY_D) && player_snake.direction.x != -1 && allowMove) {
            player_snake.direction = (Vector2i){1, 0};
            running = true;
            allowMove = false;
        }
        DrawRectangleLinesEx((Rectangle){OFFSET - 5, OFFSET - 5, CELL_SIZE * CELL_COUNT + 10, CELL_SIZE * CELL_COUNT + 10}, 5, DARKGREEN);
        DrawText(TextFormat("Player: %i", player_score), OFFSET - 5, OFFSET - 30, 25, DARKGREEN);
        DrawText(TextFormat("Computer: %i", ai_score), OFFSET + CELL_SIZE * CELL_COUNT - 150, OFFSET - 30, 25, ORANGE);
        DrawFood(&food);
        DrawSnake(&player_snake,DARKGREEN);
        DrawSnake(&ai_snake,ORANGE);
        if (gameover) {
            DrawPVCGameOver(winner, &player_snake, &ai_snake);
        }
        if(IsKeyPressed(KEY_ENTER)){
            state = MENU;
            running = false;
        }
        EndDrawing();
    }
    UnloadSound(eatSound);
    UnloadSound(wallSound);
    UnloadTexture(food.texture);
    CloseAudioDevice();
}

int main() {
    InitWindow(2 * OFFSET + CELL_SIZE * CELL_COUNT, 2 * OFFSET + CELL_SIZE * CELL_COUNT, "Retro Snake in C");
    SetTargetFPS(60);
    Rectangle startButton = {SCREEN_WIDTH/2-100, SCREEN_HEIGHT/2-30, 200, 60};
    Rectangle multiplayerButton = {SCREEN_WIDTH/2-100, SCREEN_HEIGHT/2+50, 200, 60};
    Rectangle timeAttackButton = {SCREEN_WIDTH/2-100, SCREEN_HEIGHT/2+130, 200, 60};
    Rectangle vscomputerButton = {SCREEN_WIDTH/2-100, SCREEN_HEIGHT/2+210, 200, 60};
    while (!WindowShouldClose()) {
        BeginDrawing();
        switch (state) {
            case ENTER_NAME:
                ClearBackground(LIGHTGRAY);
                DrawText("Enter Your Name:", SCREEN_WIDTH/2-120, 200, 25, BLACK);
                DrawRectangle(SCREEN_WIDTH/2-100, 250, 200, 40, WHITE);
                DrawText(playerName, SCREEN_WIDTH/2-90, 260, 20, BLACK);
                DrawText("Press ENTER to continue", SCREEN_WIDTH/2-120, 320, 20, BLACK);
                int key = GetCharPressed();
                if (key > 0 && namelength < MAX_LEN_STR) {
                    playerName[namelength++] = (char)key;
                    playerName[namelength] = '\0';
                }
                if (IsKeyPressed(KEY_BACKSPACE) && namelength > 0) {
                    playerName[--namelength] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER) && namelength > 0) {
                    state = MENU;
                }
                break;
            case MENU:
                ClearBackground(ORANGE);
                DrawText(TextFormat("Welcome, %s", playerName), SCREEN_WIDTH/2-100, 100, 25, BLACK);
                DrawRectangleRec(startButton, DARKGREEN);
                DrawRectangleLinesEx(startButton, 3, BLACK);
                DrawText("Classic Game", startButton.x+40, startButton.y+15, 20, WHITE);
                DrawRectangleRec(multiplayerButton, DARKBLUE);
                DrawRectangleLinesEx(multiplayerButton, 3, BLACK);
                DrawText("Multiplayer", multiplayerButton.x+40, multiplayerButton.y+15, 20, WHITE);
                DrawRectangleRec(timeAttackButton, DARKPURPLE);
                DrawRectangleLinesEx(timeAttackButton, 3, BLACK);
                DrawText("Time Attack", timeAttackButton.x+40, timeAttackButton.y+15, 20, WHITE);
                DrawRectangleRec(vscomputerButton, RED);
                DrawRectangleLinesEx(vscomputerButton, 3, BLACK);
                DrawText("VS Computer", vscomputerButton.x+40, vscomputerButton.y+15, 20, WHITE);
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePos = GetMousePosition();
                    if (CheckCollisionPointRec(mousePos, startButton)) {
                        state = GAME;
                    } else if (CheckCollisionPointRec(mousePos, multiplayerButton)) {
                        state = MULTIPLAYER_GAME;
                    } else if (CheckCollisionPointRec(mousePos, timeAttackButton)) {
                        state = TIME_ATTACK_GAME;
                    }
                    else if (CheckCollisionPointRec(mousePos,vscomputerButton )) {
                        state = VS_COMPUTER_GAME;
                    }
                }
                break;
                
            case GAME:
                ClassicMode(); // Start the game
                state = MENU; // After game ends, return to menu
                break;
                
            case MULTIPLAYER_GAME:
                MultiplayerMode(); 
                state = MENU;
                break;
                
            case TIME_ATTACK_GAME:
                TimeStreakMode();
                state = MENU;
                break;

            case VS_COMPUTER_GAME:
                VsComputerMode();
                state = MENU;
                break;
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
