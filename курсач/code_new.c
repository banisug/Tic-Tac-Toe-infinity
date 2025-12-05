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


#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define CELL_SIZE 100
#define VISIBLE_CELLS_X 8
#define VISIBLE_CELLS_Y 6
#define MAX_SIZE 1000
#define MIN_SIZE 3
#define MAX_WIN_LINE 1000

// клетка
typedef struct Node {
    long long x, y;
    char value;
    struct Node* next;
} Node;

// хеш-таблица (доска)
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
    int view_offset_x; // смещение камеры по горизонтали
    int view_offset_y; // смещение камеры по вертикали
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
    Button auto_play_button; // НОВАЯ КНОПКА: автоигра
    bool auto_play_mode; // НОВОЕ: режим автоигры
    clock_t game_start_time; // НОВОЕ: время начала игры
    clock_t game_end_time; // НОВОЕ: время окончания игры
    double game_duration; // НОВОЕ: продолжительность игры в секундах
} GameContext;


// лучшие ходы
typedef struct {
    long long x[64];
    long long y[64];
    int score[64];
    int n;
} best_move;

Table* create_table(unsigned long long cap);
unsigned long long hash(long long x, long long y, unsigned long long capacity);
void insert(Table* board, long long x, long long y, char value);
void remove_cell(Table* board, long long x, long long y);
char get_value(Table* board, long long x, long long y, unsigned long long size, GameContext* ctx);
bool check_win(Table* board, unsigned long long size, unsigned long long len,
    long long x, long long y, char s, GameContext* ctx);
int line_score(Table* board, unsigned long long size, unsigned long long len,
    long long x, long long y, char s, GameContext* ctx);
int eval_heuristic(Table* board, base* parameters, bounds* bbox, GameContext* ctx);
void bbox_on_place(bounds* bbox, long long x, long long y);
void bbox_on_remove(bounds* bbox, Table* board, unsigned long long size);
void best_move_push(best_move* moves, long long x, long long y, int sc, short K);
void generate_candidates(Table* board, base* parameters, bounds* bbox, bool forAI,
    short K, best_move* out, GameContext* ctx);
bool find_immediate_move(Table* board, base* parameters, bounds* bbox, bool forAI,
    long long* bx, long long* by, GameContext* ctx);
bool find_adjacent_move(Table* board, base* parameters, bounds* bbox,
    long long* bx, long long* by, GameContext* ctx);
int minimax(Table* board, base* parameters, bounds* bbox, bool isMax,
    int alpha, int beta, short depth, GameContext* ctx);
void minimax_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx);
bool reset_saved_game();
void save_game(GameContext* ctx);
bool load_game(GameContext* ctx);
void drawcircle(float center_x, float center_y, float radius);
void drawcross(float x, float y);
void drawchar(char c, float x, float y, float scale);
void drawtext(GameContext* ctx, const char* text, float x, float y, float scale);
void drawbutton(GameContext* ctx, Button btn);
void drawgrid(GameContext* ctx);
void draw_game_borders(GameContext* ctx);
void draw_menu(GameContext* ctx);
void draw_help(GameContext* ctx);
void draw_about(GameContext* ctx);
void draw_settings(GameContext* ctx);
void draw_game_over(GameContext* ctx);
void draw_game(GameContext* ctx);
bool mouse_over_button(Button btn, double mouse_x, double mouse_y);
void get_difficulty_color(int difficulty, float* r, float* g, float* b);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void update_hover_state(GLFWwindow* window, GameContext* ctx);
void computer_move(GameContext* ctx);
void init_game_context(GameContext* ctx);
void easy_ai_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx);
void medium_minimax_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx);
void hard_ai_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx);
void expert_ai_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx);
bool quick_check_win(Table* board, unsigned long long size, unsigned long long len, long long x, long long y, char s, GameContext* ctx);
int fast_eval(Table* board, base* parameters, bounds* bbox, GameContext* ctx);
void computer_as_player_move(GameContext* ctx);


/*-------------Реализация хеш-таблицы и управления доской-------------*/

// инициализация доски
Table* create_table(unsigned long long cap) {
    Table* t = (Table*)malloc(sizeof(Table));
    t->buckets = (Node**)calloc(cap, sizeof(Node*));
    if (!t) return NULL;
    if (!t->buckets) {
        free(t);
        return NULL;
    }
    t->capacity = cap;
    return t;
}

// хеш функция для таблицы
unsigned long long hash(long long x, long long y, unsigned long long capacity) {
    unsigned long long z = (unsigned long long)x;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z = z ^ ((unsigned long long)y ^ ((unsigned long long)y >> 30));
    z += 0x9e3779b97f4a7c15ULL;
    return z % capacity;
}

// добавление крестика или нолика на доску (в хеш таблицу)
void insert(Table* board, long long x, long long y, char value) {
    if (x >= MAX_SIZE || y >= MAX_SIZE) return; // защита от переполнения
    unsigned long long index = hash(x, y, board->capacity);
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
    unsigned long long index = hash(x, y, board->capacity);
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
    unsigned long long index = hash(x, y, board->capacity);
    Node* current = board->buckets[index];
    while (current != NULL) {
        if (current->x == x && current->y == y) return current->value;
        current = current->next;
    }
    return '.';
}

/*-------------Реализация алгоритмов искусственного интеллекта-------------*/

bool check_win(Table* board, unsigned long long size, unsigned long long len,
    long long x, long long y, char s, GameContext* ctx) {
    short D[4][2] = { {1,0},{0,1},{1,1},{1,-1} };

    for (int d = 0; d < 4; ++d) {
        int dx = D[d][0], dy = D[d][1];
        unsigned long long count = 1;

        // проверяем в одном направлении
        for (int k = 1; k < len; ++k) {
            char val = get_value(board, x + dx * k, y + dy * k, size, ctx);
            if (val != s) break;
            count++;
        }

        // проверяем в противоположном направлении
        for (int k = 1; k < len; ++k) {
            char val = get_value(board, x - dx * k, y - dy * k, size, ctx);
            if (val != s) break;
            count++;
        }

        if (count >= len) return true;
    }

    return false;
}


void generate_candidates(Table* board, base* parameters, bounds* bbox,
    bool forAI, short K, best_move* out, GameContext* ctx) {
    out->n = 0;

    if (!bbox->initialized) {
        // Для начала игры
        if (get_value(board, 0, 0, parameters->size, ctx) == '.') {
            best_move_push(out, 0, 0, 0, K);
        }
        return;
    }

    // Ограничиваем поиск разумными пределами для производительности
    long long search_radius = (parameters->difficulty >= 3) ? 3 : 2;
    long long x0 = bbox->minx - search_radius;
    long long x1 = bbox->maxx + search_radius;
    long long y0 = bbox->miny - search_radius;
    long long y1 = bbox->maxy + search_radius;

    // Дополнительные ограничения для больших полей
    if (x1 - x0 > 15) {
        x1 = x0 + 15;
    }
    if (y1 - y0 > 15) {
        y1 = y0 + 15;
    }

    // Для EASY уровня используем только случайные ходы
    if (parameters->difficulty == 1) {
        // Собираем все свободные клетки
        for (long long y = y0; y <= y1; ++y) {
            for (long long x = x0; x <= x1; ++x) {
                if (get_value(board, x, y, parameters->size, ctx) == '.') {
                    best_move_push(out, x, y, 0, K);
                }
            }
        }
        return;
    }

    // Для других уровней используем эвристику
    for (long long y = y0; y <= y1; ++y) {
        for (long long x = x0; x <= x1; ++x) {
            if (get_value(board, x, y, parameters->size, ctx) != '.') continue;

            // Быстрая оценка важности клетки
            int priority = 0;

            // Клетки рядом с другими фигурами более важны
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    char neighbor = get_value(board, x + dx, y + dy, parameters->size, ctx);
                    if (neighbor == parameters->ai || neighbor == parameters->player) {
                        priority += 100;
                    }
                }
            }

            best_move_push(out, x, y, priority, K);
        }
    }
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


// поиск выигрышных ходов
bool find_immediate_move(Table* board, base* parameters, bounds* bbox, bool forAI, long long* bx, long long* by, GameContext* ctx) {
    best_move cand;
    short K;
    if (parameters->infinite_field == 1) {
        K = 200;
    }
    else {
        K = 64;
    }
    generate_candidates(board, parameters, bbox, forAI, K, &cand, ctx);
    char me = forAI ? parameters->ai : parameters->player;
    for (int i = 0; i < cand.n; ++i) {
        long long x = cand.x[i], y = cand.y[i];
        insert(board, x, y, me);
        bool win = check_win(board, parameters->size, parameters->len, x, y, me, ctx);
        remove_cell(board, x, y);
        if (win) {
            *bx = x;
            *by = y;
            return true;
        }
    }
    return false;
}

// функция для проверки соседних клеток вокруг последнего хода игрока
bool find_adjacent_move(Table* board, base* parameters, bounds* bbox, long long* bx, long long* by, GameContext* ctx) {
    if (parameters->last_pl_x == LLONG_MAX || parameters->last_pl_y == LLONG_MAX)
        return false;

    int directions[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,1}, {1,-1}, {1,1}, {-1,-1} };

    for (int i = 0; i < 8; i++) {
        long long nx = parameters->last_pl_x + directions[i][0];
        long long ny = parameters->last_pl_y + directions[i][1];

        if (parameters->infinite_field == 0 &&
            (nx < 0 || nx >= parameters->size || ny < 0 || ny >= parameters->size)) {
            continue;
        }

        if (get_value(board, nx, ny, parameters->size, ctx) == '.') {
            *bx = nx;
            *by = ny;
            return true;
        }
    }
    return false;
}

// минимакс
int minimax(Table* board, base* parameters, bounds* bbox, bool isMax, int alpha, int beta, short depth, GameContext* ctx) {
    if (parameters->last_ai_x != LLONG_MAX &&
        check_win(board, parameters->size, parameters->len, parameters->last_ai_x, parameters->last_ai_y, parameters->ai, ctx)) {
        return 100000 - (int)parameters->count_moves;
    }
    if (parameters->last_pl_x != LLONG_MAX &&
        check_win(board, parameters->size, parameters->len, parameters->last_pl_x, parameters->last_pl_y, parameters->player, ctx)) {
        return -100000 + (int)parameters->count_moves;
    }
    if (depth <= 0) return eval_heuristic(board, parameters, bbox, ctx);
    int K = (depth >= 2 ? 24 : 16);
    best_move tk;
    generate_candidates(board, parameters, bbox, isMax, K, &tk, ctx);
    if (tk.n == 0) return 0;
    char me = isMax ? parameters->ai : parameters->player;
    for (int i = 0; i < tk.n; ++i) {
        long long x = tk.x[i], y = tk.y[i];
        insert(board, x, y, me);
        bool win = check_win(board, parameters->size, parameters->len, x, y, me, ctx);
        remove_cell(board, x, y);
        if (win) {
            if (isMax) return 100000 - (int)parameters->count_moves;
            else return -100000 + (int)parameters->count_moves;
        }
    }
    if (isMax) {
        int best = INT_MIN;
        for (int i = 0; i < tk.n && best < beta; ++i) {
            long long x = tk.x[i], y = tk.y[i];
            insert(board, x, y, parameters->ai);
            bbox_on_place(bbox, x, y);
            long long saved_ai_x = parameters->last_ai_x, saved_ai_y = parameters->last_ai_y;
            long long saved_pl_x = parameters->last_pl_x, saved_pl_y = parameters->last_pl_y;
            parameters->last_ai_x = x;
            parameters->last_ai_y = y;
            parameters->count_moves++;
            int val = minimax(board, parameters, bbox, false, alpha, beta, depth - 1, ctx);
            remove_cell(board, x, y);
            bbox_on_remove(bbox, board, parameters->size);
            parameters->last_ai_x = saved_ai_x;
            parameters->last_ai_y = saved_ai_y;
            parameters->last_pl_x = saved_pl_x;
            parameters->last_pl_y = saved_pl_y;
            parameters->count_moves--;
            if (val > best) best = val;
            if (best > alpha) alpha = best;
            if (alpha >= beta) break;
        }
        return best;
    }
    else {
        int best = INT_MAX;
        for (int i = 0; i < tk.n && best > alpha; ++i) {
            long long x = tk.x[i], y = tk.y[i];
            insert(board, x, y, parameters->player);
            bbox_on_place(bbox, x, y);
            long long saved_ai_x = parameters->last_ai_x, saved_ai_y = parameters->last_ai_y;
            long long saved_pl_x = parameters->last_pl_x, saved_pl_y = parameters->last_pl_y;
            parameters->last_pl_x = x;
            parameters->last_pl_y = y;
            parameters->count_moves++;
            int val = minimax(board, parameters, bbox, true, alpha, beta, depth - 1, ctx);
            remove_cell(board, x, y);
            bbox_on_remove(bbox, board, parameters->size);
            parameters->last_ai_x = saved_ai_x;
            parameters->last_ai_y = saved_ai_y;
            parameters->last_pl_x = saved_pl_x;
            parameters->last_pl_y = saved_pl_y;
            parameters->count_moves--;
            if (val < best) best = val;
            if (best < beta) beta = best;
            if (alpha >= beta) break;
        }
        return best;
    }
}

bool quick_check_win(Table* board, unsigned long long size, unsigned long long len,
    long long x, long long y, char s, GameContext* ctx) {
    short D[4][2] = { {1,0},{0,1},{1,1},{1,-1} };

    for (int d = 0; d < 4; ++d) {
        int dx = D[d][0], dy = D[d][1];
        unsigned long long count = 1;

        // Проверяем только len клеток в каждом направлении
        for (int k = 1; k < len; ++k) {
            char val = get_value(board, x + dx * k, y + dy * k, size, ctx);
            if (val != s) break;
            count++;
        }

        for (int k = 1; k < len; ++k) {
            char val = get_value(board, x - dx * k, y - dy * k, size, ctx);
            if (val != s) break;
            count++;
        }

        if (count >= len) return true;
    }

    return false;
}

// Оптимизированная оценка позиции
int fast_eval(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    if (!bbox->initialized) return 0;

    int score = 0;
    // Проверяем только ключевые клетки вокруг занятых
    for (long long y = bbox->miny - 1; y <= bbox->maxy + 1; y++) {
        for (long long x = bbox->minx - 1; x <= bbox->maxx + 1; x++) {
            char cell = get_value(board, x, y, parameters->size, ctx);
            if (cell == parameters->ai) score += 10;
            else if (cell == parameters->player) score -= 10;
        }
    }
    return score;
}

void minimax_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    long long bx, by;
    if (find_immediate_move(board, parameters, bbox, true, &bx, &by, ctx)) {
        insert(board, bx, by, parameters->ai);
        bbox_on_place(bbox, bx, by);
        parameters->last_ai_x = bx;
        parameters->last_ai_y = by;
        return;
    }
    if (find_immediate_move(board, parameters, bbox, false, &bx, &by, ctx)) {
        insert(board, bx, by, parameters->ai);
        bbox_on_place(bbox, bx, by);
        parameters->last_ai_x = bx;
        parameters->last_ai_y = by;
        return;
    }

    if (parameters->len == 3 && parameters->size > 4 && parameters->difficulty > 2) {
        if (find_adjacent_move(board, parameters, bbox, &bx, &by, ctx)) {
            insert(board, bx, by, parameters->ai);
            bbox_on_place(bbox, bx, by);
            parameters->last_ai_x = bx;
            parameters->last_ai_y = by;
            return;
        }
    }

    int bestVal = INT_MIN;
    long long bestX = -1, bestY = -1;
    best_move tk;
    generate_candidates(board, parameters, bbox, true, 32, &tk, ctx);
    int depth = 2;
    if (ctx->parameters.difficulty == 4) depth = 5;
    if (ctx->parameters.difficulty == 3) depth = 4;
    if (ctx->parameters.difficulty == 2) depth = 3;

    if (parameters->infinite_field == 0 && parameters->size == 3) depth += 2;
    else if (parameters->infinite_field == 0 && parameters->size == 4) depth++;

    for (int i = 0; i < tk.n; ++i) {
        long long x = tk.x[i], y = tk.y[i];
        insert(board, x, y, parameters->ai);
        bbox_on_place(bbox, x, y);
        long long saved_ai_x = parameters->last_ai_x, saved_ai_y = parameters->last_ai_y;
        long long saved_pl_x = parameters->last_pl_x, saved_pl_y = parameters->last_pl_y;
        parameters->last_ai_x = x;
        parameters->last_ai_y = y;
        parameters->count_moves++;
        int val = minimax(board, parameters, bbox, false, INT_MIN, INT_MAX, depth, ctx);
        remove_cell(board, x, y);
        bbox_on_remove(bbox, board, parameters->size);
        parameters->last_ai_x = saved_ai_x;
        parameters->last_ai_y = saved_ai_y;
        parameters->last_pl_x = saved_pl_x;
        parameters->last_pl_y = saved_pl_y;
        parameters->count_moves--;
        if (val > bestVal) {
            bestVal = val;
            bestX = x;
            bestY = y;
        }
    }
    if (bestX != -1) {
        insert(board, bestX, bestY, parameters->ai);
        bbox_on_place(bbox, bestX, bestY);
        parameters->last_ai_x = bestX;
        parameters->last_ai_y = bestY;
    }
}

void easy_ai_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    best_move cand;
    generate_candidates(board, parameters, bbox, true, 64, &cand, ctx);

    if (cand.n > 0) {
        // Рандомный выбор из доступных ходов
        int random_index = rand() % cand.n;
        long long x = cand.x[random_index];
        long long y = cand.y[random_index];

        insert(board, x, y, parameters->ai);
        bbox_on_place(bbox, x, y);
        parameters->last_ai_x = x;
        parameters->last_ai_y = y;
    }
}

// Функция для HARD уровня (блокировка ходов)
void hard_ai_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    long long bx, by;

    // 1. Проверка выигрышного хода для ИИ
    if (find_immediate_move(board, parameters, bbox, true, &bx, &by, ctx)) {
        insert(board, bx, by, parameters->ai);
        bbox_on_place(bbox, bx, by);
        parameters->last_ai_x = bx;
        parameters->last_ai_y = by;
        return;
    }

    // 2. Проверка выигрышного хода для игрока (блокировка)
    if (find_immediate_move(board, parameters, bbox, false, &bx, &by, ctx)) {
        insert(board, bx, by, parameters->ai);
        bbox_on_place(bbox, bx, by);
        parameters->last_ai_x = bx;
        parameters->last_ai_y = by;
        return;
    }

    // 3. Если нет срочных ходов - используем мини-макс с меньшей глубиной
    medium_minimax_move(board, parameters, bbox, ctx); // глубина 2 для скорости
}

// Функция для EXPERT уровня (блокировка приоритетна)
void expert_ai_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    long long bx, by;

    // 1. Сначала ищем ход для блокировки игрока (приоритет)
    if (find_immediate_move(board, parameters, bbox, false, &bx, &by, ctx)) {
        insert(board, bx, by, parameters->ai);
        bbox_on_place(bbox, bx, by);
        parameters->last_ai_x = bx;
        parameters->last_ai_y = by;
        return;
    }

    // 2. Затем ищем свой выигрышный ход
    if (find_immediate_move(board, parameters, bbox, true, &bx, &by, ctx)) {
        insert(board, bx, by, parameters->ai);
        bbox_on_place(bbox, bx, by);
        parameters->last_ai_x = bx;
        parameters->last_ai_y = by;
        return;
    }

    // 3. Если нет, то используем продвинутый мини-макс
    medium_minimax_move(board, parameters, bbox, ctx);
}

// Оптимизированный мини-макс для MEDIUM уровня
void medium_minimax_move(Table* board, base* parameters, bounds* bbox, GameContext* ctx) {
    // Упрощенный мини-макс с эвристикой и ограниченной глубиной
    long long bx, by;

    // Проверка выигрышных ходов
    if (find_immediate_move(board, parameters, bbox, true, &bx, &by, ctx) ||
        find_immediate_move(board, parameters, bbox, false, &bx, &by, ctx)) {
        insert(board, bx, by, parameters->ai);
        bbox_on_place(bbox, bx, by);
        parameters->last_ai_x = bx;
        parameters->last_ai_y = by;
        return;
    }

    // Эвристический выбор лучшего хода без глубокого поиска
    best_move cand;
    generate_candidates(board, parameters, bbox, true, 16, &cand, ctx); // меньше кандидатов

    if (cand.n > 0) {
        // Выбираем лучший ход по эвристике
        insert(board, cand.x[0], cand.y[0], parameters->ai);
        bbox_on_place(bbox, cand.x[0], cand.y[0]);
        parameters->last_ai_x = cand.x[0];
        parameters->last_ai_y = cand.y[0];
    }
}

/*-------------Реализация сохранения и загрузки-------------*/

// сброс сохраненной игры
bool reset_saved_game() {
    FILE* file = fopen("save.dat", "rb");
    if (file) {
        fclose(file);
        // если файл существует, удаляем его
        if (remove("save.dat") == 0) {
            return true;
        }
    }
    return false; // файла не существует или не удалось удалить
}

void save_game(GameContext* ctx) {
    FILE* file = fopen("save.dat", "wb");
    if (file) {
        // записывает данные игры
        fwrite(&ctx->parameters, sizeof(base), 1, file);
        fwrite(&ctx->bbox, sizeof(bounds), 1, file);

        fwrite(&ctx->parameters.count_moves, sizeof(unsigned long long), 1, file);

        // записывает каждый узел
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
        // очищаем текущую доску
        for (unsigned long long i = 0; i < ctx->board->capacity; i++) {
            Node* current = ctx->board->buckets[i];
            while (current) {
                Node* temp = current;
                current = current->next;
                free(temp);
            }
            ctx->board->buckets[i] = NULL;
        }

        // читаем параметры и границы
        if (fread(&ctx->parameters, sizeof(base), 1, file) != 1) {
            fclose(file);
            return false;
        }
        if (fread(&ctx->bbox, sizeof(bounds), 1, file) != 1) {
            fclose(file);
            return false;
        }

        // читаем количество ходов
        unsigned long long count_moves;
        if (fread(&count_moves, sizeof(unsigned long long), 1, file) != 1) {
            fclose(file);
            return false;
        }
        ctx->parameters.count_moves = count_moves;

        // читаем клетки
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

void computer_as_player_move(GameContext* ctx) {
    // Используем тот же алгоритм, что и для ИИ, но с символом игрока
    Table* board = ctx->board;
    base* parameters = &ctx->parameters;
    bounds* bbox = &ctx->bbox;

    // Сохраняем оригинальные символы
    char original_player = parameters->player;
    char original_ai = parameters->ai;

    // Временно меняем символы для AI функции
    parameters->player = 'X';
    parameters->ai = 'X'; // Для AI функции нужно, чтобы ai был тем, кем ходит

    // В зависимости от сложности выбираем алгоритм
    switch (ctx->parameters.difficulty) {
    case 1: // EASY
        easy_ai_move(board, parameters, bbox, ctx);
        break;
    case 2: // MEDIUM
        medium_minimax_move(board, parameters, bbox, ctx);
        break;
    case 3: // HARD
        hard_ai_move(board, parameters, bbox, ctx);
        break;
    case 4: // EXPERT
        expert_ai_move(board, parameters, bbox, ctx);
        break;
    }

    // Восстанавливаем символы
    parameters->player = original_player;
    parameters->ai = original_ai;

    // Обновляем последний ход игрока
    parameters->last_pl_x = parameters->last_ai_x;
    parameters->last_pl_y = parameters->last_ai_y;
    parameters->count_moves++;

    // Проверка победы игрока X
    if (check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
        ctx->parameters.last_pl_x, ctx->parameters.last_pl_y, 'X', ctx)) {
        ctx->winner = 1; // Игрок X выиграл
        ctx->game_end_time = clock();
        ctx->game_duration = (double)(ctx->game_end_time - ctx->game_start_time) / CLOCKS_PER_SEC;
        ctx->current_screen = GAME_OVER;
        return;
    }

    // Проверка ничьей
    best_move cand;
    generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, false, 1, &cand, ctx);
    if (cand.n == 0) {
        ctx->winner = 3;
        ctx->game_end_time = clock();
        ctx->game_duration = (double)(ctx->game_end_time - ctx->game_start_time) / CLOCKS_PER_SEC;
        ctx->current_screen = GAME_OVER;
        return;
    }
}

/*-------------Рендеринг-------------*/

void drawcircle(float center_x, float center_y, float radius) {
    int segments = 45; // количество отрезков из которых состоит окружность.
    glColor3f(0.0f, 0.0f, 1.0f); // синий
    glLineWidth(3.0f); // тощина линии 3 пикселя
    glBegin(GL_LINE_LOOP); // начало рисования линии
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * 3.1415926f * (float)i / (float)segments; // угол
        glVertex2f(center_x + radius * cosf(angle), center_y + radius * sinf(angle)); // определение вершины
    }
    glEnd(); // завершение рисования
}

// крестик
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
    glPushMatrix(); // сохраняет текущую систему координат в стек
    glTranslatef(x, y, 0); // смещение для отрисовки, чтобы (0,0) был в точке (x,y) где нужно рисовать символ
    glScalef(scale, scale, 1.0f); // масштабирование символа
    switch (toupper(c)) { // отрисовка каждого символа
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
        glVertex2f(2, 1); glVertex2f(7.5f, 10); // левая часть V (сверху вниз)
        glVertex2f(13, 1); // правая часть V
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(7.5f, 10); glVertex2f(7.5f, 20); // стержень (вниз)
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
        glVertex2f(2, 20);    // левая верхняя точка
        glVertex2f(2, 2);     // левая нижняя точка
        glVertex2f(7, 12);    // первый центральный изгиб (вверх)
        glVertex2f(12, 2);    // правая нижняя точка
        glVertex2f(12, 20);   // правая верхняя точка
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
        glVertex2f(2, 2);    // левая верхняя точка
        glVertex2f(7.5f, 19); // центральная нижняя точка
        glVertex2f(13, 2);   // правая верхняя точка
        glEnd();
        break;
    case 'Q':
        // основной круг
        glBegin(GL_LINE_LOOP);
        glVertex2f(5, 2); glVertex2f(10, 2);
        glVertex2f(13, 5); glVertex2f(13, 15);
        glVertex2f(10, 19); glVertex2f(5, 19);
        glVertex2f(2, 15); glVertex2f(2, 5);
        glEnd();
        // хвостик буквы Q
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
        glVertex2f(5, 18); glVertex2f(10, 18); // верхняя линия
        glVertex2f(13, 15); glVertex2f(13, 5); // правая сторона
        glVertex2f(10, 1); glVertex2f(5, 1); // нижняя линия
        glVertex2f(2, 5); glVertex2f(2, 15); // левая сторона
        glEnd();
        break;
    case '1':
        glBegin(GL_LINES);
        glVertex2f(7.5f, 18); glVertex2f(7.5f, 0); // вертикальная линия
        glVertex2f(5, 18); glVertex2f(10, 18); // верхняя перекладина
        glVertex2f(5, 0); glVertex2f(10, 0); // вижняя перекладина
        glEnd();
        break;
    case '2':
        glBegin(GL_LINE_STRIP);
        glVertex2f(13, 18); glVertex2f(2, 18); // верхняя линия
        glVertex2f(2, 10); glVertex2f(13, 10); // средняя линия
        glVertex2f(13, 1); glVertex2f(2, 1); // ижняя линия
        glEnd();
        break;
    case '3':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 18); glVertex2f(13, 18); // верхняя линия
        glVertex2f(13, 10); glVertex2f(2, 10); // вредняя линия
        glVertex2f(13, 1); glVertex2f(2, 1); // нижняя линия
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
        glVertex2f(1, 18); glVertex2f(14, 18); // верхняя линия
        glVertex2f(14, 10); glVertex2f(1, 10); // средняя линия
        glVertex2f(1, 1); glVertex2f(14, 1); // нижняя линия
        glEnd();
        break;
    case '6':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 18); glVertex2f(14, 18); // верхняя линия
        glVertex2f(14, 10); glVertex2f(1, 10); // средняя линия
        glVertex2f(1, 1); glVertex2f(14, 1); // нижняя линия
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(1, 18);  glVertex2f(1, 10);
        glEnd();
        break;
    case '7':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 1); glVertex2f(14, 1); // верхняя линия
        glVertex2f(1, 19); // диагональ
        glEnd();
        break;
    case '8':
        glBegin(GL_LINE_LOOP);
        glVertex2f(5, 18); glVertex2f(10, 18); // верхняя линия (верхний контур)
        glVertex2f(13, 15); glVertex2f(13, 13); // правая сторона (верхний контур)
        glVertex2f(10, 10); glVertex2f(5, 10); // средняя линия
        glVertex2f(2, 13); glVertex2f(2, 15); // левая сторона (верхний контур)
        glEnd();
        glBegin(GL_LINE_LOOP);
        glVertex2f(5, 10); glVertex2f(10, 10); // верхняя линия (нижний контур)
        glVertex2f(13, 7); glVertex2f(13, 5); // правая сторона (нижний контур)
        glVertex2f(10, 1); glVertex2f(5, 1); // нижняя линия
        glVertex2f(2, 5); glVertex2f(2, 7); // левая сторона (нижний контур)
        glEnd();
        break;
    case '9':
        glBegin(GL_LINE_STRIP);
        glVertex2f(1, 18); glVertex2f(14, 18); // верхняя линия
        glVertex2f(14, 10); glVertex2f(1, 10); // средняя линия
        glVertex2f(1, 1); glVertex2f(14, 1); // нижняя линия
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
        glVertex2f(13, 2);   // правая верхняя точка
        glVertex2f(2, 19);   // левая нижняя точка
        glEnd();
    case ',': // Запятая
        glBegin(GL_POINTS);
        glVertex2f(6, 19); // Точка
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex2f(6, 19); // Начинаем от точки
        glVertex2f(7.5, 15);    // Хвостик влево-вниз
        glEnd();
        break;
    case ':': // Двоеточие (круги)
        glPointSize(4.0f);
        glBegin(GL_POINTS);
        // Верхняя точка
        glVertex2f(7.5f, 4);
        // Нижняя точка
        glVertex2f(7.5f, 17);
        glEnd();
        glPointSize(1.0f); // Восстанавливаем размер
        break;
    }



    glPopMatrix(); // восстанавливает предыдущую систему координат
}

// рендеринг
void drawtext(GameContext* ctx, const char* text, float x, float y, float scale) {
    float total_width = 0;

    // вычисление общей шири ны текста
    for (const char* c = text; *c != '\0'; c++) {
        total_width += (*c == ' ') ? 15 * scale : ctx->char_width * scale;
    }

    // вычисление начальной позиции (центрирование)
    float start_x = x - total_width / 2;
    float start_y = y - ctx->char_height / 2 * scale;

    // посимвольная отрисовка
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == ' ') start_x += 15 * scale;
        else {
            drawchar(*c, start_x, start_y, scale);
            start_x += ctx->char_width * scale;
        }
    }
}

void drawbutton(GameContext* ctx, Button btn) {
    // фон кнопки
    glColor3f(btn.is_mouse ? 0.0f : 0.7f, btn.is_mouse ? 0.5f : 0.7f, btn.is_mouse ? 0.5f : 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(btn.x, btn.y);
    glVertex2f(btn.x + btn.width, btn.y);
    glVertex2f(btn.x + btn.width, btn.y + btn.height);
    glVertex2f(btn.x, btn.y + btn.height);
    glEnd();
    // рамка кнопки
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(btn.x, btn.y);
    glVertex2f(btn.x + btn.width, btn.y);
    glVertex2f(btn.x + btn.width, btn.y + btn.height);
    glVertex2f(btn.x, btn.y + btn.height);
    glEnd();
    // текст кнопки
    drawtext(ctx, btn.text, btn.x + btn.width / 2, btn.y + btn.height / 2, 0.7f);
}

// сетка
void drawgrid(GameContext* ctx) {
    glColor3f(0.0f, 0.1f, 0.1f);
    glLineWidth(1.0f);

    // вычиление видимой области
    long long start_x = floor((float)ctx->view_offset_x / CELL_SIZE);
    long long start_y = floor((float)ctx->view_offset_y / CELL_SIZE);

    // отрисовка вертикальных линий
    for (long long x = start_x; x <= start_x + VISIBLE_CELLS_X + 1; x++) {
        float screen_x = x * CELL_SIZE - ctx->view_offset_x;
        glBegin(GL_LINES);
        glVertex2f(screen_x, 0);
        glVertex2f(screen_x, VISIBLE_CELLS_Y * CELL_SIZE);
        glEnd();
    }

    // отрисовка горизонтальных линий
    for (long long y = start_y; y <= start_y + VISIBLE_CELLS_Y + 1; y++) {
        float screen_y = y * CELL_SIZE - ctx->view_offset_y;
        glBegin(GL_LINES);
        glVertex2f(0, screen_y);
        glVertex2f(VISIBLE_CELLS_X * CELL_SIZE, screen_y);
        glEnd();
    }

    // отрисовываем все видимые клетки без проверки границ
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

    // курсор (без проверки границ)
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
    // очистка экрана и установка фона
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    drawtext(ctx, "TIC TAC TOE", WINDOW_WIDTH / 2, 80, 1.5f);

    // проверяем существует ли сохранение
    bool save_exists = false;
    FILE* test_file = fopen("save.dat", "rb");
    if (test_file) {
        save_exists = true;
        fclose(test_file);
    }

    ctx->start_button.text = save_exists ? "CONTINUE" : "START";

    // располагаем кнопки вертикально с отступами
    ctx->start_button.y = WINDOW_HEIGHT / 2 - 100;
    ctx->auto_play_button.y = WINDOW_HEIGHT / 2 - 40; // Новая кнопка
    ctx->settings_button.y = WINDOW_HEIGHT / 2 + 20;
    ctx->help_button.y = WINDOW_HEIGHT / 2 + 80;
    ctx->about_button.y = WINDOW_HEIGHT / 2 + 140;

    drawbutton(ctx, ctx->start_button);
    drawbutton(ctx, ctx->settings_button);
    drawbutton(ctx, ctx->auto_play_button);
    drawbutton(ctx, ctx->help_button);
    drawbutton(ctx, ctx->about_button);
}

void draw_help(GameContext* ctx) {
    // очистка экрана и установка фона
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.8f, 0.9f, 0.95f, 1.0f);

    drawtext(ctx, "GAME HELP", WINDOW_WIDTH / 2, 60, 1.5f);

    // управление в игре
    glColor3f(0.3f, 0.3f, 0.3f);
    drawtext(ctx, "GAME CONTROLS:", WINDOW_WIDTH / 2, 120, 0.8f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawtext(ctx, "Arrow Keys - Move cursor", WINDOW_WIDTH / 2, 150, 0.6f);
    drawtext(ctx, "Space - Make move", WINDOW_WIDTH / 2, 170, 0.6f);
    drawtext(ctx, "Alt+Q - Save and exit to menu", WINDOW_WIDTH / 2, 190, 0.6f);
    drawtext(ctx, "Ctrl+R or F9 - Reset saved game", WINDOW_WIDTH / 2, 210, 0.6f);
    drawtext(ctx, "ESC - return to the menu", WINDOW_WIDTH / 2, 230, 0.6f);

    // главное меню
    glColor3f(0.3f, 0.3f, 0.3f);
    drawtext(ctx, "MAIN MENU KEYS:", WINDOW_WIDTH / 2, 270, 0.8f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawtext(ctx, "N - New game", WINDOW_WIDTH / 2, 290, 0.6f);
    drawtext(ctx, "S - Settings", WINDOW_WIDTH / 2, 310, 0.6f);
    drawtext(ctx, "H - open the help", WINDOW_WIDTH / 2, 330, 0.6f);
    drawtext(ctx, "A - open the about", WINDOW_WIDTH / 2, 350, 0.6f);

    // особенности
    glColor3f(0.3f, 0.3f, 0.3f);
    drawtext(ctx, "FEATURES:", WINDOW_WIDTH / 2, 390, 0.8f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawtext(ctx, "4 difficulty levels (Easy to Expert)", WINDOW_WIDTH / 2, 410, 0.6f);
    drawtext(ctx, "Support for large boards", WINDOW_WIDTH / 2, 430, 0.6f);
    drawtext(ctx, "Infinite field mode for unlimited gameplay", WINDOW_WIDTH / 2, 450, 0.6f);

    // правила
    glColor3f(0.3f, 0.3f, 0.3f);
    drawtext(ctx, "RULES:", WINDOW_WIDTH / 2, 490, 0.8f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawtext(ctx, "Get X or O in a row to win", WINDOW_WIDTH / 2, 510, 0.6f);
    drawtext(ctx, "Lines can be horizontal vertical or diagonal", WINDOW_WIDTH / 2, 530, 0.6f);

    drawbutton(ctx, ctx->back_button);
}

void draw_about(GameContext* ctx) {
    // очистка экрана и установка фона
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.85f, 0.90f, 0.95f, 1.0f);

    drawtext(ctx, "ABOUT", WINDOW_WIDTH / 2, 80, 1.5f);

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
    // очистка экрана и установка фона
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.8f, 0.9f, 0.9f, 1.0f);
    drawtext(ctx, "SETTINGS", WINDOW_WIDTH / 2, 90, 1.5f);

    char buffer[50];

    // настройка размера поля
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

    // настройка длиня линии
    glColor3f(0.0f, 0.0f, 0.0f);
    snprintf(buffer, sizeof(buffer), "WIN LINE: %d", (int)ctx->parameters.len);
    drawtext(ctx, buffer, WINDOW_WIDTH - 200, WINDOW_HEIGHT / 2 - 140, 0.9f);
    drawbutton(ctx, ctx->line_up_button);
    drawbutton(ctx, ctx->line_down_button);

    // настройка первого игрока и его символа
    glColor3f(0.0f, 0.0f, 0.0f);
    snprintf(buffer, sizeof(buffer), "FIRST PLAYER: %s",
        ctx->parameters.player_moves_first ? (ctx->parameters.player == 'X' ? "PLAYER X" : "PLAYER O") : (ctx->parameters.ai == 'X' ? "COMPUTER X" : "COMPUTER O"));
    drawtext(ctx, buffer, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 5, 0.9f);
    drawbutton(ctx, ctx->first_player_button);

    // настройка уровня
    float r, g, b;
    get_difficulty_color(ctx->parameters.difficulty, &r, &g, &b);
    glColor3f(r, g, b);
    snprintf(buffer, sizeof(buffer), "DIFFICULTY: %s",
        ctx->parameters.difficulty == 1 ? "EASY" :
        ctx->parameters.difficulty == 2 ? "MEDIUM" :
        ctx->parameters.difficulty == 3 ? "HARD" : "EXPERT");
    drawtext(ctx, buffer, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 95, 0.9f);
    drawbutton(ctx, ctx->difficulty_button);

    // настройка бесконечного поля
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
        if (ctx->auto_play_mode) {
            message = "COMPUTER X WINS!";
        }
        else {
            message = ctx->parameters.player == 'X' ? "PLAYER X WINS!" : "PLAYER O WINS!";
        }
    }
    else if (ctx->winner == 2) {
        if (ctx->auto_play_mode) {
            message = "COMPUTER O WINS!";
        }
        else {
            message = ctx->parameters.ai == 'X' ? "COMPUTER X WINS!" : "COMPUTER O WINS!";
        }
    }
    else if (ctx->winner == 3) {
        message = "DRAW!";
    }
    else {
        message = "GAME OVER";
    }

    drawtext(ctx, message, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 60, 2.0f);

    // Отображаем время игры
    char time_buffer[100];
    if (ctx->game_duration > 0) {
        snprintf(time_buffer, sizeof(time_buffer), "Time: %.3f seconds", ctx->game_duration);
        drawtext(ctx, time_buffer, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 20, 1.0f);

        // Для автоигры показываем дополнительную информацию
        if (ctx->auto_play_mode) {
            char moves_buffer[100];
            snprintf(moves_buffer, sizeof(moves_buffer), "Moves: %llu", ctx->parameters.count_moves);
            drawtext(ctx, moves_buffer, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 60, 0.8f);
        }
    }

    drawbutton(ctx, ctx->back_button);
}

bool mouse_over_button(Button btn, double mouse_x, double mouse_y) {
    return (mouse_x >= btn.x && mouse_x <= btn.x + btn.width &&
        mouse_y >= btn.y && mouse_y <= btn.y + btn.height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    GameContext* ctx = (GameContext*)glfwGetWindowUserPointer(window);

    // если не левая кнопка мыши и не нажатие, то игнорим
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos); // получаем координаты курсора

    if (ctx->current_screen == MENU_SCREEN) {
        if (mouse_over_button(ctx->start_button, xpos, ypos)) {
            if (load_game(ctx)) {
                ctx->current_screen = GAME_SCREEN;
                ctx->game_start_time = clock();
            }
            else {
                // новая игра
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
                ctx->auto_play_mode = false; // Обычная игра
                ctx->game_start_time = clock();
            }
        }
        else if (mouse_over_button(ctx->auto_play_button, xpos, ypos)) {
            // Запуск автоигры
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
            ctx->is_player_turn = true; // X ходит первым
            ctx->winner = 0;
            ctx->auto_play_mode = true; // Включаем режим автоигры
            ctx->game_start_time = clock();

            // В автоигре всегда X vs O
            ctx->parameters.player = 'X';
            ctx->parameters.ai = 'O';
            ctx->parameters.player_moves_first = true;
        }
        else if (mouse_over_button(ctx->settings_button, xpos, ypos)) {
            ctx->current_screen = SETTINGS_SCREEN;
        }
        else if (mouse_over_button(ctx->help_button, xpos, ypos)) {
            ctx->current_screen = HELP_SCREEN;
        }
        else if (mouse_over_button(ctx->about_button, xpos, ypos)) {
            ctx->current_screen = ABOUT_SCREEN;
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
                // в игровом экроме можно предложить подтверждение выхода
                ctx->current_screen = MENU_SCREEN;
                return;
            }
        }

    }

    if (ctx->current_screen == GAME_SCREEN && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_UP:
            ctx->cursor_y--;
            if (ctx->cursor_y * CELL_SIZE - ctx->view_offset_y < 0) {
                ctx->view_offset_y = ctx->cursor_y * CELL_SIZE;
            }
            break;
        case GLFW_KEY_DOWN:
            ctx->cursor_y++;
            if (ctx->cursor_y * CELL_SIZE - ctx->view_offset_y > (VISIBLE_CELLS_Y - 1) * CELL_SIZE) {
                ctx->view_offset_y = ctx->cursor_y * CELL_SIZE - (VISIBLE_CELLS_Y - 1) * CELL_SIZE;
            }
            break;
        case GLFW_KEY_LEFT:
            ctx->cursor_x--;
            if (ctx->cursor_x * CELL_SIZE - ctx->view_offset_x < 0) {
                ctx->view_offset_x = ctx->cursor_x * CELL_SIZE;
            }
            break;
        case GLFW_KEY_RIGHT:
            ctx->cursor_x++;
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
        }
    }
    else if (ctx->current_screen == MENU_SCREEN && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_S:
            ctx->current_screen = SETTINGS_SCREEN;
            break;
        case GLFW_KEY_N:
            // новая игра
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
        }
    }
}

// для подсветки кнопок
void update_hover_state(GLFWwindow* window, GameContext* ctx) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (ctx->current_screen == MENU_SCREEN) {
        ctx->start_button.is_mouse = mouse_over_button(ctx->start_button, xpos, ypos);
        ctx->settings_button.is_mouse = mouse_over_button(ctx->settings_button, xpos, ypos);
        ctx->help_button.is_mouse = mouse_over_button(ctx->help_button, xpos, ypos);
        ctx->about_button.is_mouse = mouse_over_button(ctx->about_button, xpos, ypos);
        ctx->auto_play_button.is_mouse = mouse_over_button(ctx->auto_play_button, xpos, ypos);
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
    // для бесконечного поля не рисуем границы вообще
    if (ctx->parameters.infinite_field == 1) {
        return;
    }

    glColor3f(0.4f, 0.2f, 0.0f);
    glLineWidth(5.0f);

    // определяем видимую область поля
    int start_x = floor((float)ctx->view_offset_x / CELL_SIZE);
    int start_y = floor((float)ctx->view_offset_y / CELL_SIZE);
    int end_x = start_x + VISIBLE_CELLS_X;
    int end_y = start_y + VISIBLE_CELLS_Y;

    // ограничиваем видимую область размерами поля
    end_x = (end_x > ctx->parameters.size) ? ctx->parameters.size : end_x;
    end_y = (end_y > ctx->parameters.size) ? ctx->parameters.size : end_y;
    start_x = (start_x < 0) ? 0 : start_x;
    start_y = (start_y < 0) ? 0 : start_y;

    // рисуем границы только для видимой области
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
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f); // светлый фон для игрового экрана
    draw_game_borders(ctx); // отрисовка границ поля
    drawgrid(ctx); // отрисовка игровой сетки
    drawbutton(ctx, ctx->back_button); // отрисовка кнопки "BACK"
}

void computer_move(GameContext* ctx) {
    if (ctx->auto_play_mode) {
        // В режиме автоигры поочередно вызываем ходы компьютера
        if (ctx->is_player_turn) {
            // Ход компьютера как игрока X
            computer_as_player_move(ctx);
        }
        else {
            // Ход компьютера как игрока O (обычный ИИ)
            switch (ctx->parameters.difficulty) {
            case 1: // EASY
                easy_ai_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
                break;
            case 2: // MEDIUM
                medium_minimax_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
                break;
            case 3: // HARD
                hard_ai_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
                break;
            case 4: // EXPERT
                expert_ai_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
                break;
            }

            ctx->parameters.count_moves++;

            // Проверка победы ИИ (игрок O)
            if (check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
                ctx->parameters.last_ai_x, ctx->parameters.last_ai_y, ctx->parameters.ai, ctx)) {
                ctx->winner = 2;
                ctx->game_end_time = clock();
                ctx->game_duration = (double)(ctx->game_end_time - ctx->game_start_time) / CLOCKS_PER_SEC;
                ctx->current_screen = GAME_OVER;
                return;
            }

            // Проверка ничьей
            best_move cand;
            generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, false, 1, &cand, ctx);
            if (cand.n == 0) {
                ctx->winner = 3;
                ctx->game_end_time = clock();
                ctx->game_duration = (double)(ctx->game_end_time - ctx->game_start_time) / CLOCKS_PER_SEC;
                ctx->current_screen = GAME_OVER;
                return;
            }
        }

        // Переключаем ход
        ctx->is_player_turn = !ctx->is_player_turn;

    }
    else {
        // Обычный режим (игрок против компьютера)
        switch (ctx->parameters.difficulty) {
        case 1: // EASY
            easy_ai_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
            break;
        case 2: // MEDIUM
            medium_minimax_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
            break;
        case 3: // HARD
            hard_ai_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
            break;
        case 4: // EXPERT
            expert_ai_move(ctx->board, &ctx->parameters, &ctx->bbox, ctx);
            break;
        }

        ctx->parameters.count_moves++;

        // Проверка победы ИИ
        if (check_win(ctx->board, ctx->parameters.size, ctx->parameters.len,
            ctx->parameters.last_ai_x, ctx->parameters.last_ai_y, ctx->parameters.ai, ctx)) {
            ctx->winner = 2;
            ctx->game_end_time = clock();
            ctx->game_duration = (double)(ctx->game_end_time - ctx->game_start_time) / CLOCKS_PER_SEC;
            ctx->current_screen = GAME_OVER;
            return;
        }

        // Проверка ничьей
        best_move cand;
        generate_candidates(ctx->board, &ctx->parameters, &ctx->bbox, false, 1, &cand, ctx);
        if (cand.n == 0) {
            ctx->winner = 3;
            ctx->game_end_time = clock();
            ctx->game_duration = (double)(ctx->game_end_time - ctx->game_start_time) / CLOCKS_PER_SEC;
            ctx->current_screen = GAME_OVER;
            return;
        }

        ctx->is_player_turn = true;
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
    ctx->parameters.player = 'X';
    ctx->parameters.ai = 'O';
    ctx->parameters.difficulty = 1;
    ctx->parameters.player_moves_first = true;
    ctx->parameters.infinite_field = 0;
    ctx->bbox.initialized = false;
    ctx->char_width = 15.0f;
    ctx->char_height = 20.0f;
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
    ctx->about_button = (Button){ WINDOW_WIDTH / 2 - 100, 2 +10, 200, 50, "ABOUT", false };
    ctx->auto_play_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 40, 200, 50, "AUTO PLAY", false };
    ctx->winner = 0;
    ctx->auto_play_mode = false;
    srand(time(NULL));
}

int main() {
    setlocale(LC_ALL, "");
    if (!glfwInit()) { // инициализация GLFW
        return -1;
    }

    // создание окна
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // запрет изменения размера
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Tic Tac Toe", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window); // активация контекста OpenGL

    // инициализация GLEW ДО создания контекста
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    GameContext ctx;                    // создание структуры с данными игры
    init_game_context(&ctx);           // инициализация игры
    glfwSetWindowUserPointer(window, &ctx); // сохранение контекста в окне
    glfwSetKeyCallback(window, key_callback);       // регистрация обработчиков
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // настройка OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // в главном цикле добавляем обработку нового экрана
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        update_hover_state(window, &ctx);

        if (ctx.current_screen == GAME_SCREEN && !ctx.is_player_turn && ctx.winner == 0) {
            computer_move(&ctx);
        }

        // Для автоигры делаем ходы быстрее (без задержки)
        if (ctx.current_screen == GAME_SCREEN && ctx.auto_play_mode && ctx.winner == 0) {
            // В авторежиме делаем несколько ходов за кадр для ускорения
            for (int i = 0; i < 3 && ctx.winner == 0; i++) {
                computer_move(&ctx);
            }
        }

        switch (ctx.current_screen) {
        case MENU_SCREEN: draw_menu(&ctx); break;
        case GAME_SCREEN: draw_game(&ctx); break;
        case SETTINGS_SCREEN: draw_settings(&ctx); break;
        case GAME_OVER: draw_game_over(&ctx); break;
        case HELP_SCREEN: draw_help(&ctx); break;
        case ABOUT_SCREEN: draw_about(&ctx); break;
        }

        glfwSwapBuffers(window);  // показываем нарисованный кадр
        glfwPollEvents();         // обрабатываем события (клавиатура, мышь)
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