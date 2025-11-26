
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

struct Point { int x, y; };
enum Direction { UP, DOWN, LEFT, RIGHT };

int field_Width = 0, field_Height = 0;
int consoleCols, consoleRows;
int tickMs;
const int RATIO = 2;
int targetScore;
bool repeat = false;
bool userW, userH;
bool isFullscreenLike;
bool stopInputThread;
bool keyUp = false, keyDown = false,
keyLeft = false, keyRight = false, keyEsc = false;

// Перемещение курсора в нужную позицию через ANSI-коды
void SetCursorPosition(int x, int y) {
    printf("\x1b[%d;%dH", y + 1, x + 1);
}

// Очистка экрана ANSI-кодами
void ClearScreen() {
    printf("\x1b[2J");
    SetCursorPosition(0, 0);
}

// Скрыть/показать курсор (ANSI)
void HideCursor() { printf("\x1b[?25l"); }
void ShowCursor() { printf("\x1b[?25h"); }


// Получение размеров консоли
void GetConsoleSize(int& cols, int& rows)
{
    printf("\033[999C\033[999B\x1b[6n");

    std::string resp;

    while (true) {
        resp.push_back((char)_getch());
        if (resp.back() == 'R') break;
    }

    // Парсим 
    int r = 0, c = 0;
    if (sscanf_s(resp.c_str(), "\x1b[%d;%dR", &r, &c) == 2) {
        rows = r;
        cols = c;
    }
    printf("\x1b[2J\x1b[H");
}

// Меняем размер консоли через
void TryResizeConsole(int cols, int rows) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "mode con cols=%d lines=%d", cols, rows);
    system(cmd);
}

void KeyboardListen() {
    while (!stopInputThread) {
        if (_kbhit()) {
            int ch = _getch();

            switch (ch) {
            case 'W': case 'w': keyUp = true; break;
            case 'S': case 's': keyDown = true; break;
            case 'A': case 'a': keyLeft = true; break;
            case 'D': case 'd': keyRight = true; break;
            case 27:            keyEsc = true; break;

                // стрелки
            case 224: {
                int arrow = _getch();
                if (arrow == 72) keyUp = true;
                if (arrow == 80) keyDown = true;
                if (arrow == 75) keyLeft = true;
                if (arrow == 77) keyRight = true;
                break;
            }
            }
        }
        this_thread::sleep_for(chrono::milliseconds(1));
    }
}

void ResetKeys() {
    keyUp = keyDown = keyLeft = keyRight = keyEsc = false;
}

int ReadInt(const string& text, int minVal, int maxVal, int defaultValue) {
    while (true) {
        cout << text;
        string s;
        getline(cin, s);

        if (s.empty())
            if (defaultValue > 1) return defaultValue;
            else if (defaultValue == 0) {
                userW = false;
                break;
            }
            else if (defaultValue == 1) {
                userH = false;
                break;
            }
        try {
            int v = stoi(s);
            if (v < minVal || v > maxVal) {
                cout << "Ошибка: число должно быть в диапазоне от " << minVal << " до " << maxVal << ".\n";
                continue;
            }
            return v;
        }
        catch (...) {
            cout << "Ошибка: введите целое число.\n";
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
}

// Отрисовка поля (стены + пустые клетки)
void DrawField(int offsetX, int offsetY) {
    for (int y = 0; y <= field_Height; ++y) {
        for (int x = 0; x < field_Width; ++x) {
            char ch = (y == 0 || y == field_Height || x == 0 || x == field_Width - 1) ? CHAR_WALL : CHAR_EMPTY;
            SetCursorPosition(offsetX + x, offsetY + y);
            putchar(ch);
        }
    }
}

// Меню повтора игры
char MenuReplay(int& cW, int& cH, bool& repeat, bool& isFullscreenLike, bool& victory, int& score) {
    repeat = false;
    while (true) {
        int setX = cW / 2 - 10;
        int setY = cH / 2 - 5;
        if (isFullscreenLike) {
            SetCursorPosition(setX, setY);
            if (victory) cout << "ПОЗДРАВЛЯЮ! Вы набрали " << score << " из " << targetScore << '\n';
            else cout << "Игра окончена! Счёт: " << score << '\n';
            SetCursorPosition(setX, setY + 1);
            cout << "Выберите действие:";
            SetCursorPosition(setX, setY + 2);
            cout << "1 - Повторить с теми же параметрами";
            SetCursorPosition(setX, setY + 3);
            cout << "2 - Запустить с новыми параметрами";
            SetCursorPosition(setX, setY + 4);
            cout << "3 - Выход";
            SetCursorPosition(setX, setY + 5);
            cout << "Выберите(1,2,3): ";
        }
        else {
            SetCursorPosition(0, 0);
            if (victory) cout << "ПОЗДРАВЛЯЮ! Вы набрали " << score << " из " << targetScore << '\n';
            else cout << "Игра окончена! Счёт: " << score << '\n';
            SetCursorPosition(0, 1);
            cout << "Выберите действие:";
            SetCursorPosition(0, 2);
            cout << "1 - Повторить с теми же параметрами";
            SetCursorPosition(0, 3);
            cout << "2 - Запустить с новыми параметрами";
            SetCursorPosition(0, 4);
            cout << "3 - Выход";
            SetCursorPosition(0, 5);
            cout << "Выберите(1,2,3): ";
        }

        string s;
        getline(cin, s);
        cout << "\x1b[2J";
        try {
            int v = stoi(s);
            if (v < 1 || v > 3) {
                if (isFullscreenLike) SetCursorPosition(setX, setY + 6);
                else SetCursorPosition(0, 6);
                cout << "Ошибка: число должно быть в диапазоне от " << 1 << " до " << 3 << ".\n";
                continue;
            }
            return v;
        }
        catch (...) {
            if (isFullscreenLike) SetCursorPosition(setX, setY + 6);
            cout << "Ошибка: введите целое число.\n";
        }
    }
}

int main() {
    setlocale(LC_ALL, "");
    srand((unsigned int)time(0));
    while (true) {
        if (!repeat) {
            if (!isFullscreenLike) TryResizeConsole(120, 30);

            userW = true, userH = true;
            GetConsoleSize(consoleCols, consoleRows); // начальная консоль

            cout << "Текущее окно консоли (cols x rows): " << consoleCols << " x " << consoleRows << '\n';
            cout << "Если хотите использовать размер консоли — просто нажмите Enter при вводе ширины/высоты.\n";

            tickMs = ReadInt(
                "Введите скорость (Диапазон: 20-1000, по умолчанию: Enter=150): ",
                20, 1000, 150
            );

            field_Width = ReadInt(
                "Введите ширину поля (Диапазон: 15-235, по умолчанию: Enter=(ширина консоли)): ",
                15, 235, 0
            );

            field_Height = ReadInt(
                "Введите высоту поля (Диапазон: 10-62, по умолчанию: Enter=(высота консоли)): ",
                10, 62, 1
            );

            GetConsoleSize(consoleCols, consoleRows);

            const int MIN_W = 15, MIN_H = 10;
            const int MAX_W = 235, MAX_H = 62;

            // Проверка ширины и высоты
            if (!userW && !userH) {
                field_Width = max(MIN_W, min(MAX_W, consoleCols));
                field_Height = max(MIN_W, min(MAX_W, consoleRows - 1));
            }
            // если введена только ширина
            else if (userW && !userH) {
                field_Height = max(MIN_H, min(MAX_H, consoleRows - 1));
            }
            // если введена только высота
            else if (!userW && userH) {
                field_Width = max(MIN_H, min(MAX_W, consoleCols));
            }

            // Цель
            int maxPossibleScore = max(1, (field_Width - 2) * (field_Height - 2) - 1);
            targetScore = ReadInt(
                "Введите целевое количество очков для победы (1-" + to_string(maxPossibleScore) + ", по умолчанию: Enter=10): ",
                1, maxPossibleScore, 10
            );
        }

        GetConsoleSize(consoleCols, consoleRows);
        isFullscreenLike = (consoleCols == 237 || consoleCols == 240) && consoleRows == 67;

        // Адаптация консоли под игровое поле (свернутая консоль)
        if (!isFullscreenLike) {
            if (field_Height > 62) field_Height = 61;
            TryResizeConsole(field_Width, field_Height + 2);
            GetConsoleSize(consoleCols, consoleRows);
        }
        else if (field_Height == 66) field_Height--;

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
        ResetKeys();

        stopInputThread = false;
        thread inputThread(KeyboardListen);

        auto lastTick = chrono::steady_clock::now();

        int tickMsX = tickMs;
        int tickMsY = int(tickMs * RATIO);

        // Главный цикл
        while (!gameOver && !victory) {

            auto now = chrono::steady_clock::now();
            auto delta = chrono::duration_cast<chrono::milliseconds>(now - lastTick).count();

            int currentTick = (dir == LEFT || dir == RIGHT) ? tickMsX : tickMsY;

            if (delta >= currentTick) {
                lastTick = now;

                if (keyUp && dir != DOWN)      nextDir = UP;
                else if (keyDown && dir != UP) nextDir = DOWN;
                else if (keyLeft && dir != RIGHT) nextDir = LEFT;
                else if (keyRight && dir != LEFT) nextDir = RIGHT;

                if (keyEsc) { gameOver = true; break; }

                ResetKeys();

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
                if (snake[0].x <= 0 || snake[0].x >= field_Width - 1 || snake[0].y <= 0 || snake[0].y >= field_Height) {
                    gameOver = true; break;
                }
                // Столкновение с хвостом
                for (int i = 1; i < snake_Len; ++i) if (snake[i].x == snake[0].x && snake[i].y == snake[0].y) {
                    gameOver = true; break;
                }
                if (gameOver) break;

                // Поедание еды
                if (snake[0].x == food.x && snake[0].y == food.y) {
                    ++score;
                    ++snake_Len;
                    snake.push_back(snake.back()); // новый сегмент
                    if (score == targetScore) { victory = true; break; }
                    do { food = RandomPoint(); } while (IsPointOnSnake(food, snake, snake_Len));
                    DrawCell(food, CHAR_FOOD, offsetX, offsetY);
                }
            }

            // Статусная строка (нижняя строка отрисовки)
            SetCursorPosition(offsetX, offsetY + field_Height + 1);
            printf("Счёт: %d / %d    ", score, targetScore);

            this_thread::sleep_for(chrono::milliseconds(1));
        }

        SetCursorPosition(offsetX, offsetY + field_Height + 1);
        printf("Счёт: %d / %d    ", score, targetScore);

        stopInputThread = true;
        inputThread.join();

        // Завершение / вывод результатов
        this_thread::sleep_for(chrono::milliseconds(1000));
        if (!isFullscreenLike) TryResizeConsole(50, 20);
        ShowCursor();
        cout << "\x1b[2J";

        int quest = MenuReplay(consoleCols, consoleRows, repeat, isFullscreenLike, victory, score);
        if (quest == 1) repeat = true;
        if (quest == 3) break;
        printf("\x1b[2J\x1b[H");
    }

    return 0;
}
