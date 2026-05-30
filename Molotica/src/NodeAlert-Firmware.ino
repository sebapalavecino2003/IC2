// ====================================================================
// FIRMWARE NODEALERT — Nodo sensor ESP32 con deep sleep
// ====================================================================
// Arquitectura: ciclo despertar→leer→publicar→dormir. loop() vacío.
// Los umbrales se almacenan en RTC_DATA_ATTR para sobrevivir al
// deep sleep, permitiendo reconfiguración remota sin reprovisionar.
// El buzzer mantiene su estado durante el sueño mediante gpio_hold.
// ====================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "Config.h"

WiFiClient   clienteWifi;
PubSubClient clienteMqtt(clienteWifi);
DHT          sensorDht(DHT22_PIN, DHT22);

// Variables en RTC_DATA_ATTR: el contenido del RTC persiste durante
// deep sleep. El resto de la RAM se pierde. Usamos esto para recordar:
//   - si el usuario forzó manualmente el buzzer (override manual),
//   - los umbrales actualizados por comando remoto,
//   - si es el primer arranque (para que el backend distinga fresh boot).
RTC_DATA_ATTR bool buzzerForzado   = false;
RTC_DATA_ATTR float umbralTemp     = UMBRAL_TEMP_DEFECTO;
RTC_DATA_ATTR int   umbralGas      = UMBRAL_GAS_DEFECTO;
RTC_DATA_ATTR int   umbralLlama    = UMBRAL_LLAMA_DEFECTO;
RTC_DATA_ATTR bool primerArranque  = true;

// Flag volatil para comunicacion entre ISR y setup().
// volatile es obligatorio: el compilador no debe optimizar
// esta variable porque cambia fuera del flujo normal (ISR).
volatile bool despertarPorLlama = false;

// ISR: se ejecuta en contexto de interrupcion (IRAM).
// Prohibido: Serial.print, delay, malloc, o cualquier funcion
// que no sea rapida y determinista.
void IRAM_ATTR onLlamaDetected() {
    despertarPorLlama = true;
}

void mqttCallback(char* topic, byte* payload, unsigned int len);

/**
 * Calcula la mediana de N lecturas analógicas de un pin.
 *
 * Toma 'muestras' lecturas de 'pin', las ordena y devuelve el valor
 * central (mediana). Elimina picos atípicos del ADC sin suavizar
 * la señal como haría un promedio. 5 muestras son suficientes para
 * filtrar ruido de lectura del ESP32 sin añadir latencia notable.
 *
 * @param pin      Pin GPIO analógico a leer.
 * @param muestras Cantidad de muestras a tomar (debe ser impar).
 * @return Valor mediano de las muestras.
 */
int mediana(int pin, int muestras) {
    int valores[muestras];
    for (int i = 0; i < muestras; i++) {
        valores[i] = analogRead(pin);
        delayMicroseconds(50);
    }
    for (int i = 0; i < muestras - 1; i++) {
        for (int j = i + 1; j < muestras; j++) {
            if (valores[i] > valores[j]) {
                int temp = valores[i];
                valores[i] = valores[j];
                valores[j] = temp;
            }
        }
    }
    return valores[muestras / 2];
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n[NODEALERT] Boot");

    // Configurar el temporizador que nos despertará cada INTERVALO_SUENIO_US.
    esp_sleep_enable_timer_wakeup(INTERVALO_SUENIO_US);

    // Interrupcion por flanco de bajada en KY026 digital.
    // Despierta al chip en ~1 us cuando se detecta llama,
    // sin esperar al proximo timer de 60 s.
    attachInterrupt(digitalPinToInterrupt(PIN_KY026_DIGITAL),
                    onLlamaDetected, FALLING);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_KY026_DIGITAL, LOW);

    pinMode(PIN_MQ9, INPUT);
    pinMode(PIN_KY026, INPUT);
    pinMode(PIN_KY026_DIGITAL, INPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    // gpio_hold_en: congela el nivel del pin incluso durante deep sleep.
    // Sin esto, el buzzer se apagaría al dormir aunque estuviese sonando.
    // Al despertar, si el usuario había forzado el buzzer, lo reafirmamos.
    if (buzzerForzado) {
        digitalWrite(PIN_BUZZER, HIGH);
        gpio_hold_en((gpio_num_t)PIN_BUZZER);
    }
    gpio_deep_sleep_hold_en();

    sensorDht.begin();

    // Pausa post-inicio del DHT22. El datasheet especifica >1 s antes
    // de la primera lectura fiable. Sin este delay la primera medición
    // devuelve NaN casi siempre.
    delay(1000);

    // ---------------------- CONEXIÓN WIFI ----------------------
    // Tiempo máximo de espera: 20 intentos × 500 ms = 10 s.
    // Si no se conecta en ese lapso, continuamos sin MQTT. El sensor
    // igual toma lecturas (útil para debug por serial) y duerme.
    Serial.print("[WIFI] Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int intentosWifi = 0;
    while (WiFi.status() != WL_CONNECTED && intentosWifi < 20) {
        delay(500);
        Serial.print(".");
        intentosWifi++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" OK  IP: " + WiFi.localIP().toString());
    } else {
        Serial.println(" FAIL");
    }

    // ---------------------- CONEXIÓN MQTT ----------------------
    if (WiFi.status() == WL_CONNECTED) {
        clienteMqtt.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
        clienteMqtt.setCallback(mqttCallback);

        // ID único de cliente: combinación del nombre lógico + último
        // word de la MAC. Así diferenciamos nodos iguales en el broker.
        char idCliente[32];
        uint32_t idChip = (uint32_t)(ESP.getEfuseMac() >> 32);
        snprintf(idCliente, sizeof(idCliente), "%s-%04X", ID_DISPOSITIVO, (uint16_t)idChip);

        Serial.print("[MQTT] Connecting");
        if (clienteMqtt.connect(idCliente, USUARIO_MQTT, CLAVE_MQTT)) {
            Serial.println(" OK");

            // Suscribirse al topic de comandos específico de este dispositivo.
            // El backend publica aquí para enviar órdenes individuales.
            char topicComandos[64];
            snprintf(topicComandos, sizeof(topicComandos), "%s/%s/commands",
                     PREFIJO_TOPIC_MQTT, ID_DISPOSITIVO);
            clienteMqtt.subscribe(topicComandos);
            clienteMqtt.setKeepAlive(10);
            Serial.print("[MQTT] Subscribed to ");
            Serial.println(topicComandos);

            // Bucle de procesamiento forzado: sin esto, los mensajes
            // entrantes (por ejemplo, la confirmación de suscripción)
            // pueden quedarse en el buffer y no llegar al callback antes
            // de que enviemos telemetría y nos durmamos.
            for (int i = 0; i < 50; i++) {
                clienteMqtt.loop();
                delay(50);
            }
        } else {
            Serial.println(" FAIL");
        }
    }

    // ---------------------- LECTURA DE SENSORES ----------------------
    float temperatura = sensorDht.readTemperature();
    float humedad     = sensorDht.readHumidity();

    // isNaN: el DHT22 devuelve NaN si falla la comunicación (pin suelto,
    // interferencia, tiempo insuficiente). Nunca asumir que la lectura
    // es válida sin este guarda.
    bool  dhtValido = !isnan(temperatura) && !isnan(humedad);

    // MQ9: salida analógica que aumenta con la concentración de gas LP.
    // KY026 analógico: voltaje inversamente proporcional a la intensidad
    //   de llama. KY026 digital: LOW cuando detecta llama (comparador interno).
    // Ambas lecturas analógicas pasan por un filtro de mediana (5 muestras)
    // para eliminar picos atípicos del ADC sin suavizar la señal real.
    int  lecturaGas   = mediana(PIN_MQ9, 5);
    int  lecturaLlama = mediana(PIN_KY026, 5);
    bool llamaDigital = digitalRead(PIN_KY026_DIGITAL) == LOW;

    // Si despertó por interrupción del KY026, se registra
    // para que el backend pueda distinguir wakeup por timer vs. evento.
    if (despertarPorLlama) {
        Serial.println("[WAKE] Desperto por deteccion de llama");
        despertarPorLlama = false;
    }

    // ---------------------- CONTROL AUTOMÁTICO DEL BUZZER ----------------------
    // Lógica: si el usuario NO ha hecho override manual (buzzerForzado == false)
    // y el sensor digital de llama detecta fuego, activamos la alarma.
    // Si no hay override ni llama, apagamos.
    // Si hay override manual, respetamos la decisión del usuario y no tocamos
    // el pin (el estado lo mantiene gpio_hold durante el sueño).
    if (!buzzerForzado && llamaDigital) {
        Serial.println("[AUTO] Llama digital detectada — buzzer ON");
        digitalWrite(PIN_BUZZER, HIGH);
    } else if (!buzzerForzado) {
        digitalWrite(PIN_BUZZER, LOW);
    }

    // ---------------------- CÁLCULO DE ALARMAS ----------------------
    bool alarmaTemp  = (dhtValido && temperatura > umbralTemp);
    bool alarmaGas   = (lecturaGas > umbralGas);
    bool alarmaLlama = (lecturaLlama > umbralLlama);

    // ---------------------- PUBLICACIÓN MQTT (TELEMETRÍA) ----------------------
    if (clienteMqtt.connected()) {
        // ArduinoJson 7: JsonDocument asigna automáticamente el tamaño
        // en heap. Cada publicación lleva el estado completo del nodo.
        JsonDocument doc;
        doc["device_id"]   = ID_DISPOSITIVO;
        doc["first_boot"]  = primerArranque;
        doc["timestamp"]   = millis();   // tiempo desde el reset (no RTC absoluto)
        if (dhtValido) {
            doc["temperature"]  = temperatura;
            doc["humidity"]     = humedad;
        }
        doc["gas_ppm"]        = lecturaGas;     // valor raw del ADC (0-4095)
        doc["flame"]          = lecturaLlama;   // valor raw del ADC
        doc["flame_digital"]  = llamaDigital;   // booleano: true = llama presente
        doc["buzzer_forced"]  = buzzerForzado;  // true si hay override manual activo
        doc["thresh_temp"]    = umbralTemp;
        doc["thresh_gas"]     = umbralGas;
        doc["thresh_flame"]   = umbralLlama;
        doc["alarm_temp"]     = alarmaTemp;
        doc["alarm_gas"]      = alarmaGas;
        doc["alarm_flame"]    = alarmaLlama;

        char topicTelemetria[64];
        snprintf(topicTelemetria, sizeof(topicTelemetria), "%s/%s/telemetry",
                 PREFIJO_TOPIC_MQTT, ID_DISPOSITIVO);

        // Buffer fijo de 384 bytes: suficiente para el payload actual.
        // Si se agregan campos, verificar que no exceda este límite.
        char bufferJson[384];
        serializeJson(doc, bufferJson);
        Serial.print("[MQTT] Publish: ");
        Serial.println(bufferJson);
        clienteMqtt.publish(topicTelemetria, bufferJson, false);
        delay(100);
        clienteMqtt.loop();
        clienteMqtt.disconnect();
    } else {
        Serial.println("[MQTT] Skipped (not connected)");
    }

    // primerArranque solo es true en el primer ciclo de vida del nodo.
    // Tras la primera publicación queda en false permanentemente
    // (a menos que se borre la memoria RTC con un reset completo).
    primerArranque = false;

    Serial.printf("[SLEEP] %llu seconds\n", INTERVALO_SUENIO_US / 1000000ULL);
    Serial.flush();

    // Deep sleep: consumo ~5 µA. El RTC mantiene las variables
    // RTC_DATA_ATTR y el temporizador de wakeup.
    esp_deep_sleep_start();
}

// loop() nunca se ejecuta: setup() termina en deep sleep.
// La función existe por exigencia del framework Arduino.
void loop() {}

// ====================================================================
// CALLBACK MQTT: procesa comandos entrantes ANTES de publicar telemetría
// ====================================================================
// Los comandos llegan como JSON en el topic suscrito. El firmware
// admite dos alias para el campo de acción: "cmd" y "command",
// por compatibilidad con distintos formatos del backend.
// ====================================================================
void mqttCallback(char* topic, byte* payload, unsigned int len) {
    // Copia segura con límite para evitar desbordamiento de pila.
    // len viene del broker MQTT y podría ser teóricamente grande,
    // aunque el backend nunca enviará más de ~200 bytes.
    char bufferDatos[256] = {0};
    unsigned int tamDatos = min(len, (unsigned int)sizeof(bufferDatos) - 1);
    memcpy(bufferDatos, payload, tamDatos);
    bufferDatos[tamDatos] = '\0';

    Serial.print("[CMD] ");
    Serial.println(bufferDatos);

    JsonDocument doc;
    if (deserializeJson(doc, bufferDatos) != DeserializationError::Ok) return;

    // "cmd" es el campo canónico; "command" se acepta como alternativa
    // para compatibilidad con versiones anteriores del backend.
    const char* accion = doc["cmd"] | doc["command"] | "";

    if (strcmp(accion, "reboot") == 0) {
        // Reset completo del microcontrolador.
        // Útil si el nodo se comporta erráticamente.
        Serial.println("[CMD] Rebooting...");
        ESP.restart();
    }

    else if (strcmp(accion, "status") == 0) {
        // Health-check mínimo. El backend usa este comando para
        // verificar que el nodo responde.
        Serial.println("[CMD] Status OK");
    }

    else if (strcmp(accion, "buzzer_on") == 0) {
        // Override manual: activa el buzzer y lo mantiene encendido
        // incluso tras deep sleep (gpio_hold_en). El control automático
        // queda deshabilitado hasta que se envíe return_to_auto.
        Serial.println("[CMD] Buzzer ON (override manual)");
        buzzerForzado = true;
        digitalWrite(PIN_BUZZER, HIGH);
        gpio_hold_en((gpio_num_t)PIN_BUZZER);
    }

    else if (strcmp(accion, "buzzer_off") == 0) {
        // Override manual: apaga el buzzer y lo mantiene apagado
        // (congela LOW con gpio_hold). El modo automático también
        // queda suspendido.
        Serial.println("[CMD] Buzzer OFF (override manual)");
        buzzerForzado = true;
        digitalWrite(PIN_BUZZER, LOW);
        gpio_hold_en((gpio_num_t)PIN_BUZZER);
    }

    else if (strcmp(accion, "return_to_auto") == 0) {
        // Restaura el control automático: el firmware decide el estado
        // del buzzer según la lectura del sensor de llama en cada ciclo.
        Serial.println("[CMD] Return to auto mode");
        buzzerForzado = false;
        digitalWrite(PIN_BUZZER, LOW);
        gpio_hold_dis((gpio_num_t)PIN_BUZZER);
    }

    else if (strcmp(accion, "acknowledge_alarm") == 0) {
        // El usuario reconoció la alarma (ej. desde la app móvil).
        // Silencia el buzzer y devuelve el control al modo automático.
        Serial.println("[CMD] Alarm acknowledged");
        buzzerForzado = false;
        digitalWrite(PIN_BUZZER, LOW);
        gpio_hold_dis((gpio_num_t)PIN_BUZZER);
    }

    else if (strcmp(accion, "update_thresholds") == 0) {
        // Actualización remota de umbrales. Los valores se escriben
        // directamente en las variables RTC_DATA_ATTR, por lo que
        // persisten indefinidamente entre ciclos de deep sleep.
        Serial.println("[CMD] Update thresholds");
        JsonObject parametros = doc["params"];
        if (!parametros.isNull()) {
            if (parametros["temp_max"].is<float>())  umbralTemp  = parametros["temp_max"];
            if (parametros["gas_max"].is<int>())     umbralGas   = parametros["gas_max"];
            if (parametros["flame_max"].is<int>())   umbralLlama = parametros["flame_max"];
        }
        Serial.printf("  temp_max=%.1f  gas_max=%d  flame_max=%d\n",
                      umbralTemp, umbralGas, umbralLlama);
    }
}
