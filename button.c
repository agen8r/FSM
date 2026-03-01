#include "em_gpio.h"
#include "button.h"
#include "em_cmu.h"

static ButtonCallback_t globalCallback = 0;

void Button_Init(uint32_t buttons) {
    CMU_ClockEnable(cmuClock_GPIO, true);
    
    // Botões PB9 e PB10
    if(buttons & BUTTON1) {
        GPIO_PinModeSet(gpioPortB, 9, gpioModeInput, 1);
        GPIO_ExtIntConfig(gpioPortB, 9, 9, false, true, true);
    }
    if(buttons & BUTTON2) {
        GPIO_PinModeSet(gpioPortB, 10, gpioModeInput, 1);
        GPIO_ExtIntConfig(gpioPortB, 10, 10, false, true, true);
    }
    
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

void Button_SetCallback(ButtonCallback_t callback) {
    globalCallback = callback;
}

void GPIO_ODD_IRQHandler(void) {
    uint32_t flag = GPIO_IntGet();
    GPIO_IntClear(flag);
    if (globalCallback) {
        if (flag & (1 << 9)) globalCallback(9);
    }
}

void GPIO_EVEN_IRQHandler(void) {
    uint32_t flag = GPIO_IntGet();
    GPIO_IntClear(flag);
    if (globalCallback) {
        if (flag & (1 << 10)) globalCallback(10);
    }
}