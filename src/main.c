#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define WORDLE_PADDING 10
#define WORDLE_SPACING 5

#define WORDLE_LETTER_COUNT  5
#define WORDLE_TRY_COUNT  6

#define WORDLE_LETTER_ORIGINAL_SIZE 32
#define WORDLE_LETTER_TARGET_SIZE 64

#ifdef RISCV_BUILD
#define BMP_PATH "APPS:[WHYDLE2025]letters.bmp"
#define WORDS_PATH "APPS:[WHYDLE2025]wordle-answers-alphabetical.txt"
#else
#define BMP_PATH "./res/letters.bmp"
#define WORDS_PATH "./res/wordle-answers-alphabetical.txt"
#endif

typedef enum wordle_match_t
{
    wordle_match_unknown = 0,
    wordle_match_nowhere,
    wordle_match_wrong_spot,
    wordle_match_correct
} wordle_match_t;

typedef struct game_data_t
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* letters_texture;
    int window_w;
    int window_h;

    bool end_game;
    bool won_game;

    int try_index;
    int letter_index;

    int word_count;
    char* correct_word;
    char* all_words;

    char* full_letter_board;
    wordle_match_t* full_result_board;

    Uint64 flash_timer;
    Uint64 prev_ticks;
} game_data_t;

bool wordle_try_match(char* try_word, char* correct_word, wordle_match_t* results, int len)
{
    char* correct_cpy = malloc((size_t)len * sizeof(char));
    for (int i = 0; i < len; i++)
    {
        //reset all results and copy correct word
        results[i] = wordle_match_unknown;
        correct_cpy[i] = correct_word[i];
    }

    //check for letters in te correct spot first
    for (int i = 0; i < len; i++)
    {
        if (try_word[i] == correct_cpy[i])
        {
            results[i] = wordle_match_correct;
            correct_cpy[i] = '\0';
        }
    }

    //check for letters in the wrong spot
    for (int i = 0; i < len; i++)
    {
        if (results[i] == wordle_match_correct)
        {
            continue; //skip if correct
        }
        results[i] = wordle_match_nowhere; //default to nowhere

        const char letter = try_word[i];
        for (int j = 0; j < len; j++)
        {
            if (correct_cpy[j] != '\0' && correct_cpy[j] == letter)
            {
                results[i] = wordle_match_wrong_spot;
                correct_cpy[j] = '\0';
                break;
            }
        }
    }

    free(correct_cpy);

    for (int i = 0; i < len; i++)
    {
        if (results[i] != wordle_match_correct)
        {
            return false;
        }
    }
    return true;
}

void wordle_render_bg(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    for (int t = 0; t < WORDLE_TRY_COUNT; t++)
    {
        for (int l = 0; l < WORDLE_LETTER_COUNT; l++)
        {
            SDL_FRect r = {
                .x = (float)(WORDLE_PADDING + l * (WORDLE_LETTER_TARGET_SIZE + WORDLE_SPACING)),
                .y = (float)(WORDLE_PADDING + t * (WORDLE_LETTER_TARGET_SIZE + WORDLE_SPACING)),
                .w = WORDLE_LETTER_TARGET_SIZE,
                .h = WORDLE_LETTER_TARGET_SIZE
            };

            SDL_RenderFillRect(renderer, &r);
        }
    }
}

void wordle_draw_word(SDL_Renderer* renderer, SDL_Texture* texture, const char* word, int len, int x, int y, int size)
{
    int start_x = x - (size * len) / 2;
    int start_y = y - size / 2;

    SDL_SetRenderDrawColor(renderer, 255, 150, 150, 255);
    SDL_FRect bg_rect = {
        .x = (float)start_x,
        .y = (float)start_y,
        .w = (float)(size * len),
        .h = (float)size
    };
    SDL_RenderFillRect(renderer, &bg_rect);
    for (int i = 0; i < len; i++)
    {
        char c = word[i];
        if (c < 97 || c > 122)
        {
            continue;
        }

        int tex_index = (int)c - 97;
        SDL_FRect src_rect = {
            .x = (float)tex_index * WORDLE_LETTER_ORIGINAL_SIZE,
            .y = 0,
            .w = WORDLE_LETTER_ORIGINAL_SIZE,
            .h = WORDLE_LETTER_ORIGINAL_SIZE
        };

        SDL_FRect dest_rect = {
            .x = (float)(start_x + size * i),
            .y = (float)start_y,
            .w = (float)size,
            .h = (float)size
        };

        SDL_RenderTexture(renderer, texture, &src_rect, &dest_rect);
    }
}

void wordle_render_board(SDL_Renderer* renderer, SDL_Texture* letters_texture, char* board_letters,
                         wordle_match_t* board_results, int try_index, int letter_index)
{
    for (int i = 0; i < try_index; i++)
    {
        //loop through previous tries
        for (int j = 0; j < WORDLE_LETTER_COUNT; j++)
        {
            int board_index = i * WORDLE_LETTER_COUNT + j;
            wordle_match_t entry_result = board_results[board_index];
            char entry_letter = board_letters[board_index];
            if (entry_result == wordle_match_unknown)
            {
                continue;
            }

            char letter = entry_letter;
            if (letter < 97 || letter > 122)
            {
                continue; //out of render range
            }

            switch (entry_result)
            {
            case wordle_match_correct:
                SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
                break;
            case wordle_match_wrong_spot:
                SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
                break;
            case wordle_match_nowhere:
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                break;
            default:
                break;
            }
            SDL_FRect dest_rect = {
                .x = (float)(WORDLE_PADDING + j * (WORDLE_LETTER_TARGET_SIZE + WORDLE_SPACING)),
                .y = (float)(WORDLE_PADDING + i * (WORDLE_LETTER_TARGET_SIZE + WORDLE_SPACING)),
                .w = WORDLE_LETTER_TARGET_SIZE,
                .h = WORDLE_LETTER_TARGET_SIZE
            };
            SDL_RenderFillRect(renderer, &dest_rect);

            int tex_index = (int)letter - 97;
            SDL_FRect src_rect = {
                .x = (float)(WORDLE_LETTER_ORIGINAL_SIZE * tex_index),
                .y = 0,
                .w = WORDLE_LETTER_ORIGINAL_SIZE,
                .h = WORDLE_LETTER_ORIGINAL_SIZE
            };
            SDL_RenderTexture(renderer, letters_texture, &src_rect, &dest_rect);
        }
    }

    //draw current typed word
    for (int i = 0; i < letter_index; i++)
    {
        int board_index = try_index * WORDLE_LETTER_COUNT + i;
        char entry_letter = board_letters[board_index];

        char letter = entry_letter;
        if (letter < 97 || letter > 122)
        {
            continue; //out of render range
        }

        SDL_FRect dest_rect = {
            .x = (float)(WORDLE_PADDING + i * (WORDLE_LETTER_TARGET_SIZE + WORDLE_SPACING)),
            .y = (float)(WORDLE_PADDING + try_index * (WORDLE_LETTER_TARGET_SIZE + WORDLE_SPACING)),
            .w = WORDLE_LETTER_TARGET_SIZE,
            .h = WORDLE_LETTER_TARGET_SIZE
        };

        int tex_index = (int)letter - 97;
        SDL_FRect src_rect = {
            .x = (float)(WORDLE_LETTER_ORIGINAL_SIZE * tex_index),
            .y = 0,
            .w = WORDLE_LETTER_ORIGINAL_SIZE,
            .h = WORDLE_LETTER_ORIGINAL_SIZE
        };
        SDL_RenderTexture(renderer, letters_texture, &src_rect, &dest_rect);
    }
}

char* wordle_read_words(const char* file_path, int* word_count)
{
    FILE* f = fopen(file_path, "r");
    if (!f)
    {
        printf("Could not open word list file: %s\n", file_path);
        return nullptr;
    }

    int line_count = 1;
    {
        int counted_valid = 0;
        int c;
        while ((c = fgetc(f)) != EOF)
        {
            if (c == '\n')
            {
                line_count++;
                counted_valid = 0;
            }
            else
            {
                counted_valid++;
            }
        }

        //last line bad formatting check
        if (counted_valid < WORDLE_LETTER_COUNT)
        {
            line_count--;
        }
    }
    rewind(f);

    *word_count = line_count;
    char* out_arr = malloc(sizeof(char) * (size_t)(line_count * WORDLE_LETTER_COUNT));
    {
        int word_index = 0;
        int letter_index = 0;
        int c;
        while ((c = fgetc(f)) != EOF && word_index < line_count)
        {
            char* word_ptr = out_arr + WORDLE_LETTER_COUNT * word_index;
            if (c == '\n')
            {
                word_index++;
                letter_index = 0;
            }
            else if (letter_index < WORDLE_LETTER_COUNT)
            {
                *(word_ptr + letter_index++) = c;
            }
        }
    }
    fclose(f);

    return out_arr;
}

bool wordle_validate_word(const char* word_arr, int word_count, const char* word)
{
    for (int i = 0; i < word_count; i++)
    {
        const char* word_ptr = word_arr + i * WORDLE_LETTER_COUNT;
        if (memcmp(word, word_ptr, (size_t)WORDLE_LETTER_COUNT) == 0)
        {
            return true;
        }
    }
    return false;
}

void wordle_select_word(char* word_arr, int word_count, char* buffer)
{
    srand((unsigned int)time(nullptr));

    int selected_word = rand() % word_count;
    char* selected_ptr = word_arr + (selected_word * WORDLE_LETTER_COUNT);
    for (int i = 0; i < WORDLE_LETTER_COUNT; i++)
    {
        buffer[i] = selected_ptr[i];
    }
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* e)
{
    if (appstate == NULL)
    {
        return SDL_APP_CONTINUE;
    }
    game_data_t* game_data = appstate;
    if (e->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    if (e->type == SDL_EVENT_KEY_DOWN)
    {
        const SDL_KeyboardEvent key = e->key;
        if (key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
        if (key.key == SDLK_BACKSPACE)
        {
            if (!game_data->end_game && game_data->letter_index > 0)
            {
                game_data->letter_index--;
            }
        }
        else if (key.key == SDLK_KP_ENTER || key.key == SDLK_RETURN || key.key == SDLK_RETURN2)
        {
            if (game_data->end_game)
            {
                //restart the game with a new word
                wordle_select_word(game_data->all_words, game_data->word_count, game_data->correct_word);
                game_data->end_game = game_data->won_game = false;
                game_data->letter_index = game_data->try_index = 0;
            }
            else if (game_data->letter_index == WORDLE_LETTER_COUNT)
            {
                int board_offset = game_data->try_index * WORDLE_LETTER_COUNT;
                bool correct = wordle_try_match(
                    game_data->full_letter_board + board_offset, game_data->correct_word, //words
                    game_data->full_result_board + board_offset, //results
                    WORDLE_LETTER_COUNT
                );
                if (wordle_validate_word(game_data->all_words, game_data->word_count,
                                         game_data->full_letter_board + board_offset))
                {
                    game_data->try_index++;
                    if (correct)
                    {
                        game_data->end_game = game_data->won_game = true;
                    }
                    else
                    {
                        if (game_data->try_index >= WORDLE_TRY_COUNT)
                        {
                            game_data->end_game = true; //lost
                        }
                    }
                    game_data->letter_index = 0;
                }
                else
                {
                    game_data->flash_timer = 1000; // 1000 milliseconds for 1 second flash
                }
            }
        }
        else if (key.scancode >= 4 && key.scancode <= 29)
        {
            if (!game_data->end_game && game_data->letter_index < WORDLE_LETTER_COUNT)
            {
                //submit letter
                int board_index = game_data->try_index * WORDLE_LETTER_COUNT + game_data->letter_index;
                game_data->full_letter_board[board_index] = (char)((int)key.scancode + 93);
                game_data->full_result_board[board_index] = wordle_match_unknown;
                game_data->letter_index++;
            }
        }
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    game_data_t* game_data = appstate;
    //TIMER UPDATE
    Uint64 new_ticks = SDL_GetTicks();
    Uint64 delta_ticks = new_ticks - game_data->prev_ticks;
    game_data->prev_ticks = new_ticks;
    Uint64 delta_time = delta_ticks * 3;

    game_data->flash_timer = game_data->flash_timer >= delta_time ? game_data->flash_timer - delta_time : 0;

    //RENDER
    if (game_data->end_game)
    {
        if (game_data->won_game)
        {
            SDL_SetRenderDrawColor(game_data->renderer, 150, 255, 150, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(game_data->renderer, 255, 150, 150, 255);
        }
    }
    else
    {
        Uint8 val = 255u - (Uint8)(255 * game_data->flash_timer / 1000);
        SDL_SetRenderDrawColor(game_data->renderer, 255, val, val, 255);
    }
    SDL_RenderClear(game_data->renderer);

    wordle_render_bg(game_data->renderer);
    wordle_render_board(
        game_data->renderer, game_data->letters_texture,
        game_data->full_letter_board, game_data->full_result_board,
        game_data->try_index, game_data->letter_index
    );
    if (game_data->end_game && !game_data->won_game)
    {
        //draw correct word at the center of the screen
        wordle_draw_word(
            game_data->renderer, game_data->letters_texture,
            game_data->correct_word, WORDLE_LETTER_COUNT,
            game_data->window_w / 2, game_data->window_h / 2,
            WORDLE_LETTER_ORIGINAL_SIZE
        );
    }

    SDL_RenderPresent(game_data->renderer);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    if (!SDL_SetAppMetadata("Whydle, a wordle clone", "1.0", "com.github.wordle"))
    {
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    game_data_t* game_data = SDL_calloc(1, sizeof(game_data_t));
    if (!game_data)
    {
        return SDL_APP_FAILURE;
    }

    *appstate = game_data;

    game_data->window_w = WORDLE_LETTER_COUNT * WORDLE_LETTER_TARGET_SIZE + WORDLE_PADDING * 2 + (WORDLE_LETTER_COUNT -
        1) * WORDLE_SPACING;
    game_data->window_h = WORDLE_PADDING * 2 + WORDLE_TRY_COUNT * WORDLE_LETTER_TARGET_SIZE + (WORDLE_TRY_COUNT - 1) *
        WORDLE_SPACING;
    game_data->window = SDL_CreateWindow(
        "Wordle",
        game_data->window_w, //w
        game_data->window_h, //h
        0
    );

    game_data->renderer = SDL_CreateRenderer(game_data->window, nullptr);

    {
        SDL_Surface* letters_surf = SDL_LoadBMP(BMP_PATH);
        if (letters_surf == NULL)
        {
            printf("Could not load bmp: %s\n", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        game_data->letters_texture = SDL_CreateTextureFromSurface(game_data->renderer, letters_surf);
        SDL_DestroySurface(letters_surf);
        if (game_data->letters_texture == NULL)
        {
            printf("Could not create texture: %s\n", SDL_GetError());
            return SDL_APP_FAILURE;
        }
    }

    game_data->full_letter_board = malloc((size_t)(WORDLE_TRY_COUNT * WORDLE_LETTER_COUNT) * sizeof(char));
    game_data->full_result_board = malloc((size_t)(WORDLE_TRY_COUNT * WORDLE_LETTER_COUNT) * sizeof(wordle_match_t));
    for (int i = 0; i < WORDLE_TRY_COUNT * WORDLE_LETTER_COUNT; i++)
    {
        game_data->full_result_board[i] = wordle_match_unknown;
    }

    game_data->correct_word = malloc(sizeof(char) * (size_t)WORDLE_LETTER_COUNT);
    game_data->all_words = wordle_read_words(WORDS_PATH, &game_data->word_count);
    wordle_select_word(game_data->all_words, game_data->word_count, game_data->correct_word);

    game_data->letter_index = 0;
    game_data->try_index = 0;

    game_data->end_game = false;
    game_data->won_game = false;

    game_data->flash_timer = 0; //flash screen when trying to submit an invalid word

    game_data->prev_ticks = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    if (appstate != NULL)
    {
        (void)result;
        game_data_t* game_data = appstate;
        free(game_data->correct_word);
        free(game_data->all_words);
        free(game_data->full_letter_board);
        free(game_data->full_result_board);
        SDL_DestroyTexture(game_data->letters_texture);
        SDL_DestroyRenderer(game_data->renderer);
        SDL_DestroyWindow(game_data->window);
        SDL_free(game_data);
    }
}
