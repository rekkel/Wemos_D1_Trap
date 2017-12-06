#include "config.h"

#if defined(MY_USE_UDP)
#include <WiFiUdp.h>
#endif

#include <ESP8266WiFi.h>

const char* password = WIFI_PASSWORD;
const char* ssid     = WIFI_SSID;

const char* host = "trap.local" ;

#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
#ifdef __AVR_ATtiny85__
 #include <avr/power.h>
#endif


uint8_t dataPinStrip    = D4;
uint8_t clckPinStrip    = D3;
uint8_t bewegung1       = D2;
uint8_t bewegung2       = D8;
uint8_t bewegungsstatus = 0;
uint8_t num_LEDs        = 26;
uint8_t hue             = 0;
uint32_t c              = 0;
int abstand             = 0;
Adafruit_WS2801 strip = Adafruit_WS2801(num_LEDs, dataPinStrip, clckPinStrip);
uint32_t Color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((((r << 8) | g) << 8) | b);
}
void lights(boolean order=true, boolean mode=true)
{
    int i = 0;
    hue = mode ? 250 : 0;

     Serial.println(); Serial.print("Hue = "); Serial.println(hue);

    c = Color(hue, hue, hue);
    if (order)
    {
        for (i = 0; i < num_LEDs; i++)
        {
            strip.setPixelColor(i, c);
            Serial.print("Up i = "); Serial.println(i);
            i++;
            strip.setPixelColor(i, c);
            strip.show();
            delay(250); //Anschaltgeschwindigkeit von oben nach unten
            Serial.print("Up i = "); Serial.println(i);
        }
    }
    else
    {
        for (i = num_LEDs; --i >= 0 ; )
        {
            strip.setPixelColor(i, c);
            Serial.print("Down i = "); Serial.println(i);
            i--;
            strip.setPixelColor(i, c);
            strip.show();
            delay(250); //Anschaltgeschwindigkeit von unten nach oben
            Serial.print("Down i = "); Serial.println(i);
        }
    }
}

void up()
{
    strip_clear();
    lights();
    delay(15000); //Leuchtzeit von oben nach unten
    lights(true, false);
}
void down()
{
    strip_clear();
    lights(false);
    delay(15000); //Leuchtzeit von unten nach oben
    lights(false, false);
}
void strip_clear()
{
    for (int i=0; i < num_LEDs; i++)
    {
        strip.setPixelColor(i, 0);
        Serial.print("clear i = "); Serial.println(i);
        i++;
        strip.setPixelColor(i, 0);
        Serial.print("clear i = "); Serial.println(i);
    }
    strip.show();
}
void strip_clear_up()
{
    for (int i=0; i < num_LEDs; i++)
    {
        strip.setPixelColor(i, 0);
        Serial.print("clear up i = "); Serial.println(i);
        i++;
        strip.setPixelColor(i, 0);
        Serial.print("clear up i = "); Serial.println(i);
    }
    strip.show();
}
void strip_clear_down()
{
    for (int i=num_LEDs; --i >= 0 ;)
    {
        strip.setPixelColor(i, 0);
        Serial.print("clear down i = "); Serial.println(i);
        i--;
        strip.setPixelColor(i, 0);
        Serial.print("clear down i = "); Serial.println(i);
    }
    strip.show();
}
void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)  {
    delay(500);
    if(i<10){
      Serial.print(".");
    }else{
      Serial.println("." ) ;
      i=0;
    }
    i++;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());



  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
    clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
  #endif
    Serial.println();  Serial.println("Trap Verlichting Version 1.0"); Serial.println();
    pinMode(bewegung1, INPUT);
    pinMode(bewegung2, INPUT);
    strip.begin();
    strip.show();
}
void loop()
{
    bewegungsstatus = digitalRead(bewegung1);
    if (bewegungsstatus == HIGH)
    {
        Serial.println(); Serial.println("UP");
        up();
        {
            strip_clear();
        }
    }
    bewegungsstatus = digitalRead(bewegung2);
    if (bewegungsstatus == HIGH)
    {
        Serial.println(); Serial.println("DOWN");
        down();
        {
            strip_clear();
        }
    }
}
