// ============================================================
// BIBLIOTECAS
// ============================================================
#include <Wire.h>
#include "HT_SSD1306Wire.h"
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
#define BUFFER_SIZE                                 100 // Define the payload size here --- Espaço para armazenar as mensagens
// buffer que armazena os dados recebidos
char rxdadosRTC[BUFFER_SIZE];

static RadioEvents_t RadioLora;
// Declaração da função que será executada automaticamente quando um pacote LoRa for recebido.
void OnRxDone(uint8_t *payload,uint16_t size,int16_t rssi,int8_t snr);

static SSD1306Wire display_lora(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // cria o objeto responsável pelo controle do display OLED.

// ============================================================
// VARIÁVEIS PARA MONITORAMENTO DA COMUNICAÇÃO
// ============================================================

int16_t rssi; // Representa a intensidade do sinal recebido pelo rádio (quanto mais próximo de 0, mais forte é o sinal)
int8_t snr; // Representa a relação entre a potência do sinal recebido e a potência do ruído presente no canal. ( quanto maior melhor)
uint16_t rxSize; // Armazena a quantidade de bytes presentes no último pacote recebido.

//indica se o radio esta livre para uma recepção: true = livre, false = ocupado
bool lora_idle = true;

void VextON(void)
{
  // ============================================================
  // ALIMENTAÇÃO DOS PERIFÉRICOS
  // ============================================================

  // Configura o pino Vext como saída
  pinMode(Vext,OUTPUT);
  // Coloca Vext em nível LOW para ativar a alimentação dos perifericos
  digitalWrite(Vext, LOW);
}

void setup () {
  // Liga a alimentação dos periféricos.
  VextON();
  // Inicializa os recursos internos da placa Heltec.
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
  Serial.begin(115200);

  // ==========================================================
  // CONFIGURAÇÃO DO RÁDIO LoRa
  // ==========================================================
      rssi=0;
  
  // ---------------- RADIO ----------------

    RadioLora.RxDone = OnRxDone;

    Radio.Init(&RadioLora);

    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetRxConfig(
        MODEM_LORA,LORA_BANDWIDTH,LORA_SPREADING_FACTOR,LORA_CODINGRATE,0,LORA_PREAMBLE_LENGTH,LORA_SYMBOL_TIMEOUT,LORA_FIX_LENGTH_PAYLOAD_ON,0, true,0,0,LORA_IQ_INVERSION_ON,true);

//==========================================================//

  // ==========================================================
  // CONFIGURAÇÃO DO DISPLAY OLED
  // ==========================================================

  display_lora.init();
  display_lora.clear();
  display_lora.display();
  display_lora.setContrast(255);
  display_lora.setFont(ArialMT_Plain_10);
  display_lora.setTextAlignment(TEXT_ALIGN_LEFT);
  display_lora.display();

//==========================================================//

  // ==========================================================
  // CRIAÇÃO DAS TAREFAS DO FREERTOS
  // ==========================================================
  xTaskCreate(
    TarefaRX,
    "Receptor",
    10000,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    TarefaOLED,
    "Display",
    10000,
    NULL,
    1,
    NULL
  );
}

void loop () {
  } 

// =================================================
// TAREFA RADIO
// =================================================
void TarefaRX(void*parametro) {

  while (1)
    {
      // verifica se o radio esta livre para receber dados.
        if (lora_idle)
        {
            // coloca radio como ocupado enquanto recebe algo
            lora_idle = false;

            Serial.println("Entrando em RX...");

            Radio.Rx(0);
        }

        Radio.IrqProcess();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================
// TAREFA DO DISPLAY OLED
// ============================================================
void TarefaOLED(void*parametro) {
  // variaveis que armazenam as tres partes da mensagem recebida
  char dataRecebida[20];
  char diaRecebido[15];
  char horaRecebida[15];
  while (1){
    sscanf(rxdadosRTC,"%19[^\n]\n%14[^\n]\n%14[^\n]", dataRecebida, diaRecebido,horaRecebida); // utiliza sscanf para separar a mensagem em três partes
    // %19[^\n] -> lê até 19 caracteres ou até encontrar '\n'.
    display_lora.clear();
    // escreve os dados do rtc recebidos na tela
    display_lora.drawString(0, 0, "RECEPTOR");
    display_lora.drawString(0, 16, dataRecebida);
    display_lora.drawString(0, 32, diaRecebido);
    display_lora.drawString(0, 48, horaRecebida);
    display_lora.display();
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// =================================================
// CALLBACK - RECEBEU PACOTE
// Esta função é executada automaticamente pela biblioteca LoRa quando um pacote termina de ser recebido.
// A função armazena os dados no buffer e salva as informações de qualidade da comunicação para que possam ser utilizadas posteriormente.
// =================================================

void OnRxDone(uint8_t *payload,uint16_t size,int16_t rssiRecebido, int8_t snrRecebido)
{
    if (size > BUFFER_SIZE)
    {
        size = BUFFER_SIZE;
    }
    // copia os dados recebidos para o buffer rxdadosRTC
    memcpy(rxdadosRTC, payload, size); 
    // Adiciona o caractere '\0' ao final dos dados.
    //  Esse caractere indica o final de uma string em C/C++.
    // Isso permite que funções como Serial.println() e sscanf()
    // reconheçam corretamente o conteúdo como texto.
    rxdadosRTC[size] = '\0';

    // guarda o tamanho do pacote recebido
    rxSize = size;
    // armazena o valor RSSI informado pelo radio
    rssi = rssiRecebido;
    // armazena o valor snr informado pelo radio
    snr = snrRecebido;

    // exibe informações no monitor serial
    Serial.println();
    Serial.println("PACOTE RECEBIDO!");
    Serial.print("Dados: ");
    Serial.println(rxdadosRTC);

    Serial.print("Tamanho: ");
    Serial.println(size);

    Serial.print("RSSI: ");
    Serial.println(rssi);

    Serial.print("SNR: ");
    Serial.println(snr);

    //coloca radio em repouso
    Radio.Sleep();
    //informa que o radio está livre para nova recepção
    lora_idle = true;
}
