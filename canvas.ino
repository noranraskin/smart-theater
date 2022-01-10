#include <WiFi.h>
#include <HTTPClient.h>
#include "CREDENTIALS.h"
#include "helpers.h"

// Change these two numbers to the pins connected to your encoder.
int encoderA = 12; // Green
int encoderB = 14; // Yellow
// Change this number to the pin connected to the end stop.
int endstop = 27;
// Change these numbers to the pins connected to your relays.
// VERY IMPORTANT TO NOT GET THESE WRONG
// RISK OF SHORTING THE POWER SUPPLY
int relay1 = 5;
int relay2 = 17;
int relay3 = 16;

////////////////
int updated = 0;
int turns = 0;
int state = 0;

const char* HOSTNAME = "Canvas";
const char* PROJECTOR = "Beamer";

// For HTTP request
String header;
const int httpPort = 80;
WiFiServer server(httpPort);
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;

TaskHandle_t wifi;
TaskHandle_t motor;

void setup() {
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(encoderA), READ_TURN, CHANGE);
  attachInterrupt(digitalPinToInterrupt(endstop), READ_ENDSWITCH, CHANGE);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);

  WiFi.setHostname(HOSTNAME);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("Connected!");
  server.begin();
}

// void loop() {

// }

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void turn_up() {
  digitalWrite(relay1, LOW);
  digitalWrite(relay3, LOW);
  digitalWrite(relay2, HIGH);
}

void turn_down() {
  digitalWrite(relay2, LOW);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay1, HIGH);
}


void READ_TURN() {
  // Interrupt function to read the x2 pulses of the encoder.
  if (digitalRead(encoderB) == 0) {
    if (digitalRead(encoderA) == 0) {
      // A fell, B is low
      turns--;
    } else {
      // A rose, B is high
      turns++;
    }
  } else {
    if (digitalRead(encoderA) == 0) {
      turns++;
    } else {
      // A rose, B is low
      turns--;
    }
  }
  updated = 1;
}

void READ_ENDSWITCH() {
  
}