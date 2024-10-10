//ESP32 things
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <FirebaseESP32.h> // Incluir la biblioteca Firebase

// Credenciales de WiFi
const char* ssid = "Wifi";
const char* password = "Contraseña";
int port = 80;

// Configuración de Firebase
FirebaseData firebaseData; // Crear una instancia de FirebaseData
FirebaseConfig config; // Instancia de configuración
FirebaseAuth auth; // Instancia para autenticación

const char* FIREBASE_HOST = "Host"; // Reemplaza con tu URL sin "https://"
const char* FIREBASE_AUTH = "Apikey"; // Reemplaza con tu clave de base de datos

WebServer server(port);

const int led = LED_BUILTIN;

// DHT11 things
#include "DHT.h"
#define DHTPIN 22
#define DHTTYPE DHT11
#define MOISTURE_PIN 32  // Pin para la lectura de humedad del suelo

DHT dht(DHTPIN, DHTTYPE);

// Otras variables
float asoilmoist = analogRead(MOISTURE_PIN); // Variable global para almacenar la lectura inicial de la humedad del suelo

void handleRoot() {
  Serial.println("Handling root request");
  digitalWrite(led, 1);
  String webtext;

  // Leer humedad y temperatura del sensor DHT11
  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  // Verificar si las lecturas del sensor fallaron
  if (isnan(hum) || isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Enviar los datos a Firebase usando los métodos adecuados
  if (Firebase.setFloat(firebaseData, "/soil_moisture", asoilmoist) && 
      Firebase.setFloat(firebaseData, "/temperature", temp) && 
      Firebase.setFloat(firebaseData, "/humidity", hum)) {
    Serial.println("Datos enviados a Firebase correctamente");
  } else {
    Serial.print("Error al enviar datos a Firebase: ");
    Serial.println(firebaseData.errorReason());
  }

  // Crear la respuesta HTML con los datos leídos
  webtext = "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>WEMOS HIGROW ESP32 WIFI SOIL MOISTURE SENSOR</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>WEMOS HIGROW ESP32 WIFI SOIL MOISTURE SENSOR</h1>\
    <br>\
    <p>For soil moist, high values (range of +/-3344) means dry soil, lower values (+/- 2000) means wet soil. The Soil Moist Reading is influenced by the volumetric soil moisture content and electrical capacitive properties of the soil.</p>\
    <br>\
    <p>Soil Moisture: " + String(asoilmoist) + "</p>\
    <p>Temperature: " + String(temp) + " &#176;C</p>\
    <p>Humidity: " + String(hum) + " %</p>\
  </body>\
</html>";

  // Enviar respuesta HTTP
  server.send(200, "text/html", webtext);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
  delay(1000);
  digitalWrite(led, 1);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Esperar a la conexión WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Configuración de Firebase
  config.database_url = FIREBASE_HOST; // URL de Firebase sin https://
  config.signer.tokens.legacy_token = FIREBASE_AUTH; // Clave de la base de datos
  
  // Inicializar Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true); // Reconectar automáticamente si se pierde la conexión WiFi

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  // Configurar rutas en el servidor web
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  dht.begin();
  delay(2000);
}

void loop(void) {
  // Suavizado exponencial de la lectura de humedad del suelo
  asoilmoist = 0.95 * asoilmoist + 0.05 * analogRead(MOISTURE_PIN);
  server.handleClient(); // Manejar las peticiones del servidor web
}
