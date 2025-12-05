#include <glew.h>
#include <glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define CELL_SIZE 100
#define VISIBLE_CELLS_X 8
#define VISIBLE_CELLS_Y 8

typedef enum { EMPTY, CROSS, CIRCLE } CellState; //представления состояния клетки
typedef enum { MENU_SCREEN, GAME_SCREEN, SETTINGS_SCREEN } ScreenState; //для представления текущего экрана приложения


//Хранит координаты клетки на игровом поле.
typedef struct {
    int x;//столбец
    int y;//строка
} GridPosition;

typedef struct {
    GridPosition pos; //Позиция клетки
    CellState state; // Состояние (пусто, нолик, крестик)
} Cell;

typedef struct {
    float x, y; // Позиция кнопки
    float width;// Размеры
    float height; 
    char* text;// Текст на кнопке
    bool is_mouse;// Наведена ли мышь
} Button;


//Главная структура игры
typedef struct {
    ScreenState current_screen;  // Текущий экран (меню, игра, настройки)
    Cell* game_grid;// Динамический массив клеток
    int grid_size; //сколько памяти выделено под game_grid
    int grid_count;// сколько клеток фактически используется
    int view_offset_x; // смещение "камеры" для бесконечного поля
    int view_offset_y;// Смещение камеры по Y
    GridPosition cursor;         // Позиция курсора (выделенной клетки)
    bool is_player_turn;         // Чей ход (игрок или компьютер)
    Button start_button;         // Кнопка "START"
    Button settings_button;      // Кнопка "SETTINGS"
    Button back_button;          // Кнопка "BACK"
    float char_width;            // Ширина символа для текста
    float char_height;           // Высота символа
    float char_spacing;          // Расстояние между символами
} GameContext;

//начальное состояние игры
void init_game_context(GameContext* ctx) {
    ctx->current_screen = MENU_SCREEN; //начинаем с меню
    ctx->game_grid = NULL; //пока без выделенной памяти
    ctx->grid_size = 0;
    ctx->grid_count = 0;
    ctx->view_offset_x = 0;
    ctx->view_offset_y = 0;
    ctx->cursor = (GridPosition){ 0, 0 };
    ctx->is_player_turn = true; //первый крестик, ходит игрок
    ctx->start_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2, 200, 50, "START", false };
    ctx->settings_button = (Button){ WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 70, 200, 50, "SETTINGS", false };
    ctx->back_button = (Button){ 50, 50, 100, 40, "BACK", false };
    //для символов
    ctx->char_width = 15.0f;
    ctx->char_height = 20.0f;
    ctx->char_spacing = 2.0f;
}

// Добавление/обновление клетки 
void add_cell(GameContext* ctx, int x, int y, CellState state) {
    for (int i = 0; i < ctx->grid_count; i++) { //проходим по всем клеткам
        if (ctx->game_grid[i].pos.x == x && ctx->game_grid[i].pos.y == y) { //если клетка уже сущ-ет 
            return;
        }
    }

    if (ctx->grid_count >= ctx->grid_size) { //заполнен ли текущий массив
        ctx->grid_size = ctx->grid_size == 0 ? 100 : ctx->grid_size * 2; //если пусто, выделяем 100 клеток, иначе удваиваем
        ctx->game_grid = realloc(ctx->game_grid, ctx->grid_size * sizeof(Cell));
    }
    //добавляем фигуру
    ctx->game_grid[ctx->grid_count].pos.x = x;
    ctx->game_grid[ctx->grid_count].pos.y = y;
    ctx->game_grid[ctx->grid_count].state = state;
    ctx->grid_count++;
}

//проверка состояния клетки
CellState get_cell_state(GameContext* ctx, int x, int y) {
    for (int i = 0; i < ctx->grid_count; i++) {
        if (ctx->game_grid[i].pos.x == x && ctx->game_grid[i].pos.y == y) {
            return ctx->game_grid[i].state;
        }
    }
    return EMPTY;
}



//нолик
void drawcircle(float center_x, float center_y, float radius) {
    int segments = 45;
    glColor3f(0.0f, 0.0f, 1.0f);
    glLineWidth(3.0f);//толщина линии
    glBegin(GL_LINE_LOOP);//рисование замкнутой линии
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * 3.1415926f * (float)i / (float)segments; //для кажого сигмента(угла) вычисляется координата
        glVertex2f(center_x + radius * cosf(angle), center_y + radius * sinf(angle));//добавляет точку в контур
    }
    glEnd();
}

//крестик
void drawcross(float x, float y) { //левый верхний угол
    glColor3f(0.0f, 0.5f, 1.0f);
    glLineWidth(5.0f);
    glBegin(GL_LINES);
    glVertex2f(x + 20, y + 20);//отступ 20 пикселей от края клетки
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

    switch (c) {
    /*case 'А':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 20); glVertex2f(8, 0);
        glVertex2f(14, 20);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(4, 10); glVertex2f(12, 10);
        glEnd();
        break;
    case 'Б':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 20); glVertex2f(12, 20);
        glVertex2f(14, 18); glVertex2f(14, 12);
        glVertex2f(12, 10); glVertex2f(2, 10);
        glVertex2f(2, 0); glVertex2f(14, 0);
        glEnd();
        break;
    case 'В':
        glBegin(GL_LINE_STRIP);
        glVertex2f(2, 20); glVertex2f(2, 0);
        glVertex2f(12, 0); glVertex2f(14, 2);
        glVertex2f(14, 8); glVertex2f(12, 10);
        glVertex2f(2, 10);
        glVertex2f(12, 10); glVertex2f(14, 12);
        glVertex2f(14, 18); glVertex2f(12, 20);
        glVertex2f(2, 20);
        glEnd();
        break;*/
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
//рисования текста с выравниванием
void drawtext(GameContext* ctx, char* text, float x, float y, float scale) { 
    float total_width = 0; //общая ширина текста
    char* c;

    for (c = text; *c != '\0'; c++) {
        if (*c == ' ') total_width += 15 * scale; //ширина пробела
        else total_width += ctx->char_width * scale; //ширина обычного символа
    }

    float start_x = x - total_width / 2; //выравниваем по центру
    for (c = text; *c != '\0'; c++) {
        if (*c == ' ') start_x += 15 * scale;
        else {
            drawchar(*c, start_x, y - 10 * scale, scale);
            start_x += ctx->char_width * scale;
        }
    }
}

void drawbutton(GameContext* ctx, Button btn) {
    if (btn.is_mouse) glColor3f(0.0f, 0.5f, 0.5f); //  при наведении
    else glColor3f(0.7f, 0.7f, 0.7f); // в обычном состоянии

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

    drawtext(ctx, btn.text, btn.x + btn.width / 2, btn.y + btn.height / 2, 0.7f);
}

void drawgrid(GameContext* ctx) {
    glColor3f(0.0f, 0.1f, 0.1f);//Dark blue
    glLineWidth(1.0f);

    // Рассчитываем видимую область с учетом отрицательных координат
    int start_x = floor((float)ctx->view_offset_x / CELL_SIZE);
    int start_y = floor((float)ctx->view_offset_y / CELL_SIZE);

    // Вертикальные линии
    for (int x = start_x; x <= start_x + VISIBLE_CELLS_X + 1; x++) {
        float screen_x = x * CELL_SIZE - ctx->view_offset_x;
        glBegin(GL_LINES);
        glVertex2f(screen_x, 0);
        glVertex2f(screen_x, VISIBLE_CELLS_Y * CELL_SIZE);
        glEnd();
    }


    // Горизонтальные линии
    for (int y = start_y; y <= start_y + VISIBLE_CELLS_Y + 1; y++) {
        float screen_y = y * CELL_SIZE - ctx->view_offset_y;
        glBegin(GL_LINES);
        glVertex2f(0, screen_y);
        glVertex2f(VISIBLE_CELLS_X * CELL_SIZE, screen_y);
        glEnd();
    }

    // Отрисовка содержимого клеток
    for (int x = start_x; x < start_x + VISIBLE_CELLS_X; x++) {
        for (int y = start_y; y < start_y + VISIBLE_CELLS_Y; y++) {
            float screen_x = x * CELL_SIZE - ctx->view_offset_x;
            float screen_y = y * CELL_SIZE - ctx->view_offset_y;

            CellState state = get_cell_state(ctx, x, y);
            if (state == CROSS) {
                drawcross(screen_x, screen_y);
            }
            else if (state == CIRCLE) {
                drawcircle(screen_x + CELL_SIZE / 2, screen_y + CELL_SIZE / 2, CELL_SIZE / 2 - 20);
            }
        }
    }

    // Отрисовка курсора выделение клетки
    float cursor_x = ctx->cursor.x * CELL_SIZE - ctx->view_offset_x;
    float cursor_y = ctx->cursor.y * CELL_SIZE - ctx->view_offset_y;

    glColor3f(1.0f, 0.0f, 1.0f);//Purple
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cursor_x + 5, cursor_y + 5);
    glVertex2f(cursor_x + CELL_SIZE - 5, cursor_y + 5);
    glVertex2f(cursor_x + CELL_SIZE - 5, cursor_y + CELL_SIZE - 5);
    glVertex2f(cursor_x + 5, cursor_y + CELL_SIZE - 5);
    glEnd();
}

// Отрисовка меню
void draw_menu(GameContext* ctx) {
    glClear(GL_COLOR_BUFFER_BIT);//Это аналог "заливки холста" перед рисованием нового кадра.Очистка цветового буфера
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);//Цвет очистки
    drawtext(ctx, "TIC TAC TOE", WINDOW_WIDTH / 2, 100, 1.5f);
    drawbutton(ctx, ctx->start_button);
    drawbutton(ctx, ctx->settings_button);
}

// Отрисовка настроек
void draw_settings(GameContext* ctx){
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.8f, 0.9f, 0.9f, 1.0f);

    drawtext(ctx, "SETTINGS", WINDOW_WIDTH / 2, 100, 1.5f);
    drawbutton(ctx, ctx->back_button);
}

// Отрисовка игры
void draw_game(GameContext* ctx) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);

    // Временное смещение для центрирования игрового поля
    glPushMatrix(); // Сохраняем исходное состояние
    glTranslatef((WINDOW_WIDTH - VISIBLE_CELLS_X * CELL_SIZE) / 2,
        (WINDOW_HEIGHT - VISIBLE_CELLS_Y * CELL_SIZE) / 2, 0); //выполняет центрирование игрового поля в окне

    drawgrid(ctx);
    drawbutton(ctx, ctx->back_button);
    glPopMatrix();// Восстанавливаем исходную матрицу
}

// Проверка наведения на кнопку
bool is_mouse_over_button(Button btn, double mouse_x, double mouse_y) {
    return (mouse_x >= btn.x && mouse_x <= btn.x + btn.width &&
        mouse_y >= btn.y && mouse_y <= btn.y + btn.height);
}

// Обработчик кликов мыши
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    GameContext* ctx = (GameContext*)glfwGetWindowUserPointer(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { //левая кнопка нажата
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos); //кординаты курсора

        if (ctx->current_screen == MENU_SCREEN) {
            if (is_mouse_over_button(ctx->start_button, xpos, ypos)) {
                ctx->current_screen = GAME_SCREEN;
                free(ctx->game_grid);
                ctx->grid_count = ctx->grid_size = ctx->view_offset_x = ctx->view_offset_y = ctx->cursor.x = ctx->cursor.y = 0;
                ctx->is_player_turn = true;
            }
            else if (is_mouse_over_button(ctx->settings_button, xpos, ypos)) {
                ctx->current_screen = SETTINGS_SCREEN;
            }
        }
        else if (ctx->current_screen == GAME_SCREEN) {
            if (is_mouse_over_button(ctx->back_button, xpos, ypos)) {
                ctx->current_screen = MENU_SCREEN;
            }
        }
        else if (ctx->current_screen == SETTINGS_SCREEN) {
            if (is_mouse_over_button(ctx->back_button, xpos, ypos)) {
                ctx->current_screen = MENU_SCREEN;
            }
        }
    }
}

// Обработчик клавиатуры
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    GameContext* ctx = (GameContext*)glfwGetWindowUserPointer(window); //получение доступа ко всем данным
    if (ctx->current_screen == GAME_SCREEN && action == GLFW_PRESS) { //когда в игровом экране и клавиша нажата
        switch (key) { 
        case GLFW_KEY_UP:
            ctx->cursor.y--;
            if (ctx->cursor.y * CELL_SIZE - ctx->view_offset_y < 0) {
                ctx->view_offset_y = ctx->cursor.y * CELL_SIZE;
            }
            break;
        case GLFW_KEY_DOWN:
            ctx->cursor.y++;
                    //Позиция курсора на экране                              //Границы видимой области
            if (ctx->cursor.y * CELL_SIZE - ctx->view_offset_y > (VISIBLE_CELLS_Y - 1) * CELL_SIZE) {
                ctx->view_offset_y = ctx->cursor.y * CELL_SIZE - (VISIBLE_CELLS_Y - 1) * CELL_SIZE;
            }
            break;
        case GLFW_KEY_LEFT:
            ctx->cursor.x--;
            if (ctx->cursor.x * CELL_SIZE - ctx->view_offset_x < 0) {
                ctx->view_offset_x = ctx->cursor.x * CELL_SIZE;
            }
            break;
        case GLFW_KEY_RIGHT:
            ctx->cursor.x++;
            if (ctx->cursor.x * CELL_SIZE - ctx->view_offset_x > (VISIBLE_CELLS_X - 1) * CELL_SIZE) {
                ctx->view_offset_x = ctx->cursor.x * CELL_SIZE - (VISIBLE_CELLS_X - 1) * CELL_SIZE;
            }
            break;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_SPACE:
            if (get_cell_state(ctx, ctx->cursor.x, ctx->cursor.y) == EMPTY) {
                add_cell(ctx, ctx->cursor.x, ctx->cursor.y, ctx->is_player_turn ? CROSS : CIRCLE);
                ctx->is_player_turn = !ctx->is_player_turn;
            }
            break;
        }
    }
}

// Обновление состояния при наведения
void update_hover_state(GLFWwindow* window, GameContext* ctx) {
    double xpos, ypos; //позиция курсора
    glfwGetCursorPos(window, &xpos, &ypos); //тек корд-тф мыши относительно окна

    if (ctx->current_screen == MENU_SCREEN) {
        ctx->start_button.is_mouse = is_mouse_over_button(ctx->start_button, xpos, ypos);
        ctx->settings_button.is_mouse = is_mouse_over_button(ctx->settings_button, xpos, ypos);
    }
    else if (ctx->current_screen == GAME_SCREEN || ctx->current_screen == SETTINGS_SCREEN) {
        ctx->back_button.is_mouse = is_mouse_over_button(ctx->back_button, xpos, ypos);
    }
}


// Изменение размера окна
//void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
//    glViewport(0, 0, width, height);
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//    glOrtho(0, width, height, 0, -1, 1);
//    glMatrixMode(GL_MODELVIEW);
//}

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
    GameContext ctx;
    init_game_context(&ctx);
    glfwSetWindowUserPointer(window, &ctx);
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

        update_hover_state(window, &ctx); //Обновляет состояние кнопок (например, подсветка при наведении)
       /* Обновляет состояние "наведения" (hover)для UI - элементов 
        Проверяет позицию курсора относительно кнопок*/
            switch (ctx.current_screen) {//Отрисовывает текущий экран (меню/игру/настройки)
            case MENU_SCREEN: draw_menu(&ctx); break;
            case GAME_SCREEN: draw_game(&ctx); break;
            case SETTINGS_SCREEN: draw_settings(&ctx); break;
            }


        glfwSwapBuffers(window);//Меняет буферы для плавного рендеринга.
    }

    free(ctx.game_grid);
    glfwTerminate();//Освобождает все ресурсы GLFW (закрывает окна, удаляет контексты OpenGL и т. д.).
    return 0;
}