
#ifndef INPUT_H
#define INPUT_H

// the key bits
#define INPUT_KEY_BIT_LEFT   (1 <<  0)
#define INPUT_KEY_BIT_RIGHT  (1 <<  1)
#define INPUT_KEY_BIT_UP     (1 <<  2)
#define INPUT_KEY_BIT_DOWN   (1 <<  3)
#define INPUT_KEY_BIT_A      (1 <<  4)
#define INPUT_KEY_BIT_B      (1 <<  5)
#define INPUT_KEY_BIT_X      (1 <<  6)
#define INPUT_KEY_BIT_Y      (1 <<  7)
#define INPUT_KEY_BIT_L      (1 <<  8)
#define INPUT_KEY_BIT_R      (1 <<  9)
#define INPUT_KEY_BIT_SELECT (1 << 10)
#define INPUT_KEY_BIT_START  (1 << 11)
#define INPUT_KEY_BIT_QUIT   (1 << 12)

// the current key bits
extern int inputKeyBits;

// the mouse press status (YES / NO)
extern int inputMousePressed;

// the mouse coordinates (on a 1080p virtual screen)
extern int inputMouseX;
extern int inputMouseY;

void inputInit(void);
void inputRead(void);

#endif
