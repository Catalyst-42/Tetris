#include <unordered_map>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

using namespace std;
mutex mlock;

int score = 0;
int lines = 0;
int ticks = 0;

const int X = 30 ; // field length
const int Y = 20 + 1; // field heigth

int figure_x = X/2 - 2;
int figure_y = 0;

int figure_max_x = 22;
int figure_min_x = -2;

int figure = 15;
int figure_next = 1;

int figure_color = 7;
int figure_next_color = 2;

const char figure_glyph_a = '('; // any ascii symbol
const char figure_glyph_b = ')'; // can be ' ' for falfline mode

string clear_line = "";
string full_line = "";
string lower_border = " ";

//                  White      Yellow      Cyan        Green       Red         Orange            Blue        Magenta
string colors[8] = {"\033[0m", "\033[33m", "\033[36m", "\033[32m", "\033[31m", "\033[38;5;208m", "\033[34m", "\033[35m"};
// string colors[8] = {"\033[0;49m", "\033[33;43m", "\033[36;46m", "\033[32;42m", "\033[31;41m", "\033[48;5;208;38;5;208m", "\033[34;44m", "\033[35;45m"}; // solid theme

int field_colors[Y*X];
int field_colors_clear[Y*X];

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
    string screen = "";
    string fig = getFigure(figure_next);

    for (int y = 1; y < Y; y++) {
        // field
        screen += "\033[0;49m|";
        for (int x = 0; x < X; x++) {
            screen += colors[field_colors[y*X+x]] + field[y][x];
            
            if (x == X-1 && figure_glyph_b == ' ') break;

            if (field[y][x] == ' ') screen += ' ';
            else screen += figure_glyph_b;
        }
        screen += "\033[0;49m|";

        // next figure
        if (y == 1) screen += " Следующая фигура: ";
        if (y > 1 && y < 5) {
            screen += "   ";
            for (int x = 0; x < 4; x++) {
                if (fig[(y-2)*4 + x] == '.') screen += "\033[0;49m  ";
                else { screen += colors[figure_next_color] + figure_glyph_a + figure_glyph_b; }
            }
        }
        
        if (y == 19) screen += " Линий: " + to_string(lines);
        if (y == 20) screen += " Счёт: " + to_string(score);
        
        screen += "\r\n";
    }

    screen += lower_border;
    cout << screen;
}

void setFigure() {
    for (int y=0; y<Y; y++) {
        field[y] = field_clear[y];
    }

    for (int xy=0; xy<X*Y; xy++) {
        field_colors[xy] = field_colors_clear[xy];
    }

    for (int y=0; y<4; y++) {
        for (int x=0; x<4; x++) {
            if (getFigure(figure)[x+y*4] == '@') {
                field[figure_y+y][figure_x+x] = figure_glyph_a;
                field_colors[figure_y*X + y*X + figure_x + x] = figure_color;
            }
        }
    }
}

void setMaxXY() {
    // max
    if (figure == 0 || figure == 2 || figure == 10 || figure == 14 || figure == 18) figure_max_x = X-3;
    else figure_max_x = X-4;

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
    
    //                   O  I     S     Z     L            J               T
    int rotations[19] = {0, 2, 1, 4, 3, 6, 5, 10, 7, 8, 9, 14, 11, 12, 13, 18, 15, 16, 17};
    figure = rotations[figure];

    // if rotate is not possible
    setMaxXY();
    for (int y=0; y<4; y++) {
        for (int x=0; x<4; x++) {
            if (getFigure(figure)[x+y*4] == '@' && (figure_y+y>Y-1 || field_clear[figure_y+y][figure_x+x] == figure_glyph_a)) {
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

    for (int y=0; y<4; y++) {
        for (int x=0; x<4; x++) {
            if (getFigure(figure)[x+y*4] == '@' && (field_clear[figure_y+y][figure_x+x+direction] == figure_glyph_a)) {
                return;
            }
        }
    }

    figure_x += direction;
}

void checkCollision() {
    int figures_id[7] = {0, 1, 3, 5, 7, 11, 15};
    bool clip = false;

    for (int y=0; y<4; y++) {
        for (int x=0; x<4; x++) {
            if (getFigure(figure)[x+y*4] == '@' && (figure_y+y == Y-1 || field_clear[figure_y+y+1][figure_x+x] == figure_glyph_a)) {
                clip = true;
                break;
            }
        }
    }

    if (!clip) return;

    for (int y=0; y<4; y++) {
        for (int x=0; x<4; x++) {
            if (getFigure(figure)[x+y*4] == '@') {
                if (field_clear[figure_y+y][figure_x+x] == figure_glyph_a) {
                    system("clear");
                    coutField();

                    cout << "  Игра окончена\r\n";
                    system ("/bin/stty cooked");
                    exit(0);
                }

                field_clear[figure_y+y][figure_x+x] = figure_glyph_a;
                field[figure_y+y][figure_x+x] = figure_glyph_a;

                field_colors_clear[figure_y*X + y*X + figure_x + x] = figure_color;
            }
        }
    }
    
    figure_x = X/2 - 2;

    figure_y = 0;

    figure = figure_next;
    figure_color = figure_next_color;

    figure_next = rand() % 7;
    figure_next_color = figure_next;

    figure_next = figures_id[figure_next];
    figure_next_color = figure_next_color + 1;
}

void checkLines() {
    for (int y=Y; y>0; y--) {
        if (field_clear[y] == full_line) {
            score += 10;
            lines++;
            field_clear[y] = clear_line;

            for (int yy=y-1; yy > 0; yy--) {
                for (int x = 0; x < X; x++) {
                    field_colors_clear[(yy+1)*X + x] = field_colors_clear[yy*X + x];
                }
                field_clear[yy+1] = field_clear[yy];
            }
            break;
        }
    }
}

void updateBoard() {
    system("clear");
    setMaxXY();
    checkLines();
    setFigure();
    coutField();

    // debug
    // cout << " x,y: " << figure_x << ", " << figure_y << "\r\n";
    // cout << "figure: " << figure << "\r\n";
    // cout << "ticks: " << ticks << " colors: " << sizeof(colors) / sizeof(colors[0]);
    cout << "\r\n";
}

bool block = false;
void yIncrease() {
    while (true) {
        usleep(750000 - 250*score); // 1 000 000 - 1 sec
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
        input = getchar();
        mlock.lock();
        
        if (input == 27) { // esc
            system("/bin/stty cooked");
            exit(0);
        }

        if (input == 115) {        // s
            checkCollision();
            figure_y++;
            ticks++;
            if (ticks%10==0) score++;
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

    // generate lines
    for (int x=0; x<X; x++) {
        clear_line += ' ';
        full_line += figure_glyph_a;

        if (x == X-1 && figure_glyph_b == ' ') lower_border += '-';
        else lower_border += "--";
    }
    
    // generate fields
    for (int y=0; y<Y; y++) { 
        field[y] = clear_line; 
        field_clear[y] = clear_line; 
    }

    // generate colors
    for (int xy=0; xy<X*Y; xy++) {
        field_colors[xy] = 0;
        field_colors_clear[xy] = 0;
    }

    srand(time(NULL));
    system("/bin/stty raw");

    thread thr1(yIncrease);
    thread thr2(gameLoop);
    
    thr1.join();
    thr2.join();
}
