#include <glew.h>
#include <glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define CELL_SIZE 100
#define VISIBLE_CELLS_X 8
#define VISIBLE_CELLS_Y 6

typedef enum { EMPTY, CROSS, CIRCLE } CellState;
typedef enum { MENU_SCREEN, GAME_SCREEN, SETTINGS_SCREEN } GameState;

// Структура для позиции на бесконечном поле
typedef struct {
    int x;
    int y;
} GridPosition;

// Структура для хранения клеток
typedef struct {
    GridPosition pos;
    CellState state;
} Cell;

// Глобальные переменные

GameState current_screen = MENU_SCREEN;
Cell* game_grid = NULL;
int grid_capacity = 0;
int grid_size = 0;
int view_offset_x = 0;
int view_offset_y = 0;
GridPosition cursor = { 0, 0 };
bool is_player_turn = true;

// Структура для кнопок
typedef struct {
    float x, y;
    float width, height;
    const char* text;
    bool is_hovered;
} Button;

Button start_button = { WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2, 200, 50, "START", false };
Button settings_button = { WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 70, 200, 50, "SETTINGS", false };
Button back_button = { 50, 50, 100, 40, "BACK", false };


// Размеры символов (ширина x высота)
float CHAR_WIDTH = 15.0f;
float CHAR_HEIGHT = 20.0f;
float CHAR_SPACING = 2.0f;

// Добавление/обновление клетки
void add_cell(int x, int y, CellState state) {
    for (int i = 0; i < grid_size; i++) {
        if (game_grid[i].pos.x == x && game_grid[i].pos.y == y) {
            game_grid[i].state = state;
            return;
        }
    }

    if (grid_size >= grid_capacity) {
        grid_capacity = grid_capacity == 0 ? 100 : grid_capacity * 2;
        game_grid = realloc(game_grid, grid_capacity * sizeof(Cell));
    }

    game_grid[grid_size].pos.x = x;
    game_grid[grid_size].pos.y = y;
    game_grid[grid_size].state = state;
    grid_size++;
}

// Получение состояния клетки
CellState get_cell_state(int x, int y) {
    for (int i = 0; i < grid_size; i++) {
        if (game_grid[i].pos.x == x && game_grid[i].pos.y == y) {
            return game_grid[i].state;
        }
    }
    return EMPTY;
}

//нолик
void drawcircle(float center_x, float center_y, float radius) {
    int segments = 36;
    glColor3f(1.0f, 0.2f, 0.2f); // Красный цвет
    glLineWidth(3.0f); // Толщина линии
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * 3.1415926f * (float)i / (float)segments;
        glVertex2f(center_x + radius * cosf(angle),
            center_y + radius * sinf(angle));
    }
    glEnd();
}
//крестик
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
// Функция для рисования символа линиями (улучшенная версия)
void drawchar(char c, float x, float y, float scale) {
    glPushMatrix(); // Сохраняем исходное состояние
    glTranslatef(x, y, 0); // Создаёт матрицу перемещения и умножает её на текущую матрицу
    glScalef(scale, scale, 1.0f); //Создаёт матрицу масштабирования и умножает её на текущую матрицу

    switch (toupper(c)) {
    case 'S':
        glBegin(GL_LINE_STRIP); //Соединяет точки последовательно
        glVertex2f(13, 2); glVertex2f(2, 2);
        glVertex2f(2, 10); glVertex2f(13, 10);
        glVertex2f(13, 19); glVertex2f(2, 19);
        glEnd();
        break;
    case 'T':
        glBegin(GL_LINES); //Рисует независимые пары линий
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
        glVertex2f(8, 10); glVertex2f(1, 10); glVertex2f(8, 10); glVertex2f(12, 13);
        glVertex2f(12, 15);  glVertex2f(10, 18); glVertex2f(9, 18);
        glVertex2f(1, 18); glVertex2f(1, 2);
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
        glBegin(GL_LINE_LOOP); // Замкнутая линия
        glVertex2f(5, 2); glVertex2f(10, 2);
        glVertex2f(13, 5); glVertex2f(13, 15);
        glVertex2f(10, 19); glVertex2f(5, 19);
        glVertex2f(2, 15); glVertex2f(2, 5);
        glEnd();
        break;
    }

    glPopMatrix();// Возвращаем исходную матрицу
}
// Улучшенная функция для рисования текста с выравниванием
void drawtext(char* text, float x, float y, float scale) {
    float total_width = 0;
    char* c;
    for (c = text; *c != '\0'; c++) {
        if (*c == ' ') total_width += 15 * scale;
        else total_width += CHAR_WIDTH * scale;
    }

    float start_x = x - total_width / 2;
    for (c = text; *c != '\0'; c++) {
        if (*c == ' ') start_x += 15 * scale;
        else {
            drawchar(*c, start_x, y - 10 * scale, scale);
            start_x += CHAR_WIDTH * scale;
        }
    }
}

void drawbutton(Button btn) {
    if (btn.is_hovered) glColor3f(0.5f, 0.5f, 0.8f); // Синеватый при наведении
    else glColor3f(0.7f, 0.7f, 0.7f); // Серый в обычном состоянии

    glBegin(GL_QUADS); //режим рисования четырёхугольников
    //углы
    glVertex2f(btn.x, btn.y); // Левый нижний
    glVertex2f(btn.x + btn.width, btn.y);// Правый нижний
    glVertex2f(btn.x + btn.width, btn.y + btn.height); // Правый верхний
    glVertex2f(btn.x, btn.y + btn.height); // Левый верхний
    glEnd();
    //рамка
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);// Толщина линии 2 пикселя
    glBegin(GL_LINE_LOOP); // Замкнутая линия
    glVertex2f(btn.x, btn.y);
    glVertex2f(btn.x + btn.width, btn.y);
    glVertex2f(btn.x + btn.width, btn.y + btn.height);
    glVertex2f(btn.x, btn.y + btn.height);
    glEnd();

    drawtext(btn.text, btn.x + btn.width / 2, btn.y + btn.height / 2, 0.7f);
}

void drawgrid() {
    glColor3f(0.2f, 0.2f, 0.2f);
    glLineWidth(1.0f);

    int start_x = view_offset_x / CELL_SIZE;
    int start_y = view_offset_y / CELL_SIZE;

    // Вертикальные линии
    for (int x = start_x; x <= start_x + VISIBLE_CELLS_X + 1; x++) {
        float screen_x = x * CELL_SIZE - view_offset_x;
        glBegin(GL_LINES);//Инициирует режим рисования несвязанных отрезков, 
        //где каждая пара вершин образует отдельную линию
        glVertex2f(screen_x, 0);
        glVertex2f(screen_x, VISIBLE_CELLS_Y * CELL_SIZE);
        glEnd();
    }

    // Горизонтальные линии
    for (int y = start_y; y <= start_y + VISIBLE_CELLS_Y + 1; y++) {
        float screen_y = y * CELL_SIZE - view_offset_y;
        glBegin(GL_LINES);
        glVertex2f(0, screen_y);
        glVertex2f(VISIBLE_CELLS_X * CELL_SIZE, screen_y);
        glEnd();
    }

    // Отрисовка клеток
    for (int x = start_x; x < start_x + VISIBLE_CELLS_X; x++) {
        for (int y = start_y; y < start_y + VISIBLE_CELLS_Y; y++) {
            float screen_x = x * CELL_SIZE - view_offset_x;
            float screen_y = y * CELL_SIZE - view_offset_y;

            CellState state = get_cell_state(x, y);
            if (state == CROSS) {
                glColor3f(0.0f, 0.5f, 1.0f);
                drawcross(screen_x, screen_y);
            }
            else if (state == CIRCLE) {
                glColor3f(1.0f, 0.2f, 0.2f);
                drawcircle(screen_x + CELL_SIZE / 2, screen_y + CELL_SIZE / 2, CELL_SIZE / 2 - 20);
            }
        }
    }

    // Отрисовка курсора выделение клетки
    float cursor_x = cursor.x * CELL_SIZE - view_offset_x;
    float cursor_y = cursor.y * CELL_SIZE - view_offset_y;

    glColor3f(0.0f, 1.0f, 0.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cursor_x + 5, cursor_y + 5);
    glVertex2f(cursor_x + CELL_SIZE - 5, cursor_y + 5);
    glVertex2f(cursor_x + CELL_SIZE - 5, cursor_y + CELL_SIZE - 5);
    glVertex2f(cursor_x + 5, cursor_y + CELL_SIZE - 5);
    glEnd();
}

// Отрисовка меню
void draw_menu() {
    glClear(GL_COLOR_BUFFER_BIT);//Это аналог "заливки холста" перед рисованием нового кадра.Очистка цветового буфера
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);//Цвет очистки
    drawtext("TIC TAC TOE", WINDOW_WIDTH / 2, 100, 1.5f);
    drawbutton(start_button);
    drawbutton(settings_button);
}

// Отрисовка настроек
void draw_settings() {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.8f, 0.9f, 0.9f, 1.0f);

    drawtext("SETTINGS", WINDOW_WIDTH / 2, 100, 1.5f);
    drawbutton(back_button);
}

// Отрисовка игры
void draw_game() {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);

    // Временное смещение для центрирования игрового поля
    glPushMatrix(); // Сохраняем исходное состояние
    glTranslatef((WINDOW_WIDTH - VISIBLE_CELLS_X * CELL_SIZE) / 2,
        (WINDOW_HEIGHT - VISIBLE_CELLS_Y * CELL_SIZE) / 2, 0); //выполняет центрирование игрового поля в окне

    drawgrid();
    drawbutton(back_button);

    glPopMatrix();// Восстанавливаем исходную матрицу
}

// Проверка наведения на кнопку
bool is_mouse_over_button(Button btn, double mouse_x, double mouse_y) {
    return (mouse_x >= btn.x && mouse_x <= btn.x + btn.width &&
        mouse_y >= btn.y && mouse_y <= btn.y + btn.height);
}

// Обработчик мыши
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (current_screen == MENU_SCREEN) {
            if (is_mouse_over_button(start_button, xpos, ypos)) {
                current_screen = GAME_SCREEN;
                // Сброс состояния игры
                free(game_grid);
                game_grid = NULL;
                grid_size = grid_capacity = 0;
                view_offset_x = view_offset_y = 0;
                cursor.x = cursor.y = 0;
                is_player_turn = true;
            }
            else if (is_mouse_over_button(settings_button, xpos, ypos)) {
                current_screen = SETTINGS_SCREEN;
            }
        }
        else if (current_screen == GAME_SCREEN) {
            // Проверка клика по кнопке BACK
            if (is_mouse_over_button(back_button, xpos, ypos)) {
                current_screen = MENU_SCREEN;
            }
        }
        else if (current_screen == SETTINGS_SCREEN) {
            if (is_mouse_over_button(back_button, xpos, ypos)) {
                current_screen = MENU_SCREEN;
            }
        }
    }
}

// Обработчик клавиатуры
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (current_screen == GAME_SCREEN && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_UP:
            if (cursor.y > INT_MIN) cursor.y--;
            if (cursor.y * CELL_SIZE - view_offset_y < 0)
                view_offset_y -= CELL_SIZE;
            break;
        case GLFW_KEY_DOWN:
            if (cursor.y < INT_MAX) cursor.y++;
            if (cursor.y * CELL_SIZE - view_offset_y > VISIBLE_CELLS_Y * CELL_SIZE)
                view_offset_y += CELL_SIZE;
            break;
        case GLFW_KEY_LEFT:
            if (cursor.x > INT_MIN) cursor.x--;
            if (cursor.x * CELL_SIZE - view_offset_x < 0)
                view_offset_x -= CELL_SIZE;
            break;
        case GLFW_KEY_RIGHT:
            if (cursor.x < INT_MAX) cursor.x++;
            if (cursor.x * CELL_SIZE - view_offset_x > VISIBLE_CELLS_X * CELL_SIZE)
                view_offset_x += CELL_SIZE;
            break;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_SPACE:
            if (get_cell_state(cursor.x, cursor.y) == EMPTY) {
                add_cell(cursor.x, cursor.y, is_player_turn ? CROSS : CIRCLE);
                is_player_turn = !is_player_turn;
            }
            break;
        }
    }
}

// Обновление состояния наведения
void update_hover_state(GLFWwindow* window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (current_screen == MENU_SCREEN) {
        start_button.is_hovered = is_mouse_over_button(start_button, xpos, ypos);
        settings_button.is_hovered = is_mouse_over_button(settings_button, xpos, ypos);
    }
    else if (current_screen == GAME_SCREEN || current_screen == SETTINGS_SCREEN) {
        back_button.is_hovered = is_mouse_over_button(back_button, xpos, ypos);
    }
}

// Изменение размера окна
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

int main() {

    if (!glfwInit()) {// Инициализирует библиотеку GLFW.
        return -1;
    }

    // Устанавливает параметры для будущего окна.
    //GLFW_RESIZABLE — это флаг, указывающий, можно ли изменять размер окна.
    //GLFW_FALSE (или 0) — значение, которое запрещает изменение размера.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);// Запрещает изменение размера окна (GLFW_RESIZABLE = GLFW_FALSE)

    //Возвращает: Указатель на GLFWwindow или NULL при ошибке.
    //Создает окно и контекст OpenGL
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Infinite Tic Tac Toe", NULL, NULL);
    // если NULL, окно создаётся в оконном режиме, если NULL, контекст OpenGL не разделяется с другими окнами

    if (!window) {
        glfwTerminate();
        return -1;
    }

    //Все последующие OpenGL - команды будут применяться к этому окну.
    glfwMakeContextCurrent(window);//Активирует контекст OpenGL для окна

    glfwSetKeyCallback(window, key_callback);//Регистрирует функцию key_callback для обработки нажатий 
    glfwSetMouseButtonCallback(window, mouse_button_callback);// Регистрирует функцию mouse_button_callback для кликов мыши.

    // Дублирует запрет изменения размера окна (на случай, если glfwWindowHint не сработал)
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);//позволяет изменять свойства окна после его создания

    if (glewInit() != GLEW_OK) { //Инициализирует GLEW
        return -1;
    }

    // Настройка проекции
    glMatrixMode(GL_PROJECTION);// матрица проекции (как объекты проецируются на экран)
    glLoadIdentity();//Сбрасывает её

    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1); //Устанавливает ортографическую проекцию (2D-режим)
    // Теперь координаты:
    // X: от 0 (лево) до WINDOW_WIDTH (право)
    // Y: от 0 (верх) до WINDOW_HEIGHT (низ)
    // Z: от -1 (близко) до 1 (далеко)

    glMatrixMode(GL_MODELVIEW); //Переключает обратно на модельно-видовую матрицу, чтобы начать размещать объекты в сцене.

    // Главный цикл
    while (!glfwWindowShouldClose(window)) {//пока окно не закрыто
        glfwPollEvents(); // Обрабатывает события (клавиатура, мышь)

        update_hover_state(window); //Обновляет состояние кнопок (например, подсветка при наведении)
        /* Обновляет состояние "наведения" (hover)для UI - элементов
         Проверяет позицию курсора относительно кнопок*/

        switch (current_screen) {//Отрисовывает текущий экран (меню/игру/настройки)
        case MENU_SCREEN:
            draw_menu();
            break;
        case GAME_SCREEN:
            draw_game();
            break;
        case SETTINGS_SCREEN:
            draw_settings();
            break;
        }

        glfwSwapBuffers(window);//Меняет буферы для плавного рендеринга.
    }

    free(game_grid);
    glfwTerminate();//Освобождает все ресурсы GLFW (закрывает окна, удаляет контексты OpenGL и т. д.).
    return 0;
}