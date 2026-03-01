#include <stdio.h>
#include <stdbool.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "lcd.h"

// Configuração
#define MAX_FLOOR 2  
#define TIME_DOOR   800
#define TIME_TRAVEL 1100

#define BTN_CHAMA_2_PORT gpioPortD
#define BTN_CHAMA_2_PIN  6  // Chama pro 2
#define BTN_CHAMA_3_PORT gpioPortD
#define BTN_CHAMA_3_PIN  7  // Chama pro 3

#define BTN_SELECIONA_PORT gpioPortB
#define BTN_SELECIONA_PIN  9  // Escolhe o andar na tela
#define BTN_CONFIRMA_PORT  gpioPortB
#define BTN_CONFIRMA_PIN   10 // Confirma a viagem

#define LED0_PORT gpioPortE
#define LED0_PIN  2
#define LED1_PORT gpioPortE
#define LED1_PIN  3

typedef enum {
    STATE_IDLE,
    STATE_DOOR_CLOSING,
    STATE_MOVING_UP,
    STATE_MOVING_DOWN,
    STATE_DOOR_OPENING
} State_t;

typedef struct {
    State_t state;
    uint32_t stateStartTime;
    int currentFloor;
    int targetFloor;
    int targetFloorUI; 
} Elevator_t;

static Elevator_t elevator;

volatile uint32_t msTicks = 0;
volatile uint8_t selectFlag = 0;
volatile uint8_t confirmFlag = 0;

//fila de chegada
int call_queue[2] = {-1, -1};
int q_len = 0;

bool traveling_to_call = false;     // Elevador indo buscar 
bool waiting_for_selection = false; // Esperando pra escolher o destino
bool waiting_for_exit = false;      // Esperando  pra sair no destino

void *_sbrk(int incr) {
    (void)incr;
    return (void *)-1; 
}

//Systick, utilizado para ser o contador do programa
void SysTick_Handler(void)
{
    msTicks++;
}

// FILA
void Enqueue(int floor) {
    for(int i = 0; i < q_len; i++) {
        if(call_queue[i] == floor) return;
    }
    if(q_len < 2) {
        call_queue[q_len++] = floor;
    }
}

int Dequeue(void) {
    if(q_len == 0) return -1;
    int floor = call_queue[0];
    call_queue[0] = call_queue[1];
    q_len--;
    return floor;
}

// FSM
void Elevator_Init(void)
{
    elevator.state = STATE_IDLE;
    elevator.currentFloor = 0; 
    elevator.targetFloor = 0;
    elevator.targetFloorUI = 0;
    elevator.stateStartTime = 0;
}

void Elevator_Update(uint32_t timeNow)
{
    uint32_t timeInState = timeNow - elevator.stateStartTime;
    // FSM implementada atraves do switch case
    switch (elevator.state)
    {
        case STATE_IDLE:
            // CONFIRMA DESTINO
            if (confirmFlag)
            {
                confirmFlag = 0;
                if (elevator.targetFloorUI != elevator.currentFloor)
                {
                    elevator.targetFloor = elevator.targetFloorUI;
                    elevator.state = STATE_DOOR_CLOSING;
                    elevator.stateStartTime = timeNow;
                    
                    traveling_to_call = false;     
                    waiting_for_selection = false; 
                    waiting_for_exit = false;      

                    if (elevator.currentFloor == 1) GPIO_PinOutClear(LED0_PORT, LED0_PIN);
                    if (elevator.currentFloor == 2) GPIO_PinOutClear(LED1_PORT, LED1_PIN);
                }
            }

            // TEMPO E FILA
            if (elevator.state == STATE_IDLE) 
            {
                if (waiting_for_selection)
                {
                    // 15s para escolher o andar
                    if (timeInState >= 15000)
                    {
                        waiting_for_selection = false;
                        elevator.stateStartTime = timeNow;

                        if (elevator.currentFloor == 1) GPIO_PinOutClear(LED0_PORT, LED0_PIN);
                        if (elevator.currentFloor == 2) GPIO_PinOutClear(LED1_PORT, LED1_PIN);
                    }
                }
                else if (waiting_for_exit)
                {
                    // tempo pra sair
                    if (timeInState >= 5000)
                    {
                        waiting_for_exit = false;
                        elevator.stateStartTime = timeNow; 
                    }
                }
                else
                {
                    // Verificando fila
                    if (q_len > 0)
                    {
                        elevator.targetFloor = Dequeue();
                        elevator.state = STATE_DOOR_CLOSING;
                        elevator.stateStartTime = timeNow;
                        traveling_to_call = true; 
                    }
                    else if (elevator.currentFloor != 0 && timeInState >= 15000)
                    {
                        // pro térreo
                        elevator.targetFloor = 0;
                        elevator.state = STATE_DOOR_CLOSING;
                        elevator.stateStartTime = timeNow;
                        traveling_to_call = false; 
                    }
                }
            }
            break;

        case STATE_DOOR_CLOSING:
            if (timeInState >= TIME_DOOR)
            {
                elevator.stateStartTime = timeNow;
                if (elevator.targetFloor > elevator.currentFloor)
                    elevator.state = STATE_MOVING_UP;
                else
                    elevator.state = STATE_MOVING_DOWN;
            }
            break;

        case STATE_MOVING_UP:
            if (timeInState >= TIME_TRAVEL)
            {
                elevator.currentFloor++;
                elevator.stateStartTime = timeNow;

                if (elevator.currentFloor >= elevator.targetFloor)
                    elevator.state = STATE_DOOR_OPENING;
            }
            break;

        case STATE_MOVING_DOWN:
            if (timeInState >= TIME_TRAVEL)
            {
                elevator.currentFloor--;
                elevator.stateStartTime = timeNow;

                if (elevator.currentFloor <= elevator.targetFloor)
                    elevator.state = STATE_DOOR_OPENING;
            }
            break;

        case STATE_DOOR_OPENING:
            if (timeInState >= TIME_DOOR)
            {
                elevator.state = STATE_IDLE;
                elevator.stateStartTime = timeNow;
                             
                if (traveling_to_call) {
                    waiting_for_selection = true; 
                } else {
                    waiting_for_exit = true;      
                }
            }
            break;
    }
}


void UpdateDisplay(void)
{
    char* targetTexts[] = { "ALVO: T", "ALVO: 2", "ALVO: 3" };
    char* waitTexts[]   = { "ESP: T ", "ESP: 2 ", "ESP: 3 " }; 

    switch (elevator.state)
    {
        case STATE_IDLE:          
            if (waiting_for_selection) {
                Display_WriteText(waitTexts[elevator.targetFloorUI]);
            } else if (waiting_for_exit) {
                Display_WriteText("SAINDO "); 
            } else {
                Display_WriteText(targetTexts[elevator.targetFloorUI]);
            }
            break;
            
        case STATE_DOOR_CLOSING:  
            Display_WriteText("FECHAND"); 
            break;
        case STATE_MOVING_UP:     
            Display_WriteText("SUBINDO"); 
            break;
        case STATE_MOVING_DOWN:   
            Display_WriteText("DESCEND"); 
            break;
        case STATE_DOOR_OPENING:  
            Display_WriteText("ABRINDO"); 
            break;
    }

    if (elevator.currentFloor == 0) Display_WriteNumber(0);
    else if (elevator.currentFloor == 1) Display_WriteNumber(2);
    else if (elevator.currentFloor == 2) Display_WriteNumber(3);
}

int main(void)
{
    CHIP_Init();
    CMU_ClockEnable(cmuClock_GPIO, true);
    SysTick_Config(SystemCoreClock / 1000);
    Display_Init();

    GPIO_PinModeSet(BTN_CHAMA_2_PORT, BTN_CHAMA_2_PIN, gpioModeInputPullFilter, 0);
    GPIO_PinModeSet(BTN_CHAMA_3_PORT, BTN_CHAMA_3_PIN, gpioModeInputPullFilter, 0);
    GPIO_PinModeSet(BTN_SELECIONA_PORT, BTN_SELECIONA_PIN, gpioModeInputPullFilter, 1);
    GPIO_PinModeSet(BTN_CONFIRMA_PORT,  BTN_CONFIRMA_PIN,  gpioModeInputPullFilter, 1);
    GPIO_PinModeSet(LED0_PORT, LED0_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(LED1_PORT, LED1_PIN, gpioModePushPull, 0);

    Elevator_Init();
    __enable_irq();

    uint32_t lastPressCall2 = 0;
    uint32_t lastPressCall3 = 0;
    uint32_t lastPressSelect = 0;
    uint32_t lastPressConfirm = 0;

    while (1)
    {
        uint8_t val_pd6 = GPIO_PinInGet(BTN_CHAMA_2_PORT, BTN_CHAMA_2_PIN);
        uint8_t val_pd7 = GPIO_PinInGet(BTN_CHAMA_3_PORT, BTN_CHAMA_3_PIN);

        // Chamada Andar 2
        if (val_pd6 == 1 && (msTicks - lastPressCall2) > 250)
        {
            lastPressCall2 = msTicks;
            if (elevator.currentFloor == 1 && elevator.state == STATE_IDLE && !waiting_for_exit) {
                waiting_for_selection = true;
                elevator.stateStartTime = msTicks;
            } else {
                Enqueue(1); 
                GPIO_PinOutSet(LED0_PORT, LED0_PIN); 
            }
        }

        // Chamada Andar 3
        if (val_pd7 == 1 && (msTicks - lastPressCall3) > 250) // Pra nao ocorrer erro de o programa ler o botão varias vezes em um tempo muito pequeno
        {
            lastPressCall3 = msTicks;
            if (elevator.currentFloor == 2 && elevator.state == STATE_IDLE && !waiting_for_exit) {
                waiting_for_selection = true;
                elevator.stateStartTime = msTicks;
            } else {
                Enqueue(2);
                GPIO_PinOutSet(LED1_PORT, LED1_PIN);
            }
        }

        uint8_t val_pb9  = GPIO_PinInGet(BTN_SELECIONA_PORT, BTN_SELECIONA_PIN);
        uint8_t val_pb10 = GPIO_PinInGet(BTN_CONFIRMA_PORT, BTN_CONFIRMA_PIN);

        if (val_pb9 == 0 && (msTicks - lastPressSelect) > 250)
        {
            lastPressSelect = msTicks;
            selectFlag = 1;
        }

        if (val_pb10 == 0 && (msTicks - lastPressConfirm) > 250)
        {
            lastPressConfirm = msTicks;
            confirmFlag = 1;
        }

        if (selectFlag)
        {
            selectFlag = 0; 
            if (elevator.state == STATE_IDLE)
            {
                elevator.targetFloorUI++;
                if (elevator.targetFloorUI > MAX_FLOOR)
                    elevator.targetFloorUI = 0;
            }
        }

        Elevator_Update(msTicks);
        UpdateDisplay();
    }
}