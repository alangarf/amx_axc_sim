#ifndef TERMINAL__HEADER
#define TERMINAL__HEADER

#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

#define NB_ENABLE 1
#define NB_DISABLE 2

int kbhit();
void nonblock(int state);

#endif /* TERMINAL__HEADER */
