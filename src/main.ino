// Stair light with Wemos D1 mini

#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <APA102.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

const char* wifi_password = WIFI_PASSWORD;
const char* wifi_ssid     = WIFI_SSID;

const char* mqtt_srv = MQTT_SERVER;
const char* mqtt_pw  = MQTT_password;
const char* mqtt_us  = MQTT_USER;

int i = 0;
long lastReconnectAttempt = 0;

WiFiClient espClient;
PubSubClient client(espClient);

const uint8_t dataPin = D3;
const uint8_t clockPin = D4;

APA102<dataPin, clockPin> ledStrip;
#define fDebug true
#define LED_COUNT 52
rgb_color colors[LED_COUNT];

// Set the brightness to use (the maximum is 31).
uint8_t globalBrightness = 1;
String temp_globalBrightness;
char send_globalBrightness[5];

#define NUM_STATES  13  // number of patterns to cycle through

// system timer, incremented by one every time through the main loop
unsigned int loopCount = 0;
unsigned int seed = 0;  // used to initialize random number generator
// Global vars for stairs
int actTrede = 1;
const unsigned char numTredes = 14;
int wachten = 0;

const char*             CMD_ON                     = "ON";
const char*             CMD_OFF                    = "OFF";

const char*             MQTT_WWS_STATE_TOPIC       = "trap/WWS/status";
const char*             MQTT_WWS_COMMAND_TOPIC     = "trap/WWS/switch";
const char*             MQTT_RCK_STATE_TOPIC       = "trap/RCK/status";
const char*             MQTT_RCK_COMMAND_TOPIC     = "trap/RCK/switch";
const char*             MQTT_TRC_STATE_TOPIC       = "trap/TRC/status";
const char*             MQTT_TRC_COMMAND_TOPIC     = "trap/TRC/switch";
const char*             MQTT_CEX_STATE_TOPIC       = "trap/CEX/status";
const char*             MQTT_CEX_COMMAND_TOPIC     = "trap/CEX/switch";
const char*             MQTT_GRA_STATE_TOPIC       = "trap/GRA/status";
const char*             MQTT_GRA_COMMAND_TOPIC     = "trap/GRA/switch";
const char*             MQTT_BTW_STATE_TOPIC       = "trap/BTW/status";
const char*             MQTT_BTW_COMMAND_TOPIC     = "trap/BTW/switch";
const char*             MQTT_COL_STATE_TOPIC       = "trap/COL/status";
const char*             MQTT_COL_COMMAND_TOPIC     = "trap/COL/switch";
const char*             MQTT_TRU_STATE_TOPIC       = "trap/TRU/status";
const char*             MQTT_TRU_COMMAND_TOPIC     = "trap/TRU/switch";
const char*             MQTT_TRD_STATE_TOPIC       = "trap/TRD/status";
const char*             MQTT_TRD_COMMAND_TOPIC     = "trap/TRD/switch";
const char*             MQTT_VLG_STATE_TOPIC       = "trap/VLG/status";
const char*             MQTT_VLG_COMMAND_TOPIC     = "trap/VLG/switch";
const char*             MQTT_RED_STATE_TOPIC       = "trap/RED/status";
const char*             MQTT_RED_COMMAND_TOPIC     = "trap/RED/switch";
const char*             MQTT_GRE_STATE_TOPIC       = "trap/GRE/status";
const char*             MQTT_GRE_COMMAND_TOPIC     = "trap/GRE/switch";
const char*             MQTT_BLU_STATE_TOPIC       = "trap/BLU/status";
const char*             MQTT_BLU_COMMAND_TOPIC     = "trap/BLU/switch";

const char*             MQTT_BRI_STATE_TOPIC       = "trap/BRI/status";
const char*             MQTT_BRI_COMMAND_TOPIC     = "trap/BRI/switch";

boolean WWS_State = false;
boolean RCK_State = false;
boolean TRC_State = false;
boolean CEX_State = false;
boolean GRA_State = false;
boolean BTW_State = false;
boolean COL_State = false;
boolean TRU_State = false;
boolean TRD_State = false;
boolean VLG_State = false;
boolean RED_State = false;
boolean GRE_State = false;
boolean BLU_State = false;

enum Pattern {
  WarmWhiteShimmer = 0,
  RandomColorWalk = 1,
  TraditionalColors = 2,
  ColorExplosion = 3,
  Gradient = 4,
  BrightTwinkle = 5,
  Collision = 6,
  TrapUp = 7,
  TrapDown = 8,
  Vlag = 9,
  Red = 10,
  Green = 11,
  Blue = 12,
  AllOff = 255
};

unsigned char pattern = AllOff;
unsigned int maxLoops;  // go to next state when loopCount >= maxLoops
void pattern_State(int pattern_state ){

  client.publish(MQTT_WWS_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_RCK_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_TRC_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_CEX_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_GRA_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_BTW_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_COL_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_TRU_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_TRD_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_VLG_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_RED_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_GRE_STATE_TOPIC,CMD_OFF,true);
  client.publish(MQTT_BLU_STATE_TOPIC,CMD_OFF,true);

  temp_globalBrightness = String(globalBrightness);
  temp_globalBrightness.toCharArray(send_globalBrightness, temp_globalBrightness.length() + 1);
  client.publish(MQTT_BRI_STATE_TOPIC,send_globalBrightness,true);

  WWS_State = false;
  RCK_State = false;
  TRC_State = false;
  CEX_State = false;
  GRA_State = false;
  BTW_State = false;
  COL_State = false;
  TRU_State = false;
  TRD_State = false;
  VLG_State = false;
  RED_State = false;
  GRE_State = false;
  BLU_State = false;

  switch (pattern_state)
  {
    case 0:
      WWS_State = true;
      client.publish(MQTT_WWS_STATE_TOPIC,CMD_ON,true);
      break;
    case 1:
      RCK_State = true;
      client.publish(MQTT_RCK_STATE_TOPIC,CMD_ON,true);
      break;
    case 2:
      TRC_State = true;
      client.publish(MQTT_TRC_STATE_TOPIC,CMD_ON,true);
      break;
    case 3:
      CEX_State = true;
      client.publish(MQTT_CEX_STATE_TOPIC,CMD_ON,true);
      break;
    case 4:
      GRA_State = true;
      client.publish(MQTT_GRA_STATE_TOPIC,CMD_ON,true);
      break;
    case 5:
      BTW_State = true;
      client.publish(MQTT_BTW_STATE_TOPIC,CMD_ON,true);
      break;
    case 6:
      COL_State = true;
      client.publish(MQTT_COL_STATE_TOPIC,CMD_ON,true);
      break;
    case 7:
      TRU_State = true;
      client.publish(MQTT_TRU_STATE_TOPIC,CMD_ON,true);
      break;
    case 8:
      TRD_State = true;
      client.publish(MQTT_TRD_STATE_TOPIC,CMD_ON,true);
      break;
    case 9:
      VLG_State = true;
      client.publish(MQTT_VLG_STATE_TOPIC,CMD_ON,true);
      break;
    case 10:
      RED_State = true;
      client.publish(MQTT_RED_STATE_TOPIC,CMD_ON,true);
      break;
    case 11:
      GRE_State = true;
      client.publish(MQTT_GRE_STATE_TOPIC,CMD_ON,true);
      break;
    case 12:
      BLU_State = true;
      client.publish(MQTT_BLU_STATE_TOPIC,CMD_ON,true);
      break;
    case 255:
      break;
  }
  wachten = 0;
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  // concat the payload into a string
  if (fDebug){  Serial.println("Callback...");  }

  String payload;
  for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }
  if (fDebug){ Serial.print("Payload: "); Serial.println(payload);  }
  if (payload.equals(String(CMD_OFF))) {
    wachten = 0;
  }

  // handle message topic
  if (String(MQTT_WWS_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (WWS_State != true) {
        //WWS_State = true;
        pattern = 0;
        pattern_State(pattern);
        //Logs("WarmWhSh ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        WWS_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("WarmWhSh OFF");
    }
  } else if (String(MQTT_RCK_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (RCK_State != true) {
        //RCK_State = true;
        pattern = 1;
        //Logs("RndmClr ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        RCK_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("RndmClr OFF");
    }
  } else if (String(MQTT_TRC_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (TRC_State != true) {
        //TRC_State = true;
        pattern = 2;
        //Logs("TraditClrs ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        TRC_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("TraditClrs OFF");
    }
  } else if (String(MQTT_CEX_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (CEX_State != true) {
        //CEX_State = true;
        pattern = 3;
        //Logs("ColorExpl ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        CEX_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("ColorExpl OFF");
    }
  } else if (String(MQTT_GRA_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (GRA_State != true) {
        //GRA_State = true;
        pattern = 4;
        //Logs("Gradient ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        GRA_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("Gradient OFF");
    }
  } else if (String(MQTT_BTW_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (BTW_State != true) {
        //BTW_State = true;
        pattern = 5;
        //Logs("BrightTw ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        BTW_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("BrightTw OFF");
    }
  } else if (String(MQTT_COL_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (COL_State != true) {
        //COL_State = true;
        pattern = 6;
        loopCount = 0;
        //Logs("Collision ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        COL_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("Collision OFF");
    }
  } else if (String(MQTT_TRU_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (TRU_State != true) {
        //TRU_State = true;
        pattern = 7;
        loopCount = 0;
        //Logs("TrapUp ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        TRU_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("TrapUp OFF");
    }
  } else if (String(MQTT_TRD_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (TRD_State != true) {
        //TRD_State = true;
        pattern = 8;
        loopCount = 0;
        //if (fDebug){ Serial.println(pattern); }
        //Logs("TrapDown ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        TRD_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("TrapDown OFF");
    }
  } else if (String(MQTT_VLG_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (VLG_State != true) {
        //VLG_State = true;
        pattern = 9;
        //Logs("Vlag ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        VLG_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("Vlag OFF");
    }
  } else if (String(MQTT_RED_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (RED_State != true) {
        //RED_State = true;
        pattern = 10;
        //Logs("Red ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        RED_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("Red OFF");
    }
  } else if (String(MQTT_GRE_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (GRE_State != true) {
        //GRE_State = true;
        pattern = 11;
        //Logs("Green ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        GRE_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("Green OFF");
    }
  } else if (String(MQTT_BLU_COMMAND_TOPIC).equals(p_topic)) {
    if (payload.equals(String(CMD_ON))) {
      if (BLU_State != true) {
        //BLU_State = true;
        pattern = 12;
        //Logs("Blue ON");
      }
    } else if (payload.equals(String(CMD_OFF))) {
        BLU_State = false;
        pattern = 255;
        loopCount = 0;
        //Logs("Blue OFF");
    }
  }else if (String(MQTT_BRI_COMMAND_TOPIC).equals(p_topic)) {
      globalBrightness = payload.toInt() ;
  }
  pattern_State(pattern);
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

    if (client.connect("ESP8266Client", mqtt_us, mqtt_pw)) {
      Serial.println("connected");
      client.subscribe(MQTT_WWS_COMMAND_TOPIC );
      //client.subscribe(MQTT_RCK_COMMAND_TOPIC );
      //client.subscribe(MQTT_TRC_COMMAND_TOPIC );
      //client.subscribe(MQTT_CEX_COMMAND_TOPIC );
      //client.subscribe(MQTT_GRA_COMMAND_TOPIC );
      //client.subscribe(MQTT_BTW_COMMAND_TOPIC );
      //client.subscribe(MQTT_COL_COMMAND_TOPIC );
      //client.subscribe(MQTT_TRU_COMMAND_TOPIC );
      //client.subscribe(MQTT_TRD_COMMAND_TOPIC );
      //client.subscribe(MQTT_VLG_COMMAND_TOPIC );
      //client.subscribe(MQTT_RED_COMMAND_TOPIC );
      //client.subscribe(MQTT_GRE_COMMAND_TOPIC );
      //client.subscribe(MQTT_BLU_COMMAND_TOPIC );
      //client.subscribe(MQTT_BRI_COMMAND_TOPIC );
      Serial.println("subscribe");
      pattern_State(255);
      StaticJsonBuffer<300> JSONbuffer;
      JsonObject& JSONencoder = JSONbuffer.createObject();
      JSONencoder["device"] = "ESP32Trap";
      JSONencoder["sensorType"] = "APA102";
      JsonArray& values = JSONencoder.createNestedArray("values");
      values.add(2);
      values.add(1);
      values.add(0);
      values.add(0);
      values.add(0);
      values.add(0);
      char JSONmessageBuffer[200];
      JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      Serial.println("Sending message to MQTT topic..");
        Serial.println(JSONmessageBuffer);

        if (client.publish("trap/test", JSONmessageBuffer) == true) {
          Serial.println("Success sending message");
        } else {
          Serial.println("Error sending message");
        }


    }
    return client.connected();
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
    // whenever timer resets, clear the LED colors array (all off)
    for (int i = 0; i < LED_COUNT; i++)
    {
      colors[i] = rgb_color(0, 0, 0);
    }
  }

  if (pattern == WarmWhiteShimmer || pattern == RandomColorWalk)
  {
    if (loopCount % 6 == 0)
    {
      seed = random(30000);
    }
    randomSeed(seed);
  }

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
    case Red:
      maxLoops = 10;
      if (loopCount == maxLoops){
        loopCount = 0;
      }
      rood();
      break;
    case Green:
      maxLoops = 10;
      if (loopCount == maxLoops){
        loopCount = 0;
      }
      groen();
      break;
    case Blue:
      maxLoops = 10;
      if (loopCount == maxLoops){
        loopCount = 0;
      }
      blauw();
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


// This function fades val by decreasing it by an amount proportional
// to its current value.  The fadeTime argument determines the
// how quickly the value fades.  The new value of val will be:
//   val = val - val*2^(-fadeTime)
// So a smaller fadeTime value leads to a quicker fade.
// If val is greater than zero, val will always be decreased by
// at least 1.
// val is a pointer to the byte to be faded.
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


// ***** PATTERN WarmWhiteShimmer *****
// This function randomly increases or decreases the brightness of the
// even red LEDs by changeAmount, capped at maxBrightness.  The green
// and blue LED values are set proportional to the red value so that
// the LED color is warm white.  Each odd LED is set to a quarter the
// brightness of the preceding even LEDs.  The dimOnly argument
// disables the random increase option when it is true, causing
// all the LEDs to get dimmer by changeAmount; this can be used for a
// fade-out effect.
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


// ***** PATTERN RandomColorWalk *****
// This function randomly changes the color of every seventh LED by
// randomly increasing or decreasing the red, green, and blue components
// by changeAmount (capped at maxBrightness) or leaving them unchanged.
// The two preceding and following LEDs are set to progressively dimmer
// versions of the central color.  The initializeColors argument
// determines how the colors are initialized:
//   0: randomly walk the existing colors
//   1: set the LEDs to alternating red and green segments
//   2: set the LEDs to random colors
// When true, the dimOnly argument changes the random walk into a 100%
// chance of LEDs getting dimmer by changeAmount; this can be used for
// a fade-out effect.
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


// ***** PATTERN TraditionalColors *****
// This function creates a repeating patern of traditional Christmas
// light colors: red, green, orange, blue, magenta.
// Every fourth LED is colored, and the pattern slowly moves by fading
// out the current set of lit LEDs while gradually brightening a new
// set shifted over one LED.
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

  // if LED_COUNT is not an exact multiple of our repeating pattern size,
  // it will not wrap around properly, so we pick the closest LED count
  // that is an exact multiple of the pattern period (20) and is not smaller
  // than the actual LED count.
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


// Helper function for adjusting the colors for the BrightTwinkle
// and ColorExplosion patterns.  Odd colors get brighter and even
// colors get dimmer.
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


// Helper function for adjusting the colors for the ColorExplosion
// pattern.  Odd colors get brighter and even colors get dimmer.
// The propChance argument determines the likelihood that neighboring
// LEDs are put into the brightening stage when the central LED color
// is 31 (chance is: 1 - 1/(propChance+1)).  The neighboring LED colors
// are pointed to by leftColor and rightColor (it is not important that
// the leftColor LED actually be on the "left" in your setup).
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


// ***** PATTERN ColorExplosion *****
// This function creates bursts of expanding, overlapping colors by
// randomly picking LEDs to brighten and then fade away.  As these LEDs
// brighten, they have a chance to trigger the same process in
// neighboring LEDs.  The color of the burst is randomly chosen from
// among red, green, blue, and white.  If a red burst meets a green
// burst, for example, the overlapping portion will be a shade of yellow
// or orange.
// When true, the noNewBursts argument changes prevents the generation
// of new bursts; this can be used for a fade-out effect.
// This function uses a very similar algorithm to the BrightTwinkle
// pattern.  The main difference is that the random twinkling LEDs of
// the BrightTwinkle pattern do not propagate to neighboring LEDs.
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


// ***** PATTERN Gradient *****
// This function creates a scrolling color gradient that smoothly
// transforms from red to white to green back to white back to red.
// This pattern is overlaid with waves of brightness and dimness that
// scroll at twice the speed of the color gradient.
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


// ***** PATTERN BrightTwinkle *****
// This function creates a sparkling/twinkling effect by randomly
// picking LEDs to brighten and then fade away.  Possible colors are:
//   white, red, green, blue, yellow, cyan, and magenta
// numColors is the number of colors to generate, and minColor
// indicates the starting point (white is 0, red is 1, ..., and
// magenta is 6), so colors generated are all of those from minColor
// to minColor+numColors-1.  For example, calling brightTwinkle(2, 2, 0)
// will produce green and blue twinkles only.
// When true, the noNewBursts argument changes prevents the generation
// of new twinkles; this can be used for a fade-out effect.
// This function uses a very similar algorithm to the ColorExplosion
// pattern.  The main difference is that the random twinkling LEDs of
// this BrightTwinkle pattern do not propagate to neighboring LEDs.
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


// ***** PATTERN Collision *****
// This function spawns streams of color from each end of the strip
// that collide, at which point the entire strip flashes bright white
// briefly and then fades.  Unlike the other patterns, this function
// maintains a lot of complicated state data and tells the main loop
// when it is done by returning 1 (a return value of 0 means it is
// still in progress).
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
      return 1;
    }
  }
  return 0;
}

unsigned char trapdown()
{

  const unsigned char maxBrightness = 255;  // max brightness for the colors
  const unsigned char numLEDS = 4;  // # of collisions before pattern ends

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

void rood()
{
    const unsigned char maxBrightness = 255;  // max brightness for the colors
    const unsigned char numLEDS = 4;  // # of collisions before pattern ends
      for (int i = 0; i < numLEDS * 13 ; i++)
      {
        colors[i] = rgb_color(maxBrightness, 0, 0);
      }
}

void groen()
{
    const unsigned char maxBrightness = 255;  // max brightness for the colors
    const unsigned char numLEDS = 4;  // # of collisions before pattern ends
      for (int i = 0; i < numLEDS * 13 ; i++)
      {
        colors[i] = rgb_color(0, maxBrightness, 0);
      }
}

void blauw()
{
    const unsigned char maxBrightness = 255;  // max brightness for the colors
    const unsigned char numLEDS = 4;  // # of collisions before pattern ends
      for (int i = 0; i < numLEDS * 13 ; i++)
      {
        colors[i] = rgb_color(0, 0, maxBrightness);
      }
}
