#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h"

// ==================== LORA ====================

#define RF_FREQUENCY                915000000
#define LORA_BANDWIDTH              1
#define LORA_SPREADING_FACTOR      11
#define LORA_CODINGRATE             1
#define LORA_PREAMBLE_LENGTH        8
#define LORA_SYMBOL_TIMEOUT         0
#define LORA_FIX_LENGTH_PAYLOAD_ON  false
#define LORA_IQ_INVERSION_ON        false

#define BUFFER_SIZE                 100

// ==================== OLED ====================

SSD1306Wire display_lora(0x3c,500000,SDA_OLED,SCL_OLED,GEOMETRY_128_64,RST_OLED);

// ==================== RECEPÇÃO ====================

char rxdadosGPS[BUFFER_SIZE];

int16_t rssi;
int8_t snr;
uint16_t rxSize;

bool lora_idle = true;

// ==================== RADIO ====================

static RadioEvents_t RadioEvents;

void OnRxDone(uint8_t *payload,uint16_t size,int16_t rssi,int8_t snr);


// =================================================
// SETUP
// =================================================

void setup()
{
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);

    // ---------------- RADIO ----------------

    RadioEvents.RxDone = OnRxDone;

    Radio.Init(&RadioEvents);

    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetRxConfig(
        MODEM_LORA,LORA_BANDWIDTH,LORA_SPREADING_FACTOR,LORA_CODINGRATE,0,LORA_PREAMBLE_LENGTH,LORA_SYMBOL_TIMEOUT,LORA_FIX_LENGTH_PAYLOAD_ON,0, true,0,0,LORA_IQ_INVERSION_ON,true);

    // ---------------- OLED ----------------

    display_lora.init();
    display_lora.clear();
    display_lora.setContrast(255);
    display_lora.setFont(ArialMT_Plain_10);
    display_lora.setTextAlignment(TEXT_ALIGN_LEFT);

    display_lora.drawString(0, 0, "RECEPTOR");
    display_lora.display();

    // ---------------- TASKS ----------------

    xTaskCreate(
        tarefaRadio,
        "Radio",
        10000,
        NULL,
        1,
        NULL
    );

    xTaskCreate(
        tarefaOLED,
        "OLED",
        10000,
        NULL,
        1,
        NULL
    );
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
            lora_idle = false;

            Serial.println("Entrando em RX...");

            Radio.Rx(0);
        }

        Radio.IrqProcess();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// =================================================
// CALLBACK - RECEBEU PACOTE
// =================================================

void OnRxDone(uint8_t *payload,uint16_t size,int16_t rssiRecebido,int8_t snrRecebido)
{
    if (size > BUFFER_SIZE)
    {
        size = BUFFER_SIZE;
    }

    memcpy(rxdadosGPS,payload,size);

    rxdadosGPS[size] = '\0';

    rxSize = size;
    rssi = rssiRecebido;
    snr = snrRecebido;

    Serial.println();
    Serial.println("PACOTE RECEBIDO!");
    Serial.println(rxdadosGPS);

    Serial.print("RSSI: ");
    Serial.println(rssi);

    Serial.print("SNR: ");
    Serial.println(snr);

    Radio.Sleep();

    lora_idle = true;
}


// =================================================
// TAREFA OLED
// =================================================

void tarefaOLED(void *parametro)
{
  char lat[30];
  char lng[30];
  char velocidade[30];

    while (1)
    {
        sscanf(rxdadosGPS,"%[^\n]\n%[^\n]\n%[^\n]",lat,lng,velocidade);
        display_lora.clear(); 
        display_lora.drawString(0, 0, "RECEPTOR"); 
        display_lora.drawString(0, 16, lat); 
        display_lora.drawString(0, 32, lng); 
        display_lora.drawString(0, 48, velocidade);
        display_lora.display();

        display_lora.display();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


// =================================================
// LOOP
// =================================================

void loop()
{
}