#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>

const char* ssid = "Personal-401 2.4GHz";
const char* password = "D454C15401";
const char* serverUrl = "http://192.168.0.29/guardar_datos.php";

const int sensorPin = A0; // Pin analógico para el sensor de humedad
const int relayPin = D1; // Pin digital para el relay (controla la bomba)
const int buzzerPin = D2; // Pin digital para el buzzer

const int humidityThreshold = 800; // Valor umbral para activar la bomba
const int pumpDuration = 5000; // Duración en ms para la bomba

const int ledPin = D4; // El pin del LED integrado

WiFiClient wifiClient;

void setup() {
  Serial.begin(115200);
    pinMode(ledPin, OUTPUT); // Configura el pin del LED como salida
  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Conectar a la red WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conectando a WiFi...");
    delay(10000);
  }

  Serial.println("Conectado a WiFi");
  Serial.print("IP del ESP: ");
  Serial.println(WiFi.localIP());

  // Configuración de OTA
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Inicio de actualización: " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nFin de la actualización");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progreso: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Error de autenticación");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Error al comenzar");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Error de conexión");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Error de recepción");
    } else if (error == OTA_END_ERROR) {
      Serial.println("Error al finalizar");
    }
  });

  ArduinoOTA.begin();
  Serial.println("Listo para OTA");
}

void loop() {
  Serial.print("IP del ESP: ");
  Serial.println(WiFi.localIP());
  
  digitalWrite(ledPin, HIGH); // Enciende el LED
  delay(500);                // Espera 500 ms
  digitalWrite(ledPin, LOW);  // Apaga el LED
  delay(500);                // Espera 500 ms

  // Ejecutar OTA
  ArduinoOTA.handle();

  // Leer sensor de humedad
  int humedad = analogRead(sensorPin);
  Serial.print("Humedad: ");
  Serial.println(humedad);

  // Controlar la bomba y el buzzer
  if (humedad > humidityThreshold) {
    digitalWrite(relayPin, HIGH);
    tone(buzzerPin, 1000);
    delay(pumpDuration);
    digitalWrite(relayPin, LOW);
    noTone(buzzerPin);
    delay(8000); // Espera después de activar la bomba
  } else {
    delay(5000); // Espera si la humedad está por debajo del umbral
  }

  // Enviar datos al servidor si está conectado a WiFi
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(wifiClient, serverUrl);
    
    Serial.print("Intentando conectar a: ");
    Serial.println(serverUrl);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "humedad=" + String(humedad) + "&voltaje=0"; // Actualiza el voltaje si es necesario

    int httpResponseCode = http.POST(postData);
    Serial.print("Código de respuesta HTTP: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Respuesta del servidor:");
        Serial.println(response);
    } else {
        Serial.print("Error en el envío de datos. Código de error: ");
        Serial.println(httpResponseCode);
        Serial.print("Descripción del error: ");
        Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("No conectado a WiFi");
  }

  delay(20000); // Espera antes de la próxima lectura
}
