/*
  WiFi Web Server LED Blink

  A simple web server that lets you blink an LED via the web.
  This sketch will create a new access point (with no password).
  It will then launch a new server and print out the IP address
  to the Serial Monitor. From there, you can open that address in a web browser
  to turn on and off the LED on pin 13.

  If the IP address of your board is yourAddress:
    http://yourAddress/H turns the LED on
    http://yourAddress/L turns it off

  created 25 Nov 2012
  by Tom Igoe
  adapted to WiFi AP by Adafruit

  Find the full UNO R4 WiFi Network documentation here:
  https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples#access-point
 */

#include "WiFiS3.h"
#include "Keyboard.h"
#include "URLCode.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "Rbr5ucKy";        // your network SSID (name)
char pass[] = "passphrase";        // your network password (use for WPA, or use as key for WEP)

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;

WiFiServer server(80);
URLCode urlc;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  Serial.println("Access Point Web Server");

  pinMode(led, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // by default the local IP address will be 192.168.4.1
  // you can override it with the following:
  WiFi.config(IPAddress(192,168,5,100));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    Serial.println(status);
    // don't continue
    while (true);
  }

  delay(5000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

  Keyboard.begin(KeyboardLayout_sv_SE);
  delay(1000);

  Serial.println("Started keyboard!");
}


void loop() {
  
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
  
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    String currentLine = "";                // make a String to hold incoming data from the client
    String cmd = "";
    bool cmd_done = false;
    while (client.connected()) {            // loop while the client's connected
      delayMicroseconds(10);                // This is required for the Arduino Nano RP2040 Connect - otherwise it will loop so fast that SPI will never be served.
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<form method=\"get\" action=\"/s\"><input type=\"text\" name=\"t\"></input><button type=\"submit\">Submit</button></form>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.startsWith("GET /s?t=") && !cmd_done) {
          if (c == ' ') {
            cmd_done = true;
            urlc.urlcode = cmd.substring(1);
            urlc.urldecode();
            cmd = urlc.strcode;
            processCommand(cmd);
          }
          if (c == '+') {
            cmd += ' ';
          } else {
            cmd += c;
          }
        }
      }
    }
    client.stop();
  }
}

void processCommand(String cmd) {
  Serial.print("Sending command: ");
  Serial.println(cmd);


  if (cmd.startsWith("STRING")) {
    sendString(cmd.substring(7));
    return;
  } 

  char mods[3];
  int mods_count = parseModifiers(&cmd, mods);
  Serial.println(cmd);

  if (cmd == "SPACE" ) {
    sendSpecialMod(' ', mods, mods_count);
    return;
  } 

  if (cmd == "RETURN" || cmd == "ENTER" ) {
    sendSpecialMod(KEY_RETURN, mods, mods_count);
    return;
  } 

  if (cmd == "ESCAPE") {
    sendSpecialMod(KEY_ESC, mods, mods_count);
    return;
  }

  if (cmd == "TAB") {
    sendSpecialMod(KEY_TAB, mods, mods_count);
    return;
  } 

  if (mods_count <= 0) {
    Serial.println("Unknown command");
    return;
  }

  sendStringMod(cmd, mods, mods_count);
}

int parseModifiers(String *cmd, char mods[]) {
  int count = 0;
  char str[cmd->length() + 1];
  cmd->toCharArray(str, cmd->length() + 1);

  char *token = strtok(str, " ");
  *cmd = "";
  Serial.println("Tokens:");
  while (token != NULL) {
    Serial.println(token);
    if (strcmp(token, "CTRL") == 0) {
      mods[count++] = KEY_LEFT_CTRL;
    } else if (strcmp(token, "ALT") == 0) {
      mods[count++] = KEY_LEFT_ALT;
    } else if (strcmp(token, "GUI") == 0) {
      mods[count++] = KEY_LEFT_GUI;
    } else if (strcmp(token, "SHIFT") == 0) {
      mods[count++] = KEY_LEFT_SHIFT;
    } else if (cmd->length() > 0) {
      *cmd = String(*cmd) + " " + String(token);
    } else {
      *cmd = String(token);
    }
    token = strtok(NULL, " ");
  }
  Serial.println("Done tokens");
  return count;
}

void sendSpecial(char c) {
  Keyboard.press(c);
  delay(10);
  Keyboard.releaseAll();
  delay(10);
}

void sendSpecialMod(char cmd, char modifier[], int mod_count) {
  for (int i = 0; i < mod_count; i++) {
    Keyboard.press(modifier[i]);
    delay(10);
  }
  sendSpecial(cmd);
  for (int i = 0; i < mod_count;i++) {
    Keyboard.release(modifier[i]);
    delay(10);
  }
}

void sendString(String str) {
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    Keyboard.press(c);
    delay(10);
    Keyboard.release(c);
    delay(10);
  }
}

void sendStringMod(String cmd, char modifier[], int mod_count) {
  for (int i = 0; i < mod_count; i++) {
    Keyboard.press(modifier[i]);
    delay(10);
  }
  sendString(cmd);
  for (int i = 0; i < mod_count;i++) {
    Keyboard.release(modifier[i]);
    delay(10);
  }
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}

