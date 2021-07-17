#pragma once
struct Pos {
  int x;
  int y;
};

struct Shape
{
    struct Pos center[4];
    // 4 states of rotation | y | x
    bool bitmap[4][4][4];
};

extern struct Shape T;
