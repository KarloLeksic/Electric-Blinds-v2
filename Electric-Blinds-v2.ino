#include "src/secrets.h"
#include "src/settings.h"

#include "AccelStepper.h"
#include "BlynkSimpleEsp32.h"
#include "EEPROM.h"
#include "WiFi.h"
#include "WiFiClient.h"

TaskHandle_t task_loop1;
SemaphoreHandle_t lock = NULL;

void lock_lock()
{
    while ((lock == NULL) || (xSemaphoreTake(lock, (TickType_t)0) == pdFALSE))
    {
        delay(1);
    }
}

void lock_unlock()
{
    xSemaphoreGive(lock);
}

void esploop1(void *pvParameters)
{
    for (;;)
        loop1();
}

AccelStepper stepper(1, STEP_PIN, DIR_PIN);

int currentSteps;
bool goUp = false, goDown = false;

BLYNK_CONNECTED()
{
    digitalWrite(LED_BUILTIN, HIGH);
}

// Move up
BLYNK_WRITE(V2)
{
    if (param.asInt())
    {
        Blynk.virtualWrite(V1, "Lifting");
        moveUp();
        Blynk.virtualWrite(V1, "Up");
    }
}

// Move down
BLYNK_WRITE(V3)
{
    if (param.asInt())
    {
        Blynk.virtualWrite(V1, "Lowering");
        moveDown();
        Blynk.virtualWrite(V1, "Down");
    }
}

// Stop
BLYNK_WRITE(V4)
{
    if (param.asInt())
    {
        lock_lock();
        goUp = false;
        goDown = false;
        lock_unlock();
    }
}

void setup()
{
    Serial.begin(115200);

    EEPROM.begin(EEPROM_SIZE);
    // currentSteps = readStepsFromEEPROM();
    currentSteps = 0;

    stepper.setMaxSpeed(STEPPER_MAX_SPEED);
    stepper.setSpeed(STEPPER_MAX_SPEED);
    stepper.setAcceleration(STEPPER_ACCELERATION);

    pinMode(DRIVER_ENABLE_PIN, OUTPUT);
    digitalWrite(DRIVER_ENABLE_PIN, LOW);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_NAME, WIFI_PASSWORD);

    Blynk.virtualWrite(V1, "Down");

    while (!Serial)
    {
        delay(1);
    }

    xTaskCreatePinnedToCore(esploop1, "loop1", 10000, NULL, 1, &task_loop1, !ARDUINO_RUNNING_CORE);

    lock = xSemaphoreCreateMutex();
}

void loop()
{
    Blynk.run();

    if (Blynk.connected() == false)
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void loop1()
{
    delay(100);

    if (goUp)
    {
        if (currentSteps < STEPS_PER_CIRCLE * NUM_CIRCLES_TO_FULL_OPEN)
        {
            stepper.run();
            currentSteps++;
            Serial.println(currentSteps);
        }
        else
        {
            lock_lock();
            goUp = false;
            lock_unlock();
        }
    }

    if (goDown)
    {
        if (currentSteps > 0)
        {
            stepper.run();
            currentSteps--;
            Serial.println(currentSteps);
        }
        else
        {
            lock_lock();
            goDown = false;
            lock_unlock();
        }
    }
}

void moveUp()
{
    Serial.println("Move Up");

    stepper.moveTo(STEPS_PER_CIRCLE * NUM_CIRCLES_TO_FULL_OPEN);

    lock_lock();
    goUp = true;
    lock_unlock();
}

void moveDown()
{
    Serial.println("Move Down");

    stepper.moveTo(0);

    lock_lock();
    goDown = true;
    lock_unlock();
}

void writeStepsToEEPROM(int value)
{
    byte *p = (byte *)(void *)&value;
    for (unsigned int i = 0; i < sizeof(value); i++)
    {
        EEPROM.write(EEPROM_ADDRESS + i, *p++);
    }
    EEPROM.commit();
}

int readStepsFromEEPROM()
{
    int value = 0;
    byte *p = (byte *)(void *)&value;
    for (unsigned int i = 0; i < sizeof(value); i++)
    {
        *p++ = EEPROM.read(EEPROM_ADDRESS + i);
    }
    return value;
}
