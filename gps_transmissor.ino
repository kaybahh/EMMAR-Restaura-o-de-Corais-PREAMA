#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_TinyGPS++.h"

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


// ==================== GPS ====================

TinyGPSPlus gps;

#define gpsSerial Serial2

// GPIO 45 = RX da ESP32
// GPIO 46 = TX da ESP32

// ==================== VARIÁVEIS ====================

char txdadosGPS[BUFFER_SIZE];

float latitude = 0;
float longitude = 0;
float velocidade = 0;

bool lora_idle = true;

// ==================== RÁDIO ====================

static RadioEvents_t RadioEvents;

void OnTxDone(void);
void OnTxTimeout(void);

void setup()
{
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

    gpsSerial.begin(9600,SERIAL_8N1,45,46);

    // ---------------- TASKS ----------------

    xTaskCreate(
        tarefaGPS,
        "GPS",
        10000,
        NULL,
        1,
        NULL
    );

    xTaskCreate(
        tarefaRadio,
        "Radio",
        10000,
        NULL,
        1,
        NULL
    );
}


// =================================================
// TAREFA GPS
// =================================================

void tarefaGPS(void *parametro)
{
    while (1)
    {
        // Lê todos os caracteres disponíveis do GPS
        while (gpsSerial.available() > 0)
        {
            gps.encode(gpsSerial.read());
        }

        // Verifica se o GPS está enviando dados
        if (gps.charsProcessed() > 10)
        {
            if (gps.location.isValid())
            {
                latitude = gps.location.lat();
                longitude = gps.location.lng();
                velocidade = gps.speed.mps();

                // Monta o pacote
                snprintf(txdadosGPS,BUFFER_SIZE,"Lat: %.4f\nLng: %.4f\nVel: %.2f m/s",latitude,longitude,velocidade);


                Serial.println("GPS:");
                Serial.println(txdadosGPS);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// =================================================
// TAREFA RADIO
// =================================================

void tarefaRadio(void *parametro)
{
    while (1)
    {
        if (lora_idle)
        {
         Serial.print("Enviando: ");
            Serial.println(txdadosGPS);

            Radio.Send((uint8_t *)txdadosGPS,strlen(txdadosGPS));

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
    // Serial.println("TX concluído!");

    lora_idle = true;
}


// =================================================
// CALLBACK - TIMEOUT
// =================================================

void OnTxTimeout(void)
{
    vTaskDelay(pdMS_TO_TICKS(50));
    Radio.Sleep();

    lora_idle = true;
}

void loop()
{
}