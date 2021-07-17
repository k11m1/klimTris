#pragma once
#include "stdbool.h"

struct Color
{
    float r;
    float g;
    float b;
};
struct Pos
{
    int x;
    int y;
};

struct Shape
{
    struct Pos center[4];
    struct Color color;
    // 4 states of rotation | y | x
    bool bitmap[4][4][4];
};

extern struct Shape T;
extern struct Shape I;
extern struct Shape S;
extern struct Shape Z;
extern struct Shape L;
extern struct Shape J;
extern struct Shape O;

extern struct Shape *Shapes[7];
