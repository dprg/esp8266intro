/* LED Strip */
#include <Adafruit_DotStar.h>
#include <SPI.h>

/* ArduinoJSON */
#include <ArduinoJson.h>

/* ESP8266 and SecureSSL web server */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServerSecure.h>
#include "certificate.h"

/* FileSystem */
#include <FS.h>

/* Configuration type */
struct Config {
  int delay_speed;
  int strip_brightness;
};

/* LED Strip defines */
#define NUMPIXELS 90 // Number of LEDs in strip
#define DATAPIN    14
#define CLOCKPIN   16

/* ESP8266 defines */
#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-wifi-password"
#endif

/* Json defines */
#define CONFIG_SIZE 512

/* Class instances */
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
BearSSL::ESP8266WebServerSecure server(443);
StaticJsonDocument<CONFIG_SIZE> doc;

/* Constants */
const char* ssid = STASSID;
const char* password = STAPSK;
const char *filename = "/strip_config.json";
const bool formatSpiffsNow = false; // only set to true if you need to force FS format

/* Globals */
Config _config;

/*
 * Variable used in Loop() 
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

  // Load Configuration
  loadConfiguration();

  // Wifi
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

  // led strip
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}

 


/*
 * Main LOOP
 */
void loop() {
  // handle any incoming requests
  server.handleClient();

  // update strip
  strip.setPixelColor(head, color); // 'On' pixel at head
  strip.setPixelColor(tail, 0);     // 'Off' pixel at tail
  strip.show();                     // Refresh strip
  delay(_config.delay_speed);       // Pause 20 milliseconds (~50 FPS)

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
 *  strip route controller '/strip', POST expects json object
 *  
 *  {
 *    "Speed":"0-100",         <-- 0 = Fast, 100 = Slow
 *    "Brightness":"0-255"     <-- 0 = Off, 255 = Max Brightness
 *  }
 *  
 */   
void handleStrip() {
  String message = "URI:";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":(server.method() == HTTP_POST)?"POST":"UNKNOWN";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";     
  }
  
  switch (server.method())
  {
    case HTTP_GET:
    {
      // Clear our Json document
      doc.clear();

      // Fill the document with current config values
      doc["Speed"] = _config.delay_speed;
      doc["Brightness"] = _config.strip_brightness;
      String output;
      serializeJsonPretty(doc, output);
      message += output;
      server.send(200, "application/json", output);
      break;
    }
    case HTTP_POST:
    {
      String newSpeedMatch = "Speed";
      String newBrightnessMatch = "Brightness";

      // Deserialize the JSON document
      DeserializationError error = deserializeJson(doc, server.arg(0));
      
      // Test if parsing succeeds.
      if (error) {
          server.send(500, "text/plain", error.c_str());
          return;
      }

      if(doc.containsKey(newSpeedMatch)) {
        int delaySpeed = doc[newSpeedMatch];
        _config.delay_speed = constrain(delaySpeed, 0, 100);
        message += "\nReceived new delay speed: ";
        message += _config.delay_speed;
        message += "\n";
      }

      if(doc.containsKey(newBrightnessMatch)) {
        int brightness = doc[newBrightnessMatch];
        _config.strip_brightness = constrain(brightness, 0, 255);
        strip.setBrightness(_config.strip_brightness);
        message += "\nReceived new Brightness value: ";
        message += _config.strip_brightness;
        message += "\n";
      }

      server.send(200, "text/plain", message);

      // save our new config
      if (!saveConfig(filename, _config)) {
        Serial.println("Failed to save config");
      } else {
        Serial.println("Config saved");
      }  
      
      break;        
    }
    
    default:
    {
      server.send(500, "text/plain", "Rest method not implemented");
      break;
    }
  }

  Serial.println(message);
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

/*
 * Mount SPIFFS and load sign config 
 */
void loadConfiguration()
{
  Serial.println("Mounting FS...");

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system, please set Flash size to at least 1MB SPIFFS");
    return;
  }

  Serial.println("Successfully mounted FS...");

  Serial.println("Checking for fs_info...");
  FSInfo fs_info;
  if(!SPIFFS.info(fs_info) || formatSpiffsNow)
  {
    Serial.println("Please wait, formatting new File System....");
    
    //Format File System
    if(SPIFFS.format())
    {
      Serial.println("File System Formated");
    }
    else
    {
      Serial.println("File System Formatting Error");
      return;
    }
  }

  Serial.println("loading config...");

  if (!loadConfig(filename, _config)) {
    Serial.println("Failed to load config, using default settings");
    initialize_with_default_config();
  } else {
    Serial.println("Config loaded");
  }  
}

/*
 * If we did not have any settings persisted, start with these defaults.  
 * Note: must make POST request to actual save to file in SPIFFS
 */
bool initialize_with_default_config()
{
  _config.delay_speed = 25;
  _config.strip_brightness = 100;
}

/*
 * Loads Sign configuration from SPIFFS
 */
bool loadConfig(const char *filename, Config &config) {
  File configFile = SPIFFS.open(filename, "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }
  Serial.print("File :");
  Serial.print(filename);
  Serial.println(" opened for read successfully");
  
  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    configFile.close();
    return false;
  }

  Serial.print("configFile.size :");
  Serial.print(configFile.size());
  Serial.println(" OK");
  
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error){
    Serial.println(F("Failed to read file, reset using default configuration"));
    initialize_with_default_config();
    configFile.close();
    return false;
  }

  Serial.println("deserializeJson ok:");

   // Copy values from the JsonDocument to the Config
  Serial.println("start unpacking json");
  
  if(doc.containsKey("delay_speed")) {
    config.delay_speed = doc["delay_speed"];
    Serial.print("delay_speed ok: ");
    Serial.println(config.delay_speed);
  }

  if(doc.containsKey("strip_brightness")) {
    config.strip_brightness = doc["strip_brightness"];
    Serial.print("strip_brightness ok: ");
    Serial.println(config.strip_brightness);
  }

  Serial.println("done unpacking");

  configFile.close();  
  return true;
}

/*
 * Saves Sign configuration to SPIFFS
 */
bool saveConfig(const char *filename, const Config &config) {

  File configFile = SPIFFS.open(filename, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  Serial.print("File :");
  Serial.print(filename);
  Serial.println(" opened for write successfully");

  // Set the values in the document
  doc["delay_speed"] = config.delay_speed;
  doc["strip_brightness"] = config.strip_brightness;
  
  // Serialize JSON to file
  if (serializeJson(doc, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
    configFile.close();
    return false;
  }
  
  configFile.close();
  return true;
}
