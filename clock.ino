Hey everyone! ?? Thanks for asking for the source code for the **ESP8266 Wi-Fi NTP LED Clock** project. 

Here is the complete Arduino code. Make sure you have the **ESP8266** board package and the **FastLED** library installed in your Arduino IDE before uploading!

### ?? Arduino Source Code

```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <FastLED.h>

// Wi-Fi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Timezone Configuration (Indian Standard Time UTC+5:30)
#define MY_NTP_SERVER "pool.ntp.org"
#define MY_TZ "IST-5:30"

// Hardware Configuration
#define DATA_PIN     D4    // GPIO2 on Wemos D1 Mini
#define NUM_LEDS     72
#define COLOR_ORDER  GRB

CRGB leds[NUM_LEDS];
ESP8266WebServer server(80);

// Default Colors & Brightness
uint8_t masterBrightness = 64;
CRGB colorHours   = CRGB(255, 0, 0);    // Red
CRGB colorColon   = CRGB(255, 255, 255); // White
CRGB colorMinutes = CRGB(0, 255, 0);    // Green
CRGB colorSeconds = CRGB(0, 0, 255);    // Blue

// 14-bit direct LED mapping (2 bits per segment, Seg 0..6)
const uint16_t DIGIT_MAP_14BIT[10] = {
  0b0000111111111111, // 0
  0b0000000011110000, // 1
  0b0011111100111100, // 2
  0b0011001111111100, // 3
  0b0011000011110011, // 4
  0b0011001111001111, // 5
  0b0011111111001111, // 6
  0b0000000011111100, // 7
  0b0011111111111111, // 8
  0b0011001111111111  // 9
};

void drawDigit14(int digitOffset, int number, CRGB color) {
  uint16_t mask = DIGIT_MAP_14BIT[number];
  for (int ledIndex = 0; ledIndex < 14; ledIndex++) {
    if (mask & (1 << ledIndex)) {
      leds[digitOffset + ledIndex] = color;
    }
  }
}

// Convert Hex String "#RRGGBB" to CRGB
CRGB hexToCRGB(String hex) {
  if (hex.startsWith("#")) hex = hex.substring(1);
  long num = strtol(hex.c_str(), NULL, 16);
  return CRGB((num >> 16) & 0xFF, (num >> 8) & 0xFF, num & 0xFF);
}

// Convert CRGB to Hex String "#RRGGBB" for HTML inputs
String crgbToHex(CRGB c) {
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
  return String(buf);
}

// HTML Web Page
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Project HHMM Controller</title>";
  html += "<style>";
  html += "body{font-family:sans-serif;background:#121212;color:#eee;text-align:center;padding:20px;margin:0;}";
  html += ".card{background:#1e1e1e;max-width:380px;margin:0 auto;padding:24px;border-radius:16px;box-shadow:0 8px 24px rgba(0,0,0,0.6);}";
  html += "h2{margin-top:0;color:#00e5ff;letter-spacing:1px;}";
  html += ".field{display:flex;justify-content:space-between;align-items:center;margin:16px 0;font-size:16px;}";
  html += "input[type='color']{border:none;width:48px;height:36px;border-radius:6px;cursor:pointer;background:none;}";
  html += "input[type='range']{width:100%;margin-top:8px;}";
  html += "button{background:#00e5ff;color:#000;border:none;padding:12px 24px;font-size:16px;font-weight:bold;border-radius:8px;cursor:pointer;width:100%;margin-top:16px;}";
  html += "</style></head><body>";
  html += "<div class='card'>";
  html += "<h2>PROJECT HHMM</h2>";
  html += "<form action='/set' method='GET'>";
  
  html += "<div class='field'><label>Hours</label><input type='color' name='ch' value='" + crgbToHex(colorHours) + "'></div>";
  html += "<div class='field'><label>Colon</label><input type='color' name='cc' value='" + crgbToHex(colorColon) + "'></div>";
  html += "<div class='field'><label>Minutes</label><input type='color' name='cm' value='" + crgbToHex(colorMinutes) + "'></div>";
  html += "<div class='field'><label>Seconds Ring</label><input type='color' name='cs' value='" + crgbToHex(colorSeconds) + "'></div>";
  
  html += "<div style='text-align:left;margin-top:20px;'>";
  html += "<label>Brightness: <span id='bval'>" + String(masterBrightness) + "</span></label>";
  html += "<input type='range' name='br' min='10' max='255' value='" + String(masterBrightness) + "' oninput='document.getElementById(\"bval\").innerText=this.value'>";
  html += "</div>";

  html += "<button type='submit'>Apply Settings</button>";
  html += "</form></div></body></html>";

  server.send(200, "text/html", html);
}

// Endpoint to update colors and brightness
void handleSet() {
  if (server.hasArg("ch")) colorHours   = hexToCRGB(server.arg("ch"));
  if (server.hasArg("cc")) colorColon   = hexToCRGB(server.arg("cc"));
  if (server.hasArg("cm")) colorMinutes = hexToCRGB(server.arg("cm"));
  if (server.hasArg("cs")) colorSeconds = hexToCRGB(server.arg("cs"));
  if (server.hasArg("br")) {
    masterBrightness = server.arg("br").toInt();
    FastLED.setBrightness(masterBrightness);
  }

  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  FastLED.addLeds<WS2812B, COLOR_ORDER DATA_PIN,>(leds, NUM_LEDS);
  FastLED.setBrightness(masterBrightness);
  FastLED.clear();
  FastLED.show();

  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
    leds[0] = (wifiAttempts % 2 == 0) ? CRGB(50, 25, 0) : CRGB::Black;
    FastLED.show();
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("Access Web UI at: http://");
  Serial.println(WiFi.localIP());

  configTime(0, 0, MY_NTP_SERVER, "time.google.com", "time.windows.com");
  setenv("TZ", MY_TZ, 1);
  tzset();

  Serial.print("Syncing NTP Time");
  time_t now = time(nullptr);
  int ntpAttempts = 0;
  while (now < 1700000000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    ntpAttempts++;
    FastLED.clear();
    leds[60 + (ntpAttempts % 12)] = CRGB::Cyan;
    FastLED.show();
  }

  Serial.println("\nTime synced successfully!");
  FastLED.clear();
  FastLED.show();

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
  server.handleClient();

  static unsigned long lastClockUpdate = 0;
  if (millis() - lastClockUpdate >= 100) {
    lastClockUpdate = millis();

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    int hours   = timeinfo->tm_hour;
    int minutes = timeinfo->tm_min;
    int seconds = timeinfo->tm_sec;

    if (hours == 0) {
      hours = 12;
    } else if (hours > 12) {
      hours -= 12;
    }

    int tensHour  = hours / 10;
    int unitsHour = hours % 10;
    int tensMin   = minutes / 10;
    int unitsMin  = minutes % 10;

    FastLED.clear();

    if (tensHour > 0) {
      drawDigit14(0, tensHour, colorHours);
    }
    drawDigit14(14, unitsHour, colorHours);

    if (seconds % 2 == 0) {
      leds[28] = colorColon;
      leds[29] = colorColon;
      leds[30] = colorColon;
      leds[31] = colorColon;
    }

    drawDigit14(32, tensMin, colorMinutes);
    drawDigit14(46, unitsMin, colorMinutes);

    int activeSecondsLED = seconds / 5;
    for (int i = 0; i <= activeSecondsLED; i++) {
      leds[60 + i] = colorSeconds;
    }

    FastLED.show();
  }
}