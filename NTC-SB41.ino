#include <Arduino.h>
#include <math.h>

// ////////// CONFIGURAÇÕES DO SENSOR NTC 10K //////////////////////////
const int pinoNTC = 1;
const float resistorSerie = 9920.0;
const float resNominal = 10000.0;    
const float tempNominal = 25.0;      
const float beta = 3950.0;           
const float offset = -3.0;          // Ajuste linear de calibração

// ////////// TAREFA FREERTOS: LEITURA SERIAL //////////////////////////
void taskTemperatura(void *parameter) {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(3000));

  pinMode(pinoNTC, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("\n--- Sistema de leitura NTC iniciado! ---");

  while (true) {
    long somaADC = 0;
    
    for (int i = 0; i < 10; i++) {
      somaADC += analogRead(pinoNTC);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    int valorADC = somaADC / 10;

    if (valorADC > 50 && valorADC < 4045) {
      float resistenciaNTC = resistorSerie / ((4095.0 / (float)valorADC) - 1.0);
      
      float tempKelvin = resistenciaNTC / resNominal;
      tempKelvin = log(tempKelvin);
      tempKelvin /= beta;
      tempKelvin += 1.0 / (tempNominal + 273.15);
      tempKelvin = 1.0 / tempKelvin;
      float tempCelsius = tempKelvin - 273.15;

      tempCelsius = tempCelsius + offset;

      Serial.printf("[NTC] Temp: %.2f °C\n", tempCelsius);
      
    } else {
      Serial.println("[ERRO] Leitura fora dos limites!");
    }

    vTaskDelay(pdMS_TO_TICKS(2000)); 
  }
}

void setup() {
  xTaskCreate(taskTemperatura, "TaskTemperatura", 4096, NULL, 1, NULL);
}

void loop() {

}
