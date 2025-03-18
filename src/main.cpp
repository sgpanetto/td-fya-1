#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TSL2561_U.h>

#define LED_PIN 14     // Pin di controllo del NeoPixel
#define NUM_LEDS 2    // Numero di LED nella striscia
#define SCREEN_WIDTH 128    // Larghezza display OLED in pixel
#define SCREEN_HEIGHT 32    // Altezza display OLED in pixel
#define OLED_RESET -1      // Pin reset (-1 se condivide il reset dell'Arduino)
#define BUTTON_PIN 15    // Pin del pulsante

// Crea un'istanza del sensore
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// Crea un'istanza del NeoPixel
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

// Crea oggetto display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
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

// Aggiungi queste variabili globali
float initial_lux = 0;  // Luminosità iniziale senza filamento

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

// Funzione per normalizzare un valore tra il nero e il bianco
byte normalizeValue(uint16_t value, uint16_t black, uint16_t white) {
  if (value < black) value = black;
  if (value > white) value = white;
  
  float normalized = (float)(value - black) / (white - black) * 255.0;
  return constrain((byte)normalized, 0, 255);
}

void setup() {
  Serial.begin(115200);

  // Modifica la configurazione I2C per RP2040
  Wire.begin();
  // Inizializza il display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Starting...");
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
  
  // Aggiungi questa parte alla fine del setup
  Serial.println("Calibrazione luminosità iniziale...");
  delay(2000);  // Attendi che tutto si stabilizzi

  for(int i = 0; i < 10; i++) {
    sensors_event_t event;
    tsl.getEvent(&event);    
    delay(100);
  }
  
  // Leggi la luminosità iniziale (media di 10 letture)
  float sum_lux = 0;
  for(int i = 0; i < 10; i++) {
    sensors_event_t event;
    tsl.getEvent(&event);
    sum_lux += event.light;
    delay(100);
  }
  initial_lux = sum_lux / 10;
  
  Serial.print("Luminosità iniziale calibrata: ");
  Serial.print(initial_lux);
  Serial.println(" lux");
  
  // Aggiungi questa configurazione del pin del pulsante
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Ready");
  display.display();
}

void loop() {
  // Sostituisci tutto il contenuto del loop con questo nuovo codice
  if (digitalRead(BUTTON_PIN) == LOW) {  // Il pulsante è stato premuto
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);

    // Normalizza i valori usando i punti di calibrazione
    byte r_norm = normalizeValue(r, black_cal.r, white_cal.r);
    byte g_norm = normalizeValue(g, black_cal.g, white_cal.g);
    byte b_norm = normalizeValue(b, black_cal.b, white_cal.b);

    // Leggi la luminosità
    sensors_event_t event;
    tsl.getEvent(&event);
    
    // Calcola la Transmission Distance
    float transmission_ratio = event.light / initial_lux;
    float ln_lux = log(transmission_ratio);
    float td = -(1.75/ln_lux)*10;
    
    // Aggiorna il display
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Color: #");
    if(r_norm < 16) display.print("0");
    display.print(r_norm, HEX);
    if(g_norm < 16) display.print("0");
    display.print(g_norm, HEX);
    if(b_norm < 16) display.print("0");
    display.print(b_norm, HEX);
    
    display.setCursor(0,16);
    display.print("TD: ");
    display.print(td, 1);
    display.print("mm");
    display.display();
    
    // Output seriale
    Serial.print("Colore calibrato HEX: #");
    if(r_norm < 16) Serial.print("0");
    Serial.print(r_norm, HEX);
    if(g_norm < 16) Serial.print("0");
    Serial.print(g_norm, HEX);
    if(b_norm < 16) Serial.print("0");
    Serial.println(b_norm, HEX);
    
    Serial.print("Transmission Distance: ");
    Serial.print(td, 1);
    Serial.println("mm");
  } else {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Ready");
    display.display();
  }
    
  delay(1000);  // Piccolo ritardo per evitare letture multiple
}