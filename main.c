// -- Written in C -- //
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "shapes.h"
/* #include "shapes.c" */

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
void DrawAQuad()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0f, 600.0f, 0.0f, 800.0f, 0.1f, 100.0f);
    //glOrtho(-1., 1., -1., 1., 1., 20.);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);
}

bool isPressed(Display *dpy, KeySym ks)
{
    char keys_return[32];
    XQueryKeymap(dpy, keys_return);
    KeyCode kc2 = XKeysymToKeycode(dpy, ks);
    return !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
}

void ClearScreen()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void DrawRect(struct Color color, float x, float y, float width, float height)
{
    glBegin(GL_QUADS);
    glColor3f(color.r, color.g, color.b);
    glVertex3f(x, y, 0.);
    glVertex3f(width + x, y, 0.);
    glVertex3f(width + x, height + y, 0.);
    glVertex3f(x, height + y, 0.);
    glEnd();
};
#define BLOCK_SIZE 32
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
                int new_x = current.x + x; //+ current.shape->center[current.numState].x;
                int new_y = current.y + y; //+ current.shape->center[current.numState].y;

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
                printf("current y = %d; x = %d\n", current.y + y, current.x + x);
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
    int i, n;
    time_t t;

    n = 5;

    /* Intializes random number generator */
    srand((unsigned) time(&t));

    for (size_t x = 0; x < 10; ++x) {
        for (size_t y = 0; y < 20; ++y) {
            GameBoard[y][x].isBrick = false;
            GameBoard[y][x].color = black;
        }
    }
}

void NewPiece(struct CurrentShape *current)
{
    current->shape = Shapes[rand() % 7];
    current->x = 4;
    current->y = 16;
    current->numState = 0;
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

    win = XCreateWindow(dpy, root, 0, 0, 600, 800, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

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

    struct timeval start, stop, last;
    double secs = 0;

    gettimeofday(&start, NULL);
    last = start;

    struct CurrentShape player;
    NewPiece(&player);

    while (1) {
        clock_t last_time;
        last_time = clock();
        if (XPending(dpy) > 0) {
            XNextEvent(dpy, &xev);

            if (xev.type == Expose) {
                XGetWindowAttributes(dpy, win, &gwa);
                glViewport(0, 0, gwa.width, gwa.height);
                DrawAQuad();
                glXSwapBuffers(dpy, win);
            }

            else if (xev.type == KeyPress) {
                printf("PRESS EVENT REGISTERED !!!!!! %d %d %ld \n", xev.xkey.keycode, xev.xany.send_event, XLookupKeysym(&xev.xkey, 0));
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
                if (xev.xkey.keycode == 33) {
                    player.numState = (player.numState + 1) % 4;
                    printf("current center pos: %d %d\n", player.shape->center[player.numState].x, player.shape->center[player.numState].y);
                    printf("current total pos: %d %d\n", player.shape->center[player.numState].x + player.x, player.shape->center[player.numState].y);
                }
                if (xev.xkey.keycode == 60) {
                    player.y += 1;
                }
                if (xev.xkey.keycode == 26) {
                    moveShape(&player);
                    /* player.y -= 1; */
                }
                if (xev.xkey.keycode == 24) {
                    glXMakeCurrent(dpy, None, NULL);
                    glXDestroyContext(dpy, glc);
                    XDestroyWindow(dpy, win);
                    XCloseDisplay(dpy);
                    exit(0);
                }
            } else if (xev.type == KeyRelease) {
                printf("released %d %d %ld \n", xev.xkey.keycode, xev.xany.send_event, XLookupKeysym(&xev.xkey, 0));
            }
            continue;
        } // end of the if

        /* if (isPressed(dpy, XK_period)) { */
        /*     player.y += speed; */
        /* } */
        /* if (isPressed(dpy, XK_E)) { */
        /*     player.y -= speed; */
        /* } */

        ClearScreen();
        RenderCurrentShape(player);

        RenderGameBoard();
        //DrawRect(mycolor, 50, 50);
        glXSwapBuffers(dpy, win);
        gettimeofday(&stop, NULL);

        double frames_avg = (double) (stop.tv_usec - start.tv_usec) / 1000000 + (double) (stop.tv_sec - start.tv_sec);
        secs = (double) (stop.tv_usec - last.tv_usec) / 1000000 + (double) (stop.tv_sec - last.tv_sec);
        /* printf("frame[%5d] took frame %f sec, frames avg %.2f\n", frame_counter, secs, frame_counter / frames_avg); */
        if (secs > 1) {
            last = stop;
            moveShape(&player);
            // TODO check collisions
        }
        /*     
        if (frame_counter % 60 == 0) {
            double frames_avg = (double) (stop.tv_usec - start.tv_usec) / 1000000 + (double) (stop.tv_sec - start.tv_sec);
            secs = (double) (stop.tv_usec - last.tv_usec) / 1000000 + (double) (stop.tv_sec - last.tv_sec);
            last = stop;
            printf("frame[%5d] took frame %f sec, frames avg %.2f\n", frame_counter, secs, frame_counter / frames_avg);
            //printf("frame %d\n", frame_counter);
            //XFlush(dpy);
        }*/
        ++frame_counter;
    } /* this closes while(1) { */
} /* this is the } which closes int main(int argc, char *argv[]) { */
