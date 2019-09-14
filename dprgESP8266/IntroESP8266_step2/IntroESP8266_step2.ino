/* LED Strip */
#include <Adafruit_DotStar.h>
#include <SPI.h>

/* ESP8266 and SecureSSL web server */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServerSecure.h>
#include "certificate.h"

/* LED Strip defines */
#define NUMPIXELS 90 // Number of LEDs in strip
#define DATAPIN    14
#define CLOCKPIN   16

/* ESP8266 defines */
#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-wifi-password"
#endif

/* Class instances */
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
BearSSL::ESP8266WebServerSecure server(443);

/* Constants */
const char* ssid = STASSID;
const char* password = STAPSK;

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
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting .");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // configure SSL certificate
  server.setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));

  // configure routes and map to handlers
  server.on("/", handleRoot);
  server.on("/strip", handleStrip);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTPS server started");
  
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}



 
/*
 * Main LOOP
 */
void loop() {
  // handle any incoming requests
  server.handleClient();


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


/* 
 *  Root route controller '/'
 */   
void handleRoot() {
  server.send(200, "text/plain", "Path = '/', Hello from esp8266 over HTTPS!");
}


/* 
 *  strip route controller '/strip'  
 */   
void handleStrip() {
  server.send(200, "text/plain", "Path = '/strip', Hello from esp8266 over HTTPS!");
}


/*
 * HTTP 404 not found handler
 */
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
