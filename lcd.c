#include "em_device.h"
#include "em_cmu.h"
#include "segmentlcd.h" // Driver oficial do SDK
#include "lcd.h"

void Display_Init(void) {
    // Habilita clock e inicia o driver do SDK
    CMU_ClockEnable(cmuClock_LCD, true);
    SegmentLCD_Init(false); 
}

void Display_WriteText(char *string) {
    // Escreve na parte alfanumérica (Cima)
    SegmentLCD_Write(string);
}

void Display_WriteNumber(int value) {
    // Escreve na parte numérica (Baixo)
    SegmentLCD_Number(value);
}