TaskHandle_t task_loop1;
SemaphoreHandle_t lock = NULL;

void lock_lock() {
  while ((lock == NULL) || (xSemaphoreTake(lock, (TickType_t)0) == pdFALSE)) {
    delay(1);
  }
}

void lock_unlock() {
  xSemaphoreGive(lock);
}

void esploop1(void* pvParameters) {
  for (;;)
    loop1();
}

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(1); 
  }

  xTaskCreatePinnedToCore(esploop1, "loop1", 10000, NULL, 1, &task_loop1, !ARDUINO_RUNNING_CORE);

  lock = xSemaphoreCreateMutex();
}

void loop() {
  Serial.printf("loop%d()\n", xPortGetCoreID());

  delay(1000);
}

void loop1() {
  Serial.printf("loop%d()\n", xPortGetCoreID());

  delay(1000);
}
