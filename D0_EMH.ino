/*
IR LED: 
BPW40 520...950nm
Cathode GND, Anode RX
short-circuit current is 5 mA. Even at 5V it's max 25mW. BPW40 can handle up to 150mW / 100mA
*/
#include <ESP8266WiFi.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_WIFI_PASS";

// emoncoms config
const char* emoncmsKey = "YOUR_EMON_API_KEY";
const char* host = "emoncms.org";
String nodeIdCount = "HomeCount"; 
String nodeIdEnergy = "HomeEnergy";


// Buffer for serial reading 
int  serIn;             // var that will hold the bytes-in read from the serialBuffer
byte datagram[1000];    // array that will hold the different bytes 
int  serindex = 0;      // index of serInString[] in which to insert the next incoming byte
int  serialbyte;        // Byte to store serial input data

//  Blink Function for Feedback of Status, takes about 1000ms
void blink(int count, int durationon, int durationoff, int delayafter) {
  for (int i=0; i < count; i++) {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on 
    delay(durationon);
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(durationoff);
  }
  delay(delayafter);
}

void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "0x%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}

unsigned int hexToDec(String hexString) {
  
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15); 
    decValue = (decValue * 16) + nextInt;
  }  
  return decValue;
}

void sendToEmonCMS(String nodeId, String data) {
  Serial.println("S");   
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/input/post.json?node=";
  url += nodeId;
  url += "&apikey=";
  url += emoncmsKey;
  url += "&csv=";
  url += data;

  //Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);

  // Read all the lines of the reply from server
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line);
  }
}

// Wifi
void wifiSetup() {
  delay(10);
  Serial.print("Start WiFi to SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.softAPdisconnect(false);
  WiFi.enableAP(false);
  while (WiFi.status() != WL_CONNECTED) {
    blink(5,50,100,2000);
    WiFi.begin(ssid, password);
    Serial.print(".");
  }
  // Report Data to Serial Port. 
  Serial.print("Connect with IP:");
  Serial.println(WiFi.localIP());
}

//   SETUP running once at the beginning
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
    blink(1,5000,500,1000);   // Signal startup
  Serial.begin(9600);   // Open serial communications and wait for port to open:
  while (!Serial) {
    ; // wait for serial port to connect. 
  }
  Serial.println("");
  Serial.println("ESP8266-DO-Logger init");

  wifiSetup();

  blink(6,100,500,2000);     
  Serial.println("INIT Done");
}


void loop() {
  //Serial.print("LOOP: ");          // Send some startup data to the console
  //Serial.print("SSID: ");               // SSID of Network
  //Serial.print(WiFi.SSID());            // SSID ausgeben
  //Serial.print("     IP Address: ");    // assigned IP-Address
  //IPAddress ip = WiFi.localIP();        // IP Adresse auslesen
  //Serial.println(ip);                   // IP Adresse ausgeben
  //
  // Clear Serial Data
  //
  //Serial.println("C"); 
  while (Serial.available()) {
    while (Serial.available()) {
      serialbyte = Serial.read(); //Read Buffer to clear
    }
    delay(10);  // wait approx 10 bytes at 9600 Baud to clear bytes in transmission
  }

  Serial.println("W"); 
  int flashcount = 0;
  while (!Serial.available()){
    flashcount++;
    if (flashcount == 400) { 
      digitalWrite(LED_BUILTIN, LOW);
    }
    else if (flashcount > 500) {
      flashcount=0;
    }
    else {
      delay(5);  // wait 5 ms for new packets
    }
  }

  Serial.setTimeout(500);   // Set Timeout to 500ms.
  serindex = Serial.readBytes(datagram,1000);

  if (serindex < 1000) {
    Serial.println("R");   

    //Serial.println("DEBUG");
    char fullHex[50] = {0};
    sprintf(fullHex,"%02X%02X%02X%02X",datagram[177],datagram[178],datagram[179],datagram[180]);
    //Serial.println(fullHex);
    //Serial.println(hexToDec(fullHex));
    int decPower = hexToDec(fullHex);
    float Stand = decPower/10000.0;
    Serial.print(Stand, 6); Serial.println(" kWh"); 
    char charPower[10];
    dtostrf(Stand,7, 4, charPower);
    

    char fullWirk[50] = {0};
    sprintf(fullWirk,"%02X%02X%02X%02X",datagram[218],datagram[219],datagram[220],datagram[221]);
    //Serial.println(fullWirk);
    //Serial.println(hexToDec(fullWirk));
    int decWirk = hexToDec(fullWirk);
    float StandWirk = decWirk/10.0;
    char charWirk[10];
    dtostrf(StandWirk,7, 4, charWirk);
    Serial.print(StandWirk, 2); Serial.println(" W");
    //Serial.println("DATA END");
   
    sendToEmonCMS(nodeIdCount, charPower);
    sendToEmonCMS(nodeIdEnergy, charWirk);
  }
  else {
  } 

}
