#include <Wire.h>
#include "RTClib.h"
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"

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

char txdadosRTC[BUFFER_SIZE]; // --- Mensagem que será transmitida

static SSD1306Wire display_lora(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
TwoWire Wire2(1);
RTC_DS3231 rtc;

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup() {
  // put your setup code here, to run once:
  VextON();
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE); //--- inicializa a placa heltec
  iniciarRTC();
  iniciarOLED();

}

bool iniciarRTC(){
  Wire2.begin(41,42);

  if (! rtc.begin(&Wire2)) {
        Serial.println("RTC não encontrado, verifique as conexões!");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  xTaskCreate(
    TarefaRTC,
    "RTC",
    10000,
    NULL,
    1,
    NULL
  );
  return true;
}

bool iniciarOLED(){
  display_lora.init();
  display_lora.clear();
  display_lora.display();
  display_lora.setContrast(255);
  display_lora.setFont(ArialMT_Plain_10);
  display_lora.setTextAlignment(TEXT_ALIGN_LEFT);
  display_lora.display();
  xTaskCreate(
    TarefaOLED,
    "OLED",
    10000,
    NULL,
    1,
    NULL
  );
  return true;
}

void loop() {
  // put your main code here, to run repeatedly:
  
}

void TarefaRTC(void*parametro)
  {
    char daysOfTheWeek[7][12] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};

    int ano; int dia; int mes; char dia_da_semana;
    int hora; int minutos; int segundos;

  while (1) {
        DateTime now = rtc.now();
        ano = now.year();
        mes = now.month();
        dia = now.day();
        dia_da_semana = now.dayOfTheWeek();
        hora = now.hour();
        minutos = now.minute();
        segundos = now.second();
        
        sprintf(txdadosRTC,"%02d/%02d/%04d %s %02d:%02d:%02d\n", dia, mes, ano, daysOfTheWeek[dia_da_semana], hora, minutos, segundos);
        Serial.println(txdadosRTC);
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
  }
void TarefaOLED(void*parametro){
  while (1){
    display_lora.clear();
    display_lora.drawStringMaxWidth(0, 0, 128, txdadosRTC);
    display_lora.display();
    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}