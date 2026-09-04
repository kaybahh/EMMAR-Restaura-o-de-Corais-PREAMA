#include <Arduino.h>
#include "LoRaWan_APP.h"
#include <math.h>

// ==================== LORA ====================

#define RF_FREQUENCY                915000000
#define TX_OUTPUT_POWER             22
#define LORA_BANDWIDTH              1
#define LORA_SPREADING_FACTOR      11
#define LORA_CODINGRATE             1
#define LORA_PREAMBLE_LENGTH        8
#define LORA_FIX_LENGTH_PAYLOAD_ON  false
#define LORA_IQ_INVERSION_ON        false

#define BUFFER_SIZE                 100

// ==================== RÁDIO ====================

static RadioEvents_t RadioEvents;
bool lora_idle = true;
void OnTxDone(void);
void OnTxTimeout(void);
char txdadosNTC[BUFFER_SIZE];
// ////////// CONFIGURAÇÕES DO SENSOR NTC 10K //////////////////////////
const int pinoNTC = 2;
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
      sprintf(txdadosNTC,"TEMP: %.2f °C\n",tempCelsius);
      Serial.printf("[NTC] Temp: %.2f °C\n", tempCelsius);
      
    } else {
      Serial.println("[ERRO] Leitura fora dos limites!");
    }

    vTaskDelay(pdMS_TO_TICKS(2000)); 
  }
}

void setup() {
  // Alimentação dos periféricos
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    // ---------------- RADIO ----------------

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;

    Radio.Init(&RadioEvents);

    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetTxConfig(MODEM_LORA,TX_OUTPUT_POWER,0,LORA_BANDWIDTH,LORA_SPREADING_FACTOR,LORA_CODINGRATE,LORA_PREAMBLE_LENGTH,LORA_FIX_LENGTH_PAYLOAD_ON, true, 0,0,LORA_IQ_INVERSION_ON,3000);

  xTaskCreate(taskTemperatura, "TaskTemperatura", 4096, NULL, 1, NULL);
  xTaskCreate(
        tarefaRadio,
        "Radio",
        10000,
        NULL,
        1,
        NULL
    );
}

void tarefaRadio(void *parametro)
{
    while (1)
    {
        if (lora_idle)
        {
         Serial.print("Enviando: ");
            Serial.println(txdadosNTC);

            Radio.Send((uint8_t *)txdadosNTC,strlen(txdadosNTC));

            lora_idle = false;
        }

        // Processa os eventos do rádio
        Radio.IrqProcess();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// =================================================
// CALLBACK - TRANSMISSÃO TERMINOU
// =================================================

void OnTxDone(void)
{
    Serial.println("TX concluído!");

    lora_idle = true;
}


// =================================================
// CALLBACK - TIMEOUT
// =================================================

void OnTxTimeout(void)
{
    Serial.println("TX Timeout!");
    vTaskDelay(pdMS_TO_TICKS(50));
    Radio.Sleep();

    lora_idle = true;
}

void loop() {

}