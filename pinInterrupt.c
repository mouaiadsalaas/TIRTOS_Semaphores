/*
 *  ======== main_tirtos.c ========
 */
#include <stdint.h>

/* POSIX Header files */
#include <pthread.h>

/* RTOS header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>

/* Example/Board Header files */
#include <ti/drivers/Board.h>

/* Stack size in bytes */
#define THREADSTACKSIZE    1024


#include <unistd.h>

/* Driver Header files */
#include <ti/drivers/PIN.h>

/* Example/Board Header files */
#include "Board.h"
/* XDCtools Header files */

#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/PIN.h>
#include <ti/sysbios/BIOS.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Watchdog.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/ADC.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>


#define delayMs(a)                Task_sleep(a*1000 / Clock_tickPeriod);


Char task0Stack[THREADSTACKSIZE],
     task1Stack[THREADSTACKSIZE];


/* Sytstem Tasks and clocks Structs */
Task_Struct task0Struct,
            task1Struct;

/* Pin driver handles */
static PIN_Handle buttonPinHandle;
static PIN_Handle ledPinHandle;
static Semaphore_Handle semHandle;

Semaphore_Struct semStruct,
                 sem2Struct;

/* Global memory storage for a PIN_Config table */
static PIN_State buttonPinState;
static PIN_State ledPinState;

/*
 * Initial LED pin configuration table
 *   - LEDs Board_PIN_LED0 is on.
 *   - LEDs Board_PIN_LED1 is off.
 */
PIN_Config ledPinTable[] = {
    Board_PIN_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_PIN_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

/*
 * Application button pin configuration table:
 *   - Buttons interrupts are configured to trigger on falling edge.
 */
PIN_Config buttonPinTable[] = {
    Board_PIN_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    Board_PIN_BUTTON1  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};



/*
 *  ======== mainThread ========
 */
void FirstThread(void *arg0)
{

    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if(!ledPinHandle) {
        /* Error initializing board LED pins */
        while(1);
    }



    /* Loop forever */
    while(1) {
        SemaphoreP_pend(Semaphore_handle(&sem2Struct), BIOS_WAIT_FOREVER);
        uint16_t count = 5;
        while(count--){
            PIN_setOutputValue(ledPinHandle, Board_PIN_LED1, 1);
            delayMs(500);
            PIN_setOutputValue(ledPinHandle, Board_PIN_LED1, 0);
            delayMs(500);

        }
        Semaphore_post(Semaphore_handle(&semStruct));

    }
}


void SecondThread(void *arg0)
{
    /* Loop forever */
    while(1) {
        SemaphoreP_pend(Semaphore_handle(&semStruct), BIOS_WAIT_FOREVER);
        uint16_t count = 5;
        while(count--){
            PIN_setOutputValue(ledPinHandle, Board_PIN_LED0, 1);
            delayMs(500);
            PIN_setOutputValue(ledPinHandle, Board_PIN_LED0, 0);
            delayMs(500);
        }
        Semaphore_post(Semaphore_handle(&sem2Struct));
    }
}
int main(void){
    Task_Params taskParams;


    Board_init();

    GPIO_init();
    UART_init();
    Semaphore_Params semParams;


    Task_Params_init(&taskParams);
    taskParams.stackSize = THREADSTACKSIZE;
    taskParams.stack = &task0Stack;
    taskParams.instance->name = "FirstThread";
    Task_construct(&task0Struct, (Task_FuncPtr)FirstThread, &taskParams, NULL);



    Task_Params_init(&taskParams);
    taskParams.stackSize = THREADSTACKSIZE;
    taskParams.stack = &task1Stack;
    taskParams.instance->name = "SecondThread";
    Task_construct(&task1Struct, (Task_FuncPtr)SecondThread, &taskParams, NULL);

    Semaphore_Params_init(&semParams);

    semParams.mode = Semaphore_Mode_BINARY;
    Semaphore_construct(&semStruct, 0, &semParams);

    semParams.mode = Semaphore_Mode_BINARY;
    Semaphore_construct(&sem2Struct, 1, &semParams);
    Semaphore_post(Semaphore_handle(&sem2Struct));

    BIOS_start();

    return (0);
}
