
// vcc -> 3v3 , gnd -> gnd , sck -> D1 , sdi -> d2


#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include "ThingSpeak.h" 
#include <MQ135.h>


char ssid[] = "" ;   // put your network SSID (name) 
char pass[] = "";   //put your network password
WiFiClient  client;

unsigned long myChannelNumber = 1930557;  // our thingspeak server channel number
const char * myWriteAPIKey = "";  // put your thingspeak server api key 
// our thingspeak server link :- https://thingspeak.com/channels/1930557


#define PIN_MQ135 A0 // MQ135 Analog Input Pin
MQ135 mq135_sensor(PIN_MQ135);

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);
#define BME_SCK 14
#define BME_MISO 12
#define BME_MOSI 13
#define BME_CS 15

#define SEALEVELPRESSURE_HPA (1013.25)


Adafruit_BME680 bme; // I2C
// Adafruit_BME680 bme(BME_CS); // hardware SPI
// Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

void setup() 
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); 
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }



//::::::::::::::: bme setup :::::::::::::::::::::::



  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms


}

  
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//:::::::::::::::::::::: loop:::::::::::::::::::::::::::::::::::::::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void loop() 
{

  pinMode(D0, INPUT);
  pinMode(A0,INPUT);

//:::::::::::::: variables ::::::::::::::::::::::::::::::::::

  float temperature=0;
  float humidity=0;
  float pressure=0;
  float gasResistance=0;
  float altitude=0;
  float heatIndex=0;
  float dewPoint=0;
  float airQuality=0;
  int analogAir=0;
  int analogRain=0;
  int rain=0;


//:::::::::::::::::::::: bme reading ::::::::::::::::::::::::::


  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading :("));
    return;
  }
  Serial.print(F("Reading started at "));
  Serial.print(millis());
  Serial.print(F(" and will finish at "));
  Serial.println(endTime);


  delay(50); 

  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }
  temperature=bme.temperature;
  humidity=bme.humidity;
  pressure=bme.pressure/100;
  gasResistance=bme.gas_resistance/1000;
  altitude=bme.readAltitude(SEALEVELPRESSURE_HPA);
  heatIndex=calculateHeatIndex(temperature,humidity);
  dewPoint=calculateDewPoint(temperature,humidity);
  Serial.print(F("Reading completed at "));
  Serial.println(millis());

  Serial.print(F("Temperature = "));
  Serial.print(temperature);
  Serial.println(F(" *C"));

  Serial.print(F("Pressure = "));
  Serial.print(pressure);
  Serial.println(F(" hPa"));

  Serial.print(F("Humidity = "));
  Serial.print(humidity);
  Serial.println(F(" %"));

  Serial.print(F("HI = "));
  Serial.print(heatIndex );
  Serial.println(F(" * C"));

  Serial.print(F("DewPoint = "));
  Serial.print(dewPoint );
  Serial.println(F(" * C"));

  Serial.print(F("Gas = "));
  Serial.print(gasResistance );
  Serial.println(F(" KOhms"));

  Serial.print(F("Approx. Altitude = "));
  Serial.print(altitude);
  Serial.println(F(" m"));
  Serial.println();




//::::::::::::::::: rain :::::::::::::::::::::::::::::



  if(digitalRead(D0)  == LOW)
  {
    rain=1;   
    Serial.print("It is raining\n");
  }
  else 
    rain=0;
  Serial.print("Rain :");
  Serial.println(rain);


// ::::::::::::::::::::: air ::::::::::::::::


  analogAir=analogRead(A0);
  Serial.print(F("Analog value of Air : "));
  Serial.println(analogAir); 
  float rzero = mq135_sensor.getRZero();
  float correctedRZero = mq135_sensor.getCorrectedRZero(temperature, humidity);
  float resistance = mq135_sensor.getResistance();
  float ppm = mq135_sensor.getPPM();
  float correctedPPM = mq135_sensor.getCorrectedPPM(temperature, humidity);
  airQuality=correctedPPM;
  Serial.print("AirQuality = ");
  Serial.print(airQuality);
  Serial.println(" ppm");

//::::::::::::::: thingspeak ::::::::::::::::::::::::::

  delay(1000);
  float number1=temperature;
  float number2=humidity;
  float number3=pressure;
  float number4=altitude;
  int number5=rain;
  int number6=heatIndex;
  float number7=airQuality;
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }

  // set the fields with the values
  ThingSpeak.setField(1, number1);
  ThingSpeak.setField(2, number2);
  ThingSpeak.setField(3, number3);
  ThingSpeak.setField(4, number4);
  ThingSpeak.setField(5, number5);
  ThingSpeak.setField(6, number6);
  ThingSpeak.setField(7, number7);
  
 
  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
    
  delay(20000); // Wait 20 seconds to update the channel again




//:::::::::::: display ::::::::::::::::::::::::
  
  display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(WHITE);

	display.setCursor(0,30);
	display.print("Team Alpha");
  display.display();
	delay(3000);
  display.clearDisplay();


	display.setCursor(0,8);
	display.print("Temp:");
  display.println(temperature);
  display.println("");
	display.print("Humi:");
  display.println(humidity);
	display.display();
	delay(3000);
  display.clearDisplay();

  display.setCursor(0,8);
	display.print("Press:");
  display.println(pressure,0);
  display.println("");
	display.print("Alti:");
  display.println(altitude,0);
	display.display();
  delay(3000);
  display.clearDisplay();

  display.setCursor(0,8);
	display.print("HI:");
  display.println(heatIndex,0);
  display.println("");
	display.print("DP:");
  display.println(dewPoint,0);
	display.display();
  delay(3000);
  display.clearDisplay();

  display.setCursor(0,8);
	display.print("AirQ:");
  display.println(airQuality,0);
  display.println("");
	display.print("Rain:");
  if(rain)
    display.println("YES");
  else
    display.println("NO");
	display.display();
  delay(3000);
   
}


// less than or equal to 55: dry and comfortable
// between 55 and 65: becoming "sticky" with muggy evenings
// greater than or equal to 65: lots of moisture in the air, becoming oppressive

float calculateDewPoint(float celsius, float humidity)
{
  float b = 17.271;
  float c = 237.7;
  float temp = (b * celsius) / (c + celsius) + log(humidity * 0.01);
  float Td = (c * temp) / (b - temp);
  return Td;
}
//27–32 °C	Caution: fatigue is possible with prolonged exposure and activity. Continuing activity could result in heat cramps.
//32–41 °C	Extreme caution: heat cramps and heat exhaustion are possible. Continuing activity could result in heat stroke.
//41–54 °C	Danger: heat cramps and heat exhaustion are likely; heat stroke is probable with continued activity.
//over 54 °C	Extreme danger: heat stroke is imminent.

float calculateHeatIndex(float t,float h)
{
  float c1 = -8.78469, c2 = 1.6114, c3 = 2.338, c4 = -0.1461, c5= -1.231e-2, c6=-1.6424e-2, c7=2.212e-3, c8=7.255e-4, c9=-3.58e-6  ;
  float A = (( c5 * t) + c2) * t + c1;
  float B = ((c7 * t) + c4) * t + c3;
  float C = ((c9 * t) + c8) * t + c6;

  float hi = (C * h + B) * h + A;
  
  return hi;
}


