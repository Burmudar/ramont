#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>

int main(void) {
    Display* display = XOpenDisplay(NULL);
    Screen* screen = DefaultScreenOfDisplay(display);
    int width = screen->width;
    int height = screen->height;

    fprintf(stderr, "W: %d H: %d\n", width, height);
}
