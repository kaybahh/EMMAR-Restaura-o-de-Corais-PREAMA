// ============================================================
// BIBLIOTECAS
// ============================================================

#include <Wire.h>
#include "RTClib.h"
#include "LoRaWan_APP.h"

// ============================================================
// CONFIGURAÇÕES DO RÁDIO LoRa
// ============================================================

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

// Define o tamanho máximo do buffer utilizado para guardar os dados que serão transmitidos
#define BUFFER_SIZE                                 100

// ============================================================
// VARIÁVEIS E OBJETOS GLOBAIS
// ============================================================
char txdadosRTC[BUFFER_SIZE]; //armazena os dados coletados pelo RTC que serão transmitidos

// Variáveis criadas para organizar melhor data, hora e dia da semana
char dataRTC[20];
char diaSemanaRTC[15];
char horaRTC[15];

TwoWire Wire2(1);
RTC_DS3231 rtc;

// ===================
// Radio
// ===================
// indica se o radio esta livre para uma transmissão: true = disponivel; false = ocupado
bool lora_idle=true;

// Estrutura utilizada pela biblioteca LoRa para armazenar as funções de callback relacionadas aos eventos do rádio.
static RadioEvents_t RadioLora;
void OnTxDone( void );
void OnTxTimeout( void );

// ============================================================
// FUNÇÃO PARA LIGAR A ALIMENTAÇÃO DOS PERIFÉRICOS
// ============================================================

void VextON(void)
{
  // Configura o pino Vext como saída.
  pinMode(Vext,OUTPUT);
  // Coloca Vext em nível LOW para ligar a alimentação dos periféricos conectados a essa saída da Heltec.
  digitalWrite(Vext, LOW);
}

void setup() {
  // Liga a alimentação dos periféricos da placa.
  VextON();
  Serial.begin(115200);
  delay(1000);
  Wire2.begin(41,42);  
  //GPIO 41 = SDA
  //GPIO 42 = SCL

  // Inicializa os recursos internos da placa Heltec.
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  RadioLora.TxDone = OnTxDone;
  RadioLora.TxTimeout = OnTxTimeout;

  // ==========================================================
  // INICIALIZAÇÃO E CONFIGURAÇÃO DO RÁDIO
  // ==========================================================
  Radio.Init( &RadioLora ); // --- liga a rádio LoRa
  Radio.SetChannel( RF_FREQUENCY ); // configura para frequência determinada
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
  // ==========================================================
  // INICIALIZAÇÃO DO RTC
  // ==========================================================
  if (! rtc.begin(&Wire2)) {
        Serial.println("RTC não encontrado, verifique a montagem!"); // se rtc não for encontrado, informa no serial
        Serial.flush();
        // mantém o programa parado caso o RTC não seja encontrado.
        while (1) delay(10);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // define data e hora do RTC no momento em que foi compilado
    if (rtc.lostPower()) {
      Serial.println("RTC perdeu alimentação, atualizando o tempo!");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

// ==========================================================
// CRIAÇÃO DAS TAREFAS DO FREERTOS
// ==========================================================
  xTaskCreate(
    TarefaRTC,
    "RTC",
    10000,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TarefaRadio,
    "RadioLora",
    10000,
    NULL,
    1,
    NULL
  );

}

void loop() {
  // put your main code here, to run repeatedly:
  
}

void TarefaRTC(void*parametro)
  {
    // rtc retorna o dia da semana atráves de um número: 0 = domingo; 1 = segunda...
    char daysOfTheWeek[7][12] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};
    // variáveis usadas para armazenar cada parte da data e horário
    int ano; int dia; int mes; char dia_da_semana;
    int hora; int minutos; int segundos;

  while (1) {
      // ========================================================
      // LEITURA DOS DADOS DO RTC
      // ========================================================
        DateTime now = rtc.now();
        ano = now.year();
        mes = now.month();
        dia = now.day();
        dia_da_semana = now.dayOfTheWeek();
        hora = now.hour();
        minutos = now.minute();
        segundos = now.second();
      //========================================================
      // ESCREVE OS DADOS COLETADOS NAS VARIÁVEIS CHAR
      //========================================================
        sprintf(dataRTC, "%02d/%02d/%04d", dia, mes, ano);
        sprintf(diaSemanaRTC, "%s", daysOfTheWeek[dia_da_semana]);
        sprintf(horaRTC, "%02d:%02d:%02d", hora, minutos, segundos);
        // ========================================================
        // MONTAGEM DA MENSAGEM PARA O LoRa
        // ========================================================
        sprintf(txdadosRTC,"%s\n %s\n %s", dataRTC, diaSemanaRTC, horaRTC); // unta a data, o dia da semana e o horário em uma variavel char.
        Serial.println(txdadosRTC);
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
  }

void TarefaRadio(void*parametro)
{
while(1) {
    // Verifica se o rádio está livre para uma nova transmissão.
    if (lora_idle == true) {
      // Envia a mensagem armazenada em txdadosRTC;  strlen() calcula quantos caracteres existem na mensagem, permitindo que o rádio saiba o tamanho do pacote.
      Radio.Send((uint8_t *)txdadosRTC, strlen(txdadosRTC));
      // radio fica ocupado fazendo a transmissão
      lora_idle = false;
    }
    Radio.IrqProcess();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void OnTxDone( void )
{
    lora_idle = true;
    // se transmissão for concluida, libera nova transmissão
}

void OnTxTimeout( void )
{
     // Coloca o rádio em modo de baixo consumo/repouso.
    Radio.Sleep( );
    // Libera o rádio para uma nova transmissão
    lora_idle = true;
}
