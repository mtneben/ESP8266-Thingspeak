#include <ThingSpeak.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "secrets.h"

//Define WIFI parameters
const char *ssid     = wifissid;
const char *password = wifipassword;
//Define Thingspeak details
unsigned long myChannelNumber = myThingsChannelNumber;
const char * myWriteAPIKey = myThingsWriteAPIKey;

const long utcOffsetInSeconds = 0;          //Define NTP server time offset to London time
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const int buttonPin = 13;    				// the number of the pushbutton pin
int buttonState = HIGH;;             		// the current reading from the input pin
int lastButtonState = HIGH;   				// the previous reading from the input pin
int count = 0;
float readings[2];
unsigned long lastDebounceTime = 0;  		// the last time the output pin was toggled
unsigned long debounceDelay = 50;    		// the debounce time; increase if the output flickers
unsigned long tempTime = 0;      			// the lt timme the temp reading was taken
unsigned long tempDelay = 60000;    		// delay of 1min for reading temp
int numberOfDevices;                      	// Number of temperature devices found
int odoRead = 0;							//Initialising Odo variable

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
OneWire oneWire(ONE_WIRE_BUS);            // Setup instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);      // Pass oneWire reference to Dallas Temperature
DeviceAddress tempDeviceAddress;          // Variable to store a found device address
WiFiClient  client;


void updateNextTime(){						// Function to get delay to next hour trigger
             tempTime =millis() - 10 + (59 - timeClient.getMinutes()) * 60000 + (60 - timeClient.getSeconds())* 1000;
             Serial.println("WIFI Connected!");

    }

void getTempData() {						// Function to get latest temperature readings
          sensors.begin();
          sensors.requestTemperatures(); // Send the command to get temperatures
          numberOfDevices = sensors.getDeviceCount();   // Number of temperature devices found
          // Loop through each device, save to variable
          for(int i=0;i<numberOfDevices; i++){
            // Search the wire for address
            if(sensors.getAddress(tempDeviceAddress, i)){
              readings[i] = sensors.getTempC(tempDeviceAddress);
            }
           }
    }

void writeAllData(){						// Function to writealldata to Thingspeak
          ThingSpeak.setField(1, readings[0]);
          ThingSpeak.setField(2, readings[1]);   
          if (odoRead != 0){
             ThingSpeak.setField(3, odoRead);
          }
          ThingSpeak.setField(4, count);
          int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
          if (httpCode == 200) {
            count = 0;						// Reset counter if write was successful
          }
          else {							// For testing
            //Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));    

          }    
}

void getOdoReading(){
      // Read in last ODO from Thingspeak
      odoRead = ThingSpeak.readFloatField(myChannelNumber, 3);  

      // Check the status of the read operation to see if it was successful
      float statusCode = ThingSpeak.getLastReadStatus();
      if(statusCode == 200){				// For testing
        //Serial.println("ODO is : " + String(odoRead) + " x 10dm3");
      }
      else{
        //Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
      }
}

void setup(){
  //Serial.begin(115200);  // To be used for testing
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();
  timeClient.update();
  pinMode(buttonPin, INPUT_PULLUP);
  ThingSpeak.begin(client);
  sensors.begin();                            // Start up the temperature library
  numberOfDevices = sensors.getDeviceCount(); // Get count of temperature devices
}

void loop() {
  if (tempTime == 0) {						// On startup, to get next hourly update delay
    updateNextTime();
  }

  if (millis() > tempTime) {				// Check if hourly delay has expired
    getTempData();
    updateNextTime();
    writeAllData();
  }

  if (odoRead == 0){						// On startup, get latest Odo reading from Thingspeak
    getOdoReading();
  }

  int reading = digitalRead(buttonPin);       // read the state of the switch into a local variable:
  //Debouce code start
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != buttonState) {
        buttonState = reading;
        if (buttonState == HIGH) {
          timeClient.update();
          count = count + 1;
          odoRead = odoRead + 1;
          }
        }
    }
    lastButtonState = reading;
}

  

 
