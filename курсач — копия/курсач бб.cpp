#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <time.h>
#include <stdbool.h>

#define INF 1000 // большое число, символизирующее бесконечность

typedef struct {
    char player; // для Х или О
    char opponent;
    int size; // размер доски
    int len; // длина выигрышной линии
    int last_move_row; // строка последнего хода
    int last_move_col; // столбец последнего хода
} GameState;

enum level {
    easy = 1,
    middle,
    hard,
    impossible
};

// заполняет доску "-"
void fill_board(int size, char** board) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            board[i][j] = '-';
        }
    }
}

// ввод хода игрока
int enter_player(int size, char** board) {
    int row, col;
    while (1) {
        printf("Введите номер строки (от 1 до %d): ", size);
        if (scanf("%d", &row) == 0) {
            while (getchar() != '\n'); // убрать все из буфера
            printf("Некорректный ввод. ");
            continue;
        }
        printf("Введите номер столбца (от 1 до %d): ", size);
        if (scanf("%d", &col) == 0) {
            while (getchar() != '\n'); // убрать все из буфера
            printf("Некорректный ввод. ");
            continue;
        }

        row--; col--; // Переход к индексации с 0

        if (row >= 0 && row < size && col >= 0 && col < size && board[row][col] == '-') { // проверка корректности ввода
            return row * size + col; // представляем индексы двумерного массива как индекс одномерного
        }
        printf("Недопустимый ход. ");
    }
}

//вывод доски
void print_board(int size, char** board) { 
    printf("\n");
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf(" %c ", board[i][j]);
            if (j < size - 1) printf("|"); // разделяем столбцы
        }
        printf("\n");
        if (i < size - 1) {
            for (int j = 0; j < size * 4 - 1; j++) printf("-"); // разделяем строки
            printf("\n");
        }
    }
    printf("\n");
}

// для проверки, выиграл ли игрок
bool winner(char player, char** board, GameState a) {
    int directions[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} }; // определяет направления, в которых проверяется наличие выигрышной линии (горизонталь, вертикаль, диагонали)

    for (int d = 0; d < 4; d++) { // проверка всех 4 типов выигрышных линий
        for (int i = 0; i < a.size; i++) { // циклы для прохода по массиву
            for (int j = 0; j < a.size; j++) {
                bool win = true;
                for (int k = 0; k < a.len; k++) { // цикл для нахождения выигрышной линии
                    int I = i + k * directions[d][0]; 
                    int J = j + k * directions[d][1];
                    if (I >= a.size || J >= a.size || I < 0 || board[I][J] != player) { // проверка на выход за пределы массива или корректность символа в клетке
                        win = false;
                        break;
                    }
                }
                if (win == true) { // если собралась линия
                    return true;
                }
            }
        }
    }
    return false;
}

// проверка заполнена ли доска
bool full(char** board, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (board[i][j] == '-') return false;
        }
    }
    return true;
}

// оценка состояния доски
int count_win_lines(char** board, GameState a) {
    int score = 0;
    int directions[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} }; // для проверки линий

    // центральная клетка наиболее ценна, так как участвует в максимальном числе выигрышных линий
    int center = a.size / 2;
    if (board[center][center] == a.opponent) score += 3;

    // проверка всех потенциальных линий
    for (int i = 0; i <= a.size - a.len; i++) {
        for (int j = 0; j <= a.size - a.len; j++) {
            for (int d = 0; d < 4; d++) { // проверка 4 типов линий (вертикаль, горизонталь и 2 диагонали)
                int player_count = 0, opponent_count = 0;
                for (int step = 0; step < a.len; step++) {
                    int I;
                    if (d == 0) { // только при d = 0, directions[i][0] = 0
                        I = i;
                    }
                    else {
                        I = i + step;
                    }

                    int J;
                    if (d == 1) { // для вертикали
                        J = j;
                    }
                    else if (d == 3) { // для диагонали "/"
                        J = j + a.len - 1 - step;
                    }
                    else {
                        J = j + step; // для горизонтали и диагонали "\"
                    }

                    if (board[I][J] == a.player) { // если клетка занята игроком, увеличиваем его счетчик
                        player_count++;
                    }
                    else if (board[I][J] == a.opponent) { // если клетка занята компьютером, увеличиваем его счетчик
                        opponent_count++;
                    }
                }
                if (player_count > 0 && opponent_count == 0) { // если есть выигрышная линия для игрока, увеличиваем счетчик
                    score += player_count;
                }
                if (opponent_count > 0 && player_count == 0) {
                    score -= opponent_count; // если есть выигрышная линия для компьютера, уменьшаем
                }
            }
        }
    }
    return score; // возвращаем разницу в выигрышних линиях для игрока и компьютера
}

// минимакс
int minimax(char** board, int depth, bool is_maximizing, GameState a, int alpha, int beta) {
    // базовые случаи
    if (winner(a.player, board, a)) { // если выиграл игрок
        return -INF + depth; // чем раньше игрок выигрывает, тем больше минимизируем значение
    }
    if (winner(a.opponent, board, a)) { // если выиграл комп
        return INF - depth; // чем раньше выигрывает компьютер, тем больше максимизируем (выигрыш на 4 ходе будет лучше, чем на 5)
    }
    if (full(board, a.size)) { // если доска заполнена, то ничья
        return 0;
    }
    if (depth == 0) { // если достигнута максимальная глубина рекурсии, применяем эвристику
        return count_win_lines(board, a);
    }

    //создаем массив, в котором индексы приоритетных клеток отсортированы (самые важные в начале, самые ненужные в конце)
    // это поможет быстрее найти выигрышный ход и быстрее применить альфа-бета отсечение
    // приоритетные ходы: центр -> углы -> остальные
    int* moves = (int*)malloc(a.size * a.size * sizeof(int));
    int num_moves = 0;
    int center = a.size / 2;

    // центр
    if (board[center][center] == '-') {
        moves[num_moves++] = center * a.size + center;
    }

    // углы
    int corners[4][2] = { {0,0}, {0,a.size - 1}, {a.size - 1,0}, {a.size - 1,a.size - 1} };
    for (int i = 0; i < 4; i++) {
        int row = corners[i][0];
        int col = corners[i][1];
        if (board[row][col] == '-') {
            moves[num_moves++] = row * a.size + col;
        }
    }

    // остальные клетки
    for (int i = 0; i < a.size; i++) {
        for (int j = 0; j < a.size; j++) {
            if (board[i][j] == '-' && !(i == center && j == center) &&
                !((i == 0 || i == a.size - 1) && (j == 0 || j == a.size - 1))) {
                moves[num_moves++] = i * a.size + j;
            }
        }
    }

    if (is_maximizing) { // максимизирующий игрок (комп)
        int max_eval = -INF; 
        for (int k = 0; k < num_moves; k++) { // проходимся по массиву moves
            int move = moves[k];
            int row = move / a.size;
            int col = move % a.size;

            board[row][col] = a.opponent; // делаем ход за компьютер
            a.last_move_row = row; // запоминаем его
            a.last_move_col = col;

            int eval = minimax(board, depth - 1, false, a, alpha, beta); // вызываем для минимизирующего игрока
            board[row][col] = '-'; // отменяем ход

            if (eval > max_eval) max_eval = eval; // находим максимальное значение
            if (eval > alpha) alpha = eval; // находим лучшую ветку дерева
            if (beta <= alpha) break; // отсечение худших веток
        }
        return max_eval;
    }
    else { // минимизирующий игрок
        int min_eval = INF;
        for (int k = 0; k < num_moves; k++) {
            int move = moves[k];
            int row = move / a.size;
            int col = move % a.size;

            board[row][col] = a.player;
            a.last_move_row = row;
            a.last_move_col = col;

            int eval = minimax(board, depth - 1, true, a, alpha, beta);
            board[row][col] = '-';

            if (eval < min_eval) min_eval = eval; // находим минимальное значение
            if (eval < beta) beta = eval; // находим лучшую ветку дерева
            if (beta <= alpha) break; // отсечение худших веток
        }
        return min_eval;
    }
}

// алгоритм для легкого уровня 
void easy_level(char** board, GameState a) { 
    // если центр не занят, занимае его
    int center = a.size / 2;
    if (board[center][center] == '-') {
        board[center][center] = a.opponent;
        return;
    }
    srand(time(NULL));
    while (1) {
        int row = rand() % a.size;
        int col = rand() % a.size;
        if (board[row][col] == '-') {
            board[row][col] = a.opponent;
            break;
        }
    }
}

// Основная функция для хода компьютера (максимизирует)
void computer_move(char** board, GameState* a, int level) {
    if (level == easy) { // вызов функции для легкого уровня
        easy_level(board, *a);
        return;
    }

    int max_depth; // выбираем глубину рекурсии в зависимости от уровня
    switch (level) {
    case middle: max_depth = 2; break;
    case hard: max_depth = 3; break;
    case impossible: max_depth = 4; break;
    default: max_depth = 2; break;
    }

    int best_move = -1;
    int best_eval = -INF;
    int alpha = -INF;
    int beta = INF;

    // массив для сортировки ходов
    int* moves = (int*)malloc(a->size * a->size * sizeof(int));
    int num_moves = 0;
    int center = a->size / 2;

    // центр
    if (board[center][center] == '-') {
        moves[num_moves++] = center * a->size + center;
    }

    // углы
    int corners[4][2] = { {0,0}, {0,a->size - 1}, {a->size - 1,0}, {a->size - 1,a->size - 1} };
    for (int i = 0; i < 4; i++) {
        int row = corners[i][0];
        int col = corners[i][1];
        if (board[row][col] == '-') {
            moves[num_moves++] = row * a->size + col;
        }
    }

    // остальные клетки
    for (int i = 0; i < a->size; i++) {
        for (int j = 0; j < a->size; j++) {
            if (board[i][j] == '-' && !(i == center && j == center) &&
                !((i == 0 || i == a->size - 1) && (j == 0 || j == a->size - 1))) {
                moves[num_moves++] = i * a->size + j;
            }
        }
    }

    for (int k = 0; k < num_moves; k++) { // перебираем все ходы и выбираем лучший
        int move = moves[k];
        int row = move / a->size; // находим индексы клетки
        int col = move % a->size;

        board[row][col] = a->opponent; // делаем ход
        a->last_move_row = row; // запоминаем его
        a->last_move_col = col;

        int eval = minimax(board, max_depth - 1, false, *a, alpha, beta); // оцениваем ход
        board[row][col] = '-'; // отменяем ход

        if (eval > best_eval) { // выбираем лучший вариант
            best_eval = eval;
            best_move = move;
        }
        alpha = (eval > alpha) ? eval : alpha;
    }

    if (best_move != -1) { // делаем ход
        int row = best_move / a->size;
        int col = best_move % a->size;
        board[row][col] = a->opponent;
        a->last_move_row = row;
        a->last_move_col = col;
    }
}

int main() {
    setlocale(LC_ALL, "");
    GameState a;

    printf("Введите размер доски (минимум 3): ");
    while (1) {
        if (scanf("%d", &a.size) != 1) {
            while (getchar() != '\n');
            printf("Некорректный ввод. Введите число: ");
            continue;
        }
        if (a.size < 3) {
            printf("Слишком маленький размер. Введите минимум 3: ");
            continue;
        }
        break;
    }

    printf("Введите длину выигрышной линии (минимум 3): ");
    while (1) {
        if (scanf("%d", &a.len) != 1) {
            while (getchar() != '\n');
            printf("Некорректный ввод. Введите число: ");
            continue;
        }
        if (a.len < 3) {
            printf("Слишком маленький размер. Введите минимум 3: ");
            continue;
        }
        break;
    }

    char** board = (char**)malloc(a.size * sizeof(char*));
    for (int i = 0; i < a.size; i++) {
        board[i] = (char*)malloc(a.size * sizeof(char));
    }
    fill_board(a.size, board);

    printf("Выберите символ (1 - X, 2 - O): ");
    int choice;
    short flag = 1;
    while (1) {
        if (scanf("%d", &choice) != 1 || (choice != 1 && choice != 2)) {
            while (getchar() != '\n');
            printf("Некорректный ввод. Введите 1 или 2: ");
            continue;
        }
        break;
    }

    if (choice == 1) {
        a.player = 'X';
        a.opponent = 'O';

    }
    else {
        a.player = 'O';
        a.opponent = 'X';
        flag = 0;
    }

    printf("Выберите уровень сложности (1-4): ");
    int level;
    while (1) {
        if (scanf("%d", &level) != 1 || level < 1 || level > 4) {
            while (getchar() != '\n');
            printf("Некорректный ввод. Введите число от 1 до 4: ");
            continue;
        }
        break;
    }

    // Основной игровой цикл
    while (1) {
        print_board(a.size, board);

        if (winner(a.player, board, a)) {
            printf("Вы победили!\n");
            break;
        }
        if (winner(a.opponent, board, a)) {
            printf("Компьютер победил!\n");
            break;
        }
        if (full(board, a.size)) {
            printf("Ничья!\n");
            break;
        }

        if (flag) { // ход игрока
            int move = enter_player(a.size, board);
            int row = move / a.size;
            int col = move % a.size;
            board[row][col] = a.player;
            a.last_move_row = row;
            a.last_move_col = col;
        }
        else { // ход компьютера
            printf("Ход компьютера...\n");
            computer_move(board, &a, level);
        }

        flag = (flag == 1) ? 0 : 1;
    }

    for (int i = 0; i < a.size; i++) {
        free(board[i]);
    }
    free(board);

    return 0;
}