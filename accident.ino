#include <Arduino_LSM6DS3.h>                    // include the library for LSM6DS3 accelerometer sensor
#include <TinyGPS++.h>                          //Library to extract lattiude and longtitude from NMEA format output of GPS .
#include <WiFiNINA.h>                           //Library for wifi-connection 
#include<ArduinoJson.h>                         ///Library to parse Json Data
TinyGPSPlus gps;                                //initialising the gps object .

WiFiSSLClient client; 
WiFiServer server(80);                          //Initialising wifi server

String hospitalsdata = "hospitals ";
String mapsLink ;               
float latitude  ;
float longtitude ;

void setup() {
  Serial.begin(115200);                        // initialize serial communication with baud rate 115200
  Serial1.begin(9600);
  char ssid[] = "----";      
  char pass[] = "-----";
  // initialize serial1 communication with baud rate 9600
  int status = WL_IDLE_STATUS;  

   while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }

  // if you get a connection, print WiFi status:
  Serial.println("Connected to WiFi");
  printWiFiStatus(ssid);
  // start the server
  server.begin();
  
  pinMode(9, INPUT_PULLUP);
  pinMode(4, OUTPUT);                                 // Set the buzzer pin as an output
  
  if (!IMU.begin()) {                                 // start the accelerometer sensor
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
}

void loop() {
   int buttonstate = 0;         // variable for reading the pushbutton status
 
     while (Serial1.available())                //while GPS is sending the data .
  { 
    if (gps.encode(Serial1.read()))
    {                                         // If valid data is received from GPS module, extract latitude and longitude
      if (gps.location.isUpdated())
      {
          latitude = gps.location.lat();
          longitude = gps.location.lng();
                                              // Construct Google Maps link                                       
        mapsLink = "https://www.google.com/maps?q=@" + String(latitude, 6) + "," + String(longitude, 6);
                      
        Serial.println(mapsLink);           //Print the Google Maps link to serial monitor
      }
    }
    else{
      Serial.println("Encoding the GPS Data....");
    }
  }
  Serial.println(mapsLink);
  float x, y, z;                            // variables to store accelerometer readings

                                            // read accelerometer values from the sensor
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);          // store the readings in x, y, and z variables
  }

  float net = sq(x*x+y*y+z*z);
  
  if(net > 1.75){                            //dummy thershold value is as 1.5 for testing phase , this can be manipulated accordingly 
//if the acc magnitude is greater than the Threshold value of crash acceleration 

  Serial.println("Crash Detected");
  digitalWrite(4, HIGH);                    // Turn the buzzer on
  delay(1000);                              
  digitalWrite(4, LOW);                     // Turn the buzzer off

    buttonstate = digitalRead(9);       //Reading the push button state from pin 4
    delay(10000);
    
    if(buttonstate != HIGH){
    SendMessage();
    delay(2000);
    MakeCall();  
    fetch_hospitals();
    delay(2000);
    SendMessage1();             
    hospitalsdata = "";
    delay(20000);                           // Wait for 20 seconds
  }
    else
    {
      Serial.println("SAFE , Thank GOD !");
    }
} 
  delay(100);                                     // wait for 100 milliseconds before taking the next reading
}


 void SendMessage()   
  {
  String mapsLink1 = "Vehicle registered by  +919885061950 just encountered an accident at - ";
  mapsLink1 += "\n";
  mapsLink1 += mapsLink;
     
  Serial.println ("Sending Message");   
  Serial1.println("AT+CMGF=1");                   //Sets the GSM Module in Text Mode   
  delay(5000);   
  Serial.println ("Setting SMS Number");   
  Serial1.println("AT+CMGS=\"$$$$$$$$$\"\r"); //Write Mobile number to send message  , do include the country code
  delay(1000);   
  Serial.println ("Setting SMS Content");
  Serial1.println(mapsLink1);                      // Messsage content   
  delay(100);   
  Serial.println ("Finish");   
  Serial1.println((char)26);                      // ASCII code of CTRL+Z   
  delay(1000);   
  Serial.println ("Message has been sent");   
  }  

 void SendMessage1()   
  {     
  Serial.println ("Sending Message");   
  Serial1.println("AT+CMGF=1");                   //Sets the GSM Module in Text Mode   
  delay(5000);   
  Serial.println ("Setting SMS Number");   
  Serial1.println("AT+CMGS=\"$$$$$$$$$$$$\"\r"); //Write Mobile number to send message   
  delay(1000);   
  Serial.println ("Setting SMS Content");
  Serial1.println(hospitalsdata);                      // Messsage content   
  Serial.println(hospitalsdata);
  delay(100);   
  Serial.println ("Finish");   
  Serial1.println((char)26);                      // ASCII code of CTRL+Z   
  delay(1000);   
  Serial.println ("Message has been sent");   
  } 


void MakeCall()
{
  Serial.println("Making a Call ");
  Serial1.println("AT+CMGS=0");                   //Unset the Gsm module from text mode
  delay(5000);
  Serial1.println("ATD$$$$$$$;");               //include the phone number instead of $$
  delay(1000);                                  //Wait for 10 seconds to connect the call and then disconnect 
//  Serial1.println("ATH");                     //ATH command to cut the call 
  Serial.println("Succesfully Notified Via a call ");
}


void fetch_hospitals(){
   if (WiFi.status() == WL_CONNECTED) {
    Serial.println("API");
    if (client.connect("browse.search.hereapi.com", 443)) {
      client.println("GET /v1/browse?at=" + String(latitude,6)+","+ String(longtitude, 6)+"&limit=3&categories=800-8000-0000&apiKey=X62ZFkTVetzJyRyb5kgM-9qVlcjwJ5-Cqex6gSq56Ig HTTP/1.1");
      client.println("Host: browse.search.hereapi.com");
      client.println("Connection: close");
      client.println();
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
      }
      
     while (client.peek() != '{')
      {
            client.read();
      }

      // Read the JSON response
      String payload = "";
      while (client.available()) {
         char c = client.read();
         payload += c;
      }

      // Remove the extra "0" at the end ( this is for the cases when we get some extra 0's in the json response);
      
      payload.trim();
      if (payload.endsWith("0")) {
        payload.remove(payload.length() - 1);
      }

      Serial.println(payload); // Add this line to print the payload
      delay(2000);
      Serial.println("parsing");
      parsee(payload);
      Serial.println("Parsed");
      client.stop();
    }
  }
  else{
     Serial.println("wifi not connected");
    }
  delay(2000);
}


void parsee(String json){
  Serial.println("entered the parsing stage");
  DynamicJsonDocument doc(5000);
  Serial.println("DynamicJsonDocument");
  delay(1000);
  DeserializationError error=  deserializeJson(doc, json);
 
  Serial.println("DeserializationError");
  delay(1000);
  
   if (error) {
    Serial.println("Failed to parse JSON");
    return ;
  }
// Get the array of hospitals from the JSON response
  JsonArray hospitals = doc["items"];

  // Iterate over the array of hospitals
  
  for (JsonObject hospital : hospitals) {
    // Get the name of the hospital
    String name = hospital["title"].as<String>();

    // Get the address of the hospital
    JsonObject address = hospital["address"];
    String street = address["street"].as<String>();
    String subdistrict = address["subdistrict"].as<String>();
    String district = address["district"].as<String>();
    String city = address["city"].as<String>();
    String state = address["state"].as<String>();
    String postalCode = address["postalCode"].as<String>();
    String fullAddress = street + ", " + subdistrict + ", " + district + ", " + city + ", " + state + " " + postalCode;

    // Get the phone number of the hospital
    String phoneNumber = "";
    
    Serial.println("Bw the Parsing");
    
    if (hospital["contacts"].size() > 0) {
      JsonArray phoneNumbers = hospital["contacts"][0]["phone"];
      int cnt = 0;
      if (phoneNumbers.size() > 0) {
        while(cnt < phoneNumbers.size()){
          phoneNumber += phoneNumbers[cnt]["value"].as<String>();
          phoneNumber += " , " ;
          cnt++;
        }
        
      }
    }

    
    // Print the name, address, and phone number of the hospital
    Serial.println("Hospital: " + name);
    Serial.println("Address: " + fullAddress);
    Serial.println("Phone: " + phoneNumber);
    Serial.println();
    
    Serial.println(phoneNumber.length());
    delay(1000);
    if(phoneNumber.length()>0){
      hospitalsdata += "Name:- " + name + "\n";
      hospitalsdata += "Adress:- " + fullAddress + "\n";
      hospitalsdata += "Phone number:- " + phoneNumber + "\n";
      hospitalsdata += "\n";
      hospitalsdata += "\n";
    }
    
    Serial.println();
    Serial.println("DONE Parsing");
    Serial.println();
  } 
    Serial.println("go to display");
    delay(1000);
    displayy();
    Serial.println("done displaying");
    hospitalsdata = "";
}

//display function to display the accident and nearest hospitals details on server 
void displayy(){
  Serial.println("displaying");
  delay(1000);
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");

    // read the HTTP request
    String request = client.readStringUntil('\r');
    Serial.println(request);

    // send a response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<html><body><h1>" + hospitalsdata +  "</h1></body></html>");

    // close the connection
    client.stop();
    Serial.println("Client disconnected");
  }else{
    Serial.println("Server Unavailable");
  }

  Serial.println("displaying....");
}


void printWiFiStatus(char ssid[]) {
  // print the SSID of the network you're connected to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// comment added