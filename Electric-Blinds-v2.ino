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

bool stopPressed = false, isMoving = false, syncStopedPosition = false;
unsigned long lastRefreshed = 0;

BLYNK_CONNECTED()
{
    digitalWrite(LED_BUILTIN, HIGH);
}

// Move up
BLYNK_WRITE(V2)
{
    if (param.asInt())
    {
        Serial.println("Move Up");
        lock_lock();
        stepper.moveTo(STEPS_PER_CIRCLE * NUM_CIRCLES_TO_FULL_OPEN);
        isMoving = true;
        syncStopedPosition = true;
        lock_unlock();
    }
}

// Move down
BLYNK_WRITE(V3)
{
    if (param.asInt())
    {
        Serial.println("Move Down");
        lock_lock();
        stepper.moveTo(0);
        isMoving = true;
        syncStopedPosition = true;
        lock_unlock();
    }
}

// Stop
BLYNK_WRITE(V4)
{
    if (param.asInt())
    {
        Serial.println("Stop");

        lock_lock();
        stopPressed = true;
        syncStopedPosition = true;
        lock_unlock();
    }
}

// Wanted state slider
BLYNK_WRITE(V6)
{
    int wantedPercentage = param.asInt();
    stepper.moveTo(map(param.asInt(), 0, 100, 0, STEPS_PER_CIRCLE * NUM_CIRCLES_TO_FULL_OPEN));

    lock_lock();
    isMoving = true;
    lock_unlock();
}

void setup()
{
    Serial.begin(115200);

    // EEPROM.begin(EEPROM_SIZE);

    stepper.setMaxSpeed(STEPPER_MAX_SPEED);
    stepper.setSpeed(STEPPER_MAX_SPEED);
    stepper.setAcceleration(STEPPER_ACCELERATION);

    pinMode(DRIVER_ENABLE_PIN, OUTPUT);
    digitalWrite(DRIVER_ENABLE_PIN, LOW);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_NAME, WIFI_PASSWORD);

    while (!Serial)
    {
        delay(1);
    }

    xTaskCreatePinnedToCore(esploop1, "loop1", 10000, NULL, 0, &task_loop1, !ARDUINO_RUNNING_CORE);
    lock = xSemaphoreCreateMutex();
}

void loop()
{
    Blynk.run();

    if (Blynk.connected() == false)
    {
        digitalWrite(LED_BUILTIN, LOW);
    }

    lock_lock();
    if (isMoving && millis() - lastRefreshed > BLYNK_REFRESH_INTRERVAL)
    {
        Blynk.virtualWrite(V5, map(stepper.currentPosition(), 0, STEPS_PER_CIRCLE * NUM_CIRCLES_TO_FULL_OPEN, 0, 100));
        lastRefreshed = millis();
    }
    lock_unlock();
}

void loop1()
{
    lock_lock();
    if (stepper.distanceToGo() != 0)
    {
        stepper.run();
    }
    else
    {
        isMoving = false;
    }

    if (!isMoving && syncStopedPosition)
    {
        Blynk.virtualWrite(V6, map(stepper.currentPosition(), 0, STEPS_PER_CIRCLE * NUM_CIRCLES_TO_FULL_OPEN, 0, 100));
        syncStopedPosition = false;
    }

    if (stopPressed)
    {
        stepper.stop();

        stopPressed = false;
    }
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
