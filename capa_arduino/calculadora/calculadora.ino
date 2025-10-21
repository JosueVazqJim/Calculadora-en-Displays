/*
  esp32_display_server.ino
  Servidor HTTP para controlar dos displays 7-seg (cátodo común) vía endpoints:
    GET /display/{valor}        -> valor: "--" o "00".."99" o single digit -> muestra en display
    GET /conteo/up/{valor}      -> equivalente a set display
    GET /conteo/down/{valor}    -> equivalente a set display
    GET /conteo/stop            -> placeholder
    GET /conteo/restart         -> set display a 00

  Configuración:
    - Pines de segmentos (a..g) según tu conexión
    - Pines de dígitos (opcional). Si los dejas en -1, el sketch intentará
      mostrar ambos dígitos sin multiplexado (si están cableados en paralelo
      ambos mostrarán el mismo patrón). Para mostrar números distintos se
      requiere multiplexado y por tanto los pines DIGIT1_PIN/DIGIT2_PIN.

  Nota: Asumo displays catodo común. Para habilitar una cifra con multiplexado
  colocamos su catodo a LOW (activar), y al desactivarla ponemos HIGH.
*/

#include <WiFi.h>
#include <WebServer.h>

// ------ CONFIG ------
const char* ssid = "BUAP_Trabajadores";
const char* password = "BuaPW0rk.2017";

// Segment pins (seg a..g)
const int PIN_A = 21; // a -> GPIO21
const int PIN_B = 19; // b -> GPIO19
const int PIN_C = 18; // c -> GPIO18
const int PIN_D = 5;  // d -> GPIO5
const int PIN_E = 4;  // e -> GPIO4
const int PIN_F = 2;  // f -> GPIO2
const int PIN_G = 15; // g -> GPIO15

// Digit common pins (cátodo). Si no los tienes, déjalos en -1.
// Para multiplexado pon aquí los pines que controlan cada cátodo.
// En tu sketch de prueba usabas 22 y 23 para COM1/COM2 (decenas/unidades).
const int DIGIT1_PIN = 22; // Display decenas (COM1)
const int DIGIT2_PIN = 23; // Display unidades (COM2)

// Multiplex interval (ms)
const unsigned long MULTIPLEX_MS = 5;

// If your 7-seg displays are common anode set this to true. Default false => common cathode.
const bool COMMON_ANODE = false;

WebServer server(80);

// Helper: log details about the incoming HTTP request to Serial
void logRequest() {
  // Method
  Serial.print("HTTP Method: ");
  switch (server.method()) {
    case HTTP_GET: Serial.println("GET"); break;
    case HTTP_POST: Serial.println("POST"); break;
    case HTTP_PUT: Serial.println("PUT"); break;
    case HTTP_DELETE: Serial.println("DELETE"); break;
    case HTTP_PATCH: Serial.println("PATCH"); break;
    case HTTP_HEAD: Serial.println("HEAD"); break;
    default: Serial.println("OTHER"); break;
  }

  // URI and client
  Serial.print("URI: "); Serial.println(server.uri());
  IPAddress rip = server.client().remoteIP();
  Serial.print("From: "); Serial.print(rip); Serial.print(":"); Serial.println(server.client().remotePort());

  // Query args
  int argcnt = server.args();
  Serial.print("Args count: "); Serial.println(argcnt);
  for (int i = 0; i < argcnt; ++i) {
    Serial.print("  "); Serial.print(server.argName(i)); Serial.print(" = "); Serial.println(server.arg(i));
  }

  // Raw body payload (if any)
  if (server.hasArg("plain")) {
    Serial.print("Body: "); Serial.println(server.arg("plain"));
  }
  Serial.println("---");
}

// Estado a mostrar (dos caracteres)
char showLeft = '0';
char showRight = '0';

unsigned long lastMuxMs = 0;
int muxState = 0; // 0 -> left, 1 -> right

// Counting (conteo) state
bool countingActive = false;
int countDir = 1; // 1 up, -1 down
int currentCount = 0; // 0..99
unsigned long lastCountMs = 0;
const unsigned long COUNT_INTERVAL_MS = 1000; // ms between increments

// Segment bit order: a b c d e f g  (bit0 = a ... bit6 = g)
const uint8_t DIGIT_PATTERNS[11] = {
  // 0-9
  0b0111111, // 0 -> segments a b c d e f
  0b0000110, // 1 -> b c
  0b1011011, // 2 -> a b d e g
  0b1001111, // 3 -> a b c d g
  0b1100110, // 4 -> f g b c
  0b1101101, // 5 -> a f g c d
  0b1111101, // 6 -> a f e d c g
  0b0000111, // 7 -> a b c
  0b1111111, // 8 -> all
  0b1101111, // 9 -> a b c d f g
  // index 10 -> dash
};
const uint8_t DASH_PATTERN = 0b1000000; // g segment only

void setSegmentsByMask(uint8_t mask) {
  // For common-cathode: HIGH turns segment ON. For common-anode: LOW turns ON.
  digitalWrite(PIN_A, (mask & (1 << 0)) ? (COMMON_ANODE ? LOW : HIGH) : (COMMON_ANODE ? HIGH : LOW));
  digitalWrite(PIN_B, (mask & (1 << 1)) ? (COMMON_ANODE ? LOW : HIGH) : (COMMON_ANODE ? HIGH : LOW));
  digitalWrite(PIN_C, (mask & (1 << 2)) ? (COMMON_ANODE ? LOW : HIGH) : (COMMON_ANODE ? HIGH : LOW));
  digitalWrite(PIN_D, (mask & (1 << 3)) ? (COMMON_ANODE ? LOW : HIGH) : (COMMON_ANODE ? HIGH : LOW));
  digitalWrite(PIN_E, (mask & (1 << 4)) ? (COMMON_ANODE ? LOW : HIGH) : (COMMON_ANODE ? HIGH : LOW));
  digitalWrite(PIN_F, (mask & (1 << 5)) ? (COMMON_ANODE ? LOW : HIGH) : (COMMON_ANODE ? HIGH : LOW));
  digitalWrite(PIN_G, (mask & (1 << 6)) ? (COMMON_ANODE ? LOW : HIGH) : (COMMON_ANODE ? HIGH : LOW));
}

uint8_t charToPattern(char c) {
  if (c >= '0' && c <= '9') {
    return DIGIT_PATTERNS[c - '0'];
  }
  if (c == '-') return DASH_PATTERN;
  // unsupported -> blank (all off)
  return 0;
}

void showBothSame(char left, char right) {
  // If DIGIT pins are not provided we attempt to show same segments for both
  // displays by OR-ing the patterns. If they are different, result will be garbled.
  uint8_t m = charToPattern(left) | charToPattern(right);
  setSegmentsByMask(m);
}

void activateDigit(int digitPin) {
  if (digitPin < 0) return;
  // For common-cathode a LOW on the cathode enables the digit.
  // For common-anode a HIGH on the anode enables the digit.
  digitalWrite(digitPin, COMMON_ANODE ? HIGH : LOW);
}
void deactivateDigit(int digitPin) {
  if (digitPin < 0) return;
  digitalWrite(digitPin, COMMON_ANODE ? LOW : HIGH);
}

void updateMultiplex() {
  if (DIGIT1_PIN < 0 || DIGIT2_PIN < 0) {
    // No digit control pins: set union of both digits
    showBothSame(showLeft, showRight);
    return;
  }

  unsigned long now = millis();
  if (now - lastMuxMs < MULTIPLEX_MS) return;
  lastMuxMs = now;

  if (muxState == 0) {
    // show left
    deactivateDigit(DIGIT2_PIN);
    setSegmentsByMask(charToPattern(showLeft));
    activateDigit(DIGIT1_PIN);
    muxState = 1;
  } else {
    deactivateDigit(DIGIT1_PIN);
    setSegmentsByMask(charToPattern(showRight));
    activateDigit(DIGIT2_PIN);
    muxState = 0;
  }
}

void handleDisplaySet(String v) {
  // v can be "--" or numeric like "7","42","07","100" etc.
  if (v == "--") {
    showLeft = '-';
    showRight = '-';
    server.send(200, "text/plain", "OK");
    return;
  }
  // try numeric
  bool neg = false;
  if (v.length() > 0 && v.charAt(0) == '-') {
    neg = true;
    v = v.substring(1);
  }
  // if numeric value larger than 99 -> show -- as spec
  long val = 0;
  bool isn = true;
  for (size_t i = 0; i < v.length(); ++i) {
    if (!isDigit(v.charAt(i))) { isn = false; break; }
    val = val * 10 + (v.charAt(i) - '0');
    if (val > 9999) break; // cap
  }
  if (!isn) {
    server.send(400, "text/plain", "Bad value");
    return;
  }
  if (val > 99) {
    showLeft = '-'; showRight = '-';
    server.send(200, "text/plain", "OK");
    return;
  }
  int ival = (int)val;
  int tens = ival / 10;
  int ones = ival % 10;
  showLeft = '0' + tens;
  showRight = '0' + ones;
  server.send(200, "text/plain", "OK");
}

void updateShowFromCurrentCount() {
  if (currentCount < 0) currentCount = 0;
  if (currentCount > 99) currentCount = 99;
  int tens = currentCount / 10;
  int ones = currentCount % 10;
  showLeft = '0' + tens;
  showRight = '0' + ones;
}

void updateCounting() {
  if (!countingActive) return;
  unsigned long now = millis();
  if (now - lastCountMs < COUNT_INTERVAL_MS) return;
  lastCountMs = now;
  currentCount += countDir;
  if (currentCount > 99) currentCount = 0;
  if (currentCount < 0) currentCount = 99;
  updateShowFromCurrentCount();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, OUTPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);
  pinMode(PIN_G, OUTPUT);

  if (DIGIT1_PIN >= 0) { pinMode(DIGIT1_PIN, OUTPUT); digitalWrite(DIGIT1_PIN, HIGH); }
  if (DIGIT2_PIN >= 0) { pinMode(DIGIT2_PIN, OUTPUT); digitalWrite(DIGIT2_PIN, HIGH); }

  // Inicialmente mostrar 00
  showLeft = '0'; showRight = '0';

  // Quick hardware test: briefly turn all segments on to validate wiring
  Serial.println("Hardware test: encendiendo todos los segmentos por 200ms");
  setSegmentsByMask(0b1111111);
  // If using digit pins, enable both digits briefly so segments can be seen
  if (DIGIT1_PIN >= 0 || DIGIT2_PIN >= 0) {
    if (DIGIT1_PIN >= 0) activateDigit(DIGIT1_PIN);
    if (DIGIT2_PIN >= 0) activateDigit(DIGIT2_PIN);
    delay(200);
    if (DIGIT1_PIN >= 0) deactivateDigit(DIGIT1_PIN);
    if (DIGIT2_PIN >= 0) deactivateDigit(DIGIT2_PIN);
  } else {
    delay(200);
  }
  setSegmentsByMask(0);

  Serial.print("DIGIT pins: "); Serial.print(DIGIT1_PIN); Serial.print(", "); Serial.println(DIGIT2_PIN);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](){ logRequest(); server.send(200, "text/plain", "OK"); });

  server.on("/display/", HTTP_GET, [](){ server.send(400, "text/plain", "Missing value"); });
  server.on("/display", HTTP_GET, [](){ server.send(400, "text/plain", "Missing value"); });
  server.on("/display/--", HTTP_GET, [](){ logRequest(); handleDisplaySet(String("--")); });

  // Test endpoint: enciende todos los segmentos (muestra 88) para verificar conexiones
  server.on("/test", HTTP_GET, [](){ logRequest(); showLeft = '8'; showRight = '8'; server.send(200, "text/plain", "OK"); });

  // Raw mask test: /raw/<0..127> -> escribe el mask directamente a segmentos
  server.on("/raw/", HTTP_GET, [](){ server.send(400, "text/plain", "Missing mask"); });
  server.on("/raw", HTTP_GET, [](){ server.send(400, "text/plain", "Missing mask"); });
  // handled in notFound branch generically

  // Pin control: /pin/<pinNumber>/<0|1> -> force digitalWrite on a segment or digit pin
  server.on("/pin/", HTTP_GET, [](){ server.send(400, "text/plain", "Missing params"); });
  server.on("/pin", HTTP_GET, [](){ server.send(400, "text/plain", "Missing params"); });

  // generic /display/<value>
  server.onNotFound([](){
    logRequest();
    String uri = server.uri();
    // check /display/
    if (uri.startsWith("/display/")) {
      logRequest();
      String v = uri.substring(9); // after /display/
      handleDisplaySet(v);
      return;
    }
    // raw mask: /raw/<mask>
    if (uri.startsWith("/raw/")) {
      logRequest();
      String sval = uri.substring(5);
      long mask = sval.toInt();
      if (mask < 0 || mask > 127) { server.send(400, "text/plain", "Bad mask"); return; }
      setSegmentsByMask((uint8_t)mask);
      server.send(200, "text/plain", "OK");
      return;
    }
    // conteo endpoints
    if (uri.startsWith("/conteo/")) {
      String rest = uri.substring(8);
      if (rest.startsWith("up/")) {
        logRequest();
        // start counting up from provided value
        String v = rest.substring(3);
        int val = v.toInt();
        if (val < 0) val = 0; if (val > 99) val = 0;
        currentCount = val;
        countDir = 1;
        countingActive = true;
        lastCountMs = millis();
        updateShowFromCurrentCount();
        server.send(200, "text/plain", "OK");
        return;
      } else if (rest.startsWith("down/")) {
        logRequest();
        // start counting down from provided value
        String v = rest.substring(5);
        int val = v.toInt();
        if (val < 0) val = 0; if (val > 99) val = 99;
        currentCount = val;
        countDir = -1;
        countingActive = true;
        lastCountMs = millis();
        updateShowFromCurrentCount();
        server.send(200, "text/plain", "OK");
        return;
      } else if (rest == "stop") {
        logRequest();
        countingActive = false;
        server.send(200, "text/plain", "OK");
        return;
      } else if (rest == "restart") {
        logRequest();
        countingActive = false;
        currentCount = 0;
        updateShowFromCurrentCount();
        server.send(200, "text/plain", "OK");
        return;
      }
    }
    // pin control: /pin/<pin>/<0|1>
    if (uri.startsWith("/pin/")) {
      logRequest();
      String rest = uri.substring(5);
      int slash = rest.indexOf('/');
      if (slash < 0) { server.send(400, "text/plain", "Bad params"); return; }
      String ps = rest.substring(0, slash);
      String vs = rest.substring(slash+1);
      int p = ps.toInt();
      int v = vs.toInt();
      if (p < 0) { server.send(400, "text/plain", "Bad pin"); return; }
      digitalWrite(p, v ? (COMMON_ANODE ? LOW : HIGH) : (COMMON_ANODE ? HIGH : LOW));
      server.send(200, "text/plain", "OK");
      return;
    }
    if (uri == "/destruction") {
      logRequest();
      // apagar todo
      showLeft = '-'; showRight = '-';
      setSegmentsByMask(0);
      server.send(200, "text/plain", "OK");
      return;
    }
    server.send(404, "text/plain", "Not Found");
  });

  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();
  updateMultiplex();
  updateCounting();
}
