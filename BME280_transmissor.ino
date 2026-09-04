#include <Wire.h>
#include <Adafruit_BME280.h>
#include "LoRaWan_APP.h"

Adafruit_BME280 bme;

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
char txdadosBME280[BUFFER_SIZE];

//////////// Função BME280 ////////////////////////////////////////////////
void taskBME280(void *parameter) {
  while (true) {
    float temp = bme.readTemperature(); //le temperatura
    float pres = bme.readPressure() / 100.0F; //le pressão
    float umid = bme.readHumidity(); //le humildade

    //mostra os valores lidos em uma unica linha
    Serial.printf("[BME280] Temp: %.2f °C | Pressão: %.2f hPa | Umidade: %.2f %%\n", temp, pres, umid);
    sprintf(txdadosBME280,"%.2f °C\n %.2f hPa\n %.2f %\n",temp, pres, umid);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// chamar a função "iniciarBME280()" no void setup pra iniciar a leitura do BME
bool iniciarBME280() {
  Wire.begin(41, 42); //SDA = 41, SCL = 42

  if (!bme.begin(0x76, &Wire)) {
    Serial.println("BME280 não encontrado");
    return false;
  }

  // Cria a tarefa no FreeRTOS Alocando 4096 bytes
  xTaskCreate(taskBME280, "TaskBME280", 4096, NULL, 1, NULL);
  
  Serial.println("BME280 OK");
  return true;
}
////////////////////////////////////////////////////////////////////////

void setup(){
  Serial.begin(115200);
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
    xTaskCreate(
        tarefaRadio,
        "Radio",
        10000,
        NULL,
        1,
        NULL
    );
  iniciarBME280();

}

void tarefaRadio(void *parametro)
{
    while (1)
    {
        if (lora_idle)
        {
         Serial.println("Enviando: ");
            Serial.println(txdadosBME280);

            Radio.Send((uint8_t *)txdadosBME280,strlen(txdadosBME280));

            lora_idle = false;
        }

        // Processa os eventos do rádio
        Radio.IrqProcess();

        vTaskDelay(pdMS_TO_TICKS(1000));
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

void loop(){

}