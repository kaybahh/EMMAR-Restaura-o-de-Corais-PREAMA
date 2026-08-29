#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "HT_TinyGPS++.h"

// CONFIGURAÇÕES DO RADIO LORA

#define RF_FREQUENCY                                915000000 // Hz --- FREQUÊNCIA

#define TX_OUTPUT_POWER                             22        // dBm ---- potencia de saida

#define LORA_BANDWIDTH                              1         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       11         // [SF7..SF12] --- VELOCIDADE DE TRANSMISSÃO (Quanto maior, mais lento e maior alcance)????
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 50 // Define the payload size here --- Espaço para armazenar as mensagens

static SSD1306Wire display_lora(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
TinyGPSPlus gps;
#define gpsSerial Serial2

char txdadosGPS[BUFFER_SIZE]; // arquivo para futura transmissão dos dados coletado pelo gps

float latitude, longitude, velocidade = 0;

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup() {
  VextON();
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 45, 46); // 44 (RX PIN da LORA), 43 (TXPIN LORA)
  Serial.println("Waiting for GPS fix and satellites...");
  xTaskCreate(
    tarefaGPS,
    "GPS",
    10000,
    NULL,
    1,
    NULL
  );

  display_lora.init();
  display_lora.clear();
  display_lora.display();
  display_lora.setContrast(255);
  display_lora.setFont(ArialMT_Plain_10);
  display_lora.setTextAlignment(TEXT_ALIGN_LEFT);
  display_lora.display();
  xTaskCreate(
    tarefaOLED,
    "GPS",
    10000,
    NULL,
    1,
    NULL
  );
}

void loop() {
}

void tarefaGPS(void*parametro){
  while (1){
    while (gpsSerial.available() > 0){ // verifica se existem dados na porta serial
      gps.encode(gpsSerial.read()); // biblioteca do gps interpreta os dados
    }
    
    if (millis() > 5000 && gps.charsProcessed() < 10) {
      Serial.println(F("No GPS detected: check wiring.")); // avisa que o GPS não foi detectado
      while (true);
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); 
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      velocidade = gps.speed.mps();
      Serial.printf("Latitude: %.4f\nLongitude: %.4f\nVelocidade: %.2f m/s\n",latitude, longitude, velocidade);
      sprintf(txdadosGPS,"Lat: %.4f\nLong: %.4f\nVel: %.2f m/s\n",latitude, longitude, velocidade);
      vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}

void tarefaOLED(void*parametro){
  while (1){
    display_lora.clear();
    display_lora.drawString(0, 0, "Lat: " + String(latitude, 4));
    display_lora.drawString(0, 20, "Lng: " + String(longitude, 4));
    display_lora.drawString(0, 40, "Vel: " + String(velocidade, 2) + " m/s");
    display_lora.display();
    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}
