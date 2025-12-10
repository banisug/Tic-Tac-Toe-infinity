//#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <glew.h>
#include <glfw3.h>
#include <string.h>
#include <ctype.h>

#define C 1.41 // константа для монте карло
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define CELL_SIZE 100
#define VISIBLE_CELLS_X 8
#define VISIBLE_CELLS_Y 6
#define MAX_SIZE 100
#define MIN_SIZE 3
#define MAX_WIN_LINE 100

// доска
typedef struct Node {
    long long x, y;
    char value;
    struct Node* next;
} Node;

// хеш-таблица
typedef struct {
    Node** buckets;
    unsigned long long capacity;
} Table;

// параметры игры
typedef struct {
    unsigned long long size;
    unsigned long long len;
    unsigned long long count_moves;
    long long last_ai_x, last_ai_y;
    long long last_pl_x, last_pl_y;
    short depth;
    char player;
    char ai;
    short difficulty; // 1: easy, 2: middle, 3: hard, 4: impossible
    bool player_moves_first;
    int infinite_field;
} base;

// ограничительная рамка для активной игровой зоны
typedef struct {
    long long minx, maxx, miny, maxy;
    bool initialized;
} bounds;

// состояния пользовательского интерфейса
typedef enum {
    MENU_SCREEN,
    GAME_SCREEN,
    SETTINGS_SCREEN,
    GAME_OVER,
    HELP_SCREEN,
    ABOUT_SCREEN
} ScreenState;

// структура кнопок
typedef struct {
    float x, y;
    float width;
    float height;
    const char* text;
    bool is_mouse;
} Button;

// игровой контекст
typedef struct {
    ScreenState current_screen;
    Table* board;
    base parameters;
    bounds bbox;
    float char_width;
    float char_height;
    float char_spacing;
    int view_offset_x;
    int view_offset_y;
    long long cursor_x, cursor_y;
    bool is_player_turn;
    Button help_button;
    Button start_button;
    Button settings_button;
    Button back_button;
    Button size_up_button;
    Button size_down_button;
    Button line_up_button;
    Button line_down_button;
    Button first_player_button;
    Button difficulty_button;
    Button infinite_field_button;
    Button about_button;
    int winner; // 0: нет, 1: игрок, 2: ИИ, 3: ничья
} GameContext;

// типы алгоритмов
typedef enum {
    MINIMAX,
    MCTS
} algorithms;

// хеш функция для таблицы
unsigned long long hash_mix64(long long x, long long y, unsigned long long capacity) {
    unsigned long long z = (unsigned long long)x;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z = z ^ ((unsigned long long)y ^ ((unsigned long long)y >> 30));
    z += 0x9e3779b97f4a7c15ULL;
    return z % capacity;
}

// инициализация доски
Table* create_table(unsigned long long cap) {
    Table* t = (Table*)malloc(sizeof(Table));
    t->buckets = (Node**)calloc(cap, sizeof(Node*));
    t->capacity = cap;
    return t;
}

// сброс сохраненной игры
bool reset_saved_game() {
    FILE* file = fopen("save.dat", "rb");
    if (file) {
        fclose(file);
        // Если файл существует, удаляем его
        if (remove("save.dat") == 0) {
            return true;
        }
    }
    return false; // Файла не существует или не удалось удалить
}

// добавление крестика или нолика на доску (в хеш таблицу)
void insert(Table* board, long long x, long long y, char value) {
    if (x >= MAX_SIZE || y >= MAX_SIZE) return; // защита от переполнения
    unsigned long long index = hash_mix64(x, y, board->capacity);
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) return;

    newNode->x = x;
    newNode->y = y;
    newNode->value = value;
    newNode->next = board->buckets[index];
    board->buckets[index] = newNode;
}

// удаление крестика или нолика с доски (из хеш таблицы)
void remove_cell(Table* board, long long x, long long y) {
    unsigned long long index = hash_mix64(x, y, board->capacity);
    Node* current = board->buckets[index];
    Node* prev = NULL;
    while (current != NULL) {
        if (current->x == x && current->y == y) {
            if (prev == NULL) board->buckets[index] = current->next;
            else prev->next = current->next;
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

char get_value(Table* board, long long x, long long y, unsigned long long size, GameContext* ctx) {
    if ((x >= size || y >= size) && ctx->parameters.infinite_field == 0) return '\0';
    unsigned long long index = hash_mix64(x, y, board->capacity);
    Node* current = board->buckets[index];
    while (current != NULL) {
        if (current->x == x && current->y == y) return current->value;
        current = current->next;
    }
    return '.';
}

bool check_win(Table* board, unsigned long long size, unsigned long long len,
    long long x, long long y, char s, GameContext* ctx) {
    short D[4][2] = { {1,0},{0,1},{1,1},{1,-1} };

    for (int d = 0; d < 4; ++d) {
        int dx = D[d][0], dy = D[d][1];
        unsigned long long count = 1;

        // Проверяем в одном направлении
        for (int k = 1; k < len; ++k) {
            char val = get_value(board, x + dx * k, y + dy * k, size, ctx);
            if (val != s) break;
            count++;
        }

        // Проверяем в противоположном направлении
        for (int k = 1; k < len; ++k) {
            char val = get_value(board, x - dx * k, y - dy * k, size, ctx);
            if (val != s) break;
            count++;
        }

        if (count >= len) return true;
    }

    return false;
}

// оценка потенциала линии
int line_score(Table* board, unsigned long long size, unsigned long long len,
    long long x, long long y, char s, GameContext* ctx) {

    if (get_value(board, x, y, size, ctx) != '.') return INT_MIN / 4;

    int sum = 0;
    short D[4][2] = { {1,0},{0,1},{1,1},{1,-1} };

    for (int d = 0; d < 4; ++d) {
        int dx = D[d][0], dy = D[d][1];
        unsigned long long count = 1, open = 0;

        // проверяем в одном направлении (ограничиваем длину проверки)
        for (int k = 1; k <= len; ++k) {
            char val = get_value(board, x + dx * k, y + dy * k, size, ctx);
            if (val == s) {
                count++;
                continue;
            }
            if (val == '.') {
                open++;
                break;
            }
            break;
        }

        // проверяем в противоположном направлении
        for (int k = 1; k <= len; ++k) {
            char val = get_value(board, x - dx * k, y - dy * k, size, ctx);
            if (val == s) {
                count++;
                continue;
            }
            if (val == '.') {
                open++;
                break;
            }
            break;
        }

        if (count >= len) return 1000000;

        sum += count * count * 3 + open;
    }

    return sum;
}


// оценивает текущее состояние игры с точки зрения ИИ (у кого преимущество)
int eval_heuristic(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    int best_ai = 0, best_pl = 0;
    short R = 2;
    if (!bbox->initialized) return 0;
    long long x0 = bbox->minx - R;
    long long x1 = bbox->maxx + R;
    long long y0 = bbox->miny - R;
    long long y1 = bbox->maxy + R;
    for (long long y = y0; y <= y1; ++y) {
        for (long long x = x0; x <= x1; ++x) {
            if (get_value(board, x, y, parameters->size, ctx) != '.') continue;
            int score_ai = line_score(board, parameters->size, parameters->len, x, y, parameters->ai, ctx);
            int score_pl = line_score(board, parameters->size, parameters->len, x, y, parameters->player, ctx);
            if (score_ai > best_ai) best_ai = score_ai;
            if (score_pl > best_pl) best_pl = score_pl;
        }
    }
    return best_ai - best_pl;
}

/*обновляет ограничивающую рамку игрового поля при добавлении новой фигуры
она отслеживает текущие границы занятых клеток для оптимизации работы алгоритмов*/
void bbox_on_place(bounds* bbox, long long x, long long y) {
    if (!bbox->initialized) {
        bbox->minx = bbox->maxx = x;
        bbox->miny = bbox->maxy = y;
        bbox->initialized = true;
    }
    else {
        if (x < bbox->minx) bbox->minx = x;
        if (x > bbox->maxx) bbox->maxx = x;
        if (y < bbox->miny) bbox->miny = y;
        if (y > bbox->maxy) bbox->maxy = y;
    }
}

// обновляет ограничивающую рамку игрового поля при удалении фигуры
void bbox_on_remove(bounds* bbox, Table* board, unsigned long long size) {
    if (!bbox->initialized) return;
    long long nx = LLONG_MAX, ny = LLONG_MAX, xx = LLONG_MIN, yy = LLONG_MIN;
    bool found = false;
    for (unsigned long long i = 0; i < board->capacity; ++i) {
        Node* current = board->buckets[i];
        while (current) {
            found = true;
            if (current->x < nx) nx = current->x;
            if (current->x > xx) xx = current->x;
            if (current->y < ny) ny = current->y;
            if (current->y > yy) yy = current->y;
            current = current->next;
        }
    }
    if (!found) {
        bbox->initialized = false;
        return;
    }
    bbox->minx = nx;
    bbox->maxx = xx;
    bbox->miny = ny;
    bbox->maxy = yy;
    bbox->initialized = true;
}

// лучшие ходы
typedef struct {
    long long x[64];
    long long y[128];
    int score[128];
    int n;
} best_move;

// добавление хода в список лучших ходов
void best_move_push(best_move* moves, long long x, long long y, int sc, short K) {
    if (moves->n < K) {
        int i = moves->n++;
        moves->x[i] = x;
        moves->y[i] = y;
        moves->score[i] = sc;
        while (i > 0 && moves->score[i] > moves->score[i - 1]) {
            int temp_sc = moves->score[i - 1];
            moves->score[i - 1] = moves->score[i];
            moves->score[i] = temp_sc;
            long long temp_x = moves->x[i - 1], temp_y = moves->y[i - 1];
            moves->x[i - 1] = moves->x[i];
            moves->y[i - 1] = moves->y[i];
            moves->x[i] = temp_x;
            moves->y[i] = temp_y;
            i--;
        }
        return;
    }
    if (sc <= moves->score[moves->n - 1]) return;
    int i = moves->n - 1;
    while (i > 0 && sc > moves->score[i - 1]) {
        moves->score[i] = moves->score[i - 1];
        moves->x[i] = moves->x[i - 1];
        moves->y[i] = moves->y[i - 1];
        i--;
    }
    moves->score[i] = sc;
    moves->x[i] = x;
    moves->y[i] = y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// генерация лучших клеток для хода
void generate_candidates(Table* board, base* parameters, bounds* bbox, bool forAI, short K, best_move* out, GameContext* ctx) {
    out->n = 0;

    if (!bbox->initialized) {
        long long c = 0;
        if (get_value(board, c, c, parameters->size, ctx) == '.') {
            best_move_push(out, c, c, 0, K);
        }
        return;
    }

    // ДЛЯ БЕСКОНЕЧНОГО ПОЛЯ - ОГРАНИЧИТЬ ПОИСК!
    long long search_range = 8; // Вместо 20!

    long long x0 = bbox->minx - search_range;
    long long x1 = bbox->maxx + search_range;
    long long y0 = bbox->miny - search_range;
    long long y1 = bbox->maxy + search_range;

    // Дополнительное ограничение
    if (x1 - x0 > 16) {
        long long center_x = (bbox->minx + bbox->maxx) / 2;
        x0 = center_x - 8;
        x1 = center_x + 8;
    }
    if (y1 - y0 > 16) {
        long long center_y = (bbox->miny + bbox->maxy) / 2;
        y0 = center_y - 8;
        y1 = center_y + 8;
    }

    for (long long y = y0; y <= y1; ++y) {
        for (long long x = x0; x <= x1; ++x) {
            if (get_value(board, x, y, parameters->size, ctx) != '.') continue;

            // Упрощенная проверка соседей
            int neighbors = 0;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (!dx && !dy) continue;
                    char value = get_value(board, x + dx, y + dy, parameters->size, ctx);
                    if (value == parameters->ai || value == parameters->player) {
                        neighbors++;
                        if (neighbors >= 3) break; // Быстрый выход
                    }
                }
                if (neighbors >= 3) break;
            }

            // НЕ пропускаем клетки без соседей на первых ходах!
            if (!neighbors && parameters->count_moves > 5) continue;

            int score_ai = line_score(board, parameters->size, parameters->len, x, y, parameters->ai, ctx);
            int score_pl = line_score(board, parameters->size, parameters->len, x, y, parameters->player, ctx);
            int sc = forAI ? score_ai - (score_pl / 2) : score_pl - (score_ai / 2);
            sc += neighbors * 1000;

            best_move_push(out, x, y, sc, K);
        }
    }
}

// поиск выигрышных ходов
bool find_immediate_move(Table* board, base* parameters, bounds* bbox, bool forAI, long long* bx, long long* by, GameContext* ctx) {
    best_move cand;
    short K = (ctx->parameters.infinite_field == 1) ? 15 : 10;

    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, forAI, K, &cand, ctx);
    char me = forAI ? ctx->parameters.ai : ctx->parameters.player;

    for (int i = 0; i < cand.n; ++i) {
        long long x = cand.x[i], y = cand.y[i];
        insert(ctx->board, x, y, me);
        bool win = check_win(ctx->board, ctx->parameters.size, ctx->parameters.len, x, y, me, ctx);
        remove_cell(ctx->board, x, y);
        if (win) {
            *bx = x;
            *by = y;
            return true;
        }
    }
    return false;
}


//оценка эффективности блокирующего хода
int evaluate_blocking_move(GameContext* ctx, long long row, long long col) {
    int score = 0;
    int directions[8][2] = {
        {0,1}, {1,0}, {1,1}, {1,-1}, {0,-1}, {-1,0}, {-1,-1}, {-1,1}
    };

    for (int dir = 0; dir < 8; dir++) {
        int deltaRow = directions[dir][0];
        int deltaCol = directions[dir][1];

        int playerSequenceLength = 0;
        int canBlock = 1;

        for (int dist = 1; dist < ctx->parameters.len; dist++) {
            long long newRow = row + dist * deltaRow;
            long long newCol = col + dist * deltaCol;
            if (ctx->parameters.infinite_field == 0 &&
                (newRow < 0 || newRow >= ctx->parameters.size ||
                    newCol < 0 || newCol >= ctx->parameters.size)) {
                break;
            }

            char val = get_value(ctx->board, newRow, newCol, ctx->parameters.size, ctx);
            if (val == ctx->parameters.player) {
                playerSequenceLength++;
            }
            else if (val == ctx->parameters.ai) {
                canBlock = 0;
                break;
            }
            else if (val != '.') {
                break;
            }
        }

        for (int dist = 1; dist < ctx->parameters.len; dist++) {
            long long newRow = row - dist * deltaRow;
            long long newCol = col - dist * deltaCol;

            if (ctx->parameters.infinite_field == 0 &&
                (newRow < 0 || newRow >= ctx->parameters.size ||
                    newCol < 0 || newCol >= ctx->parameters.size)) {
                break;
            }

            char val = get_value(ctx->board, newRow, newCol, ctx->parameters.size, ctx);
            if (val == ctx->parameters.player) {
                playerSequenceLength++;
            }
            else if (val == ctx->parameters.ai) {
                canBlock = 0;
                break;
            }
            else if (val != '.') {
                break;
            }
        }

        if (canBlock && playerSequenceLength > 0) {
            score += playerSequenceLength * playerSequenceLength * 10;
        }
    }

    int adjacentPlayers = 0;
    for (int dir = 0; dir < 8; dir++) {
        long long newRow = row + directions[dir][0];
        long long newCol = col + directions[dir][1];


        if (ctx->parameters.infinite_field == 0 &&
            (newRow < 0 || newRow >= ctx->parameters.size ||
                newCol < 0 || newCol >= ctx->parameters.size)) {
            continue;
        }

        char val = get_value(ctx->board, newRow, newCol, ctx->parameters.size, ctx);
        if (val == ctx->parameters.player) {
            adjacentPlayers++;
        }
    }
    score += adjacentPlayers * 5;

    if (adjacentPlayers == 0 && ctx->parameters.infinite_field == 0) {
        if ((row == 0 || row == ctx->parameters.size - 1) &&
            (col == 0 || col == ctx->parameters.size - 1)) {
            score -= 20;
        }
        else if (row == 0 || row == ctx->parameters.size - 1 ||
            col == 0 || col == ctx->parameters.size - 1) {
            score -= 10;
        }
    }

    if (ctx->parameters.infinite_field == 0 && ctx->parameters.size >= 5) {
        long long center = ctx->parameters.size / 2;
        long long distanceFromCenter = llabs(row - center) + llabs(col - center);
        score += (ctx->parameters.size - distanceFromCenter) * 2;
    }
    else if (ctx->parameters.infinite_field == 1) {
        long long distanceFromOrigin = llabs(row) + llabs(col);
        score -= distanceFromOrigin * 2;
    }

    return score;
}

void computer_move_hard(GameContext* ctx) {
    best_move moves;

    //проверяем выигрышные ходы 
    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, true, 10, &moves, ctx);

    for (int i = 0; i < moves.n; i++) {
        long long x = moves.x[i];
        long long y = moves.y[i];

        insert(ctx->board, x, y, ctx->parameters.ai);
        bool win = check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
            x, y, ctx->parameters.ai, ctx);
        remove_cell(ctx->board, x, y);

        if (win) {
            insert(ctx->board, x, y, ctx->parameters.ai);
            bbox_on_place(&ctx->bbox, x, y);
            ctx->parameters.last_ai_x = x;
            ctx->parameters.last_ai_y = y;
            return;
        }
    }

    //проверяем блокировку игрока 
    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, false, 10, &moves, ctx);

    for (int i = 0; i < moves.n; i++) {
        long long x = moves.x[i];
        long long y = moves.y[i];

        insert(ctx->board, x, y, ctx->parameters.player);
        bool player_wins = check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
            x, y, ctx->parameters.player, ctx);
        remove_cell(ctx->board, x, y);

        if (player_wins) {
            insert(ctx->board, x, y, ctx->parameters.ai);
            bbox_on_place(&ctx->bbox, x, y);
            ctx->parameters.last_ai_x = x;
            ctx->parameters.last_ai_y = y;
            return;
        }
    }

    //хард не всегда выбирает оптимальный ход, может ошибаться
    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, true, 15, &moves, ctx);

    if (moves.n == 0) return;

    int best_score = INT_MIN;
    long long best_x = moves.x[0];
    long long best_y = moves.y[0];

    for (int i = 0; i < moves.n; i++) {
        long long x = moves.x[i];
        long long y = moves.y[i];

        insert(ctx->board, x, y, ctx->parameters.ai);
        int attack = line_score(ctx->board, ctx->parameters.size, ctx->parameters.len,
            x, y, ctx->parameters.ai, ctx);
        remove_cell(ctx->board, x, y);

        int defense = evaluate_blocking_move(ctx, x, y);

        int position_bonus = 0;

        if (ctx->parameters.infinite_field == 1) {
            long long dist = llabs(x) + llabs(y);
            if (dist <= 3) position_bonus += 500;  //бонус за центр
        }

        int random_factor = 0;
        if (rand() % 100 < 20) {  // 20% шанс на "ошибку"
            random_factor = rand() % 500 - 250;  // Случайное отклонение ±250
        }

        // атака важнее защиты, позиция почти не учитывается
        int score = (attack * 3) + (defense * 1) + position_bonus + random_factor;

        //если защита очень низкая, но атака высокая, HARD все равно выберет этот ход
        if (defense < 100 && attack > 500) {
            score += 1000; 
        }

        if (score > best_score) {
            best_score = score;
            best_x = x;
            best_y = y;
        }
    }

    insert(ctx->board, best_x, best_y, ctx->parameters.ai);
    bbox_on_place(&ctx->bbox, best_x, best_y);
    ctx->parameters.last_ai_x = best_x;
    ctx->parameters.last_ai_y = best_y;
}

void computer_move_expert(GameContext* ctx) {
    best_move moves;

    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, true, 12, &moves, ctx);
    for (int i = 0; i < moves.n; i++) {
        long long x = moves.x[i];
        long long y = moves.y[i];

        insert(ctx->board, x, y, ctx->parameters.ai);
        bool win = check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
            x, y, ctx->parameters.ai, ctx);
        remove_cell(ctx->board, x, y);

        if (win) {
            insert(ctx->board, x, y, ctx->parameters.ai);
            bbox_on_place(&ctx->bbox, x, y);
            ctx->parameters.last_ai_x = x;
            ctx->parameters.last_ai_y = y;
            return;
        }
    }

    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, false, 12, &moves, ctx);
    for (int i = 0; i < moves.n; i++) {
        long long x = moves.x[i];
        long long y = moves.y[i];

        insert(ctx->board, x, y, ctx->parameters.player);
        bool player_wins = check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
            x, y, ctx->parameters.player, ctx);
        remove_cell(ctx->board, x, y);

        if (player_wins) {
            insert(ctx->board, x, y, ctx->parameters.ai);
            bbox_on_place(&ctx->bbox, x, y);
            ctx->parameters.last_ai_x = x;
            ctx->parameters.last_ai_y = y;
            return;
        }
    }

    //находим кандидатов для глубокого анализа
    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, true, 20, &moves, ctx);

    if (moves.n == 0) return;

    int best_score = INT_MIN;
    long long best_x = moves.x[0];
    long long best_y = moves.y[0];

    for (int i = 0; i < moves.n; i++) {
        long long x = moves.x[i];
        long long y = moves.y[i];

        if (get_value(ctx->board, x, y, ctx->parameters.size, ctx) != '.') {
            continue;
        }

        // оценка атаки
        int attack_score = 0;
        insert(ctx->board, x, y, ctx->parameters.ai);

        // проверяет ВСЕ направления, а не только 4
        int directions[8][2] = { {1,0},{0,1},{1,1},{1,-1},{-1,0},{0,-1},{-1,-1},{-1,1} };

        for (int dir = 0; dir < 8; dir++) {
            int dx = directions[dir][0];
            int dy = directions[dir][1];

            int ai_count = 1;

            //проверяем в обе стороны
            for (int step = 1; step < ctx->parameters.len; step++) {
                char val = get_value(ctx->board, x + dx * step, y + dy * step,
                    ctx->parameters.size, ctx);
                if (val == ctx->parameters.ai) {
                    ai_count++;
                }
                else if (val != '.') {
                    break;
                }
            }

            for (int step = 1; step < ctx->parameters.len; step++) {
                char val = get_value(ctx->board, x - dx * step, y - dy * step,
                    ctx->parameters.size, ctx);
                if (val == ctx->parameters.ai) {
                    ai_count++;
                }
                else if (val != '.') {
                    break;
                }
            }

            if (ai_count >= ctx->parameters.len) {
                attack_score += 200000;  //больший бонус за выигрыш
            }
            else if (ai_count == ctx->parameters.len - 1) {
                // есть ли свободные клетки для завершения
                int free_spaces = 0;
                // можно ли завершить линию
                for (int step = 1; step < ctx->parameters.len; step++) {
                    if (get_value(ctx->board, x + dx * step, y + dy * step,
                        ctx->parameters.size, ctx) == '.') {
                        free_spaces++;
                    }
                    if (get_value(ctx->board, x - dx * step, y - dy * step,
                        ctx->parameters.size, ctx) == '.') {
                        free_spaces++;
                    }
                }
                if (free_spaces >= 1) {
                    attack_score += 10000;  //можно завершить линию
                }
            }
            else if (ai_count == ctx->parameters.len - 2) {
                attack_score += 2000;  //хорошая перспектива
            }
            else if (ai_count >= 2) {
                attack_score += ai_count * 200;  //слабая угроза
            }
        }

        //оценка защиты 
        remove_cell(ctx->board, x, y); 

        //проверяет все угрозы
        int defense_score = 0;

        //какие угрозы игрока мы блокируем
        insert(ctx->board, x, y, ctx->parameters.player);

        for (int dir = 0; dir < 8; dir++) {
            int dx = directions[dir][0];
            int dy = directions[dir][1];

            int player_count = 1;  

            for (int step = 1; step < ctx->parameters.len; step++) {
                char val = get_value(ctx->board, x + dx * step, y + dy * step,
                    ctx->parameters.size, ctx);
                if (val == ctx->parameters.player) {
                    player_count++;
                }
                else if (val != '.') {
                    break;
                }
            }

            for (int step = 1; step < ctx->parameters.len; step++) {
                char val = get_value(ctx->board, x - dx * step, y - dy * step,
                    ctx->parameters.size, ctx);
                if (val == ctx->parameters.player) {
                    player_count++;
                }
                else if (val != '.') {
                    break;
                }
            }
            if (player_count >= ctx->parameters.len - 1) {
                defense_score += 5000;  //очень опасная угроза
            }
            else if (player_count >= ctx->parameters.len - 2) {
                defense_score += 1500;  //потенциальная угроза
            }
        }

        remove_cell(ctx->board, x, y);

        defense_score += evaluate_blocking_move(ctx, x, y);

        insert(ctx->board, x, y, ctx->parameters.ai);

        int position_score = 0;

        //ценит центр еще больше
        if (ctx->parameters.infinite_field == 1) {
            long long dist = llabs(x) + llabs(y);
            position_score += (20 - dist * 2) * 100;  //сильнее штрафуем за удаление от центра
        }
        else if (ctx->parameters.size >= 5) {
            long long center = ctx->parameters.size / 2;
            long long dist_from_center = llabs(x - center) + llabs(y - center);
            position_score += (ctx->parameters.size - dist_from_center) * 200;
        }

        int our_neighbors = 0;
        int player_neighbors = 0;

        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                char val = get_value(ctx->board, x + dx, y + dy, ctx->parameters.size, ctx);
                if (val == ctx->parameters.ai) {
                    our_neighbors++;
                }
                else if (val == ctx->parameters.player) {
                    player_neighbors++;
                }
            }
        }

        position_score += our_neighbors * 800;     
        position_score += player_neighbors * 400; 

        int future_score = 0;
        if (ctx->parameters.count_moves < 15) {  
            //смотрит на 1 ход вперед
            best_move next_moves;
            generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, false, 5, &next_moves, ctx);

            if (next_moves.n > 0) {
                //не даем ли мы игроку выигрышную позицию
                for (int j = 0; j < next_moves.n && j < 3; j++) {
                    long long nx = next_moves.x[j];
                    long long ny = next_moves.y[j];

                    if (get_value(ctx->board, nx, ny, ctx->parameters.size, ctx) != '.') continue;

                    insert(ctx->board, nx, ny, ctx->parameters.player);
                    bool player_can_win = check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
                        nx, ny, ctx->parameters.player, ctx);
                    remove_cell(ctx->board, nx, ny);

                    if (player_can_win) {
                        future_score -= 3000;  //штраф за опасную позицию
                    }
                }
            }
        }

        remove_cell(ctx->board, x, y);

        int total_score = (attack_score * 2) + (defense_score * 4)  + future_score;

        if (attack_score > 2000 && defense_score > 2000) {
            total_score += 10000;  //идеальный ход
        }

        if (total_score > best_score) {
            best_score = total_score;
            best_x = x;
            best_y = y;
        }
    }

    insert(ctx->board, best_x, best_y, ctx->parameters.ai);
    bbox_on_place(&ctx->bbox, best_x, best_y);
    ctx->parameters.last_ai_x = best_x;
    ctx->parameters.last_ai_y = best_y;
}


//для хард и эксперт
void new_computer_move(GameContext* ctx) {
    if (ctx->parameters.infinite_field == 1 && ctx->parameters.count_moves < 3) {
        if (ctx->parameters.last_pl_x != LLONG_MAX && ctx->parameters.last_pl_y != LLONG_MAX) {
            int dirs[8][2] = { {1,0},{0,1},{-1,0},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };
            for (int i = 0; i < 8; i++) {
                long long x = ctx->parameters.last_pl_x + dirs[i][0];
                long long y = ctx->parameters.last_pl_y + dirs[i][1];
                if (get_value(ctx->board, x, y, ctx->parameters.size, ctx) == '.') {
                    insert(ctx->board, x, y, ctx->parameters.ai);
                    bbox_on_place(&ctx->bbox, x, y);
                    ctx->parameters.last_ai_x = x;
                    ctx->parameters.last_ai_y = y;
                    return;
                }
            }
        }
    }


    if (ctx->parameters.difficulty == 3) {  // HARD
        computer_move_hard(ctx);
    }
    else if (ctx->parameters.difficulty == 4) {  // EXPERT
        computer_move_expert(ctx);
    }
}


void easy_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    best_move cand;
    generate_candidates(board, parameters, bbox, true, 64, &cand, ctx);

    if (cand.n > 0) {
        //рандомный выбор из доступных ходов
        int random_index = rand() % cand.n;
        long long x = cand.x[random_index];
        long long y = cand.y[random_index];

        insert(board, x, y, parameters->ai);
        bbox_on_place(bbox, x, y);
        parameters->last_ai_x = x;
        parameters->last_ai_y = y;
    }
}


void medium_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    long long bx, by;

    //проверка выигрышных ходов
    if (find_immediate_move(board, parameters, bbox, true, &bx, &by, ctx) ||
        find_immediate_move(board, parameters, bbox, false, &bx, &by, ctx)) {
        insert(board, bx, by, parameters->ai);
        bbox_on_place(bbox, bx, by);
        parameters->last_ai_x = bx;
        parameters->last_ai_y = by;
        return;
    }
    best_move cand;
    generate_candidates(board, parameters, bbox, true, 16, &cand, ctx);

    if (cand.n > 0) {
        insert(board, cand.x[0], cand.y[0], parameters->ai);
        bbox_on_place(bbox, cand.x[0], cand.y[0]);
        parameters->last_ai_x = cand.x[0];
        parameters->last_ai_y = cand.y[0];
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////

void save_game(GameContext* ctx) {
    FILE* file = fopen("save.dat", "wb");
    if (file) {
        // Сохраняем параметры и границы
        fwrite(&ctx->parameters, sizeof(base), 1, file);
        fwrite(&ctx->bbox, sizeof(bounds), 1, file);

        // Сохраняем количество ходов
        fwrite(&ctx->parameters.count_moves, sizeof(unsigned long long), 1, file);

        // Сохраняем все клетки
        for (unsigned long long i = 0; i < ctx->board->capacity; i++) {
            Node* current = ctx->board->buckets[i];
            while (current) {
                fwrite(current, sizeof(Node), 1, file);
                current = current->next;
            }
        }
        fclose(file);
    }
}

bool load_game(GameContext* ctx) {
    FILE* file = fopen("save.dat", "rb");
    if (file) {
        // Очищаем текущую доску
        for (unsigned long long i = 0; i < ctx->board->capacity; i++) {
            Node* current = ctx->board->buckets[i];
            while (current) {
                Node* temp = current;
                current = current->next;
                free(temp);
            }
            ctx->board->buckets[i] = NULL;
        }

        // Читаем параметры и границы
        if (fread(&ctx->parameters, sizeof(base), 1, file) != 1) {
            fclose(file);
            return false;
        }
        if (fread(&ctx->bbox, sizeof(bounds), 1, file) != 1) {
            fclose(file);
            return false;
        }

        // Читаем количество ходов
        unsigned long long count_moves;
        if (fread(&count_moves, sizeof(unsigned long long), 1, file) != 1) {
            fclose(file);
            return false;
        }
        ctx->parameters.count_moves = count_moves;

        // Читаем клетки
        Node node;
        while (fread(&node, sizeof(Node), 1, file) == 1) {
            insert(ctx->board, node.x, node.y, node.value);
        }

        fclose(file);
        ctx->is_player_turn = ctx->parameters.player_moves_first;
        return true;
    }
    return false;
}
// Graphics functions
void drawcircle(float center_x, float center_y, float radius) {
    int segments = 45;
    glColor3f(0.0f, 0.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * 3.1415926f * (float)i / (float)segments;
        glVertex2f(center_x + radius * cosf(angle), center_y + radius * sinf(angle));
    }
    glEnd();
}

void drawcross(float x, float y) {
    glColor3f(0.0f, 0.5f, 1.0f);
    glLineWidth(5.0f);
    glBegin(GL_LINES);
    glVertex2f(x + 20, y + 20);
    glVertex2f(x + CELL_SIZE - 20, y + CELL_SIZE - 20);
    glVertex2f(x + CELL_SIZE - 20, y + 20);
    glVertex2f(x + 20, y + CELL_SIZE - 20);
    glEnd();
}

void drawchar(char c, float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0); // Смещение для отрисовки
    glScalef(scale, scale, 1.0f); // Масштабирование символа
    switch (toupper(c)) { // Отрисовка каждого символа
    case 'S':
        glBegin(GL_LINE_STRIP);
        glVertex2f(13, 2); glVertex2f(2, 2);
        glVertex2f(2, 10); glVertex2f(13, 10);
        glVertex2f(13, 19); glVertex2f(2, 19);
        glEnd();
        break;
    case 'T':
        glBegin(GL_LINES);
        glVertex2f(1, 2); glVertex2f(15, 2);
        glVertex2f(7.5f, 2); glVertex2f(7.5f, 20);
        glEnd();
        break;
    case 'A':
        glBegin(GL_LINE_STRIP);
        glVertex2f(0, 19); glVertex2f(7.5f, 0);
        glVertex2f(14, 20);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(4, 11); glVertex2f(11, 11);
        glEnd();
        break;
    case 'R':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 20);
        glVertex2f(2, 2); glVertex2f(11, 2);
        glVertex2f(14, 5); glVertex2f(14, 9);
        glVertex2f(11, 12); glVertex2f(1, 12);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(2, 12); glVertex2f(14, 20);
        glEnd();
        break;
    case 'E':
        glBegin(GL_LINE_STRIP);
        glVertex2f(13, 2); glVertex2f(2, 2);
        glVertex2f(2, 19); glVertex2f(13, 19);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(2, 10); glVertex2f(10, 10);
        glEnd();
        break;
    case 'I':
        glBegin(GL_LINES);
        glVertex2f(7.5f, 1); glVertex2f(7.5f, 20);
        glEnd();
        break;
    case 'N':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 20); glVertex2f(1, 0);
        glVertex2f(12, 20); glVertex2f(12, 0);
        glEnd();
        break;
    case 'G':
        glBegin(GL_LINE_STRIP);
        glVertex2f(13, 5); glVertex2f(12, 1);
        glVertex2f(5, 1); glVertex2f(2, 5);
        glVertex2f(2, 15); glVertex2f(5, 19);
        glVertex2f(10, 19); glVertex2f(13, 15);
        glVertex2f(13, 10); glVertex2f(8, 10);
        glEnd();
        break;
    case 'B':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 2); glVertex2f(8, 2);
        glVertex2f(10, 5); glVertex2f(10, 7);
        glVertex2f(8, 10); glVertex2f(1, 10);
        glVertex2f(8, 10); glVertex2f(12, 13);
        glVertex2f(12, 15); glVertex2f(10, 18);
        glVertex2f(9, 18); glVertex2f(1, 18);
        glVertex2f(1, 2);
        glEnd();
        break;
    case 'C':
        glBegin(GL_LINE_STRIP);
        glVertex2f(13, 2); glVertex2f(5, 2);
        glVertex2f(2, 5); glVertex2f(2, 15);
        glVertex2f(5, 19); glVertex2f(13, 19);
        glEnd();
        break;
    case 'K':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 0); glVertex2f(1, 20);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(1, 10); glVertex2f(12, 1);
        glVertex2f(1, 10); glVertex2f(12, 19);
        glEnd();
        break;
    case 'O':
        glBegin(GL_LINE_LOOP);
        glVertex2f(5, 2); glVertex2f(10, 2);
        glVertex2f(13, 5); glVertex2f(13, 15);
        glVertex2f(10, 19); glVertex2f(5, 19);
        glVertex2f(2, 15); glVertex2f(2, 5);
        glEnd();
        break;
    case 'P':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 20); glVertex2f(2, 2);
        glVertex2f(11, 2); glVertex2f(14, 5);
        glVertex2f(14, 9); glVertex2f(11, 12);
        glVertex2f(2, 12);
        glEnd();
        break;
    case 'Z':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 2); glVertex2f(15, 2);
        glVertex2f(2, 19);
        glVertex2f(15, 19);
        glEnd();
        break;
    case 'L':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 2);
        glVertex2f(2, 19);

        glVertex2f(13, 19);
        glEnd();
        break;
    case 'W':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 2);
        glVertex2f(2, 20);
        glVertex2f(7, 10);
        glVertex2f(12, 20);
        glVertex2f(12, 2);
        glEnd();
        break;
    case 'Y':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 1); glVertex2f(7.5f, 10); // Левая часть V (сверху вниз)
        glVertex2f(13, 1); // Правая часть V
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(7.5f, 10); glVertex2f(7.5f, 20); // Стержень (вниз)
        glEnd();
        break;
    case 'X':
        glBegin(GL_LINES);
        glVertex2f(2, 20); glVertex2f(13, 2);
        glVertex2f(2, 2); glVertex2f(13, 20);
        glEnd();
        break;
    case 'M':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 20);    // Левая верхняя точка
        glVertex2f(2, 2);     // Левая нижняя точка
        glVertex2f(7, 12);    // Первый центральный изгиб (вверх)
        glVertex2f(12, 2);    // Правая нижняя точка
        glVertex2f(12, 20);   // Правая верхняя точка
        glEnd();
        break;
    case 'U':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 2); glVertex2f(2, 17);
        glVertex2f(4, 20); glVertex2f(12, 20);
        glVertex2f(14, 17); glVertex2f(14, 2);
        glEnd();
        break;
    case 'D':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 20); glVertex2f(2, 2);
        glVertex2f(10, 2); glVertex2f(13, 5);
        glVertex2f(13, 15); glVertex2f(10, 18);
        glVertex2f(2, 18);
        glEnd();
        break;
    case 'H':
        glBegin(GL_LINES);
        glVertex2f(2, 20); glVertex2f(2, 1);
        glVertex2f(13, 20); glVertex2f(13, 1);
        glVertex2f(2, 10); glVertex2f(13, 10);
        glEnd();
        break;
    case 'F':
        glBegin(GL_LINE_STRIP);
        glVertex2f(13, 2); glVertex2f(2, 2);
        glVertex2f(2, 20);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(2, 10); glVertex2f(10, 10);
        glEnd();
        break;
    case 'V':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 2);    // Левая верхняя точка
        glVertex2f(7.5f, 19); // Центральная нижняя точка
        glVertex2f(13, 2);   // Правая верхняя точка
        glEnd();
        break;
    case 'Q':
        // Основной круг
        glBegin(GL_LINE_LOOP);
        glVertex2f(5, 2); glVertex2f(10, 2);
        glVertex2f(13, 5); glVertex2f(13, 15);
        glVertex2f(10, 19); glVertex2f(5, 19);
        glVertex2f(2, 15); glVertex2f(2, 5);
        glEnd();
        // Хвостик буквы Q
        glBegin(GL_LINES);
        glVertex2f(9, 12); glVertex2f(14, 20);
        glEnd();
        break;
    case '+':
        glBegin(GL_LINES);
        glVertex2f(7.5f, 2); glVertex2f(7.5f, 18);
        glVertex2f(2, 10); glVertex2f(13, 10);
        glEnd();
        break;
    case '-':
        glBegin(GL_LINES);
        glVertex2f(2, 10); glVertex2f(13, 10);
        glEnd();
        break;
    case '0':
        glBegin(GL_LINE_LOOP);
        glVertex2f(5, 18); glVertex2f(10, 18); // Верхняя линия
        glVertex2f(13, 15); glVertex2f(13, 5); // Правая сторона
        glVertex2f(10, 1); glVertex2f(5, 1); // Нижняя линия
        glVertex2f(2, 5); glVertex2f(2, 15); // Левая сторона
        glEnd();
        break;
    case '1':
        glBegin(GL_LINES);
        glVertex2f(7.5f, 18); glVertex2f(7.5f, 0); // Вертикальная линия
        glVertex2f(5, 18); glVertex2f(10, 18); // Верхняя перекладина
        glVertex2f(5, 0); glVertex2f(10, 0); // Нижняя перекладина
        glEnd();
        break;
    case '2':
        glBegin(GL_LINE_STRIP);
        glVertex2f(13, 18); glVertex2f(2, 18); // Верхняя линия
        glVertex2f(2, 10); glVertex2f(13, 10); // Средняя линия
        glVertex2f(13, 1); glVertex2f(2, 1); // Нижняя линия
        glEnd();
        break;
    case '3':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 18); glVertex2f(13, 18); // Верхняя линия
        glVertex2f(13, 10); glVertex2f(2, 10); // Средняя линия
        glVertex2f(13, 1); glVertex2f(2, 1); // Нижняя линия
        glEnd();
        break;
    case '4':
        glBegin(GL_LINE_STRIP);
        glVertex2f(14, 19); glVertex2f(14, 1);
        glVertex2f(1, 13); glVertex2f(14, 13);
        glEnd();
        break;
    case '5':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 18); glVertex2f(14, 18); // Верхняя линия
        glVertex2f(14, 10); glVertex2f(1, 10); // Средняя линия
        glVertex2f(1, 1); glVertex2f(14, 1); // Нижняя линия
        glEnd();
        break;
    case '6':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 18); glVertex2f(14, 18); // Верхняя линия
        glVertex2f(14, 10); glVertex2f(1, 10); // Средняя линия
        glVertex2f(1, 1); glVertex2f(14, 1); // Нижняя линия
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(1, 18);  glVertex2f(1, 10);
        glEnd();
        break;
    case '7':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 1); glVertex2f(14, 1); // Верхняя линия
        glVertex2f(1, 19); // Диагональ
        glEnd();
        break;
    case '8':
        glBegin(GL_LINE_LOOP);
        glVertex2f(5, 18); glVertex2f(10, 18); // Верхняя линия (верхний контур)
        glVertex2f(13, 15); glVertex2f(13, 13); // Правая сторона (верхний контур)
        glVertex2f(10, 10); glVertex2f(5, 10); // Средняя линия
        glVertex2f(2, 13); glVertex2f(2, 15); // Левая сторона (верхний контур)
        glEnd();
        glBegin(GL_LINE_LOOP);
        glVertex2f(5, 10); glVertex2f(10, 10); // Верхняя линия (нижний контур)
        glVertex2f(13, 7); glVertex2f(13, 5); // Правая сторона (нижний контур)
        glVertex2f(10, 1); glVertex2f(5, 1); // Нижняя линия
        glVertex2f(2, 5); glVertex2f(2, 7); // Левая сторона (нижний контур)
        glEnd();
        break;
    case '9':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 18); glVertex2f(14, 18); // Верхняя линия
        glVertex2f(14, 10); glVertex2f(1, 10); // Средняя линия
        glVertex2f(1, 1); glVertex2f(14, 1); // Нижняя линия
        glVertex2f(14, 18);
        glEnd();
        break;
    case '!':
        glBegin(GL_LINES);
        glVertex2f(7.5f, 1); glVertex2f(7.5f, 17);

        glVertex2f(7.5f, 18.5f); glVertex2f(7.5f, 19.5f);
        glEnd();
        break;
    case '/':
        glBegin(GL_LINES);
        glVertex2f(13, 2);   // Правая верхняя точка
        glVertex2f(2, 19);   // Левая нижняя точка
        glEnd();
        break;
    }


    glPopMatrix();
}

void drawtext(GameContext* ctx, const char* text, float x, float y, float scale) {
    float total_width = 0;
    for (const char* c = text; *c != '\0'; c++) {
        total_width += (*c == ' ') ? 15 * scale : ctx->char_width * scale;
    }
    float start_x = x - total_width / 2;
    float start_y = y - ctx->char_height / 2 * scale;
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == ' ') start_x += 15 * scale;
        else {
            drawchar(*c, start_x, start_y, scale);
            start_x += ctx->char_width * scale;
        }
    }
}

void drawbutton(GameContext* ctx, Button btn) {
    glColor3f(btn.is_mouse ? 0.0f : 0.7f, btn.is_mouse ? 0.5f : 0.7f, btn.is_mouse ? 0.5f : 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(btn.x, btn.y);
    glVertex2f(btn.x + btn.width, btn.y);
    glVertex2f(btn.x + btn.width, btn.y + btn.height);
    glVertex2f(btn.x, btn.y + btn.height);
    glEnd();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(btn.x, btn.y);
    glVertex2f(btn.x + btn.width, btn.y);
    glVertex2f(btn.x + btn.width, btn.y + btn.height);
    glVertex2f(btn.x, btn.y + btn.height);
    glEnd();
    drawtext(ctx, btn.text, btn.x + btn.width / 2, btn.y + btn.height / 2, 0.7f);
}

void drawgrid(GameContext* ctx) {
    glColor3f(0.0f, 0.1f, 0.1f);
    glLineWidth(1.0f);

    long long start_x = floor((float)ctx->view_offset_x / CELL_SIZE);
    long long start_y = floor((float)ctx->view_offset_y / CELL_SIZE);

    // Рисуем сетку для видимой области без ограничений
    for (long long x = start_x; x <= start_x + VISIBLE_CELLS_X + 1; x++) {
        float screen_x = x * CELL_SIZE - ctx->view_offset_x;
        glBegin(GL_LINES);
        glVertex2f(screen_x, 0);
        glVertex2f(screen_x, VISIBLE_CELLS_Y * CELL_SIZE);
        glEnd();
    }

    for (long long y = start_y; y <= start_y + VISIBLE_CELLS_Y + 1; y++) {
        float screen_y = y * CELL_SIZE - ctx->view_offset_y;
        glBegin(GL_LINES);
        glVertex2f(0, screen_y);
        glVertex2f(VISIBLE_CELLS_X * CELL_SIZE, screen_y);
        glEnd();
    }

    // Отрисовываем все видимые клетки без проверки границ
    for (long long x = start_x; x < start_x + VISIBLE_CELLS_X; x++) {
        for (long long y = start_y; y < start_y + VISIBLE_CELLS_Y; y++) {
            float screen_x = x * CELL_SIZE - ctx->view_offset_x;
            float screen_y = y * CELL_SIZE - ctx->view_offset_y;

            char state = get_value(ctx->board, x, y, ctx->parameters.size, ctx);

            if (state == ctx->parameters.player && ctx->parameters.player == 'X') {
                drawcross(screen_x, screen_y);
            }
            else if (state == ctx->parameters.ai && ctx->parameters.ai == 'O') {
                drawcircle(screen_x + CELL_SIZE / 2, screen_y + CELL_SIZE / 2, CELL_SIZE / 2 - 20);
            }
            else if (state == ctx->parameters.player && ctx->parameters.player == 'O') {
                drawcircle(screen_x + CELL_SIZE / 2, screen_y + CELL_SIZE / 2, CELL_SIZE / 2 - 20);
            }
            else if (state == ctx->parameters.ai && ctx->parameters.ai == 'X') {
                drawcross(screen_x, screen_y);
            }
        }
    }

    // Курсор (без проверки границ)
    float cursor_x = ctx->cursor_x * CELL_SIZE - ctx->view_offset_x;
    float cursor_y = ctx->cursor_y * CELL_SIZE - ctx->view_offset_y;
    glColor3f(1.0f, 0.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cursor_x + 5, cursor_y + 5);
    glVertex2f(cursor_x + CELL_SIZE - 5, cursor_y + 5);
    glVertex2f(cursor_x + CELL_SIZE - 5, cursor_y + CELL_SIZE - 5);
    glVertex2f(cursor_x + 5, cursor_y + CELL_SIZE - 5);
    glEnd();
}

void draw_menu(GameContext* ctx) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    drawtext(ctx, "TIC TAC TOE", WINDOW_WIDTH / 2, 80, 1.5f);

    // Проверяем существует ли сохранение
    bool save_exists = false;
    FILE* test_file = fopen("save.dat", "rb");
    if (test_file) {
        save_exists = true;
        fclose(test_file);
    }

    ctx->start_button.text = save_exists ? "CONTINUE" : "START";

    // Располагаем кнопки вертикально с отступами
    ctx->start_button.y = WINDOW_HEIGHT / 2 - 70;
    ctx->settings_button.y = WINDOW_HEIGHT / 2;
    ctx->help_button.y = WINDOW_HEIGHT / 2 + 70;
    ctx->about_button.y = WINDOW_HEIGHT / 2 + 140;

    drawbutton(ctx, ctx->start_button);
    drawbutton(ctx, ctx->settings_button);
    drawbutton(ctx, ctx->help_button);
    drawbutton(ctx, ctx->about_button);


}



void draw_help(GameContext* ctx) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.8f, 0.9f, 0.95f, 1.0f);

    drawtext(ctx, "GAME HELP", WINDOW_WIDTH / 2, 60, 1.5f);

    // Управление в игре
    glColor3f(0.3f, 0.3f, 0.3f);
    drawtext(ctx, "GAME CONTROLS:", WINDOW_WIDTH / 2, 120, 0.8f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawtext(ctx, "Arrow Keys - Move cursor", WINDOW_WIDTH / 2, 150, 0.6f);
    drawtext(ctx, "Space - Make move", WINDOW_WIDTH / 2, 170, 0.6f);
    drawtext(ctx, "Alt+Q - Save and exit to menu", WINDOW_WIDTH / 2, 190, 0.6f);
    drawtext(ctx, "Ctrl+R or F9 - Reset saved game", WINDOW_WIDTH / 2, 210, 0.6f);
    drawtext(ctx, "ESC - return to the menu", WINDOW_WIDTH / 2, 230, 0.6f);



    // Главное меню
    glColor3f(0.3f, 0.3f, 0.3f);
    drawtext(ctx, "MAIN MENU KEYS:", WINDOW_WIDTH / 2, 270, 0.8f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawtext(ctx, "N - New game", WINDOW_WIDTH / 2, 290, 0.6f);
    drawtext(ctx, "S - Settings", WINDOW_WIDTH / 2, 310, 0.6f);
    drawtext(ctx, "H - open the help", WINDOW_WIDTH / 2, 330, 0.6f);
    drawtext(ctx, "A - open the about", WINDOW_WIDTH / 2, 350, 0.6f);

    // Особенности
    glColor3f(0.3f, 0.3f, 0.3f);
    drawtext(ctx, "FEATURES:", WINDOW_WIDTH / 2, 390, 0.8f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawtext(ctx, "4 difficulty levels (Easy to Expert)", WINDOW_WIDTH / 2, 410, 0.6f);
    drawtext(ctx, "Support for large boards", WINDOW_WIDTH / 2, 430, 0.6f);
    drawtext(ctx, "Infinite field mode for unlimited gameplay", WINDOW_WIDTH / 2, 450, 0.6f);

    // Правила
    glColor3f(0.3f, 0.3f, 0.3f);
    drawtext(ctx, "RULES:", WINDOW_WIDTH / 2, 490, 0.8f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawtext(ctx, "Get X or O in a row to win", WINDOW_WIDTH / 2, 510, 0.6f);
    drawtext(ctx, "Lines can be horizontal vertical or diagonal", WINDOW_WIDTH / 2, 530, 0.6f);

    drawbutton(ctx, ctx->back_button);
}

void draw_about(GameContext* ctx) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.85f, 0.90f, 0.95f, 1.0f);

    drawtext(ctx, "ABOUT", WINDOW_WIDTH / 2, 80, 1.5f);

    // Основная информация
    //glColor3f(0.0f, 0.0f, 0.0f);

    glColor3f(0.8f, 0.7f, 0.2f);
    drawtext(ctx, "AUTHORS:", WINDOW_WIDTH / 2, 140, 0.8f);
    drawtext(ctx, "Sharibzyanova A E / Idieva S F", WINDOW_WIDTH / 2, 170, 0.7f);

    drawtext(ctx, "GROUP: 5131001/40002", WINDOW_WIDTH / 2, 210, 0.8f);
    drawtext(ctx, "2025", WINDOW_WIDTH / 2, 240, 0.8f);

    glColor3f(0.0f, 0.5f, 0.5f);
    drawtext(ctx, "Saint Petersburg Polytechnic University", WINDOW_WIDTH / 2, 290, 0.7f);
    drawtext(ctx, "Institute of Computer Science and Cybersecurity", WINDOW_WIDTH / 2, 320, 0.7f);
    drawtext(ctx, "Higher School of Cybersecurity", WINDOW_WIDTH / 2, 350, 0.7f);

    drawbutton(ctx, ctx->back_button);
}


void get_difficulty_color(int difficulty, float* r, float* g, float* b) {
    switch (difficulty) {
    case 1: // EASY и бесконечность 
        *r = 0.0f; *g = 1.0f; *b = 0.0f; // Зеленый
        break;
    case 2: // MEDIUM
        *r = 1.0f; *g = 1.0f; *b = 0.0f; // Желтый
        break;
    case 3: // HARD
        *r = 1.0f; *g = 0.5f; *b = 0.0f; // Оранжевый
        break;
    case 4: // EXPERT
        *r = 1.0f; *g = 0.0f; *b = 0.0f; // Красный
        break;
    case 0: // не бесконечность
        *r = 1.0f; *g = 0.0f; *b = 0.0f; // Красный
        break;
    default:
        *r = 1.0f; *g = 1.0f; *b = 1.0f; // Белый
        break;
    }
}

void draw_settings(GameContext* ctx) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.8f, 0.9f, 0.9f, 1.0f);
    drawtext(ctx, "SETTINGS", WINDOW_WIDTH / 2, 90, 1.5f);

    char buffer[50];

    // BOARD SIZE (белый)
    glColor3f(0.0f, 0.0f, 0.0f);
    if (ctx->parameters.infinite_field == 1) {
        snprintf(buffer, sizeof(buffer), "BOARD SIZE: INFINITY");
    }
    else {
        snprintf(buffer, sizeof(buffer), "BOARD SIZE: %d", (int)ctx->parameters.size);
    }
    drawtext(ctx, buffer, WINDOW_WIDTH / 4, WINDOW_HEIGHT / 2 - 140, 0.9f);
    drawbutton(ctx, ctx->size_up_button);
    drawbutton(ctx, ctx->size_down_button);

    // WIN LINE (белый)
    glColor3f(0.0f, 0.0f, 0.0f);
    snprintf(buffer, sizeof(buffer), "WIN LINE: %d", (int)ctx->parameters.len);
    drawtext(ctx, buffer, WINDOW_WIDTH - 200, WINDOW_HEIGHT / 2 - 140, 0.9f);
    drawbutton(ctx, ctx->line_up_button);
    drawbutton(ctx, ctx->line_down_button);

    // FIRST PLAYER (белый)
    glColor3f(0.0f, 0.0f, 0.0f);
    snprintf(buffer, sizeof(buffer), "FIRST PLAYER: %s",
        ctx->parameters.player_moves_first ? (ctx->parameters.player == 'X' ? "PLAYER X" : "PLAYER O") : (ctx->parameters.ai == 'X' ? "COMPUTER X" : "COMPUTER O"));
    drawtext(ctx, buffer, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 5, 0.9f);
    drawbutton(ctx, ctx->first_player_button);

    // DIFFICULTY (разные цвета)
    float r, g, b;
    get_difficulty_color(ctx->parameters.difficulty, &r, &g, &b);
    glColor3f(r, g, b);
    snprintf(buffer, sizeof(buffer), "DIFFICULTY: %s",
        ctx->parameters.difficulty == 1 ? "EASY" :
        ctx->parameters.difficulty == 2 ? "MEDIUM" :
        ctx->parameters.difficulty == 3 ? "HARD" : "EXPERT");
    drawtext(ctx, buffer, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 95, 0.9f);
    drawbutton(ctx, ctx->difficulty_button);

    // INFINITE FIELD
    float ri, gi, bi;
    glColor3f(0.0f, 0.0f, 0.0f);
    get_difficulty_color(ctx->parameters.infinite_field, &ri, &gi, &bi);
    glColor3f(ri, gi, bi);
    snprintf(buffer, sizeof(buffer), "INFINITE FIELD: %s", ctx->parameters.infinite_field == 1 ? "ON" : "OFF");
    drawtext(ctx, buffer, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 205, 0.9f);
    drawbutton(ctx, ctx->infinite_field_button);
    drawbutton(ctx, ctx->back_button);
}



void draw_game_over(GameContext* ctx) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);

    const char* message;
    if (ctx->winner == 1) {
        message = ctx->parameters.player == 'X' ? "PLAYER X WINS!" : "PLAYER O WINS!";
    }
    else if (ctx->winner == 2) {
        message = ctx->parameters.ai == 'X' ? "COMPUTER X WINS!" : "COMPUTER O WINS!";
    }
    else if (ctx->winner == 3) {
        message = "DRAW!"; // Сообщение о ничье
    }
    else {
        message = "GAME OVER";
    }

    drawtext(ctx, message, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 3.0f);
    drawbutton(ctx, ctx->back_button);
}


bool mouse_over_button(Button btn, double mouse_x, double mouse_y) {
    return (mouse_x >= btn.x && mouse_x <= btn.x + btn.width &&
        mouse_y >= btn.y && mouse_y <= btn.y + btn.height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    GameContext* ctx = (GameContext*)glfwGetWindowUserPointer(window);
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (ctx->current_screen == MENU_SCREEN) {
        if (mouse_over_button(ctx->start_button, xpos, ypos)) {
            if (load_game(ctx)) {
                ctx->current_screen = GAME_SCREEN;
            }
            else {
                // Новая игра
                ctx->current_screen = GAME_SCREEN;
                for (unsigned long long i = 0; i < ctx->board->capacity; i++) {
                    Node* current = ctx->board->buckets[i];
                    while (current) {
                        Node* temp = current;
                        current = current->next;
                        free(temp);
                    }
                    ctx->board->buckets[i] = NULL;
                }
                ctx->parameters.count_moves = 0;
                ctx->bbox.initialized = false;
                if (ctx->parameters.infinite_field == 1) {
                    // Для бесконечного поля начинаем с видимого центра
                    ctx->cursor_x = 0;
                    ctx->cursor_y = 0;
                    // Центрируем вид так, чтобы центр поля был в центре экрана
                    ctx->view_offset_x = -VISIBLE_CELLS_X / 2 * CELL_SIZE + CELL_SIZE / 2;
                    ctx->view_offset_y = -VISIBLE_CELLS_Y / 2 * CELL_SIZE + CELL_SIZE / 2;
                }
                else {
                    // Для ограниченного поля как раньше
                    ctx->cursor_x = 0;
                    ctx->cursor_y = 0;
                    ctx->view_offset_x = 0;
                    ctx->view_offset_y = 0;
                }
                ctx->is_player_turn = ctx->parameters.player_moves_first;
                ctx->winner = 0;
            }
        }
        else if (mouse_over_button(ctx->settings_button, xpos, ypos)) {
            ctx->current_screen = SETTINGS_SCREEN;
        }
        else if (mouse_over_button(ctx->help_button, xpos, ypos)) {
            ctx->current_screen = HELP_SCREEN; // Переход к справке
        }
        else if (mouse_over_button(ctx->about_button, xpos, ypos)) {
            ctx->current_screen = ABOUT_SCREEN; // Новый экран
        }
    }
    else if (ctx->current_screen == ABOUT_SCREEN) {
        if (mouse_over_button(ctx->back_button, xpos, ypos)) {
            ctx->current_screen = MENU_SCREEN;
        }
    }
    else if (ctx->current_screen == HELP_SCREEN) {
        if (mouse_over_button(ctx->back_button, xpos, ypos)) {
            ctx->current_screen = MENU_SCREEN;
        }
    }
    else if (ctx->current_screen == GAME_SCREEN || ctx->current_screen == GAME_OVER) {
        if (mouse_over_button(ctx->back_button, xpos, ypos)) {
            ctx->current_screen = MENU_SCREEN;
        }
    }
    else if (ctx->current_screen == SETTINGS_SCREEN) {
        if (mouse_over_button(ctx->back_button, xpos, ypos)) {
            ctx->current_screen = MENU_SCREEN;
        }
        else if (mouse_over_button(ctx->size_up_button, xpos, ypos) && 0 == ctx->parameters.infinite_field) {
            ctx->parameters.size++;
        }
        else if (mouse_over_button(ctx->size_down_button, xpos, ypos) && 0 == ctx->parameters.infinite_field) {
            if (ctx->parameters.size > MIN_SIZE) ctx->parameters.size--;
        }
        else if (mouse_over_button(ctx->line_up_button, xpos, ypos)) {
            if (ctx->parameters.infinite_field == 1) {
                ctx->parameters.len++;
            }
            else {
                if (ctx->parameters.len < ctx->parameters.size && ctx->parameters.len < MAX_WIN_LINE) ctx->parameters.len++;
            }
        }
        else if (mouse_over_button(ctx->line_down_button, xpos, ypos)) {
            if (ctx->parameters.len > 3) ctx->parameters.len--;
        }
        else if (mouse_over_button(ctx->first_player_button, xpos, ypos)) {
            ctx->parameters.player_moves_first = !ctx->parameters.player_moves_first;
            if (!ctx->parameters.player_moves_first) {
                char temp = ctx->parameters.player;
                ctx->parameters.player = ctx->parameters.ai;
                ctx->parameters.ai = temp;
            }
        }
        else if (mouse_over_button(ctx->difficulty_button, xpos, ypos)) {
            ctx->parameters.difficulty = (ctx->parameters.difficulty % 4) + 1;
            if (ctx->parameters.difficulty == 3) ctx->parameters.depth = 2;
            else ctx->parameters.depth = 10;
           
        }
        else if (mouse_over_button(ctx->infinite_field_button, xpos, ypos) && ctx->parameters.infinite_field == 0) {
            ctx->parameters.infinite_field = 1;
        }
        else if (mouse_over_button(ctx->infinite_field_button, xpos, ypos) && ctx->parameters.infinite_field == 1) {
            ctx->parameters.infinite_field = 0;
        }

    }
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    GameContext* ctx = (GameContext*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        // Комбинация клавиш для сброса сохраненной игры
        if ((key == GLFW_KEY_R && (mods & GLFW_MOD_CONTROL)) || // Ctrl+R
            (key == GLFW_KEY_DELETE && (mods & GLFW_MOD_CONTROL)) || // Ctrl+Delete
            (key == GLFW_KEY_F9)) { // F9
            if (reset_saved_game()) {
                printf("Saved game reset successfully!\n");
                // Если мы в главном меню, обновляем текст кнопки
                if (ctx->current_screen == MENU_SCREEN) {
                    ctx->start_button.text = "START";
                }
            }
            else {
                printf("No saved game found or error resetting.\n");
            }
            return; // Выходим чтобы не обрабатывать дальше
        }
        if (key == GLFW_KEY_A) {
            if (ctx->current_screen == MENU_SCREEN ||
                ctx->current_screen == GAME_SCREEN ||
                ctx->current_screen == SETTINGS_SCREEN) {
                ctx->current_screen = ABOUT_SCREEN;
                return;
            }
        }
        // Быстрая клавиша для справки
        if (key == GLFW_KEY_H) {
            if (ctx->current_screen == MENU_SCREEN ||
                ctx->current_screen == GAME_SCREEN ||
                ctx->current_screen == SETTINGS_SCREEN) {
                ctx->current_screen = HELP_SCREEN;
                return;
            }
        }
        if (key == GLFW_KEY_ESCAPE) {
            if (ctx->current_screen == HELP_SCREEN ||
                ctx->current_screen == SETTINGS_SCREEN ||
                ctx->current_screen == GAME_OVER || ctx->current_screen == ABOUT_SCREEN) {
                ctx->current_screen = MENU_SCREEN;

                return;
            }
            else if (ctx->current_screen == GAME_SCREEN) {
                // В игровом экроме можно предложить подтверждение выхода
                ctx->current_screen = MENU_SCREEN;
                return;
            }
        }

    }

    if (ctx->current_screen == GAME_SCREEN && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_UP:
            ctx->cursor_y--;
            // Для бесконечного поля нет границ
            if (ctx->cursor_y * CELL_SIZE - ctx->view_offset_y < 0) {
                ctx->view_offset_y = ctx->cursor_y * CELL_SIZE;
            }
            break;
        case GLFW_KEY_DOWN:
            ctx->cursor_y++;
            // Для бесконечного поля нет границ
            if (ctx->cursor_y * CELL_SIZE - ctx->view_offset_y > (VISIBLE_CELLS_Y - 1) * CELL_SIZE) {
                ctx->view_offset_y = ctx->cursor_y * CELL_SIZE - (VISIBLE_CELLS_Y - 1) * CELL_SIZE;
            }
            break;
        case GLFW_KEY_LEFT:
            ctx->cursor_x--;
            // Для бесконечного поля нет границ
            if (ctx->cursor_x * CELL_SIZE - ctx->view_offset_x < 0) {
                ctx->view_offset_x = ctx->cursor_x * CELL_SIZE;
            }
            break;
        case GLFW_KEY_RIGHT:
            ctx->cursor_x++;
            // Для бесконечного поля нет границ
            if (ctx->cursor_x * CELL_SIZE - ctx->view_offset_x > (VISIBLE_CELLS_X - 1) * CELL_SIZE) {
                ctx->view_offset_x = ctx->cursor_x * CELL_SIZE - (VISIBLE_CELLS_X - 1) * CELL_SIZE;
            }
            break;
        case GLFW_KEY_SPACE:
            if (ctx->is_player_turn && get_value(ctx->board, ctx->cursor_x, ctx->cursor_y, ctx->parameters.size, ctx) == '.') {
                insert(ctx->board, ctx->cursor_x, ctx->cursor_y, ctx->parameters.player);
                bbox_on_place(&ctx->bbox, ctx->cursor_x, ctx->cursor_y);
                ctx->parameters.last_pl_x = ctx->cursor_x;
                ctx->parameters.last_pl_y = ctx->cursor_y;
                ctx->parameters.count_moves++;

                if (check_win(ctx->board, ctx->parameters.size, ctx->parameters.len, ctx->cursor_x, ctx->cursor_y, ctx->parameters.player, ctx)) {
                    ctx->winner = 1;
                    ctx->current_screen = GAME_OVER;
                }
                else if (0 == ctx->parameters.infinite_field &&
                    ctx->parameters.count_moves >= ctx->parameters.size * ctx->parameters.size) {
                    ctx->winner = 3;
                    ctx->current_screen = GAME_OVER;
                }
                else {
                    ctx->is_player_turn = false;
                }
            }
            break;
        case GLFW_KEY_Q:
            if (mods & GLFW_MOD_ALT) {
                save_game(ctx);
                ctx->current_screen = MENU_SCREEN;
            }
            break;
            // Сброс сохраненной игры из игрового экрана
        case GLFW_KEY_R:
            if (mods & GLFW_MOD_CONTROL) {
                if (reset_saved_game()) {
                    printf("Saved game reset successfully!\n");
                }
            }
            break;
        case GLFW_KEY_H:
            // H для справки из игрового экрана
            ctx->current_screen = HELP_SCREEN;
            break;
        }
    }
    else if (ctx->current_screen == MENU_SCREEN && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_S:
            ctx->current_screen = SETTINGS_SCREEN;
            break;
        case GLFW_KEY_H:
            ctx->current_screen = HELP_SCREEN;
            break;
        case GLFW_KEY_N:
            // Новая игра
            ctx->current_screen = GAME_SCREEN;
            for (unsigned long long i = 0; i < ctx->board->capacity; i++) {
                Node* current = ctx->board->buckets[i];
                while (current) {
                    Node* temp = current;
                    current = current->next;
                    free(temp);
                }
                ctx->board->buckets[i] = NULL;
            }
            ctx->parameters.count_moves = 0;
            ctx->bbox.initialized = false;
            ctx->cursor_x = ctx->cursor_y = 0;
            ctx->view_offset_x = ctx->view_offset_y = 0;
            ctx->is_player_turn = ctx->parameters.player_moves_first;
            ctx->winner = 0;
            break;
        case GLFW_KEY_L:
            if (load_game(ctx)) ctx->current_screen = GAME_SCREEN;
            break;
            // Сброс сохраненной игры из главного меню
        case GLFW_KEY_R:
            if (mods & GLFW_MOD_CONTROL) {
                if (reset_saved_game()) {
                    ctx->start_button.text = "START";
                    printf("Saved game reset successfully!\n");
                }
            }
            break;
        case GLFW_KEY_F9:
            // F9 для сброса без комбинации
            if (reset_saved_game()) {
                ctx->start_button.text = "START";
                printf("Saved game reset successfully!\n");
            }
            break;
        }
    }
    else if (ctx->current_screen == HELP_SCREEN && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            ctx->current_screen = MENU_SCREEN;
            break;
        }
    }
    else if (ctx->current_screen == SETTINGS_SCREEN && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            ctx->current_screen = MENU_SCREEN;
            break;
        }
    }
    else if (ctx->current_screen == GAME_OVER && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            ctx->current_screen = MENU_SCREEN;
            break;
        }
    }

}


void update_hover_state(GLFWwindow* window, GameContext* ctx) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (ctx->current_screen == MENU_SCREEN) {
        ctx->start_button.is_mouse = mouse_over_button(ctx->start_button, xpos, ypos);
        ctx->settings_button.is_mouse = mouse_over_button(ctx->settings_button, xpos, ypos);
        ctx->help_button.is_mouse = mouse_over_button(ctx->help_button, xpos, ypos);
        ctx->about_button.is_mouse = mouse_over_button(ctx->about_button, xpos, ypos);
    }
    else if (ctx->current_screen == ABOUT_SCREEN) {
        ctx->back_button.is_mouse = mouse_over_button(ctx->back_button, xpos, ypos);
    }
    else if (ctx->current_screen == HELP_SCREEN) {
        ctx->back_button.is_mouse = mouse_over_button(ctx->back_button, xpos, ypos);
    }
    else if (ctx->current_screen == GAME_SCREEN || ctx->current_screen == GAME_OVER) {
        ctx->back_button.is_mouse = mouse_over_button(ctx->back_button, xpos, ypos);
    }
    else if (ctx->current_screen == SETTINGS_SCREEN) {
        ctx->back_button.is_mouse = mouse_over_button(ctx->back_button, xpos, ypos);
        ctx->size_up_button.is_mouse = mouse_over_button(ctx->size_up_button, xpos, ypos);
        ctx->size_down_button.is_mouse = mouse_over_button(ctx->size_down_button, xpos, ypos);
        ctx->line_up_button.is_mouse = mouse_over_button(ctx->line_up_button, xpos, ypos);
        ctx->line_down_button.is_mouse = mouse_over_button(ctx->line_down_button, xpos, ypos);
        ctx->first_player_button.is_mouse = mouse_over_button(ctx->first_player_button, xpos, ypos);
        ctx->difficulty_button.is_mouse = mouse_over_button(ctx->difficulty_button, xpos, ypos);
        ctx->infinite_field_button.is_mouse = mouse_over_button(ctx->infinite_field_button, xpos, ypos);
    }
}



void draw_game_borders(GameContext* ctx) {
    // Для бесконечного поля не рисуем границы вообще
    if (ctx->parameters.infinite_field == 1) {
        return;
    }

    glColor3f(0.4f, 0.2f, 0.0f);
    glLineWidth(5.0f);

    // Определяем видимую область поля
    int start_x = floor((float)ctx->view_offset_x / CELL_SIZE);
    int start_y = floor((float)ctx->view_offset_y / CELL_SIZE);
    int end_x = start_x + VISIBLE_CELLS_X;
    int end_y = start_y + VISIBLE_CELLS_Y;

    // Ограничиваем видимую область размерами поля
    end_x = (end_x > ctx->parameters.size) ? ctx->parameters.size : end_x;
    end_y = (end_y > ctx->parameters.size) ? ctx->parameters.size : end_y;
    start_x = (start_x < 0) ? 0 : start_x;
    start_y = (start_y < 0) ? 0 : start_y;

    // Рисуем границы только для видимой области
    glBegin(GL_LINE_LOOP);
    float x0 = start_x * CELL_SIZE - ctx->view_offset_x;
    float y0 = start_y * CELL_SIZE - ctx->view_offset_y;
    float x1 = end_x * CELL_SIZE - ctx->view_offset_x;
    float y1 = end_y * CELL_SIZE - ctx->view_offset_y;

    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glVertex2f(x0, y1);
    glEnd();

    glLineWidth(1.0f);
}


void draw_game(GameContext* ctx) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f); // Светлый фон для игрового экрана
    draw_game_borders(ctx);
    drawgrid(ctx); // Отрисовка игровой сетки
    drawbutton(ctx, ctx->back_button); // Отрисовка кнопки "BACK"
    // Отображение текущего хода
}

void computer_move(GameContext* ctx) {
    best_move cand;
    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, true, 64, &cand, ctx);

    if (cand.n == 0) { // ничья, если нет кандидатов
        ctx->winner = 3;
        ctx->current_screen = GAME_OVER;
        return;
    }

    if (ctx->parameters.difficulty == 3 || ctx->parameters.difficulty == 4) {
        new_computer_move(ctx);
    }
    else if (ctx->parameters.difficulty == 2) {
        medium_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
    }
    else if (ctx->parameters.difficulty == 1) {
        easy_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
    }
    ctx->parameters.count_moves++;

    if (check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
        ctx->parameters.last_ai_x, ctx->parameters.last_ai_y, ctx->parameters.ai, ctx)) {
        ctx->winner = 2;
        ctx->current_screen = GAME_OVER;
    }
    else {
        generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, false, 64, &cand, ctx);
        if (cand.n == 0) { // Ничья, если нет кандидатов
            ctx->winner = 3;
            ctx->current_screen = GAME_OVER;
        }
        else {
            ctx->is_player_turn = true;
        }
    }
}

void init_game_context(GameContext* ctx) {
    ctx->current_screen = MENU_SCREEN;
    ctx->board = create_table(1024);
    ctx->parameters.size = 3;
    ctx->parameters.len = 3;
    ctx->parameters.count_moves = 0;
    ctx->parameters.last_ai_x = ctx->parameters.last_ai_y = LLONG_MAX;
    ctx->parameters.last_pl_x = ctx->parameters.last_pl_y = LLONG_MAX;
    ctx->parameters.depth = 3;
    ctx->parameters.player = 'X';
    ctx->parameters.ai = 'O';
    ctx->parameters.difficulty = 1;
    ctx->parameters.player_moves_first = true;
    ctx->parameters.infinite_field = 0;
    ctx->bbox.initialized = false;
    ctx->char_width = 15.0f;
    ctx->char_height = 20.0f;
    ctx->char_spacing = 2.0f;
    ctx->view_offset_x = 0;
    ctx->view_offset_y = 0;
    ctx->cursor_x = 0;
    ctx->cursor_y = 0;
    ctx->is_player_turn = true;
    ctx->start_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2, 200, 50, "START", false };
    ctx->settings_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 70, 200, 50, "SETTINGS", false };
    ctx->back_button = (Button){ 50, 50, 100, 40, "BACK", false };
    ctx->size_up_button = (Button){ WINDOW_WIDTH / 4 + 23, WINDOW_HEIGHT / 2 - 120, 50, 50, "+", false };
    ctx->size_down_button = (Button){ WINDOW_WIDTH / 4 - 80, WINDOW_HEIGHT / 2 - 120, 50, 50, "-", false };
    ctx->line_up_button = (Button){ WINDOW_WIDTH - 175, WINDOW_HEIGHT / 2 - 120, 50, 50, "+", false };
    ctx->line_down_button = (Button){ WINDOW_WIDTH - 275, WINDOW_HEIGHT / 2 - 120, 50, 50, "-", false };
    ctx->first_player_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 10, 200, 50, "SELECT", false };
    ctx->difficulty_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 110, 200, 50, "SELECT", false };
    ctx->infinite_field_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 220, 200, 50, "SELECT", false };
    ctx->help_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 70, 200, 50, "HELP", false };
    ctx->about_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 140, 200, 50, "ABOUT", false };
    ctx->winner = 0;
}

int main() {
    srand(time(NULL));
    setlocale(LC_ALL, "");
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Tic Tac Toe", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Инициализация GLEW ДО создания контекста
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    GameContext ctx;
    init_game_context(&ctx);
    glfwSetWindowUserPointer(window, &ctx);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Настройка OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // В главном цикле добавляем обработку нового экрана
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        update_hover_state(window, &ctx);

        if (ctx.current_screen == GAME_SCREEN && !ctx.is_player_turn && ctx.winner == 0) {
            computer_move(&ctx);
        }

        switch (ctx.current_screen) {
        case MENU_SCREEN: draw_menu(&ctx); break;
        case GAME_SCREEN: draw_game(&ctx); break;
        case SETTINGS_SCREEN: draw_settings(&ctx); break;
        case GAME_OVER: draw_game_over(&ctx); break;
        case HELP_SCREEN: draw_help(&ctx); break;
        case ABOUT_SCREEN: draw_about(&ctx); break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (unsigned long long i = 0; i < ctx.board->capacity; i++) {
        Node* current = ctx.board->buckets[i];
        while (current) {
            Node* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(ctx.board->buckets);
    free(ctx.board);
    glfwTerminate();
    return 0;
}