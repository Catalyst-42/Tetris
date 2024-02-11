# Tetris
A simple C++ implementation of tetris. For Windows use a different code file with output to console via windows api (it will work faster than `cout` output). For other devices the main file will do. 

compilation for unix systems:

```
c++ -std=c++20 -Ofast -march=native tetris.cpp -o tetris
./tetris
```

In the code, you can conveniently change the size of the field, the type of glyphs, and their style of their backgrounds

### Examples
| ![default tetris](/img/tetris_common.png) | ![default tetris](/img/tetris_colored.png) |
|-|-|

![fat tetris](/img/tetris_fat.png)
