#include <stdint.h>
#include <stddef.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>
#include "ti_drivers_config.h"
// Michelle Lewis
// CS-350-11207-M01

// Enumeration for states
typedef enum {
    DOT,
    DASH,
    CHARACTER_GAP,
    WORD_GAP,
    MESSAGE_END
} State;

// Define a Morse code element with state and duration
typedef struct {
    State state;  // State
    uint32_t duration;  // Duration for the timer in microseconds
} MorseCode;

// Morse code pattern for SOS
MorseCode sosPattern[] = {
    {DOT, 500000}, {CHARACTER_GAP, 1500000}, {DOT, 500000}, {CHARACTER_GAP, 1500000}, {DOT, 500000}, // S
    {CHARACTER_GAP, 3500000}, // Gap between S and O
    {DASH, 1500000}, {CHARACTER_GAP, 1500000}, {DASH, 1500000}, {CHARACTER_GAP, 1500000}, {DASH, 1500000}, // O
    {CHARACTER_GAP, 3500000}, // Gap between O and S
    {DOT, 500000}, {CHARACTER_GAP, 1500000}, {DOT, 500000}, {CHARACTER_GAP, 1500000}, {DOT, 500000}, // S
    {WORD_GAP, 7500000}, // Gap between words
    {MESSAGE_END, 0}
};

// Morse code pattern for OK
MorseCode okPattern[] = {
    {DASH, 1500000}, {CHARACTER_GAP, 1500000}, {DASH, 1500000},  {CHARACTER_GAP, 1500000}, {DASH, 1500000}, // O
    {CHARACTER_GAP, 3500000}, // Gap between O and K
    {DASH, 1500000}, {CHARACTER_GAP, 1500000}, {DOT, 500000},  {CHARACTER_GAP, 1500000}, {DASH, 1500000},// K
    {WORD_GAP, 7500000}, // Gap between words
    {MESSAGE_END, 0}
};

MorseCode *currentPattern = sosPattern;  // Current pattern pointer
int patternIndex = 0;
bool changeMessage = false;
static uint32_t accumulatedTime = 0;  // Accumulated time to handle state transitions

// GPIO interrupt handler for Button 0 (Left Button)
void gpioButtonFxn0(uint_least8_t index) {
    changeMessage = true; // Signal to change message after current completes
}

// GPIO interrupt handler for Button 1 (Right Button)
void gpioButtonFxn1(uint_least8_t index) {
    changeMessage = true; // Signal to change message when button 1 is pressed
}

// Timer callback function to manage Morse code state transitions
void timerCallback(Timer_Handle myHandle, int_fast16_t status) {
    MorseCode step = currentPattern[patternIndex];

    // Increment accumulated time by 500 ms each callback call
    accumulatedTime += 500000;

    // Check if it's time to transition to the next state
    if (accumulatedTime >= step.duration) {
        accumulatedTime = 0; // Reset accumulated time for the next state
        patternIndex++; // Move to the next Morse code element

        // Check for end of message
        if (currentPattern[patternIndex].state == MESSAGE_END) {
            if (changeMessage) {
                changeMessage = false;
                currentPattern = (currentPattern == sosPattern) ? okPattern : sosPattern;
                patternIndex = 0;
            } else {
                patternIndex = 0; // Restart the pattern from the beginning
            }
        }

        step = currentPattern[patternIndex]; // Update the step to the new state
    }

    // Control LEDs based on the current state
    switch (step.state) {
        case DOT:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON); // Red LED on
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);  // Green LED off
            break;
        case DASH:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF); // Red LED off
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);  // Green LED on
            break;
        case CHARACTER_GAP:  // Gap between characters (letters)
        case WORD_GAP:       // Gap between words
        case MESSAGE_END:    // State to check which message is next
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF); // Red LED off
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF); // Green LED off
            break;
    }
}

// Initialize the timer and set its parameters
void initTimer(void) {
    Timer_Handle timer0;
    Timer_Params params;

    Timer_init();
    Timer_Params_init(&params);
    params.period = 500000;  // Set timer to activate every 500 ms
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    timer0 = Timer_open(CONFIG_TIMER_0, &params);
    if (timer0 == NULL) {
        while (1) {} // Handle error
    }
    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        while (1) {} // Handle error
    }
}

// Main thread to initialize GPIO and start the timer
void *mainThread(void *arg0) {
    GPIO_init();

    // Configure Red and Green LEDs
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    // Configure Button 0 (Left Button)
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    // Configure Button 1 (Right Button)
    GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_1);

    initTimer(); // Start the timer and the state machine
    return NULL;
}

