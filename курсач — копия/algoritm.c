//#define _CRT_SECURE_NO_WARNINGS
//#include <stdio.h>
//#include <stdlib.h>
//#include <limits.h>
//#include <locale.h>
//#include <time.h>
//
//#define INF 1000 // для сравнения оценок
//
//
//// Для заполнения доски
//void fill_board(int s, char* board) {
//    for (int i = 0; i < s * s; i++) {
//        board[i] = '-';
//    }
//}
//
//// Ввод хода игрока
//int enter_player(char* board) {
//    int move;
//    printf("Введите ваш ход (0-8): ");
//    scanf("%d", &move);
//    while (move < 0 || move > 8 || board[move] != '-') {
//        printf("Недопустимый ход. Введите ваш ход (0-8): ");
//        scanf("%d", &move);
//    }
//    return move;
//}
//
//// Вывод доски в текущий момент
//void print_board(char* board) {
//    printf("%c %c %c\n", board[0], board[1], board[2]);
//    printf("%c %c %c\n", board[3], board[4], board[5]);
//    printf("%c %c %c\n\n", board[6], board[7], board[8]);
//}
//
//// Для проверки, выиграл ли игрок
//int winner(char p, char* board, int **win_comb) {
//    for (int i = 0; i < 8; i++) {
//        if (board[win_comb[i][0]] == p &&
//            board[win_comb[i][1]] == p &&
//            board[win_comb[i][2]] == p) {
//            return 1;  // Если найдена выигрышная комбинация
//        }
//    }
//    return 0;
//}
//
//// Заполнена ли доска
//int full(int s, char* board) {
//    for (int i = 0; i < s * s; i++) {
//        if (board[i] == '-') {
//            return 0;
//        }
//    }
//    return 1;
//}
//
//// Для выполнения хода
//void make_move(int m, char p, char* board) {
//    board[m] = p;
//}
//
//// Для отмены хода
//void cancel_move(int m, char* board) {
//    board[m] = '-';
//}
//
////функция для оценки состояния доски
//int count_win_lines(char player, char opponent, int** win_comb) {
//    int player_score = 0; // Количество выигрышных линий для игрока
//    int opponent_score = 0; // Количество выигрышных линий для оппонента
//
//    for (int i = 0; i < 8; i++) {
//        int player_count = 0; // Количество символов игрока в линии
//        int opponent_count = 0; // Количество символов оппонента в линии
//        for (int j = 0; j < 3; j++) {
//            if (win_comb[i][j] == player) { // Если игрок занял эту клетку
//                player_count++;
//            }
//            else if (win_comb[i][j] == opponent) { // Если оппонент занял эту клетку
//                opponent_count++;
//            }
//        }
//        // Если линия свободна для игрока, увеличиваем его оценку
//        if (player_count > 0 && opponent_count == 0) {
//            player_score++;
//        }
//        // Если линия свободна для противника, увеличиваем его оценку
//        if (opponent_count > 0 && player_count == 0) {
//            opponent_score++;
//        }
//    }
//    // Возвращаем разницу в количестве возможных выигрышных линий
//    return player_score - opponent_score;
//}
//
//// Функция минимакс
//int minimax(int depth, int maximizing_player, int max_depth, int size, char* board, char player, char opponent, int** win_comb) {
//    // Базовые случаи
//    if (winner(player, board, win_comb)) { // Если выиграл игрок
//        return 1;
//    }
//    if (winner(opponent, board, win_comb)) { // Если выиграл противник (компьютер)
//        return -1;
//    }
//    if (full(size, board)) {
//        return 0;
//    }
//    if (depth >= max_depth) { // Если достигнута нужная глубина рекурсии
//        return count_win_lines(player, opponent, win_comb); // Считаем количество возможных выигрышных линий
//    }
//
//
//        if (maximizing_player) {
//            int max_score = -INF;
//            for (int move = 0; move < 9; move++) {
//                if (board[move] == '-') { // Если пустая
//                    make_move(move, player, board);  // Делаем ход за игрока
//                    int score = minimax(depth + 1, 0, max_depth, size, board, player, opponent, win_comb); // Рекурсивный вызов
//                    cancel_move(move, board); // Отменяем ход
//                    if (score > max_score) {
//                        max_score = score; // Обновляем максимальную оценку
//                    }
//                }
//            }
//            return max_score;
//        }
//        else {
//            int min_score = INF;
//            for (int move = 0; move < 9; move++) {
//                if (board[move] == '-') { // Если пустая
//                    make_move(move, opponent, board); // Делаем ход за противника
//                    int score = minimax(depth + 1, 1, max_depth, size, board, player, opponent, win_comb);
//                    cancel_move(move, board);
//                    if (score < min_score) {
//                        min_score = score; // Обновляем минимальную оценку
//                    }
//                }
//            }
//            return min_score;
//        }
//}
//
//// Функция для нахождения лучшего хода с использованием минимакс
//int best_move(int max_depth, int size, char* board, char player, char opponent,int ** win_comb) {
//    int best_score = INF; // Для минимизации
//    int best_move=0;
//    for (int move = 0; move < size * size; move++) {
//        if (board[move] == '-') { // Если пусто
//            make_move(move, opponent, board); // Пробуем ход за противника
//            int score = minimax(0, 1, max_depth, size, board, player, opponent, win_comb); // Оцениваем ход (компьютер минимизирует)
//            cancel_move(move, board); // Отменяем ход
//            if (score < best_score) { // Выбираем ход с минимальной оценкой 
//                best_score = score;
//                best_move = move;
//            }
//        }
//    }
//    return best_move;
//}
//
//// Простой уровень 
//void easy_level(char* board, char player, char opponent) {
//    int k = 0; // Флаг завершения хода
//    srand(time(NULL));
//    // Занимаем центр, если он свободен
//    if (board[4] == '-') { board[4] = opponent; k = 1; }
//    // Случайный ход
//    if (k == 0) {
//        while (1) {
//            int a = rand() % 9;
//            if (board[a] == '-') {
//                board[a] = opponent;
//                break;
//            }
//        }
//    }
//}
//
//
//int main() {
//    int size_board = 3;
//    //while (scanf("%d", &size_board) == 0) {
//    //    printf("Недопустимый ввод. Введите заново: ");
//    //}
//    char* board = (char*)malloc(size_board * size_board * sizeof(char));
//    fill_board(size_board, board);
//    setlocale(LC_CTYPE, "");
//    int flag = 0; // Флаг для определения, кто ходит (0 - игрок, 1 - компьютер)
//    int level;
//    int move;
//    int p;
//    // Выигрышные комбинации
//    int win_comb[8][3] = {
//        {0, 1, 2}, {3, 4, 5}, {6, 7, 8},  // Строки
//        {0, 3, 6}, {1, 4, 7}, {2, 5, 8},  // Столбцы
//        {0, 4, 8}, {2, 4, 6}              // Диагонали
//    };
//    // Определение игроков
//    char player = 'X';  // Игрок
//    char opponent = 'O';  // Противник (компьютер)
//    printf("Выберети чем хотите играть:\n0 - Х\n1 - О\n");
//    scanf("%d", &p);
//    while (p != 0 && p != 1) {
//        printf("Недопустимый ввод. Введите заново: ");
//        scanf("%d", &p);
//    }
//    if (p == 1) {
//        player = 'O';
//        opponent = 'X';
//        flag = 1;
//    }
//    printf("Выберите уровень:\n1-простой\n2-средний\n3-сложный\n4-очень сложный\n");
//    scanf("%d", &level);
//    while (level < 1 || level > 4) {
//        printf("Недопустимый ввод. Введите заново: ");
//        scanf("%d", &level);
//    }
//    if (level == 1) {
//        while (1) {
//            print_board(board);
//            // Проверка завершения игры
//            if (full(size_board, board) || winner(player, board, win_comb) || winner(opponent, board, win_comb)) {
//                break;
//            }
//            if (flag == 0) { // Если ходит человек
//                move = enter_player(board);
//                make_move(move, player, board);
//                flag = 1;
//            }
//            else { // Если ходит компьютер
//                easy_level(board, player, opponent);
//                flag = 0;
//            }
//        }
//    }
//    else {
//        int max_depth;
//        switch (level) {
//        case 2: max_depth = 5; break;
//        case 3: max_depth = 6; break;
//        case 4: max_depth = 8; break;
//        default: max_depth = 5; break;
//        }
//        while (1) {
//            print_board(board);
//            if (full(size_board, board) || winner(player, board, win_comb) || winner(opponent, board, win_comb)) {
//                break;
//            }
//            if (flag == 0) { // Если ходит человек
//                move = enter_player(board);
//                make_move(move, player, board);
//                flag = 1;
//            }
//            else { // Если ходит компьютер
//                move = best_move(max_depth, size_board, board, player, opponent, win_comb);
//                make_move(move, opponent, board);
//                flag = 0;
//            }
//        }
//    }
//    // Игра завершена
//    print_board(board);
//    if (winner(player, board, win_comb)) {
//        printf("Вы выиграли!\n");
//    }
//    else if (winner(opponent, board, win_comb)) {
//        printf("Вы проиграли!\n");
//    }
//    else {
//        printf("Ничья!\n");
//    }
//
//    free(board);
//    return 0;
//}