#include <WiFi.h>
#include "DHT.h"

// WiFi credentials
const char* ssid = "Makers Asylum";
const char* password = "Makeithappen";

// Sensor pins
const int soilPin = 32;
const int phPin = 35;
const int gasPin = 25;

// DHT22 setup
#define DHTPIN 33
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Create web server on port 80
WiFiServer server(80);

// HTML and assets
const char* indexHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>BioBarrel</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <h1>Welcome to BioBarrel</h1>
  <a href="/readings.html" class="button">Sensor Readings</a>
  <a href="/suggestions.html" class="button">Suggestions</a>
  <script src="/script.js"></script>
</body>
</html>
)rawliteral";

const char* style = R"rawliteral(
body { font-family: Arial, sans-serif; background: #f8f8f8; }
.button { display: inline-block; padding: 10px; background: #007bff; color: white; text-decoration: none; margin: 5px; }
table { border-collapse: collapse; width: 80%; margin: 20px auto; }
th, td { border: 1px solid #444; padding: 8px; text-align: center; }
th { background-color: #f2f2f2; }
)rawliteral";

const char* script = R"rawliteral(
console.log('Hello from ESP32 script.js!');
)rawliteral";

const char* readingsHtml = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Compost Sensor Readings</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <h1>Compost Sensor Readings</h1>
  <table>
    <tr>
      <th>Sensor</th>
      <th>Readings</th>
    </tr>
    <tr>
      <td>Soil Moisture</td>
      <td id="soil">Loading...</td>
    </tr>
    <tr>
      <td>pH</td>
      <td id="ph">Loading...</td>
    </tr>
    <tr>
      <td>Gas Index</td>
      <td id="gas">Loading...</td>
    </tr>
    <tr>
      <td>Temperature</td>
      <td id="temperature">Loading...</td>
    </tr>
  </table>
  <a href="/">Back</a>
  <script>
    function updateReadings() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          document.getElementById('soil').textContent = data.soil.toFixed(1) + "%";
          document.getElementById('ph').textContent = data.ph.toFixed(1);
          document.getElementById('gas').textContent = data.gas.toFixed(1) + "%";
          document.getElementById('temperature').textContent = data.temperature.toFixed(1) + " °C";
        })
        .catch(error => console.error('Error fetching data:', error));
    }
    updateReadings();
    setInterval(updateReadings, 2000);
  </script>
</body>
</html>
)rawliteral";

String generateSuggestionsPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Composting Suggestions</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #f0f0f0; padding: 20px; }
    h1 { color: #2c3e50; }
    h2 { color: #007bff; margin-top: 20px; }
    p, li { color: #333; line-height: 1.5; }
    ul { margin-top: 5px; }
    a.button { display: inline-block; margin-top: 20px; padding: 10px 15px; background: #007bff; color: #fff; text-decoration: none; border-radius: 4px; }
  </style>
</head>
<body>
  <h1>Personalized Composting Suggestions</h1>
)rawliteral";

  // Simulated sensor readings
  float sensorPH = 8.5;
  float sensorTemp = 25.0;
  float sensorMoisture = 45.0;
  float airQuality = 60.0;

  // pH suggestions
  if (!(sensorPH >= 5.5 && sensorPH <= 8)) {
    html += "<h2>pH</h2><ul>";
    if (sensorPH > 8) {
      html += "<li><strong>Too High:</strong> Add pine needles, coffee grounds or oak leaves</li>";
    } else {
      html += "<li><strong>Too Low:</strong> Add a layer of wood ash, limestone or baking soda</li>";
    }
    html += "</ul>";
  }

  // Temperature suggestions
  if (!(sensorTemp >= 30 && sensorTemp <= 60)) {
    html += "<h2>Temperature</h2><ul>";
    if (sensorTemp > 60) {
      html += "<li><strong>Too High:</strong> Air out the pile and mix it more often</li>";
    } else {
      html += "<li><strong>Too Low:</strong> Add more nitrogen-rich materials (grass clippings, coffee grounds)</li>";
    }
    html += "</ul>";
  }

  // Soil Moisture suggestions
  if (!(sensorMoisture >= 55 && sensorMoisture <= 60)) {
    html += "<h2>Soil Moisture</h2><ul>";
    if (sensorMoisture > 60) {
      html += "<li><strong>Too Wet:</strong> Add dry, carbon-rich materials (cardboard, newspaper, sawdust)</li>";
    } else {
      html += "<li><strong>Too Dry:</strong> Add water evenly until moist but not soggy and use more nitrogen-rich materials (vegetable scraps, grass clippings, coffee grounds)</li>";
    }
    html += "</ul>";
  }

  // Air Quality suggestions
  if (airQuality < 50) {
    html += "<h2>Air Quality</h2><ul>";
    html += "<li><strong>Too Low:</strong> Ensure a balance of browns and greens</li>";
    html += "</ul>";
  }

  html += R"rawliteral(
  <a class="button" href="/">Back to Home</a>
</body>
</html>
)rawliteral";

  return html;
}

// Helper to send HTTP response
void sendResponse(WiFiClient& client, const char* status, const char* contentType, const char* content) {
  client.println(String("HTTP/1.1 ") + status);
  client.println(String("Content-Type: ") + contentType);
  client.println("Connection: close");
  client.println();
  client.println(content);
}

// Get JSON string with sensor data
String getSensorJson() {
  float soil = 100.0 - (analogRead(soilPin) / 4095.0 * 100.0);
  soil = constrain(soil, 0.0, 100.0);

  float ph = 14.0 - (analogRead(phPin) / 4095.0 * 14.0);
  ph = constrain(ph, 0.0, 14.0);

  float gas = analogRead(gasPin) / 4095.0 * 100.0;
  gas = constrain(gas, 0.0, 100.0);

  float temp = dht.readTemperature();
  if (isnan(temp)) {
    temp = -999;
  }

  char json[200];
  snprintf(json, sizeof(json),
    "{\"soil\": %.1f, \"ph\": %.1f, \"gas\": %.1f, \"temperature\": %.1f}",
    soil, ph, gas, temp
  );

  return String(json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();

  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String reqLine = "";
    bool gotFirstLine = false;
    String path = "/";

    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();

        if (!gotFirstLine && line.length() > 0) {
          reqLine = line;
          gotFirstLine = true;

          int idx1 = reqLine.indexOf(' ');
          int idx2 = reqLine.indexOf(' ', idx1 + 1);
          if (idx1 != -1 && idx2 != -1) {
            path = reqLine.substring(idx1 + 1, idx2);
          }
        }

        if (line.length() == 0) break;
      }
    }

    if (path == "/favicon.ico") {
      client.println("HTTP/1.1 204 No Content");
      client.println("Connection: close");
      client.println();
      client.stop();
      return;
    }

    if (path == "/") {
      sendResponse(client, "200 OK", "text/html", indexHtml);
    } else if (path == "/style.css") {
      sendResponse(client, "200 OK", "text/css", style);
    } else if (path == "/script.js") {
      sendResponse(client, "200 OK", "application/javascript", script);
    } else if (path == "/readings.html") {
      sendResponse(client, "200 OK", "text/html", readingsHtml);
    } else if (path == "/suggestions.html") {
      String page = generateSuggestionsPage();
      sendResponse(client, "200 OK", "text/html", page.c_str());
    } else if (path == "/data") {
      String json = getSensorJson();
      sendResponse(client, "200 OK", "application/json", json.c_str());
    } else {
      sendResponse(client, "404 Not Found", "text/html", "<h1>404 - Not Found</h1>");
    }

    delay(1);
    client.stop();
  }
}
