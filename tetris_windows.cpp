#include <unordered_map>
#include <windows.h> // instead of unitstd.h
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

#include <conio.h> // for Sleep
#include <algorithm> // for replace

using namespace std;
mutex mlock;

int ScreenX = 120; // windows console dimensions
int ScreenY = 40;

int score = 0;
int lines = 0;
int ticks = 0;

const int X = 10; // field length
const int Y = 20 + 1; // field heigth

int figure_x = X/2 - 2;
int figure_y = 0;

int figure_max_x = 22;
int figure_min_x = -2;

int figure = 15;
int figure_next = 1;

int figure_color = 4;
int figure_next_color = 2;

const char figure_glyph_a = '['; // any ascii symbol
const char figure_glyph_b = ']'; // can be ' ' for falfline mode

int fst = figure_glyph_b == ' ' ? 0 : 1; // offset for falfline mode

string clear_line = "";
string full_line = "";
wstring lower_border = L" ";

wchar_t* screen_buffer = new wchar_t[ScreenX * ScreenY];
HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
DWORD dwBytesWritten = 0;

WORD colors[7] = { // standard theme
    0x0000,
    FOREGROUND_RED,
    FOREGROUND_GREEN,
    FOREGROUND_RED | FOREGROUND_GREEN,
    FOREGROUND_BLUE,
    FOREGROUND_RED | FOREGROUND_BLUE,
    FOREGROUND_GREEN | FOREGROUND_BLUE
}; //*/  

/*WORD colors[7] = { // solid theme
    0x0000,
    BACKGROUND_RED, 
    BACKGROUND_GREEN,
    BACKGROUND_RED | BACKGROUND_GREEN, 
    BACKGROUND_BLUE, 
    BACKGROUND_RED | BACKGROUND_BLUE, 
    BACKGROUND_GREEN | BACKGROUND_BLUE,
}; //*/

int field_colors[Y * X];
int field_colors_clear[Y * X];

string field[Y];
string field_clear[Y];

string getFigure(int figure) {
    unordered_map <int, string> figures;
    figures[0] = ".....@@..@@....."; // O

    figures[1] = "....@@@@........"; // I
    figures[2] = "..@...@...@...@.";

    figures[3] = "......@@.@@....."; // S
    figures[4] = "..@...@@...@....";

    figures[5] = ".....@@...@@...."; // Z
    figures[6] = "...@..@@..@.....";

    figures[7] = ".....@@@.@......"; // L
    figures[8] = "..@...@...@@....";
    figures[9] = "...@.@@@........";
    figures[10] = ".@@...@...@.....";

    figures[11] = ".....@@@...@...."; // J
    figures[12] = "..@@..@...@.....";
    figures[13] = ".@...@@@........";
    figures[14] = "..@...@..@@.....";

    figures[15] = ".....@@@..@....."; // T
    figures[16] = "..@...@@..@.....";
    figures[17] = "..@..@@@........";
    figures[18] = "..@..@@...@.....";

    return figures[figure];
}

void coutField() {
    wstring screen = L"";
    string fig = getFigure(figure_next);

    for (int y = 1; y < Y; y++) {
        // field
        screen += L"|";
        for (int x = 0; x < X; x++) {
            screen += field[y][x];
            WriteConsoleOutputAttribute(hConsole, &(colors[field_colors[y * X + x]]), 1, { (short) (x*2 + 1), (short) (y-1) }, &dwBytesWritten);
            
            if (x == X-1 && figure_glyph_b == ' ') break;
 
            WriteConsoleOutputAttribute(hConsole, &(colors[field_colors[y * X + x]]), 1, { (short) (x*2 + 2), (short) (y-1) }, &dwBytesWritten);
            if (field[y][x] == ' ') screen += ' ';
            else {
                screen += figure_glyph_b;
            }
        }
        screen += L"|";

        // next figure
        if (y == 1) screen += L" Следующая фигура: ";
        if (y > 1 && y < 5) {
            screen += L"   ";
            for (int x = 0; x < 4; x++) {
                WriteConsoleOutputAttribute(hConsole, &colors[0], 1, { (short) ((x + X) * 2 + 4 + fst), (short) (y - 1) }, &dwBytesWritten);
                WriteConsoleOutputAttribute(hConsole, &colors[0], 1, { (short) ((x + X) * 2 + 5 + fst), (short) (y - 1) }, &dwBytesWritten);
                if (fig[(y - 2) * 4 + x] == '.') screen += L"  ";
                else { 
                    WriteConsoleOutputAttribute(hConsole, &colors[figure_next_color], 1, { (short) ((x + X) * 2 + 4 + fst), (short) (y - 1) }, &dwBytesWritten);
                    WriteConsoleOutputAttribute(hConsole, &colors[figure_next_color], 1, { (short) ((x + X) * 2 + 5 + fst), (short) (y - 1) }, &dwBytesWritten);

                    screen += figure_glyph_a; 
                    screen += figure_glyph_b; 
                }
            }
        }

        if (y == 19) screen += L" Линий: " + to_wstring(lines);
        if (y == 20) screen += L" Счёт: " + to_wstring(score);

        screen += L"\n";
    }

    screen += lower_border;
    screen += L"\n ";

    // display map
    int y = 0;
    int x = 0;
    for (int i = 0; i < screen.length() - 1; i++) {
        if (screen[i] == '\n') { y++; x = 0; }
        else { screen_buffer[ScreenX*y + x] = screen[i]; x++; }
    }

    screen_buffer[ScreenX * ScreenY - 1] = '\0';
    WriteConsoleOutputCharacterW(hConsole, screen_buffer, ScreenX * ScreenY, { 0,0 }, &dwBytesWritten);
}

void setFigure() {
    for (int y = 0; y < Y; y++) {
        field[y] = field_clear[y];
    }

    for (int xy = 0; xy < X * Y; xy++) {
        field_colors[xy] = field_colors_clear[xy];
    }

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (getFigure(figure)[x + y * 4] == '@') {
                field[figure_y + y][figure_x + x] = figure_glyph_a;
                field_colors[figure_y * X + y * X + figure_x + x] = figure_color;
            }
        }
    }
}

void setMaxXY() {
    // max
    if (figure == 0 || figure == 2 || figure == 10 || figure == 14 || figure == 18) figure_max_x = X - 3;
    else figure_max_x = X - 4;

    // min
    if (figure == 2 || figure == 4 || figure == 6 || figure == 8 || figure == 12 || figure == 16) figure_min_x = -2;
    else if (figure == 1) figure_min_x = 0;
    else figure_min_x = -1;

    // x normalization, especially when rotating
    if (figure_x < figure_min_x) figure_x = figure_min_x;
    if (figure_x > figure_max_x) figure_x = figure_max_x;
}

void rotate() {
    int figure_heap = figure;
    int figure_x_heap = figure_x;
    int figure_y_heap = figure_y;

    //                    O  I     S     Z     L            J               T
    int rotations[19] = { 0, 2, 1, 4, 3, 6, 5, 10, 7, 8, 9, 14, 11, 12, 13, 18, 15, 16, 17 };
    figure = rotations[figure];

    // if rotate is not possible
    setMaxXY();
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (getFigure(figure)[x + y * 4] == '@' && (figure_y + y > Y - 1 || field_clear[figure_y + y][figure_x + x] == figure_glyph_a)) {
                figure = figure_heap;
                figure_x = figure_x_heap;
                figure_y = figure_y_heap;
                setMaxXY();
                return;
            }
        }
    }
}

void move(int direction) {
    if (figure_x + direction < figure_min_x || figure_x + direction > figure_max_x) return;

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (getFigure(figure)[x + y * 4] == '@' && (field_clear[figure_y + y][figure_x + x + direction] == figure_glyph_a)) {
                return;
            }
        }
    }

    figure_x += direction;
}

void checkCollision() {
    int figures_id[7] = { 0, 1, 3, 5, 7, 11, 15 };
    bool clip = false;

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (getFigure(figure)[x + y * 4] == '@' && (figure_y + y == Y - 1 || field_clear[figure_y + y + 1][figure_x + x] == figure_glyph_a)) {
                clip = true;
                break;
            }
        }
    }

    if (!clip) return;

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (getFigure(figure)[x + y * 4] == '@') {
                if (field_clear[figure_y + y][figure_x + x] == figure_glyph_a) {
                    for (int i = 0; i < 14; i++) screen_buffer[(Y - 1) * ScreenX + X * 2 + 2 + fst + i] = L"Игра окончена"[i];

                    coutField();
                    exit(0);
                }

                field_clear[figure_y + y][figure_x + x] = figure_glyph_a;
                field[figure_y + y][figure_x + x] = figure_glyph_a;

                field_colors_clear[figure_y * X + y * X + figure_x + x] = figure_color;
            }
        }
    }

    figure_x = X / 2 - 2;
    figure_y = 0;

    figure = figure_next;

    figure_next = rand() % 7;
    figure_next = figures_id[figure_next];

    figure_color = figure_next_color;
    figure_next_color = 1 + rand() % (sizeof(colors) / sizeof(colors[0]) - 1);
}

void checkLines() {
    for (int y = Y; y > 0; y--) {
        if (field_clear[y] == full_line) {
            score += 10;
            lines++;
            field_clear[y] = clear_line;

            for (int yy = y - 1; yy > 0; yy--) {
                for (int x = 0; x < X; x++) {
                    field_colors_clear[(yy + 1) * X + x] = field_colors_clear[yy * X + x];
                }
                field_clear[yy + 1] = field_clear[yy];
            }
            break;
        }
    }
}

void updateBoard() {
    setMaxXY();
    checkLines();
    setFigure();
    coutField();
}

bool block = false;
void yIncrease() {
    while (true) {
        Sleep((750000 - 250 * score) / 1000); // 1 000 000 - 1 sec
        mlock.lock();

        checkCollision();
        figure_y++;
        ticks++;
        if (ticks % 10 == 0) score += 1;

        updateBoard();
        mlock.unlock();
    }
}

void gameLoop() {
    int input = -1;
    while (true) {
        // check input
        input = -1;
        input = _getch();
        mlock.lock();

        if (input == 27) exit(0);  // esc

        if (input == 115) {        // s
            checkCollision();
            figure_y++;
            ticks++;
            if (ticks % 10 == 0) score++;
        }

        if (input == 97) move(-1); // a
        if (input == 100) move(1); // d
        if (input == 32) rotate(); // space

        updateBoard();
        mlock.unlock();
    }
}

int main() {
    setlocale(LC_ALL, "Russian");
    
    // set clear buffer
    SetConsoleActiveScreenBuffer(hConsole);
    for (int i = 0; i < ScreenX * ScreenY; i++) screen_buffer[i] = ' ';

    // generate lines
    for (int x = 0; x < X; x++) {
        clear_line += " ";
        full_line += figure_glyph_a;

        if (x == X - 1 && figure_glyph_b == ' ') lower_border += L"-";
        else lower_border += L"--";
    }

    // generate fields
    for (int y = 0; y < Y; y++) {
        field[y] = clear_line;
        field_clear[y] = clear_line;
    }

    // generate colors
    for (int xy = 0; xy < X * Y; xy++) {
        field_colors[xy] = 0;
        field_colors_clear[xy] = 0;
    }

    srand(time(NULL));

    thread thr1(yIncrease);
    thread thr2(gameLoop);

    thr1.join();
    thr2.join();
}
