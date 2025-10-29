
#include <iostream> 
#include <string>   
#include <vector>   
#include <cstdlib> 
#include <ctime>  
#include <cstdio>  
#include <cctype>  
#include <thread>   
#include <chrono>  
#include <conio.h> 

using namespace std;

const char CHAR_EMPTY = ' ';
const char CHAR_WALL = '#';
const char CHAR_SNAKE = 'O';
const char CHAR_SNAKE_HEAD = '@';
const char CHAR_FOOD = 'o';

// Структура для координат точек (x, y)
struct Point { int x, y; };
enum Direction { UP, DOWN, LEFT, RIGHT };

int field_Width = 40;
int field_Height = 20;
bool KEY[256];  // массив клавиш


// Перемещение курсора в нужную позицию через ANSI-коды
void SetCursorPosition(int x, int y) {
    printf("\x1b[%d;%dH", y + 1, x + 1);
    fflush(stdout);
}

// Очистка экрана ANSI-кодами
void ClearScreen() {
    printf("\x1b[2J");
    SetCursorPosition(0, 0);
    fflush(stdout);
}

// Скрыть/показать курсор (ANSI)
void HideCursor() { printf("\x1b[?25l"); fflush(stdout); }
void ShowCursor() { printf("\x1b[?25h"); fflush(stdout); }

// Получение размера консоли через PowerShell
void GetConsoleSize(int& cols, int& rows) {

    FILE* pipe = _popen(
        "powershell -NoProfile -Command \"[Console]::WindowWidth; [Console]::WindowHeight\"",
        "rt"
    );
    if (pipe) {
        int w = 0, h = 0;
        if (fscanf_s(pipe, "%d %d", &w, &h) == 2) {
            if (w > 0 && h > 0) {
                cols = w;
                rows = h;
                _pclose(pipe);
                return;
            }
        }
        _pclose(pipe);
    }
}


// Меняем размер консоли через
void TryResizeConsole(int cols, int rows) {
    if (cols < 10) cols = 10;
    if (rows < 6) rows = 6;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "mode con cols=%d lines=%d", cols, rows);
    system(cmd);
}


void GetKEY() {
    // Обнуляем массив состояний
    for (int i = 0; i < 256; ++i) KEY[i] = 0;

    // Считываем все нажатия в буфере
    while (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            int s = _getch();
            if (s == 72) { KEY['W'] = KEY['w'] = 1; }
            else if (s == 80) { KEY['S'] = KEY['s'] = 1; }
            else if (s == 75) { KEY['A'] = KEY['a'] = 1; }
            else if (s == 77) { KEY['D'] = KEY['d'] = 1; }
        }
        else {
            KEY[toupper(ch & 0xFF)] = 1;
            if (ch == 27) KEY[27] = 1; // ESC
        }
    }
}

// Очищаем ввод 
void ClearInputBuffer() {
    while (_kbhit()) _getch();
}

// Генерация точки еды в границах поля
Point RandomPoint() {
    Point p;
    p.x = 1 + rand() % max(1, field_Width - 2);
    p.y = 1 + rand() % max(1, field_Height - 2);
    return p;
}

// Проверка на попадание в тело змеи
bool IsPointOnSnake(const Point& p, const vector<Point>& snake, int snake_Len) {
    for (int i = 0; i < snake_Len; ++i) if (snake[i].x == p.x && snake[i].y == p.y) return true;
    return false;
}

// Рисование одной ячейки с учётом смещения (offsetX, offsetY) — нужно для центрирования
void DrawCell(const Point& p, char ch, int offsetX, int offsetY) {
    SetCursorPosition(offsetX + p.x, offsetY + p.y);
    putchar(ch);
    fflush(stdout);
}

// Отрисовка поля (стены + пустые клетки)
void DrawField(int offsetX, int offsetY) {
    for (int y = 0; y < field_Height; ++y) {
        for (int x = 0; x < field_Width; ++x) {
            char ch = (y == 0 || y == field_Height - 1 || x == 0 || x == field_Width - 1) ? CHAR_WALL : CHAR_EMPTY;
            SetCursorPosition(offsetX + x, offsetY + y);
            putchar(ch);
        }
    }
    fflush(stdout);
}

// Запрос на повтор игры
char AskReplay() {
    while (true) {
        ClearInputBuffer();
        cout << "Сыграть ещё раз? (Y/N): ";
        string input;
        getline(cin, input);
        if (input.empty()) continue;
        char c = toupper(input[0]);
        if (c == 'Y' || c == 'N') return c;
        cout << "Введите только Y или N.\n";
    }
}

int main() {
    setlocale(LC_ALL, "");          
    srand((unsigned int)time(0));

    while (true) {

        int consoleCols = 80, consoleRows = 25;
        GetConsoleSize(consoleCols, consoleRows); // начальная консоль

        cout << "Текущее окно консоли (cols x rows): " << consoleCols << " x " << consoleRows << '\n';
        cout << "Если хотите использовать размер консоли — просто нажмите Enter при вводе ширины/высоты.\n";

        int tickMs = 150; // скорость
        cout << "Введите скорость игры (20-1000, по умолчанию 150): ";
        string input;
        getline(cin, input);
        if (!input.empty()) {
            try { int v = stoi(input); if (v >= 20 && v <= 1000) tickMs = v; }
            catch (...) {}
        }

        cout << "Введите ширину поля (15-235) или Enter: ";
        string wStr; getline(cin, wStr);
        bool userW = false, userH = false;
        int inW = 0, inH = 0;
        if (!wStr.empty()) {
            try { int v = stoi(wStr); if (v >= 10 && v <= 235) { inW = v; userW = true; } }
            catch (...) {}
        }

        cout << "Введите высоту поля (10-65) или Enter: ";
        string hStr; getline(cin, hStr);
        if (!hStr.empty()) {
            try { int v = stoi(hStr); if (v >= 10 && v <= 65) { inH = v; userH = true; } }
            catch (...) {}
        }

        GetConsoleSize(consoleCols, consoleRows);

        const int MIN_W = 15, MIN_H = 10;
        const int MAX_W = 235, MAX_H = 65;


        // Проверка ширины и высоты
        if (!userW && !userH) {
            field_Width  = max(MIN_W, min(MAX_W, consoleCols));
            field_Height = max(MIN_H, min(MAX_H, consoleRows - 2));
        }
        // если введена только ширина
        else if (userW && !userH) {
            field_Width  = max(MIN_W, min(MAX_W, inW));
            field_Height = max(MIN_H, min(MAX_H, consoleRows - 2));
        }
        // если введена только высота
        else if (!userW && userH) {
            field_Width  = max(MIN_W, min(MAX_W, consoleCols));
            field_Height = max(MIN_H, min(MAX_H, inH));
        }
        // если обе введены
        else {
            field_Width  = max(MIN_W, min(MAX_W, inW));
            field_Height = max(MIN_H, min(MAX_H, inH));
        }

        // Цель
        int maxPossibleScore = max(1, (field_Width - 2) * (field_Height - 2) - 1);
        int targetScore = 10;
        cout << "Введите целевое количество очков для победы (1-" << maxPossibleScore << ", Enter=10): ";
        string scoreStr; getline(cin, scoreStr);
        if (!scoreStr.empty()) {
            try { int v = stoi(scoreStr); if (v >= 1 && v <= maxPossibleScore) targetScore = v; }
            catch (...) {}
        }

        GetConsoleSize(consoleCols, consoleRows);
        bool isFullscreenLike = (consoleCols >= 237 && consoleRows >= 63);
        
        // Адаптация консоли под игровое поле (свернутая консоль)
        if (!isFullscreenLike) {
            TryResizeConsole(field_Width, field_Height + 2);
            GetConsoleSize(consoleCols, consoleRows);
        }

        // Вычисляем смещение (центрирование поля полностью - стены + содержимое)
        int offsetX = 0, offsetY = 0;
        if (consoleCols > field_Width) offsetX = (consoleCols - field_Width) / 2;
        if (consoleRows > (field_Height + 2)) offsetY = (consoleRows - (field_Height + 2)) / 2;

        // Рисуем поле и запускаем игру
        ClearScreen();
        DrawField(offsetX, offsetY);
        HideCursor();

        // Инициализация
        vector<Point> snake;
        snake.clear();
        snake.push_back({ field_Width / 2, field_Height / 2 });
        int snake_Len = 1;
        Point food = RandomPoint();
        while (IsPointOnSnake(food, snake, snake_Len)) food = RandomPoint();

        // Нарисовать начальные объекты с учётом ограничений
        DrawCell(food, CHAR_FOOD, offsetX, offsetY);
        DrawCell(snake[0], CHAR_SNAKE_HEAD, offsetX, offsetY);

        Direction dir = RIGHT, nextDir = RIGHT;
        bool gameOver = false, victory = false;
        int score = 0;

        // Перед стартом чистим мусор
        ClearInputBuffer();
        GetKEY();

        // Главный цикл
        while (!gameOver && !victory) {
            GetKEY();
            if ((KEY['W'] || KEY['w']) && dir != DOWN) nextDir = UP;
            else if ((KEY['S'] || KEY['s']) && dir != UP) nextDir = DOWN;
            else if ((KEY['A'] || KEY['a']) && dir != RIGHT) nextDir = LEFT;
            else if ((KEY['D'] || KEY['d']) && dir != LEFT) nextDir = RIGHT;
            if (KEY[27]) { gameOver = true; break; }

            dir = nextDir;

            // Стираем хвост (визуально)
            if (snake_Len > 0) {
                DrawCell(snake[snake_Len - 1], CHAR_EMPTY, offsetX, offsetY);
            }

            // Сдвигаем тело
            if (snake_Len > 1) for (int i = snake_Len - 2; i >= 0; --i) snake[i + 1] = snake[i];

            // Передвижение головы
            switch (dir) {
            case UP: snake[0].y--; break;
            case DOWN: snake[0].y++; break;
            case LEFT: snake[0].x--; break;
            case RIGHT: snake[0].x++; break;
            }

            // Рисуем голову и следующий сегмент
            DrawCell(snake[0], CHAR_SNAKE_HEAD, offsetX, offsetY);
            if (snake_Len > 1) DrawCell(snake[1], CHAR_SNAKE, offsetX, offsetY);

            // Столкновение со стеной
            if (snake[0].x <= 0 || snake[0].x >= field_Width - 1 || snake[0].y <= 0 || snake[0].y >= field_Height - 1) {
                gameOver = true; break;
            }
            // Столкновение с хвостом
            for (int i = 1; i < snake_Len; ++i) if (snake[i].x == snake[0].x && snake[i].y == snake[0].y) { gameOver = true; break; }
            if (gameOver) break;

            // Поедание еды
            if (snake[0].x == food.x && snake[0].y == food.y) {
                ++score;
                ++snake_Len;
                snake.push_back(snake.back()); // новый сегмент
                if (score >= targetScore) { victory = true; break; }
                do { food = RandomPoint(); } while (IsPointOnSnake(food, snake, snake_Len));
                DrawCell(food, CHAR_FOOD, offsetX, offsetY);
            }

            // Статусная строка (нижняя строка отрисовки)
            SetCursorPosition(offsetX, offsetY + field_Height);
            printf("Счёт: %d / %d    ", score, targetScore);
            fflush(stdout);

            std::this_thread::sleep_for(std::chrono::milliseconds(tickMs));
        }

        // Завершение / вывод результатов
        ShowCursor();
        SetCursorPosition(0, offsetY + field_Height + 1);
        if (victory) cout << "ПОЗДРАВЛЯЮ! Вы набрали " << score << " из " << targetScore << '\n';
        else cout << "Игра окончена! Счёт: " << score << '\n';

        if (AskReplay() == 'N') break;
    }

    return 0;
}
