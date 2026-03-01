#ifndef LCD_H
#define LCD_H
#include <stdint.h>

// Renomeado para evitar conflito com em_lcd.h
void Display_Init(void);

// Escreve texto na linha de cima (Alfanumérico)
void Display_WriteText(char *string);

// Escreve número na linha de baixo (Numérico)
void Display_WriteNumber(int value);

#endif