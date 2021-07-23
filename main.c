// -- Written in C -- //
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "brick.h"
#include "shapes.h"

#define BLOCK_SIZE 32

Display *dpy;
Window root;
GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo *vi;
Colormap cmap;
XSetWindowAttributes swa;
Window win;
GLXContext glc;
XWindowAttributes gwa;
XEvent xev;
bool GameOver = false;

struct Color black = { 0, 0, 0 };
struct Color green = { 0, 1, 0 };
struct Block
{
    bool isBrick;
    struct Color color;
};

struct Block GameBoard[20][10];

float x = 0;
float y = 0;
float speed = 1;
void ClearScreen()
{
    glClearColor(.2, .2, .2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void SetupGl()
{
    ClearScreen();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0f, 480.0f, 0.0f, 640.0f, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);
}

void init_gl_render()
{
    uint8_t image[8 * 8 * 3] = {
        0,
    };
    int counter = 0;
    for (int x = 0; x < 8; x += 1) {
        for (int y = 0; y < 8; y += 2) {
            for (int channel = 0; channel < 3; ++channel) {
                image[3 * (8 * y + x) + channel] = 255;
                counter++;
            }
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 16, 16, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 16, 16, GL_RGB, GL_UNSIGNED_BYTE, image_data);
}

bool isPressed(Display *dpy, KeySym ks)
{
    char keys_return[32];
    XQueryKeymap(dpy, keys_return);
    KeyCode kc2 = XKeysymToKeycode(dpy, ks);
    return !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
}

void DrawRect(struct Color color, float x, float y, float width, float height)
{
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor3f(color.r, color.g, color.b);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(x, y, 0.);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(width + x, y, 0.);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(width + x, height + y, 0.);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(x, height + y, 0.);
    glEnd();
    glDisable(GL_TEXTURE_2D);
};
void RenderBlock(struct Block block, float x, float y)
{
    DrawRect(block.color, x, y, BLOCK_SIZE, BLOCK_SIZE);
}

void RenderGameBoard()
{
    for (size_t x = 0; x < 10; ++x) {
        for (size_t y = 0; y < 20; ++y) {
            RenderBlock(GameBoard[y][x], x * BLOCK_SIZE, y * BLOCK_SIZE);
        }
    }
}

struct CurrentShape
{
    int x;
    int y;
    short numState;
    struct Shape *shape;
};
void NewPiece(struct CurrentShape *current);
struct Color red = { 1, 0, 0 };

void RenderCurrentShape(struct CurrentShape current)
{
    for (size_t x = 0; x < 4; ++x) {
        for (int y = 3; y >= 0; --y) {
            if (current.shape->bitmap[current.numState][y][x]) {
                int new_x = current.x + x;
                int new_y = current.y + y;

                DrawRect(current.shape->color, new_x * BLOCK_SIZE, new_y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
            }
        }
    }
}
bool check_no_collision(struct CurrentShape current)
{
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 4; ++y) {
            if (current.shape->bitmap[current.numState][y][x]) {
                if (current.y + y < 0) {
                    return false;
                }
                if (current.x + x < 0 || current.x + x >= 10) {
                    return false;
                }

                if (GameBoard[current.y + y][current.x + x].isBrick) {
                    return false;
                }
            }
        }
    }
    return true;
}

void imprint(struct CurrentShape current)
{
    for (size_t x = 0; x < 4; ++x) {
        for (size_t y = 0; y < 4; ++y) {
            if (current.shape->bitmap[current.numState][y][x]) {
                GameBoard[current.y + y][current.x + x].isBrick = true;
                GameBoard[current.y + y][current.x + x].color = current.shape->color;
            }
        }
    }
}

void moveShape(struct CurrentShape *current)
{
    current->y--;
    if (check_no_collision(*current)) {
    } else {
        current->y++;
        imprint(*current);
        NewPiece(current);
    }
}

void InitGameBoard()
{
    /* Intializes random number generator */
    srand((unsigned) time(NULL));

    for (size_t x = 0; x < 10; ++x) {
        for (size_t y = 0; y < 20; ++y) {
            GameBoard[y][x].isBrick = false;
            GameBoard[y][x].color = black;
        }
    }
}

void moveLines(int to, int from)
{
    assert(to < from);
    int target = to;
    for (int y = from; y < 20; ++y) {
        // TODO memcpy?
        for (int x = 0; x < 10; x++) {
            GameBoard[target][x] = GameBoard[y][x];
        }
        target++;
    }
}

void clearLines(struct CurrentShape *current)
{
    int maximum_height = 20;
    int cleared = 0;
    int bottom_line = 0;
    for (int i = 0; i < maximum_height; i++) {
        int x = 0;
        for (; x < 10; x++) {
            if (!GameBoard[i][x].isBrick) {
                break;
            }
        }
        if (x == 10) {
            moveLines(i, i + 1);
            cleared++;
            i--;
            maximum_height--; // little optimization
        }
    }
    printf("CLEARED %d lines\n", cleared);
}

void NewPiece(struct CurrentShape *current)
{
    current->shape = Shapes[rand() % 7];
    current->x = 4;
    current->y = 16;
    current->numState = 0;
    if (!check_no_collision(*current)) {
        GameOver = true;
    }
    clearLines(current);
}

void do_wall_kick(struct CurrentShape *current)
{
    int direction = (current->x < 5) ? 1 : -1;

    for (int i = 0; i < 3; ++i) {
        current->x += direction;
        if (check_no_collision(*current)) {
            return;
        }
    }
    current->x -= 3 * direction;
    current->numState = (current->numState + 3) % 4;
}

int main(int argc, char *argv[])
{
    dpy = XOpenDisplay(NULL);

    if (dpy == NULL) {
        printf("\n\tcannot connect to X server\n\n");
        exit(0);
    }

    root = DefaultRootWindow(dpy);

    vi = glXChooseVisual(dpy, 0, att);

    if (vi == NULL) {
        printf("\n\tno appropriate visual found\n\n");
        exit(0);
    } else {
        printf("\n\tvisual %p selected\n", (void *) vi->visualid); /* %p creates hexadecimal output like in glxinfo */
    }

    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask;

    win = XCreateWindow(dpy, root, 0, 0, 480, 640, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(dpy, win);
    XStoreName(dpy, win, "KlimTris");

    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);

    glEnable(GL_DEPTH_TEST);

    printf("GL Renderer  %s\n", glGetString(GL_RENDERER));
    printf("GL Version   %s\n", glGetString(GL_VERSION));
    printf("GLSL Version %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    int frame_counter = 0;
    InitGameBoard();
    init_gl_render();

    struct timeval start, stop, last;
    double secs = 0;

    gettimeofday(&start, NULL);
    last = start;

    struct CurrentShape player;
    NewPiece(&player);

    while (!GameOver) {
        clock_t last_time;
        last_time = clock();
        if (XPending(dpy) > 0) {
            XNextEvent(dpy, &xev);

            if (xev.type == Expose) {
                XGetWindowAttributes(dpy, win, &gwa);
                glViewport(0, 0, gwa.width, gwa.height);
                SetupGl();
                glXSwapBuffers(dpy, win);
            }

            else if (xev.type == KeyPress) {
                if (xev.xkey.keycode == 32) {
                    player.x -= speed;
                    if (!check_no_collision(player)) {
                        player.x += speed;
                    }
                }

                if (xev.xkey.keycode == 30) {
                    player.x += speed;
                    if (!check_no_collision(player)) {
                        player.x -= speed;
                    }
                }
                if (xev.xkey.keycode == 60) {
                    player.numState = (player.numState + 1) % 4;
                    if (!check_no_collision(player)) {
                        do_wall_kick(&player);
                    }
                }
                if (xev.xkey.keycode == 26) {
                    moveShape(&player);
                    secs = 0; // reset the drop down timer
                }
                if (xev.xkey.keycode == 24) { // q - quit
                    glXMakeCurrent(dpy, None, NULL);
                    glXDestroyContext(dpy, glc);
                    XDestroyWindow(dpy, win);
                    XCloseDisplay(dpy);
                    exit(0);
                }
            }
            continue;
        }

        ClearScreen();
        RenderCurrentShape(player);

        RenderGameBoard();
        glXSwapBuffers(dpy, win);
        gettimeofday(&stop, NULL);

        double frames_avg = (double) (stop.tv_usec - start.tv_usec) / 1000000 + (double) (stop.tv_sec - start.tv_sec);
        secs = (double) (stop.tv_usec - last.tv_usec) / 1000000 + (double) (stop.tv_sec - last.tv_sec);
        /* printf("frame[%5d] took frame %f sec, frames avg %.2f\n", frame_counter, secs, frame_counter / frames_avg); */
        if (secs > 1) {
            last = stop;
            moveShape(&player);
        }
        ++frame_counter;
    }
}
