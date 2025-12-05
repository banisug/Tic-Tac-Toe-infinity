//#define _CRT_SECURE_NO_WARNINGS
//#include <stdio.h>
//#include <stdlib.h>
//#include <stdbool.h>
//#include <limits.h>
//#include <locale.h>
//#define SIZE 3
//
//// Функция для проверки, есть ли выигрышная комбинация
//bool is_winner(char board[SIZE][SIZE], char player) {
//    // Проверка строк и столбцов
//    for (int i = 0; i < SIZE; i++) {
//        if (board[i][0] == player && board[i][1] == player && board[i][2] == player)
//            return true;
//        if (board[0][i] == player && board[1][i] == player && board[2][i] == player)
//            return true;
//    }
//    // Проверка диагоналей
//    if (board[0][0] == player && board[1][1] == player && board[2][2] == player)
//        return true;
//    if (board[0][2] == player && board[1][1] == player && board[2][0] == player)
//        return true;
//    return false;
//}
//
//// Функция для проверки, остались ли свободные клетки
//bool is_moves_left(char board[SIZE][SIZE]) {
//    for (int i = 0; i < SIZE; i++)
//        for (int j = 0; j < SIZE; j++)
//            if (board[i][j] == ' ')
//                return true;
//    return false;
//}
//
//// Функция для оценки текущего состояния доски
//int evaluate(char board[SIZE][SIZE]) {
//    // Проверка, выиграл ли компьютер ('O')
//    if (is_winner(board, 'O'))
//        return +10;
//    // Проверка, выиграл ли игрок ('X')
//    if (is_winner(board, 'X'))
//        return -10;
//    // Если никто не выиграл, возвращаем 0
//    return 0;
//}
//
//// Рекурсивная функция минимакс
//int minimax(char board[SIZE][SIZE], int depth, bool is_maximizing) {
//    int score = evaluate(board);
//
//    // Если игра окончена, возвращаем оценку
//    if (score == 10 || score == -10)
//        return score;
//
//    // Если нет свободных клеток, возвращаем 0
//    if (!is_moves_left(board))
//        return 0;
//
//    // Если это ход максимизирующего игрока (компьютер)
//    if (is_maximizing) {
//        int best = -INT_MAX;
//        for (int i = 0; i < SIZE; i++) {
//            for (int j = 0; j < SIZE; j++) {
//                if (board[i][j] == ' ') {
//                    board[i][j] = 'O';
//                    best = max(best, minimax(board, depth + 1, !is_maximizing));
//                    board[i][j] = ' ';
//                }
//            }
//        }
//        return best;
//    }
//    // Если это ход минимизирующего игрока (игрок)
//    else {
//        int best = INT_MAX;
//        for (int i = 0; i < SIZE; i++) {
//            for (int j = 0; j < SIZE; j++) {
//                if (board[i][j] == ' ') {
//                    board[i][j] = 'X';
//                    best = min(best, minimax(board, depth + 1, !is_maximizing));
//                    board[i][j] = ' ';
//                }
//            }
//        }
//        return best;
//    }
//}
//
//// Функция для нахождения лучшего хода для компьютера
//void find_best_move(char board[SIZE][SIZE]) {
//    int best_val = -INT_MAX;
//    int best_row = -1, best_col = -1;
//
//    for (int i = 0; i < SIZE; i++) {
//        for (int j = 0; j < SIZE; j++) {
//            if (board[i][j] == ' ') {
//                board[i][j] = 'O';
//                int move_val = minimax(board, 0, false);
//                board[i][j] = ' ';
//                if (move_val > best_val) {
//                    best_row = i;
//                    best_col = j;
//                    best_val = move_val;
//                }
//            }
//        }
//    }
//    setlocale(LC_CTYPE, "");
//    printf("Лучший ход для компьютера: [%d][%d]\n", best_row, best_col);
//    board[best_row][best_col] = 'O';
//}
//
//// Функция для вывода доски на экран
//void print_board(char board[SIZE][SIZE]) {
//    for (int i = 0; i < SIZE; i++) {
//        for (int j = 0; j < SIZE; j++) {
//            printf(" %c ", board[i][j]);
//            if (j < SIZE - 1)
//                printf("|");
//        }
//        printf("\n");
//        if (i < SIZE - 1)
//            printf("---+---+---\n");
//    }
//}
//
//int main() {
//    char board[SIZE][SIZE] = {
//        {' ', ' ', ' '},
//        {' ', ' ', ' '},
//        {' ', ' ', ' '}
//    };
//    setlocale(LC_CTYPE, "");
//    while (true) {
//        print_board(board);
//
//        // Ход игрока
//        int row, col;
//        printf("Введите строку и столбец для хода (0-2): ");
//        scanf("%d %d", &row, &col);
//        if (board[row][col] != ' ') {
//            printf("Недопустимый ход. Попробуйте снова.\n");
//            continue;
//        }
//        board[row][col] = 'X';
//
//        // Проверка, выиграл ли игрок
//        if (is_winner(board, 'X')) {
//            print_board(board);
//            printf("Вы выиграли!\n");
//            break;
//        }
//
//        // Проверка, остались ли свободные клетки
//        if (!is_moves_left(board)) {
//            print_board(board);
//            printf("Ничья!\n");
//            break;
//        }
//
//        // Ход компьютера
//        find_best_move(board);
//
//        // Проверка, выиграл ли компьютер
//        if (is_winner(board, 'O')) {
//            print_board(board);
//            printf("Компьютер выиграл!\n");
//            break;
//        }
//
//        // Проверка, остались ли свободные клетки
//        if (!is_moves_left(board)) {
//            print_board(board);
//            printf("Ничья!\n");
//            break;
//        }
//    }
//
//    return 0;
//}