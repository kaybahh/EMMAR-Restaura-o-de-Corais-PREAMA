#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

/////////////////////////// Configurações e Limites de Segurança da Boia/////////////////////////////////
#define LIMITE_INCLINACAO_GRAUS  45.0f   // Ângulo máximo de inclinação seguro
#define TEMPO_INCLINADO_MS       5000    // Tempo limite inclinado antes do alerta
#define LIMITE_IMPACTO_MS2       25.0f   // Limite de aceleração linear em m/s² (~2.5G)

TwoWire I2C_BNO = TwoWire(1); 
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &I2C_BNO);

// Converte o ângulo da bússola em ponto cardeal
const char* getDirecaoCardeal(float deg) {
  if (deg >= 337.5 || deg < 22.5)  return "N";
  if (deg >= 22.5  && deg < 67.5)  return "NE";
  if (deg >= 67.5  && deg < 112.5) return "E";
  if (deg >= 112.5 && deg < 157.5) return "SE";
  if (deg >= 157.5 && deg < 202.5) return "S";
  if (deg >= 202.5 && deg < 247.5) return "SW";
  if (deg >= 247.5 && deg < 292.5) return "W";
  if (deg >= 292.5 && deg < 337.5) return "NW";
  return "N";
}

/////////////TAREFA FREERTOS MONITORAMENTO E ALERTAS DA BOIA///////////////////////
void taskMonitoramentoBoia(void *parameter) {
  unsigned long inicioInclinacao = 0;
  bool alertaTombamentoAtivo = false;
  unsigned long ultimoRelatorio = 0;

  while (true) {
     
/////////////////Leitura de Orientação (Bússola / Pitch / Roll)///////////////////////////
    sensors_event_t orientData;
    bno.getEvent(&orientData, Adafruit_BNO055::VECTOR_EULER);

    float bussola = orientData.orientation.x; // 0° a 360° em relação ao Norte
    float pitch   = abs(orientData.orientation.y);
    float roll    = abs(orientData.orientation.z);
    float maiorInclinacao = max(pitch, roll);
  
//////////////////Leitura de Aceleração Linear (Livre da gravidade)/////////////////
    imu::Vector<3> linAccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    float forcaImpacto = sqrt(linAccel.x() * linAccel.x() + 
                              linAccel.y() * linAccel.y() + 
                              linAccel.z() * linAccel.z());

//////////////////ALERTA 1: IMPACTO / COLISÃO BRUSCA////////////////////////////////
    if (forcaImpacto >= LIMITE_IMPACTO_MS2) {
      Serial.printf("\n[ALERTA CRÍTICO] IMPACTO DETECTADO! Força: %.2f m/s²\n\n", forcaImpacto);
    }

///////////////////ALERTA 2: TOMBAMENTO OU INCLINAÇÃO PROLONGADA///////////////////
    if (maiorInclinacao > LIMITE_INCLINACAO_GRAUS) {
      if (inicioInclinacao == 0) {
        inicioInclinacao = millis(); 
      } else if (millis() - inicioInclinacao >= TEMPO_INCLINADO_MS) {
        if (!alertaTombamentoAtivo) {
          Serial.printf("\n[ALERTA CRÍTICO] BOIA TOMBADA! Inclinada em %.1f° por mais de 5s!\n\n", maiorInclinacao);
          alertaTombamentoAtivo = true;
        }
      }
    } else {

/////////////////////// Boia voltou à posição normal//////////////////////////////////
      if (alertaTombamentoAtivo) {
        Serial.println("[INFO] Boia retornou à posição normal de navegação.");
      }
      inicioInclinacao = 0;
      alertaTombamentoAtivo = false;
    }

///////////////////////LOG PERIÓDICO (A cada 500ms)///////////////////////////////////
    if (millis() - ultimoRelatorio >= 500) {
      ultimoRelatorio = millis();
      Serial.printf("[BOIA STATUS] Bússola: %.1f° (%s) | Pitch: %.1f° | Roll: %.1f° | Impacto: %.2f m/s²\n",
                    bussola, getDirecaoCardeal(bussola), pitch, roll, forcaImpacto);
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Executa amostragem interna a 10 Hz
  }
}

//////////////////Função de Inicialização////////////////////////////
bool iniciarMonitoramentoBoia() {
  I2C_BNO.begin(41, 42, 100000); // Pinos 41 e 42

  if (!bno.begin()) {
    Serial.println("Erro: BNO055 não encontrado!");
    return false;
  }

  bno.setExtCrystalUse(true);

  xTaskCreate(taskMonitoramentoBoia, "TaskBoia", 4096, NULL, 1, NULL);
  return true;
}

void setup() {
  
  Serial.begin(115200);
  iniciarMonitoramentoBoia();
}

void loop() {}