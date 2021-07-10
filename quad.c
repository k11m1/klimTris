// -- Written in C -- //
#include <stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<X11/X.h>
#include<X11/Xlib.h>
#include <X11/keysym.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>

Display                 *dpy;
Window                  root;
GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo             *vi;
Colormap                cmap;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              glc;
XWindowAttributes       gwa;
XEvent                  xev;
float x = 0;
float y = 0;
void DrawAQuad() {
 glClearColor(1.0, 1.0, 1.0, 1.0);
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 glOrtho(-1., 1., -1., 1., 1., 20.);

 glMatrixMode(GL_MODELVIEW);
 glLoadIdentity();
 gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

 glBegin(GL_QUADS);
  glColor3f(1., 0., 0.); glVertex3f(-.75+x, -.75+y, 0.);
  glColor3f(0., 1., 0.); glVertex3f( .75+x, -.75+y, 0.);
  glColor3f(0., 0., 1.); glVertex3f( .75+x,  .75+y, 0.);
  glColor3f(1., 1., 0.); glVertex3f(-.75+x,  .75+y, 0.);
 glEnd();
}

int main(int argc, char *argv[]) {

 dpy = XOpenDisplay(NULL);

 if(dpy == NULL) {
 	printf("\n\tcannot connect to X server\n\n");
        exit(0);
 }

 root = DefaultRootWindow(dpy);

 vi = glXChooseVisual(dpy, 0, att);

 if(vi == NULL) {
	printf("\n\tno appropriate visual found\n\n");
        exit(0);
 }
 else {
	printf("\n\tvisual %p selected\n", (void *)vi->visualid); /* %p creates hexadecimal output like in glxinfo */
 }


 cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

 swa.colormap = cmap;
 swa.event_mask = ExposureMask | KeyPressMask;

 win = XCreateWindow(dpy, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

 XMapWindow(dpy, win);
 XStoreName(dpy, win, "KlimTris");

 glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
 glXMakeCurrent(dpy, win, glc);

 glEnable(GL_DEPTH_TEST);

        printf("GL Renderer  %s\n", glGetString(GL_RENDERER));
        printf("GL Version   %s\n", glGetString(GL_VERSION));
        printf("GLSL Version %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
        int frame_counter = 0;
 while(1) {
     if (XPending(dpy) > 0) {
 	XNextEvent(dpy, &xev);

        if(xev.type == Expose) {
        	XGetWindowAttributes(dpy, win, &gwa);
                glViewport(0, 0, gwa.width, gwa.height);
        	DrawAQuad();
                glXSwapBuffers(dpy, win);
        }

	else if(xev.type == KeyPress) {
        printf("PRESS EVENT REGISTERED !!!!!! %d %d %ld \n", xev.xkey.keycode, xev.xany.send_event, XLookupKeysym(&xev.xkey, 0));
        if (xev.xkey.keycode == 60) {
            y += 0.1;
        }
        if (xev.xkey.keycode == 26) {
            y -= 0.1;
        }
        if (xev.xkey.keycode == 30) {
            x += 0.1;
        }
        if (xev.xkey.keycode == 32) {
            x -= 0.1;
        }
        if (xev.xkey.keycode == 24) {
        	glXMakeCurrent(dpy, None, NULL);
            glXDestroyContext(dpy, glc);
            XDestroyWindow(dpy, win);
            XCloseDisplay(dpy);
            exit(0);

            }
        }
        else if (xev.type == KeyRelease) {
        printf("released %d %d %ld \n", xev.xkey.keycode, xev.xany.send_event, XLookupKeysym(&xev.xkey, 0));


        }


     }
    KeySym ks = XK_Tab;
    char keys_return[32];
    XQueryKeymap(dpy, keys_return);
    KeyCode kc2 = XKeysymToKeycode(dpy, ks);
    bool isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
    printf("it's frame %d and the key tab is %s\n", frame_counter, isPressed ? "pressed" : "not pressed" );



        	DrawAQuad();
                glXSwapBuffers(dpy, win);
                ++frame_counter;
        //printf("frame %d\n", frame_counter);
        //XFlush(dpy);

    } /* this closes while(1) { */
} /* this is the } which closes int main(int argc, char *argv[]) { */
