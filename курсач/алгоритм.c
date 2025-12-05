//#define _CRT_SECURE_NO_WARNINGS
//#include <locale.h>
//
//#include <stdio.h>
//#include <stdlib.h>
//#include <time.h>
//
//// Настройки игры
//int BOARD_SIZE = 3;
//int WIN_LENGTH = 3;
//char player = 'X';
//char computer = 'O';
//
//// Динамическое игровое поле
//char** board;
//
//// Инициализация игрового поля
//void initializeBoard() {
//    board = (char**)malloc(BOARD_SIZE * sizeof(char*));
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        board[i] = (char*)malloc(BOARD_SIZE * sizeof(char));
//        for (int j = 0; j < BOARD_SIZE; j++) {
//            board[i][j] = ' ';
//        }
//    }
//}
//
//// Освобождение памяти
//void freeBoard() {
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        free(board[i]);
//    }
//    free(board);
//}
//
//// Отображение игрового поля
//void displayBoard() {
//    printf("\n   ");
//    for (int j = 0; j < BOARD_SIZE; j++) {
//        printf("%2d  ", j + 1);
//    }
//    printf("\n");
//
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        printf("%2d ", i + 1);
//        for (int j = 0; j < BOARD_SIZE; j++) {
//            printf(" %c ", board[i][j]);
//            if (j < BOARD_SIZE - 1) {
//                printf("|");
//            }
//        }
//        printf("\n");
//
//        if (i < BOARD_SIZE - 1) {
//            printf("   ");
//            for (int j = 0; j < BOARD_SIZE; j++) {
//                printf("---");
//                if (j < BOARD_SIZE - 1) {
//                    printf("+");
//                }
//            }
//            printf("\n");
//        }
//    }
//    printf("\n");
//}
//
//// Проверка линии на победу
//int checkLine(char symbol, int startRow, int startCol, int deltaRow, int deltaCol) {
//    int count = 0;
//    int row = startRow;
//    int col = startCol;
//
//    while (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
//        if (board[row][col] == symbol) {
//            count++;
//            if (count >= WIN_LENGTH) {
//                return 1;
//            }
//        }
//        else {
//            count = 0;
//        }
//        row += deltaRow;
//        col += deltaCol;
//    }
//    return 0;
//}
//
//// Проверка на победу
//int checkWin(char symbol) {
//    // Проверка горизонталей
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        if (checkLine(symbol, i, 0, 0, 1)) return 1;
//    }
//
//    // Проверка вертикалей
//    for (int j = 0; j < BOARD_SIZE; j++) {
//        if (checkLine(symbol, 0, j, 1, 0)) return 1;
//    }
//
//    // Проверка диагоналей (слева-направо)
//    for (int i = 0; i <= BOARD_SIZE - WIN_LENGTH; i++) {
//        if (checkLine(symbol, i, 0, 1, 1)) return 1;
//    }
//    for (int j = 1; j <= BOARD_SIZE - WIN_LENGTH; j++) {
//        if (checkLine(symbol, 0, j, 1, 1)) return 1;
//    }
//
//    // Проверка диагоналей (справа-налево)
//    for (int i = 0; i <= BOARD_SIZE - WIN_LENGTH; i++) {
//        if (checkLine(symbol, i, BOARD_SIZE - 1, 1, -1)) return 1;
//    }
//    for (int j = BOARD_SIZE - 2; j >= WIN_LENGTH - 1; j--) {
//        if (checkLine(symbol, 0, j, 1, -1)) return 1;
//    }
//
//    return 0;
//}
//
//// Проверка на ничью
//int checkDraw() {
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        for (int j = 0; j < BOARD_SIZE; j++) {
//            if (board[i][j] == ' ') {
//                return 0;
//            }
//        }
//    }
//    return 1;
//}
//
//// Ход игрока
//void playerMove() {
//    int row, col;
//    setlocale(LC_ALL, "Rus");
//
//    while (1) {
//        printf("Ваш ход (строка и столбец от 1 до %d через пробел): ", BOARD_SIZE);
//        scanf("%d %d", &row, &col);
//
//        row--;
//        col--;
//
//        if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && board[row][col] == ' ') {
//            board[row][col] = player;
//            break;
//        }
//        else {
//            printf("Неверный ход! Попробуйте снова.\n");
//        }
//    }
//}
//
//// Поиск САМЫХ ОПАСНЫХ угроз (игрок может выиграть следующим ходом)
//int findCriticalThreat() {
//    // ВСЕ 8 направлений
//    int directions[8][2] = {
//        {0, 1}, {1, 0}, {1, 1}, {1, -1},
//        {0, -1}, {-1, 0}, {-1, -1}, {-1, 1}
//    };
//
//    for (int threatLength = WIN_LENGTH - 1; threatLength >= 2; threatLength--) {
//        for (int dir = 0; dir < 8; dir++) {
//            int deltaRow = directions[dir][0];
//            int deltaCol = directions[dir][1];
//
//            for (int i = 0; i < BOARD_SIZE; i++) {
//                for (int j = 0; j < BOARD_SIZE; j++) {
//                    int playerCount = 0;
//                    int emptyCount = 0;
//                    int emptyRow = -1, emptyCol = -1;
//                    int valid = 1;
//
//                    // Проверяем линию длиной WIN_LENGTH
//                    for (int k = 0; k < WIN_LENGTH; k++) {
//                        int row = i + k * deltaRow;
//                        int col = j + k * deltaCol;
//
//                        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
//                            valid = 0;
//                            break;
//                        }
//
//                        if (board[row][col] == player) {
//                            playerCount++;
//                        }
//                        else if (board[row][col] == ' ') {
//                            emptyCount++;
//                            emptyRow = row;
//                            emptyCol = col;
//                        }
//                        else if (board[row][col] == computer) {
//                            valid = 0;
//                            break;
//                        }
//                    }
//
//                    // КРИТИЧЕСКАЯ УГРОЗА: игрок может выиграть следующим ходом
//                    if (valid && playerCount == threatLength && emptyCount == 1) {
//                        board[emptyRow][emptyCol] = computer;
//                        return 1;
//                    }
//                }
//            }
//        }
//    }
//    return 0;
//}
//
//
//// Поиск и блокировка последовательностей из 2+ крестиков с приоритетом позиций рядом с крестиками
//// Упрощенная и эффективная блокировка
//int findAndBlockSequences() {
//    int directions[8][2] = {
//        {0,1}, {1,0}, {1,1}, {1,-1}, {0,-1}, {-1,0}, {-1,-1}, {-1,1}
//    };
//
//    // Сначала ищем самые длинные последовательности (3+ крестиков)
//    for (int targetLength = WIN_LENGTH - 1; targetLength >= 2; targetLength--) {
//        for (int dir = 0; dir < 8; dir++) {
//            int deltaRow = directions[dir][0];
//            int deltaCol = directions[dir][1];
//
//            for (int i = 0; i < BOARD_SIZE; i++) {
//                for (int j = 0; j < BOARD_SIZE; j++) {
//                    int row = i, col = j;
//                    int count = 0;
//                    int emptyFound = 0;
//                    int emptyRow = -1, emptyCol = -1;
//
//                    // Простой подсчет крестиков подряд
//                    while (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
//                        if (board[row][col] == player) {
//                            count++;
//                        }
//                        else if (board[row][col] == ' ' && !emptyFound) {
//                            emptyFound = 1;
//                            emptyRow = row;
//                            emptyCol = col;
//                        }
//                        else {
//                            break;
//                        }
//
//                        if (count >= targetLength && emptyFound) {
//                            board[emptyRow][emptyCol] = computer;
//                            return 1;
//                        }
//
//                        row += deltaRow;
//                        col += deltaCol;
//                    }
//                }
//            }
//        }
//    }
//
//    return 0;
//}
//
//
//// Улучшенный поиск ходов рядом с фигурами игрока - с приоритетом позиций между крестиками
//int findMoveNearPlayer() {
//    // Все возможные направления вокруг клетки (8 направлений)
//    int directions[8][2] = {
//        {-1, -1}, {-1, 0}, {-1, 1},
//        {0, -1},           {0, 1},
//        {1, -1},  {1, 0},  {1, 1}
//    };
//
//    // Сначала ищем позиции, которые находятся МЕЖДУ двумя крестиками
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        for (int j = 0; j < BOARD_SIZE; j++) {
//            if (board[i][j] == ' ') {
//                int playerPairs = 0;
//
//                // Проверяем все пары противоположных направлений
//                for (int dir1 = 0; dir1 < 8; dir1++) {
//                    int row1 = i + directions[dir1][0];
//                    int col1 = j + directions[dir1][1];
//
//                    // Находим противоположное направление
//                    int oppositeDir = (dir1 + 4) % 8;
//                    int row2 = i + directions[oppositeDir][0];
//                    int col2 = j + directions[oppositeDir][1];
//
//                    if (row1 >= 0 && row1 < BOARD_SIZE && col1 >= 0 && col1 < BOARD_SIZE &&
//                        row2 >= 0 && row2 < BOARD_SIZE && col2 >= 0 && col2 < BOARD_SIZE) {
//                        if (board[row1][col1] == player && board[row2][col2] == player) {
//                            playerPairs++;
//                        }
//                    }
//                }
//
//                // Если нашли позицию между двумя крестиками - это идеальный ход
//                if (playerPairs > 0) {
//                    board[i][j] = computer;
//                    return 1;
//                }
//            }
//        }
//    }
//
//    // Если не нашли позиций между крестиками, ищем клетки с максимальным количеством соседей-игроков
//    int bestRow = -1, bestCol = -1;
//    int maxPlayerNeighbors = 0;
//
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        for (int j = 0; j < BOARD_SIZE; j++) {
//            if (board[i][j] == ' ') {
//                int playerNeighbors = 0;
//
//                // Считаем соседей-игроков
//                for (int dir = 0; dir < 8; dir++) {
//                    int newRow = i + directions[dir][0];
//                    int newCol = j + directions[dir][1];
//
//                    if (newRow >= 0 && newRow < BOARD_SIZE &&
//                        newCol >= 0 && newCol < BOARD_SIZE) {
//                        if (board[newRow][newCol] == player) {
//                            playerNeighbors++;
//                        }
//                    }
//                }
//
//                // Обновляем лучшую позицию (только если есть хотя бы один сосед-игрок)
//                if (playerNeighbors > maxPlayerNeighbors && playerNeighbors > 0) {
//                    maxPlayerNeighbors = playerNeighbors;
//                    bestRow = i;
//                    bestCol = j;
//                }
//            }
//        }
//    }
//
//    // Если нашли хорошую позицию рядом с игроком
//    if (maxPlayerNeighbors >= 1 && bestRow != -1) {
//        board[bestRow][bestCol] = computer;
//        return 1;
//    }
//
//    return 0;
//}
//
//
//// Случайный ход компьютера (запасной вариант)
//void computerRandomMove() {
//    int emptyCells = 0;
//
//    // Считаем пустые клетки
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        for (int j = 0; j < BOARD_SIZE; j++) {
//            if (board[i][j] == ' ') {
//                emptyCells++;
//            }
//        }
//    }
//
//    if (emptyCells == 0) return;
//
//    // Выбираем случайную пустую клетку
//    int target = rand() % emptyCells;
//    int count = 0;
//
//    for (int i = 0; i < BOARD_SIZE; i++) {
//        for (int j = 0; j < BOARD_SIZE; j++) {
//            if (board[i][j] == ' ') {
//                if (count == target) {
//                    board[i][j] = computer;
//                    return;
//                }
//                count++;
//            }
//        }
//    }
//}
//
//// Умный ход компьютера с улучшенной логикой блокировки
//void computerSmartMove() {
//    setlocale(LC_ALL, "Rus");
//
//    printf("Ход компьютера...\n");
//
//    // 1. 🚨 САМЫЕ ОПАСНЫЕ УГРОЗЫ - блокировать немедленно!
//    if (findCriticalThreat()) {
//        printf("Компьютер блокирует критическую угрозу!\n");
//        return;
//    }
//
//    // 2. 🔍 ПОИСК ПОСЛЕДОВАТЕЛЬНОСТЕЙ - блокировать развивающиеся линии
//    if (findAndBlockSequences()) {
//        printf("Компьютер блокирует развивающуюся последовательность!\n");
//        return;
//    }
//
//    // 3. 🎯 АКТИВНАЯ БЛОКИРОВКА - ставить рядом с фигурами игрока
//    if (findMoveNearPlayer()) {
//        printf("Компьютер ставит рядом с вашими фигурами!\n");
//        return;
//    }
//
//    // 4. 🎲 СЛУЧАЙНЫЙ ХОД - последний вариант
//    printf("Компьютер делает случайный ход.\n");
//    computerRandomMove();
//}
//
//// Настройка параметров игры
//void setupGame() {
//    setlocale(LC_ALL, "Rus");
//
//    printf("Введите размер игрового поля: ");
//    scanf("%d", &BOARD_SIZE);
//
//    printf("Введите длину выигрышной линии (от 3 до %d): ", BOARD_SIZE);
//    scanf("%d", &WIN_LENGTH);
//
//    if (WIN_LENGTH < 3 || WIN_LENGTH > BOARD_SIZE) {
//        printf("Некорректная длина выигрышной линии! Установлено значение %d\n", BOARD_SIZE);
//        WIN_LENGTH = BOARD_SIZE;
//    }
//}
//
//int main() {
//    srand(time(NULL));
//    setlocale(LC_ALL, "Rus");
//
//    printf("Добро пожаловать в СУПЕР-УМНЫЕ Крестики-нолики!\n");
//    printf("Компьютер будет находить и блокировать ВСЕ ваши последовательности!\n");
//
//    // Настройка игры
//    setupGame();
//
//    // Инициализация поля
//    initializeBoard();
//
//    printf("Вы играете за 'X', компьютер играет за 'O'\n");
//    printf("Размер поля: %dx%d, длина выигрышной линии: %d\n", BOARD_SIZE, BOARD_SIZE, WIN_LENGTH);
//
//    int gameOver = 0;
//    int currentPlayer = 1; // 1 - игрок, 0 - компьютер
//
//    while (!gameOver) {
//        displayBoard();
//
//        if (currentPlayer) {
//            // Ход игрока
//            playerMove();
//
//            if (checkWin(player)) {
//                displayBoard();
//                printf("Поздравляем! Вы выиграли!\n");
//                gameOver = 1;
//            }
//            else if (checkDraw()) {
//                displayBoard();
//                printf("Ничья!\n");
//                gameOver = 1;
//            }
//        }
//        else {
//            // Ход компьютера
//            computerSmartMove();
//
//            if (checkWin(computer)) {
//                displayBoard();
//                printf("Компьютер выиграл!\n");
//                gameOver = 1;
//            }
//            else if (checkDraw()) {
//                displayBoard();
//                printf("Ничья!\n");
//                gameOver = 1;
//            }
//        }
//
//        currentPlayer = !currentPlayer;
//    }
//
//    freeBoard();
//    return 0;
//}