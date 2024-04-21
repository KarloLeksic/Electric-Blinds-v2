#ifndef __SETTINGS__
#define __SETTINGS__

// Pins
#define STEP_PIN          4
#define DIR_PIN           5
#define DRIVER_ENABLE_PIN 14

// Stepper
#define STEPS_PER_CIRCLE     200
#define STEPPER_MAX_SPEED    1000
#define STEPPER_ACCELERATION 500

// Blinds specific
#define NUM_CIRCLES_TO_FULL_OPEN 480

// EEPROM
#define EEPROM_SIZE    4
#define EEPROM_ADDRESS 0

#endif