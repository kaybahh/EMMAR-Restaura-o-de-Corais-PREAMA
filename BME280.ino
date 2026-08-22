#include <Wire.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

//////////// Função BME280 ////////////////////////////////////////////////
void taskBME280(void *parameter) {
  while (true) {
    float temp = bme.readTemperature(); //le temperatura
    float pres = bme.readPressure() / 100.0F; //le pressão
    float umid = bme.readHumidity(); //le humildade

    //mostra os valores lidos em uma unica linha
    Serial.printf("[BME280] Temp: %.2f °C | Pressão: %.2f hPa | Umidade: %.2f %%\n", temp, pres, umid);

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// chamar a função "iniciarBME280()" no void setup pra iniciar a leitura do BME
bool iniciarBME280() {
  Wire.begin(20, 19); //SDA = 20, SCL = 19

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
  iniciarBME280();

}

void loop(){

}