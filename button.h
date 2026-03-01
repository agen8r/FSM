#ifndef BUTTON_H
#define BUTTON_H
#include <stdint.h>

#define BUTTON1 (1<<9)  // PB9
#define BUTTON2 (1<<10) // PB10

typedef void (*ButtonCallback_t)(uint32_t);

void Button_Init(uint32_t buttons);
void Button_SetCallback(ButtonCallback_t callback);

#endif