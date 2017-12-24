// Stair light with Wemos D1 mini

#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <APA102.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
//#define MQTT_MAX_PACKET_SIZE 512

#define fDebug true
#define LED_COUNT 52
#define NUM_STATES  10  // number of patterns to cycle through

const char* wifi_password = WIFI_PASSWORD;
const char* wifi_ssid     = WIFI_SSID;

const char* mqtt_srv = MQTT_SERVER;
const char* mqtt_pw  = MQTT_password;
const char* mqtt_us  = MQTT_USER;

const char* on_cmd = "on";
const char* off_cmd = "off";
const char* effect = "AllOff";
String effectString = "AllOff";

uint8_t red = 255;
uint8_t green = 255;
uint8_t blue = 255;

byte brightness = 125;

bool stateOn = true;
/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(10);

int i = 0;
long lastReconnectAttempt = 0;

WiFiClient espClient;
PubSubClient client(espClient);

const uint8_t dataPin = D3;
const uint8_t clockPin = D4;

APA102<dataPin, clockPin> ledStrip;

rgb_color colors[LED_COUNT];

// Set the brightness to use (the maximum is 31).
uint8_t globalBrightness =16;

// system timer, incremented by one every time through the main loop
unsigned int loopCount = 0;
unsigned int seed = 0;  // used to initialize random number generator
// Global vars for stairs
int actTrede = 1;
const unsigned char numTredes = 14;
int wachten = 0;

const char*             CMD_ON                     = "on";
const char*             CMD_OFF                    = "off";

const char*             MQTT_STATE_TOPIC       = "trap/status";
const char*             MQTT_COMMAND_TOPIC     = "trap/switch";

boolean TRU_State = false;
boolean TRD_State = false;

#define   WarmWhiteShimmer  0
#define   RandomColorWalk  1
#define   TraditionalColors  2
#define   ColorExplosion  3
#define   Gradient  4
#define   BrightTwinkle  5
#define   Collision  6
#define   TrapUp  7
#define   TrapDown  8
#define   Vlag  9
#define   Solidcolor 10
#define   AllOff  255

unsigned char pattern = 255;

unsigned int maxLoops;  // go to next state when loopCount >= maxLoops

// function called when a MQTT message arrived
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  if (!wachten){
    if (!processJson(message)) {
        
        return;
    }
    sendState();
  }
}


int bepaal_pattern(){
  int pattern = 255;
  
  if ( String(effect) == "WarmWhiteShimmer") {
    pattern = 0;
  } else if (String(effect) == "RandomColorWalk") {
    pattern = 1;
  }else if (String(effect) == "TraditionalColors") {
    pattern = 2;
  }else if (String(effect) == "ColorExplosion") {
    pattern = 3;
  }else if (String(effect) == "Gradient") {
    pattern = 4;
  }else if (String(effect) == "BrightTwinkle") {
    pattern = 5;
  }else if (String(effect) == "Collision") {
    pattern = 6;
  }else if (String(effect) == "TrapUp") {
    pattern = 7;
  }else if (String(effect) == "TrapDown") {
    pattern = 8;
  }else if (String(effect) == "Vlag") {
    pattern = 9;
  }else if (String(effect) == "AllOff") {
    pattern = 255;
  }
  loopCount = 0;
  Serial.print("pattern :"); Serial.println(pattern);

  return pattern;
}

void stripuit(){
    // whenever timer resets, clear the LED colors array (all off)
    for (int i = 0; i < LED_COUNT; i++)
    {
      colors[i] = rgb_color(0, 0, 0);
    }
    // Zet de strip uit
    ledStrip.write(colors, LED_COUNT, globalBrightness);
}

/********************************** START PROCESS JSON*****************************************/
bool processJson(char* message) {

  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
      pattern = 255;
    }
  }
  Serial.print("stateOn :");Serial.println(stateOn);

    if (root.containsKey("color")) {
      red =  root["color"]["r"];
      green = root["color"]["g"];
      blue = root["color"]["b"];
      //red = 10;
      //green = 20;
      //blue = 30;
      pattern = 10;
      Serial.print("pattern :");Serial.print(pattern); Serial.print(" red :");Serial.print(red);Serial.print("  green :");Serial.print(green);Serial.print("    blue :");Serial.println(blue);
    }

    if (root.containsKey("brightness")) {
      globalBrightness = root["brightness"];
      globalBrightness = round(globalBrightness/8);
      Serial.print("brightness :");Serial.println(brightness);
      Serial.print("globalBrightness :");Serial.println(globalBrightness);
    }
    if (!root.containsKey("color")) {
      if (root.containsKey("effect")) {
        effect = root["effect"];
        effectString = effect;
        Serial.print("effect :");Serial.println(effect);
        
        pattern = bepaal_pattern();
      }
    } 
  return true;
}

/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? on_cmd : off_cmd;
  JsonObject& color = root.createNestedObject("color");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;


  brightness = globalBrightness * 8;
  root["brightness"] = brightness;
  root["effect"] = effectString.c_str();

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  if (client.publish(MQTT_STATE_TOPIC, buffer, true) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  setup_wifi();
  client.setServer(mqtt_srv, 1883);
  client.setCallback(callback);

  for (int i = 0; i < 2; i++)
  {
      seed += analogRead(i);
  }
  seed += EEPROM.read(0);  // get part of the seed from EEPROM
  randomSeed(seed);
  // save a random number in EEPROM to be used for random seed
  // generation the next time the program runs
  EEPROM.write(0, random(256));

  lastReconnectAttempt = 0;
  delay(10);  // give pull-ups time raise the input voltage

}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(i<10){
      Serial.print(".");
    }else{
      Serial.println("." );
      i=0;
    }
    i++;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

boolean reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("ESP8266Client", mqtt_us, mqtt_pw)) {
        Serial.println("connected");
        client.subscribe(MQTT_COMMAND_TOPIC);
        sendState();
        
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    return true;
}

void loop()
{
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    client.loop();
  }

  uint8_t startTime = millis();

  if (pattern == 255){
    loopCount = 0;
    TRU_State = LOW;
    TRD_State = LOW;
  }

  if (loopCount == 0)
  {
    stripuit();
  }
  
  if (pattern == WarmWhiteShimmer || pattern == RandomColorWalk)
  {
    if (loopCount % 6 == 0)
    {
      seed = random(30000);
    }
    randomSeed(seed);
  }
  //Serial.print("switch pattern :");Serial.println(effect);
  //pattern = &effect;
  switch (pattern)
  {
    case WarmWhiteShimmer:
      // warm white shimmer for 300 loopCounts, fading over last 70
      maxLoops = 300;
      if (loopCount == maxLoops){
        loopCount = 0;
      }
      //warmWhiteShimmer(loopCount > maxLoops - 70);
      warmWhiteShimmer(loopCount > maxLoops);
      break;

    case RandomColorWalk:
      maxLoops = 400;
      if (loopCount == maxLoops){
        loopCount = 0;
      }

      randomColorWalk(loopCount == 0 ? 1 : 0, loopCount > maxLoops - 80);
      break;

    case TraditionalColors:
      maxLoops = 400;
      if (loopCount == maxLoops){
        loopCount = 0;
      }

      traditionalColors();
      break;

    case ColorExplosion:
      maxLoops = 630;
      if (loopCount == maxLoops){
        loopCount = 0;
      }

      colorExplosion((loopCount % 200 > 130) || (loopCount > maxLoops - 100));
      break;

    case Gradient:
      maxLoops = 250;
      if (loopCount == maxLoops){
        loopCount = 0;
      }

      gradient();
      delay(6);  // add an extra 6ms delay to slow things down
      break;

    case BrightTwinkle:
      maxLoops = 1200;
      if (loopCount == maxLoops){
        loopCount = 0;
      }

      if (loopCount < 400)
      {
        brightTwinkle(0, 1, 0);  // only white for first 400 loopCounts
      }
      else if (loopCount < 650)
      {
        brightTwinkle(0, 2, 0);  // white and red for next 250 counts
      }
      else if (loopCount < 900)
      {
        brightTwinkle(1, 2, 0);  // red, and green for next 250 counts
      }
      else
      {
        // red, green, blue, cyan, magenta, yellow for the rest of the time
        brightTwinkle(1, 6, loopCount > maxLoops - 100);
      }
      break;

    case Collision:
      if (!collision())
      {
        maxLoops = loopCount + 2;
      }
      break;
    case TrapUp:
      maxLoops = ((numTredes * 15) * 2) + 600; // 15 cycles between steps  2 times for "ON" and "OFF" and plus count cycles for "ON"
      if (!trapup())
      {
        maxLoops = loopCount + 2;
      }
      break;
    case TrapDown:
      maxLoops = ((numTredes * 15) * 2) + 600; // 15 cycles between steps  2 times for "ON" and "OFF" and plus count cycles for "ON"
      if (!trapdown())
      {
        //Serial.println(loopCount);
        maxLoops = loopCount + 2;

      }
      break;
    case Vlag:
      maxLoops = 100;
      if (loopCount == maxLoops){
        loopCount = 0;
      }
      vlag();
      break;
    case Solidcolor:
      maxLoops = 100;
      if (loopCount == maxLoops){
        loopCount = 0;
      }
      solidcolor();
      break;
    
  }

  // update the LED strips with the colors in the colors array
  ledStrip.write(colors, LED_COUNT, globalBrightness);

  while((uint8_t)(millis() - startTime) < 20) { }
  loopCount++;  // increment our loop counter/timer.

  if (loopCount >= maxLoops )//&& digitalRead(AUTOCYCLE_SWITCH_PIN))
  {
    loopCount = 0;  // reset timer
  }
}
void randomWalk(unsigned char *val, unsigned char maxVal, unsigned char changeAmount, unsigned char directions)
{
  unsigned char walk = random(directions);  // direction of random walk
  if (walk == 0)
  {
    // decrease val by changeAmount down to a min of 0
    if (*val >= changeAmount)
    {
      *val -= changeAmount;
    }
    else
    {
      *val = 0;
    }
  }
  else if (walk == 1)
  {
    // increase val by changeAmount up to a max of maxVal
    if (*val <= maxVal - changeAmount)
    {
      *val += changeAmount;
    }
    else
    {
      *val = maxVal;
    }
  }
  //delay(6);
}

void fade(unsigned char *val, unsigned char fadeTime)
{
  if (*val != 0)
  {
    unsigned char subAmt = *val >> fadeTime;  // val * 2^-fadeTime
    if (subAmt < 1)
      subAmt = 1;  // make sure we always decrease by at least 1
    *val -= subAmt;  // decrease value of byte pointed to by val
  }
}

void warmWhiteShimmer(unsigned char dimOnly)
{
  const unsigned char maxBrightness = 120;  // cap on LED brighness
  const unsigned char changeAmount = 2;   // size of random walk step

  for (int i = 0; i < LED_COUNT; i += 2)
  {
    // randomly walk the brightness of every even LED
    randomWalk(&colors[i].red, maxBrightness, changeAmount, dimOnly ? 1 : 2);

    // warm white: red = x, green = 0.8x, blue = 0.125x
    colors[i].green = colors[i].red*4/5;  // green = 80% of red
    colors[i].blue = colors[i].red >> 3;  // blue = red/8

    // every odd LED gets set to a quarter the brighness of the preceding even LED
    if (i + 1 < LED_COUNT)
    {
      colors[i+1] = rgb_color(colors[i].red >> 2, colors[i].green >> 2, colors[i].blue >> 2);
    }
  }
}

void randomColorWalk(unsigned char initializeColors, unsigned char dimOnly)
{
  const unsigned char maxBrightness = 180;  // cap on LED brightness
  const unsigned char changeAmount = 3;  // size of random walk step

  // pick a good starting point for our pattern so the entire strip
  // is lit well (if we pick wrong, the last four LEDs could be off)
  unsigned char start;
  switch (LED_COUNT % 7)
  {
    case 0:
      start = 3;
      break;
    case 1:
      start = 0;
      break;
    case 2:
      start = 1;
      break;
    default:
      start = 2;
  }

  for (int i = start; i < LED_COUNT; i+=7)
  {
    if (initializeColors == 0)
    {
      // randomly walk existing colors of every seventh LED
      // (neighboring LEDs to these will be dimmer versions of the same color)
      randomWalk(&colors[i].red, maxBrightness, changeAmount, dimOnly ? 1 : 3);
      randomWalk(&colors[i].green, maxBrightness, changeAmount, dimOnly ? 1 : 3);
      randomWalk(&colors[i].blue, maxBrightness, changeAmount, dimOnly ? 1 : 3);
    }
    else if (initializeColors == 1)
    {
      // initialize LEDs to alternating red and green
      if (i % 2)
      {
        colors[i] = rgb_color(maxBrightness, 0, 0);
      }
      else
      {
        colors[i] = rgb_color(0, maxBrightness, 0);
      }
    }
    else
    {
      // initialize LEDs to a string of random colors
      colors[i] = rgb_color(random(maxBrightness), random(maxBrightness), random(maxBrightness));
    }

    // set neighboring LEDs to be progressively dimmer versions of the color we just set
    if (i >= 1)
    {
      colors[i-1] = rgb_color(colors[i].red >> 2, colors[i].green >> 2, colors[i].blue >> 2);
    }
    if (i >= 2)
    {
      colors[i-2] = rgb_color(colors[i].red >> 3, colors[i].green >> 3, colors[i].blue >> 3);
    }
    if (i + 1 < LED_COUNT)
    {
      colors[i+1] = colors[i-1];
    }
    if (i + 2 < LED_COUNT)
    {
      colors[i+2] = colors[i-2];
    }
  }
}

void traditionalColors()
{
  // loop counts to leave strip initially dark
  const unsigned char initialDarkCycles = 10;
  // loop counts it takes to go from full off to fully bright
  const unsigned char brighteningCycles = 20;

  if (loopCount < initialDarkCycles)  // leave strip fully off for 20 cycles
  {
    return;
  }

  unsigned int extendedLEDCount = (((LED_COUNT-1)/20)+1)*20;

  for (int i = 0; i < extendedLEDCount; i++)
  {
    unsigned char brightness = (loopCount - initialDarkCycles)%brighteningCycles + 1;
    unsigned char cycle = (loopCount - initialDarkCycles)/brighteningCycles;

    // transform i into a moving idx space that translates one step per
    // brightening cycle and wraps around
    unsigned int idx = (i + cycle)%extendedLEDCount;
    if (idx < LED_COUNT)  // if our transformed index exists
    {
      if (i % 4 == 0)
      {
        // if this is an LED that we are coloring, set the color based
        // on the LED and the brightness based on where we are in the
        // brightening cycle
        switch ((i/4)%5)
        {
           case 0:  // red
             colors[idx].red = 200 * brightness/brighteningCycles;
             colors[idx].green = 10 * brightness/brighteningCycles;
             colors[idx].blue = 10 * brightness/brighteningCycles;
             break;
           case 1:  // green
             colors[idx].red = 10 * brightness/brighteningCycles;
             colors[idx].green = 200 * brightness/brighteningCycles;
             colors[idx].blue = 10 * brightness/brighteningCycles;
             break;
           case 2:  // orange
             colors[idx].red = 200 * brightness/brighteningCycles;
             colors[idx].green = 120 * brightness/brighteningCycles;
             colors[idx].blue = 0 * brightness/brighteningCycles;
             break;
           case 3:  // blue
             colors[idx].red = 10 * brightness/brighteningCycles;
             colors[idx].green = 10 * brightness/brighteningCycles;
             colors[idx].blue = 200 * brightness/brighteningCycles;
             break;
           case 4:  // magenta
             colors[idx].red = 200 * brightness/brighteningCycles;
             colors[idx].green = 64 * brightness/brighteningCycles;
             colors[idx].blue = 145 * brightness/brighteningCycles;
             break;
        }
      }
      else
      {
        // fade the 3/4 of LEDs that we are not currently brightening
        fade(&colors[idx].red, 3);
        fade(&colors[idx].green, 3);
        fade(&colors[idx].blue, 3);
      }
    }
  }
}

void brightTwinkleColorAdjust(unsigned char *color)
{
  if (*color == 255)
  {
    // if reached max brightness, set to an even value to start fade
    *color = 254;
  }
  else if (*color % 2)
  {
    // if odd, approximately double the brightness
    // you should only use odd values that are of the form 2^n-1,
    // which then gets a new value of 2^(n+1)-1
    // using other odd values will break things
    *color = *color * 2 + 1;
  }
  else if (*color > 0)
  {
    fade(color, 4);
    if (*color % 2)
    {
      (*color)--;  // if faded color is odd, subtract one to keep it even
    }
  }
}

void colorExplosionColorAdjust(unsigned char *color, unsigned char propChance,
 unsigned char *leftColor, unsigned char *rightColor)
{
  if (*color == 31 && random(propChance+1) != 0)
  {
    if (leftColor != 0 && *leftColor == 0)
    {
      *leftColor = 1;  // if left LED exists and color is zero, propagate
    }
    if (rightColor != 0 && *rightColor == 0)
    {
      *rightColor = 1;  // if right LED exists and color is zero, propagate
    }
  }
  brightTwinkleColorAdjust(color);
}

void colorExplosion(unsigned char noNewBursts)
{
  // adjust the colors of the first LED
  colorExplosionColorAdjust(&colors[0].red, 9, (unsigned char*)0, &colors[1].red);
  colorExplosionColorAdjust(&colors[0].green, 9, (unsigned char*)0, &colors[1].green);
  colorExplosionColorAdjust(&colors[0].blue, 9, (unsigned char*)0, &colors[1].blue);

  for (int i = 1; i < LED_COUNT - 1; i++)
  {
    // adjust the colors of second through second-to-last LEDs
    colorExplosionColorAdjust(&colors[i].red, 9, &colors[i-1].red, &colors[i+1].red);
    colorExplosionColorAdjust(&colors[i].green, 9, &colors[i-1].green, &colors[i+1].green);
    colorExplosionColorAdjust(&colors[i].blue, 9, &colors[i-1].blue, &colors[i+1].blue);
  }

  // adjust the colors of the last LED
  colorExplosionColorAdjust(&colors[LED_COUNT-1].red, 9, &colors[LED_COUNT-2].red, (unsigned char*)0);
  colorExplosionColorAdjust(&colors[LED_COUNT-1].green, 9, &colors[LED_COUNT-2].green, (unsigned char*)0);
  colorExplosionColorAdjust(&colors[LED_COUNT-1].blue, 9, &colors[LED_COUNT-2].blue, (unsigned char*)0);

  if (!noNewBursts)
  {
    // if we are generating new bursts, randomly pick one new LED
    // to light up
    for (int i = 0; i < 1; i++)
    {
      int j = random(LED_COUNT);  // randomly pick an LED

      switch(random(7))  // randomly pick a color
      {
        // 2/7 chance we will spawn a red burst here (if LED has no red component)
        case 0:
        case 1:
          if (colors[j].red == 0)
          {
            colors[j].red = 1;
          }
          break;

        // 2/7 chance we will spawn a green burst here (if LED has no green component)
        case 2:
        case 3:
          if (colors[j].green == 0)
          {
            colors[j].green = 1;
          }
          break;

        // 2/7 chance we will spawn a white burst here (if LED is all off)
        case 4:
        case 5:
          if ((colors[j].red == 0) && (colors[j].green == 0) && (colors[j].blue == 0))
          {
            colors[j] = rgb_color(1, 1, 1);
          }
          break;

        // 1/7 chance we will spawn a blue burst here (if LED has no blue component)
        case 6:
          if (colors[j].blue == 0)
          {
            colors[j].blue = 1;
          }
          break;

        default:
          break;
      }
    }
  }
}

void gradient()
{
  unsigned int j = 0;

  // populate colors array with full-brightness gradient colors
  // (since the array indices are a function of loopCount, the gradient
  // colors scroll over time)
  while (j < LED_COUNT)
  {
    // transition from red to green over 8 LEDs
    for (int i = 0; i < 8; i++)
    {
      if (j >= LED_COUNT){ break; }
      colors[(loopCount/2 + j + LED_COUNT)%LED_COUNT] = rgb_color(160 - 20*i, 20*i, (160 - 20*i)*20*i/160);
      j++;
    }
    // transition from green to red over 8 LEDs
    for (int i = 0; i < 8; i++)
    {
      if (j >= LED_COUNT){ break; }
      colors[(loopCount/2 + j + LED_COUNT)%LED_COUNT] = rgb_color(20*i, 160 - 20*i, (160 - 20*i)*20*i/160);
      j++;
    }
  }

  // modify the colors array to overlay the waves of dimness
  // (since the array indices are a function of loopCount, the waves
  // of dimness scroll over time)
  const unsigned char fullDarkLEDs = 10;  // number of LEDs to leave fully off
  const unsigned char fullBrightLEDs = 5;  // number of LEDs to leave fully bright
  const unsigned char cyclePeriod = 14 + fullDarkLEDs + fullBrightLEDs;

  // if LED_COUNT is not an exact multiple of our repeating pattern size,
  // it will not wrap around properly, so we pick the closest LED count
  // that is an exact multiple of the pattern period (cyclePeriod) and is not
  // smaller than the actual LED count.
  unsigned int extendedLEDCount = (((LED_COUNT-1)/cyclePeriod)+1)*cyclePeriod;

  j = 0;
  while (j < extendedLEDCount)
  {
    unsigned int idx;

    // progressively dim the LEDs
    for (int i = 1; i < 8; i++)
    {
      idx = (j + loopCount) % extendedLEDCount;
      if (j++ >= extendedLEDCount){ return; }
      if (idx >= LED_COUNT){ continue; }

      colors[idx].red >>= i;
      colors[idx].green >>= i;
      colors[idx].blue >>= i;
    }

    // turn off these LEDs
    for (int i = 0; i < fullDarkLEDs; i++)
    {
      idx = (j + loopCount) % extendedLEDCount;
      if (j++ >= extendedLEDCount){ return; }
      if (idx >= LED_COUNT){ continue; }

      colors[idx].red = 0;
      colors[idx].green = 0;
      colors[idx].blue = 0;
    }

    // progressively bring these LEDs back
    for (int i = 0; i < 7; i++)
    {
      idx = (j + loopCount) % extendedLEDCount;
      if (j++ >= extendedLEDCount){ return; }
      if (idx >= LED_COUNT){ continue; }

      colors[idx].red >>= (7 - i);
      colors[idx].green >>= (7 - i);
      colors[idx].blue >>= (7 - i);
    }

    // skip over these LEDs to leave them at full brightness
    j += fullBrightLEDs;
  }
}

void brightTwinkle(unsigned char minColor, unsigned char numColors, unsigned char noNewBursts)
{
  // Note: the colors themselves are used to encode additional state
  // information.  If the color is one less than a power of two
  // (but not 255), the color will get approximately twice as bright.
  // If the color is even, it will fade.  The sequence goes as follows:
  // * Randomly pick an LED.
  // * Set the color(s) you want to flash to 1.
  // * It will automatically grow through 3, 7, 15, 31, 63, 127, 255.
  // * When it reaches 255, it gets set to 254, which starts the fade
  //   (the fade process always keeps the color even).
  for (int i = 0; i < LED_COUNT; i++)
  {
    brightTwinkleColorAdjust(&colors[i].red);
    brightTwinkleColorAdjust(&colors[i].green);
    brightTwinkleColorAdjust(&colors[i].blue);
  }

  if (!noNewBursts)
  {
    // if we are generating new twinkles, randomly pick four new LEDs
    // to light up
    for (int i = 0; i < 4; i++)
    {
      int j = random(LED_COUNT);
      if (colors[j].red == 0 && colors[j].green == 0 && colors[j].blue == 0)
      {
        // if the LED we picked is not already lit, pick a random
        // color for it and seed it so that it will start getting
        // brighter in that color
        switch (random(numColors) + minColor)
        {
          case 0:
            colors[j] = rgb_color(1, 1, 1);  // white
            break;
          case 1:
            colors[j] = rgb_color(1, 0, 0);  // red
            break;
          case 2:
            colors[j] = rgb_color(0, 1, 0);  // green
            break;
          case 3:
            colors[j] = rgb_color(0, 0, 1);  // blue
            break;
          case 4:
            colors[j] = rgb_color(1, 1, 0);  // yellow
            break;
          case 5:
            colors[j] = rgb_color(0, 1, 1);  // cyan
            break;
          case 6:
            colors[j] = rgb_color(1, 0, 1);  // magenta
            break;
          default:
            colors[j] = rgb_color(1, 1, 1);  // white
        }
      }
    }
  }
}

unsigned char collision()
{
  const unsigned char maxBrightness = 180;  // max brightness for the colors
  const unsigned char numCollisions = 5;  // # of collisions before pattern ends
  static unsigned char state = 0;  // pattern state
  static unsigned int count = 0;  // counter used by pattern

  if (loopCount == 0)
  {
    state = 0;
  }

  if (state % 3 == 0)
  {
    // initialization state
    switch (state/3)
    {
      case 0:  // first collision: red streams
        colors[0] = rgb_color(maxBrightness, 0, 0);
        break;
      case 1:  // second collision: green streams
        colors[0] = rgb_color(0, maxBrightness, 0);
        break;
      case 2:  // third collision: blue streams
        colors[0] = rgb_color(0, 0, maxBrightness);
        break;
      case 3:  // fourth collision: warm white streams
        colors[0] = rgb_color(maxBrightness, maxBrightness*4/5, maxBrightness>>3);
        break;
      default:  // fifth collision and beyond: random-color streams
        colors[0] = rgb_color(random(maxBrightness), random(maxBrightness), random(maxBrightness));
    }

    // stream is led by two full-white LEDs
    colors[1] = colors[2] = rgb_color(255, 255, 255);
    // make other side of the strip a mirror image of this side
    colors[LED_COUNT - 1] = colors[0];
    colors[LED_COUNT - 2] = colors[1];
    colors[LED_COUNT - 3] = colors[2];

    state++;  // advance to next state
    count = 8;  // pick the first value of count that results in a startIdx of 1 (see below)
    return 0;
  }

  if (state % 3 == 1)
  {
    // stream-generation state; streams accelerate towards each other
    unsigned int startIdx = count*(count + 1) >> 6;
    unsigned int stopIdx = startIdx + (count >> 5);
    count++;
    if (startIdx < (LED_COUNT + 1)/2)
    {
      // if streams have not crossed the half-way point, keep them growing
      for (int i = 0; i < startIdx-1; i++)
      {
        // start fading previously generated parts of the stream
        fade(&colors[i].red, 5);
        fade(&colors[i].green, 5);
        fade(&colors[i].blue, 5);
        fade(&colors[LED_COUNT - i - 1].red, 5);
        fade(&colors[LED_COUNT - i - 1].green, 5);
        fade(&colors[LED_COUNT - i - 1].blue, 5);
      }
      for (int i = startIdx; i <= stopIdx; i++)
      {
        // generate new parts of the stream
        if (i >= (LED_COUNT + 1) / 2)
        {
          // anything past the halfway point is white
          colors[i] = rgb_color(255, 255, 255);
        }
        else
        {
          colors[i] = colors[i-1];
        }
        // make other side of the strip a mirror image of this side
        colors[LED_COUNT - i - 1] = colors[i];
      }
      // stream is led by two full-white LEDs
      colors[stopIdx + 1] = colors[stopIdx + 2] = rgb_color(255, 255, 255);
      // make other side of the strip a mirror image of this side
      colors[LED_COUNT - stopIdx - 2] = colors[stopIdx + 1];
      colors[LED_COUNT - stopIdx - 3] = colors[stopIdx + 2];
    }
    else
    {
      // streams have crossed the half-way point of the strip;
      // flash the entire strip full-brightness white (ignores maxBrightness limits)
      for (int i = 0; i < LED_COUNT; i++)
      {
        colors[i] = rgb_color(255, 255, 255);
      }
      state++;  // advance to next state
    }
    return 0;
  }

  if (state % 3 == 2)
  {
    // fade state
    if (colors[0].red == 0 && colors[0].green == 0 && colors[0].blue == 0)
    {
      // if first LED is fully off, advance to next state
      state++;

      // after numCollisions collisions, this pattern is done
      return state >= 3*numCollisions;
    }

    // fade the LEDs at different rates based on the state
    for (int i = 0; i < LED_COUNT; i++)
    {
      switch (state/3)
      {
        case 0:  // fade through green
          fade(&colors[i].red, 3);
          fade(&colors[i].green, 4);
          fade(&colors[i].blue, 2);
          break;
        case 1:  // fade through red
          fade(&colors[i].red, 4);
          fade(&colors[i].green, 3);
          fade(&colors[i].blue, 2);
          break;
        case 2:  // fade through yellow
          fade(&colors[i].red, 4);
          fade(&colors[i].green, 4);
          fade(&colors[i].blue, 3);
          break;
        case 3:  // fade through blue
          fade(&colors[i].red, 3);
          fade(&colors[i].green, 2);
          fade(&colors[i].blue, 4);
          break;
        default:  // stay white through entire fade
          fade(&colors[i].red, 4);
          fade(&colors[i].green, 4);
          fade(&colors[i].blue, 4);
      }
    }
  }

  return 0;
}

unsigned char trapup()
{

  const unsigned char maxBrightness = 255;  // max brightness for the colors
  const unsigned char numLEDS = 4;  // # of collisions before pattern ends
 
  if (!wachten){
    stripuit();
  }
  wachten = 1;
  
  if( loopCount != 0 ){
    if( loopCount == 1){
      actTrede = 1;
    }


    if (loopCount % 15 == 0 && loopCount < 210 )
    {
      for (int i = 0; i < numLEDS * actTrede ; i++)
      {
        colors[i] = rgb_color(maxBrightness, maxBrightness, maxBrightness);
      }
      actTrede++;
    }

    if( loopCount == 800){
      actTrede = 1;
    }

    if (loopCount % 15 == 0 && loopCount > 810 )
    {
      for (int i = 0; i < numLEDS * actTrede ; i++)
      {
        colors[i] = rgb_color(0, 0, 0);
      }
      actTrede++;
    }

    if( actTrede == numTredes){
      actTrede = 1;
    }

    if (loopCount == 1020){
      pattern = 255;
      wachten = 0;
      sendState();
      return 1;
    }
  }
  return 0;
}

unsigned char trapdown()
{

  const unsigned char maxBrightness = 255;  // max brightness for the colors
  const unsigned char numLEDS = 4;  // # of collisions before pattern ends
  
  if (!wachten){
    stripuit();
  }
  wachten = 1;
  if( loopCount != 0 ){
    if( loopCount == 1 ){
      actTrede = 13;
      if (fDebug){ Serial.print("actTrede ");}
      if (fDebug){ Serial.println(actTrede);}
    }

    if (loopCount % 15 == 0 && loopCount < 210 )
    {
      for (int i = numLEDS * actTrede ; --i >= (numLEDS * (actTrede -1)) ;)
      {
        //Serial.println(i);
        colors[i] = rgb_color(maxBrightness, maxBrightness, maxBrightness);
      }
      actTrede--;
    }


    if( loopCount == 800){
      actTrede = 13;
    }

    if (loopCount % 15 == 0 && loopCount > 810 )
    {
      for (int i = numLEDS * actTrede ; --i >= (numLEDS * (actTrede -1 )) ;)
      {
        colors[i] = rgb_color(0, 0, 0);
      }
      actTrede--;
    }
    if (loopCount == 1020){
      wachten = 0;
      pattern = 255;
      sendState();
      return 1;
    }
  }
return 0;
}

void vlag()
{
    const unsigned char maxBrightness = 255;  // max brightness for the colors
    const unsigned char numLEDS = 4;  // # of collisions before pattern ends

      for (int i = 0; i < numLEDS * 5 ; i++)
      {
        colors[i] = rgb_color(maxBrightness, 0, 0);
      }
      for (int i = numLEDS * 5; i < numLEDS * 9 ; i++)
      {
        colors[i] = rgb_color(maxBrightness, maxBrightness, maxBrightness);
      }
      for (int i = numLEDS * 9; i < numLEDS * 13 ; i++)
      {
        colors[i] = rgb_color(0, 0, maxBrightness);
      }
}

void solidcolor(){
  
  for (int i = 0; i < LED_COUNT ; i++)
      {
        colors[i] = rgb_color(red, green, blue);
      }
  
}


