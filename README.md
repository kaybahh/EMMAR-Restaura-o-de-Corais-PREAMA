# EMMAR-Restaura-o-de-Corais-PREAMA

🌊 Boia de Monitoramento Oceânico (Heltec WiFi LoRa32 V3) 

Sistema embedded de telemetria e segurança marítima para monitoramento de condições ambientais e
dinâmica de movimento em alto-mar, desenvolvido sobre o microcontrolador Heltec WiFi LoRa32 V3

🛠️ Metodologia de Desenvolvimento

Desenvolvimento Modular: Cada sensor possui seu próprio código estruturado em funções reutilizáveis e executado via tarefas independentes do FreeRTOS.
Integração Futura: Após os testes individuais de bancada, todos os módulos serão unificados no projeto principal utilizando a estrutura de abas (.ino) da IDE do Arduino.

📂 Estrutura do Projeto (o que temos ate agora)

BME280 — Módulo sensor de pressão, temperatura e umidade.

MCU055 — Módulo de giroscópio, bússola e algoritmo de alertas.
