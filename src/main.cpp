#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TSL2561_U.h>

#define LED_PIN 6     // Pin di controllo del NeoPixel
#define NUM_LEDS 2    // Numero di LED nella striscia
#define SCREEN_WIDTH 128    // Larghezza display OLED in pixel
#define SCREEN_HEIGHT 32    // Altezza display OLED in pixel
#define OLED_RESET -1      // Pin reset (-1 se condivide il reset dell'Arduino)

// Crea un'istanza del sensore
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// Crea un'istanza del NeoPixel
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

// Crea oggetto display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* void setup() {
  Serial.begin(9600);
  
  if (!tcs.begin()) {
    Serial.println("Errore: sensore non trovato!");
    while (1);
  }
  Serial.println("Sensore TCS34725 trovato!");
}

void loop() {
  uint16_t r, g, b, c;
  
  // Leggi i valori RGB e Clear
  tcs.getRawData(&r, &g, &b, &c);

  // Stampa prima i valori RAW per debug
  Serial.print("Valori RAW -> R: "); Serial.print(r);
  Serial.print(" G: "); Serial.print(g);
  Serial.print(" B: "); Serial.print(b);
  Serial.print(" C: "); Serial.println(c);

  // Converti in valori RGB 0-255 con un fattore di scala diverso
  float r_norm = (float)r / c * 255.0;
  float g_norm = (float)g / c * 255.0;
  float b_norm = (float)b / c * 255.0;

  // Stampa i valori normalizzati
  Serial.print("Normalizzati (0-255) -> R: "); Serial.print(r_norm);
  Serial.print(" G: "); Serial.print(g_norm);
  Serial.print(" B: "); Serial.println(b_norm);

   // Converti in valori interi per hex
  byte r_hex = (byte)r_norm;
  byte g_hex = (byte)g_norm;
  byte b_hex = (byte)b_norm;

  // Stampa i valori normalizzati in decimale e hex
  Serial.print("Colore HEX: #");
  if(r_hex < 16) Serial.print("0"); // Aggiunge lo zero iniziale se necessario
  Serial.print(r_hex, HEX);
  if(g_hex < 16) Serial.print("0");
  Serial.print(g_hex, HEX);
  if(b_hex < 16) Serial.print("0");
  Serial.println(b_hex, HEX);
  
  delay(5000);
} */
 
// Crea un'istanza del sensore di luminosità
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// Strutture per memorizzare i valori di calibrazione
struct CalibrationValues {
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t c;
};

// Valori di calibrazione globali con valori fissi
CalibrationValues white_cal = {4633, 6182, 4215, 15337};  // Valori del bianco
CalibrationValues black_cal = {405, 646, 419, 1500};      // Valori del nero

// Funzione per leggere più valori e farne la media
CalibrationValues getAverageReading(int num_readings = 10) {
  CalibrationValues avg = {0, 0, 0, 0};
  uint32_t sum_r = 0, sum_g = 0, sum_b = 0, sum_c = 0;
  
  for(int i = 0; i < num_readings; i++) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    sum_r += r;
    sum_g += g;
    sum_b += b;
    sum_c += c;
    delay(100);
  }
  
  avg.r = sum_r / num_readings;
  avg.g = sum_g / num_readings;
  avg.b = sum_b / num_readings;
  avg.c = sum_c / num_readings;
  return avg;
}

/* Commento la funzione di calibrazione poiché ora usiamo valori fissi
void calibrate() {
  Serial.println("\n=== Inizia Calibrazione ===");
  
  // Calibrazione del nero
  Serial.println("Posiziona il sensore su una superficie NERA");
  Serial.println("Inizio tra 5 secondi...");
  delay(15000);
  
  black_cal = getAverageReading();
  Serial.println("Calibrazione nero completata!");
  Serial.print("Nero - R: "); Serial.print(black_cal.r);
  Serial.print(" G: "); Serial.print(black_cal.g);
  Serial.print(" B: "); Serial.print(black_cal.b);
  Serial.print(" C: "); Serial.println(black_cal.c);
  
  // Calibrazione del bianco
  Serial.println("\nPosiziona il sensore su una superficie BIANCA");
  Serial.println("Inizio tra 5 secondi...");
  delay(15000);
  
  white_cal = getAverageReading();
  Serial.println("Calibrazione bianco completata!");
  Serial.print("Bianco - R: "); Serial.print(white_cal.r);
  Serial.print(" G: "); Serial.print(white_cal.g);
  Serial.print(" B: "); Serial.print(white_cal.b);
  Serial.print(" C: "); Serial.println(white_cal.c);
  
  Serial.println("\n=== Calibrazione Completata ===");
}
*/

// Funzione per normalizzare un valore tra il nero e il bianco
byte normalizeValue(uint16_t value, uint16_t black, uint16_t white) {
  if (value < black) value = black;
  if (value > white) value = white;
  
  float normalized = (float)(value - black) / (white - black) * 255.0;
  return constrain((byte)normalized, 0, 255);
}

void setup() {
  Serial.begin(9600);
  
  // Inizializza il display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  pixels.begin();
  pixels.setBrightness(50); // Imposta la luminosità (0-255)
  
  // Imposta entrambi i LED su bianco
  pixels.setPixelColor(0, pixels.Color(0, 0, 0, 255)); // R, G, B, W
  pixels.setPixelColor(1, pixels.Color(0, 0, 0, 255)); // R, G, B, W per il secondo LED
  pixels.show();
  
  if (!tcs.begin()) {
    Serial.println("Errore: sensore non trovato");
    while (1);
  }
  
  tcs.setGain(TCS34725_GAIN_4X);
  tcs.setIntegrationTime(TCS34725_INTEGRATIONTIME_154MS);
  
  if (!tsl.begin()) {
    Serial.println("Errore: sensore TSL2561 non trovato!");
    while (1);
  }
  
  // Configura il sensore TSL2561
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
}

void loop() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  // Normalizza i valori usando i punti di calibrazione
  byte r_norm = normalizeValue(r, black_cal.r, white_cal.r);
  byte g_norm = normalizeValue(g, black_cal.g, white_cal.g);
  byte b_norm = normalizeValue(b, black_cal.b, white_cal.b);

  // Stampa i valori normalizzati in hex
  Serial.print("Colore calibrato HEX: #");
  if(r_norm < 16) Serial.print("0");
  Serial.print(r_norm, HEX);
  if(g_norm < 16) Serial.print("0");
  Serial.print(g_norm, HEX);
  if(b_norm < 16) Serial.print("0");
  Serial.println(b_norm, HEX);

  // Stampa anche i valori raw per debug
  Serial.print("RAW - R: "); Serial.print(r);
  Serial.print(" G: "); Serial.print(g);
  Serial.print(" B: "); Serial.print(b);
  Serial.print(" C: "); Serial.println(c);
  
  // Leggi la luminosità
  sensors_event_t event;
  tsl.getEvent(&event);
  
  // Stampa la luminosità su Serial
  Serial.print("Luminosità: ");
  Serial.print(event.light);
  Serial.println(" lux");
  
  // Aggiorna il display con il colore e la luminosità
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Color: #");
  if(r_norm < 16) display.print("0");
  display.print(r_norm, HEX);
  if(g_norm < 16) display.print("0");
  display.print(g_norm, HEX);
  if(b_norm < 16) display.print("0");
  display.print(b_norm, HEX);
  
  // Aggiungi la luminosità nella seconda riga
  display.setCursor(0,16);
  display.print("Lux: ");
  display.print(event.light);
  display.display();
  
  delay(5000); // Modificato a 5 secondi
}