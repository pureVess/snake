#include <iostream>
#include <string>
#include <windows.h>
#include <vector>

using namespace std;

bool KEY[256];

int field_Width = 40;
int field_Height = 20;

const char CHAR_EMPTY = ' ';     
const char CHAR_WALL = '#';     
const char CHAR_SNAKE = 'O';    
const char CHAR_SNAKE_HEAD = '@';
const char CHAR_FOOD = 'o';

// Структура для представления координат точки (x, y)
struct Point { int x, y; };

// Возможные направления движения змейки
enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// Перемещает курсор в консоли на координаты (x, y)
void SetCursorPosition(int x, int y) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { x, y };
    SetConsoleCursorPosition(h, pos);
}

// Скрывает курсор
void HideCursor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info{ 100, FALSE };
    SetConsoleCursorInfo(h, &info);
}

// Показывает курсор
void ShowCursor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info{ 100, TRUE };
    SetConsoleCursorInfo(h, &info);
}

// Возвращает случайную точку внутри поля
Point RandomPoint() {
    Point p;
    p.x = 1 + rand() % (field_Width - 2);
    p.y = 1 + rand() % (field_Height - 2);
    return p;
}

// Сканирует состояние и записывает их в массив KEY[]
void GetKEY()
{
    int i = 0;
    while (i < 256)
    {
        if (GetAsyncKeyState(i)) KEY[i] = 1;
        else KEY[i] = 0;
        i++;
    }
}

// Проверяет, равны ли две точки (x и y одинаковы)
bool Equals(const Point& a, const Point& b) {
    return a.x == b.x && a.y == b.y;
}

// Запрос на повторение игры
char AskReplay() {
    while (true) {
        // Очищаем буфер ввода
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        cout << "Сыграть ещё раз? (Y/N): ";
        string input;
        getline(cin, input);
        if (input.empty()) continue; // если пустая строка – повторить
        char c = toupper(input[0]);
        if (c == 'Y' || c == 'N') return c;
        cout << "Введите только Y или N.\n";
    }
}

// Отрисовывает игровое поле
void DrawField() {
    for (int y = 0; y < field_Height; ++y) {
        for (int x = 0; x < field_Width; ++x)
            cout << ((y == 0 || y == field_Height - 1 || x == 0 || x == field_Width - 1)
                ? CHAR_WALL : CHAR_EMPTY);
        cout << '\n';
    }
}

// Отрисовывает змею (голову и тело)
void DrawSnake(const vector<Point>& snake, int snake_Len) {
    for (int i = 0; i < snake_Len; i++) {
        SetCursorPosition(snake[i].x, snake[i].y);
        cout << (i == 0 ? CHAR_SNAKE_HEAD : CHAR_SNAKE);
    }
}

// Очищает хвост змеи
void ClearSnakeTail(const vector<Point>& snake, int snake_Len) {
    if (snake_Len > 0) {
        SetCursorPosition(snake[snake_Len - 1].x, snake[snake_Len - 1].y);
        cout << CHAR_EMPTY;
    }
}

// Проверка на нахождение точки внутри тела змеи
bool IsPointOnSnake(const Point& p, const vector<Point>& snake, int snake_Len) {
    for (int i = 0; i < snake_Len; i++) {
        if (Equals(p, snake[i])) {
            return true;
        }
    }
    return false;
}

int main() {
    setlocale(LC_ALL, "Russian");
    srand(GetTickCount64());     // Инициализация генератора случайных чисел

    while (true) {

        int tickMs = 150; // скорость обновления экрана (мс)
        cout << "Введите скорость игры (50-1000, по умолчанию 150): ";
        string input;
        getline(cin, input);
        if (!input.empty()) {
            try {
                int val = stoi(input);
                if (val >= 50 && val <= 1000) tickMs = val;
            }
            catch (...) {}
        }

        cout << "\nВведите ширину поля (10-120, по умолчанию 40): ";
        getline(cin, input);
        if (!input.empty()) {
            try {
                int w = stoi(input);
                if (w >= 10 && w <= 120) field_Width = w;
            }
            catch (...) {}
        }

        cout << "Введите высоту поля (10-25, по умолчанию 20): ";
        getline(cin, input);
        if (!input.empty()) {
            try {
                int h = stoi(input);
                if (h >= 10 && h <= 25) field_Height = h;
            }
            catch (...) {}
        }

        // Расчёт макс очков (всё поле)
        int maxPossibleScore = (field_Width - 2) * (field_Height - 2) - 1;

        // Цель на игру
        int targetScore = 10;
        cout << "Введите целевое количество очков для победы (1-" << maxPossibleScore << ", по умолчанию 10): ";
        getline(cin, input);
        if (!input.empty()) {
            try {
                int scoreVal = stoi(input);
                if (scoreVal >= 1 && scoreVal <= maxPossibleScore) {
                    targetScore = scoreVal;
                }
            }
            catch (...) {
            }
        }

        system("cls");
        DrawField();
        HideCursor();

        Direction dir = RIGHT;
        Direction nextDir = dir;

        bool gameOver = false;
        bool victory = false;

        vector<Point> snake;
        int snake_Len = 1; 
        int score = 0; 

        Point food;

        // Начальная позиция змеи (центр поля)
        snake.push_back({ field_Width / 2, field_Height / 2 });

        // Размещаем первую еду вне змейки
        food = RandomPoint();
        while (IsPointOnSnake(food, snake, snake_Len)) {
            food = RandomPoint();
        }
        SetCursorPosition(food.x, food.y);
        cout << CHAR_FOOD;


        while (!gameOver && !victory) {
            GetKEY(); // Обновляем состояние всех клавиш

            if (KEY['W'] || KEY[VK_UP])
            {
                if (dir != DOWN) nextDir = UP;
            }
            else if (KEY['S'] || KEY[VK_DOWN])
            {
                if (dir != UP) nextDir = DOWN;
            }
            else if (KEY['A'] || KEY[VK_LEFT])
            {
                if (dir != RIGHT) nextDir = LEFT;
            }
            else if (KEY['D'] || KEY[VK_RIGHT])
            {
                if (dir != LEFT) nextDir = RIGHT;
            }

            // ESC – выход из игры
            if (KEY[VK_ESCAPE])
            {
                gameOver = true;
            }

            dir = nextDir; // Обновляем направление

            ClearSnakeTail(snake, snake_Len); // очищаем последний сегмент

            // Сдвигаем всё тело вперёд
            if (snake_Len > 1) {
                for (int i = snake_Len - 2; i >= 0; i--) {
                    snake[i + 1] = snake[i];
                }
            }

            // Перемещаем голову змейки
            switch (dir) {
            case UP: snake[0].y--; break;
            case DOWN: snake[0].y++; break;
            case LEFT: snake[0].x--; break;
            case RIGHT: snake[0].x++; break;
            }

            DrawSnake(snake, snake_Len); // Новая позиция змеи

            // Проверка столкновений со стенами
            if (snake[0].x <= 0 || snake[0].x >= field_Width - 1 ||
                snake[0].y <= 0 || snake[0].y >= field_Height - 1) {
                gameOver = true;
            }

            // Проверка столкновений с хвостом
            for (int i = 1; i < snake_Len; i++) {
                if (Equals(snake[0], snake[i])) {
                    gameOver = true;
                    break;
                }
            }

            // Проверка столкновения с едой
            if (Equals(snake[0], food)) {
                score++;               
                snake_Len++;
                snake.push_back(snake.back()); // Добавляем новый сегмент

                // Проверка победы
                if (score >= targetScore) {
                    victory = true;
                }

                // Создаём новую еду (вне змеи)
                Point newFood = RandomPoint();
                while (IsPointOnSnake(newFood, snake, snake_Len)) {
                    newFood = RandomPoint();
                }
                food = newFood;
                SetCursorPosition(food.x, food.y);
                cout << CHAR_FOOD;
            }

            SetCursorPosition(0, field_Height);
            cout << "Счёт: " << score << " / " << targetScore << "    "; // Отображение счёта

            Sleep(tickMs); // Частота обнорвления (скорость)
        }

        ShowCursor();
        SetCursorPosition(0, field_Height + 1);

        if (victory) {
            cout << "ПОЗДРАВЛЯЮ! Вы набрали " << score << " очков из " << targetScore << "!" << endl;
        }
        else {
            cout << "Игра окончена! Ваш счёт: " << score << " из " << targetScore << endl;
        }

        if (AskReplay() == 'N') {
            break;
        }
    }

    return 0;
}
