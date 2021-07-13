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

struct Color
{
    float r;
    float g;
    float b;
};
struct Color black = { 0, 0, 0 };
struct Color green = { 0, 1, 0 };
struct Block
{
    bool isBrick;
    struct Color *color;
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
    //glVertex3f(-.75 + x, -.75 + y, 0.);
    //glVertex3f(.75 + x, -.75 + y, 0.);
    //glVertex3f(.75 + x, .75 + y, 0.);
    //glVertex3f(-.75 + x, .75 + y, 0.);
    glVertex3f(x, y, 0.);
    glVertex3f(width + x, y, 0.);
    glVertex3f(width + x, height + y, 0.);
    glVertex3f(x, height + y, 0.);
    glEnd();
};
#define BLOCK_SIZE 32
void RenderBlock(struct Block block, float x, float y)
{
    DrawRect(*block.color, x, y, BLOCK_SIZE, BLOCK_SIZE);
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
    float x;
    float y;
    struct Color color;
};
struct Color red = { 1, 0, 0 };

void RenderCurrentShape(struct CurrentShape current)
{
    DrawRect(red, current.x * BLOCK_SIZE, current.y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
}

void InitGameBoard()
{
    bool now = false;
    for (size_t x = 0; x < 10; ++x) {
        for (size_t y = 0; y < 20; ++y) {
            GameBoard[y][x].isBrick = false;
            GameBoard[y][x].color = now ? &black : &green;
            now = !now;
        }
    }
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
    player.x = 3;
    player.y = 5;
    player.color = red;

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
            printf("####### inside\n");
            continue;
        } // end of the if

        if (isPressed(dpy, XK_period)) {
            player.y += speed;
        }
        if (isPressed(dpy, XK_E)) {
            player.y -= speed;
        }
        if (isPressed(dpy, XK_O)) {
            player.x -= speed;
        }
        if (isPressed(dpy, XK_U)) {
            player.x += speed;
        }

        ClearScreen();
        RenderCurrentShape(player);

        RenderGameBoard();
        //DrawRect(mycolor, 50, 50);
        glXSwapBuffers(dpy, win);
        gettimeofday(&stop, NULL);
        if (frame_counter % 60 == 0) {
            double frames_avg = (double) (stop.tv_usec - start.tv_usec) / 1000000 + (double) (stop.tv_sec - start.tv_sec);
            secs = (double) (stop.tv_usec - last.tv_usec) / 1000000 + (double) (stop.tv_sec - last.tv_sec);
            last = stop;
            printf("frame[%5d] took frame %f sec, frames avg %.2f\n", frame_counter, secs, frame_counter / frames_avg);
            //printf("frame %d\n", frame_counter);
            //XFlush(dpy);
        }
        ++frame_counter;
    } /* this closes while(1) { */
} /* this is the } which closes int main(int argc, char *argv[]) { */
