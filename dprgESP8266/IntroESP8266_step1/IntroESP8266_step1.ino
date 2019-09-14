/* LED Strip */
#include <Adafruit_DotStar.h>
#include <SPI.h>

/* LED Strip defines */
#define NUMPIXELS 90 // Number of LEDs in strip
#define DATAPIN    14
#define CLOCKPIN   16

/* Class instances */
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

/*
 * Variable used in Loop 
 *
 * Run 10 LEDs at a time along strip, cycling through red, green and blue.
 * This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.
  */
int      head  = 0, tail = -10; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)


 
 
/*
 * Setup Code
 */
void setup() {
  Serial.begin(115200);
  Serial.println("");
  
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}




/*
 * Main LOOP
 */
void loop() {
  strip.setPixelColor(head, color); // 'On' pixel at head
  strip.setPixelColor(tail, 0);     // 'Off' pixel at tail
  strip.show();                     // Refresh strip
  delay(0);                        // Pause 20 milliseconds (~50 FPS)

  if(++head >= NUMPIXELS) {         // Increment head index.  Off end of strip?
    head = 0;                       //  Yes, reset head index to start
    if((color >>= 8) == 0)          //  Next color (R->G->B) ... past blue now?
      color = 0xFF0000;             //   Yes, reset to red
  }
  if(++tail >= NUMPIXELS) tail = 0; // Increment, reset tail index
}
