#include <FirebaseArduino.h>
#include <ESP8266WiFi.h>

// Set these to run example.
#define FIREBASE_HOST "YOUR_FIREBASE_HOST"
#define FIREBASE_AUTH "YOUR_FIREBASE_AUTH"
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define pinLock 4
#define pinSwitch 16

const String DOOR = "YOUR_DOOR_NAME";

int state = 0;
int unlocking = -1;

int valSwitch = 1;
int valSwitchOld = 1;

String logId = "";

StaticJsonBuffer<200> jsonBuffer;
JsonObject& data = jsonBuffer.createObject();
JsonObject& root = jsonBuffer.createObject();

void(* resetFunc) (void) = 0;

bool CheckIsFireBaseFailed(String message)
{
  if (Firebase.failed()) {
    Serial.println(message);
    Serial.println(Firebase.error());
    return true;  
  }
  return false;      
}


void setup() {
  Serial.begin(9600);

  pinMode(pinLock, OUTPUT);
  pinMode(pinSwitch, INPUT);

  // connect to wifi.
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    count++;
    // reset if not connected in 10 seconds (20 * 500ms)
    if ((count % 20) == 0)
      resetFunc();
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  data[".sv"] = "timestamp";
  root["opened"] = data;

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  
  int countReset = Firebase.getInt("/" + DOOR + "/count");
  if (CheckIsFireBaseFailed("Firebase error in setup: getInt") == false){
    countReset++;
    Firebase.setInt("/" + DOOR + "/count", countReset);
    CheckIsFireBaseFailed("Firebase error in setup: setInt");    
  }
}



void loop() {
   if (WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi error");
    Serial.println(WiFi.status());
    // reset Arduino
    resetFunc();
  }

  state = Firebase.getInt("/" + DOOR + "/State/unlock");
  CheckIsFireBaseFailed("Firebase error: getInt");
    
  if (unlocking == -1 && state == 1){
    digitalWrite(pinLock, HIGH);
    delay(100);
    unlocking = 1;
  }

  if (unlocking > 0){
    unlocking++;
  }

  // if there is estimated 20 seconds (40 * 500 ms loop delay) from activating lock or door are closed 
  if ((state == 1) && (((unlocking % 40) == 0) || (valSwitch == 1))){
    digitalWrite(pinLock, LOW);
    delay(100);
    Firebase.setInt("/" + DOOR + "/State/unlock", 0);
    if (CheckIsFireBaseFailed("Firebase error: setInt") == false){
      state = 0;
      unlocking = -1;
    }else{
      unlocking--; 
      state = 1; // wrong read from firebase is setting state to 0
    }
  }
 
  valSwitch = digitalRead(pinSwitch);
  
  if (valSwitch != valSwitchOld)
  {
    if (valSwitch == 1)
    {
      // door are opened
      logId = Firebase.push("/" + DOOR + "/Log", root);
      CheckIsFireBaseFailed("Firebase error: push");
    }
    else if (logId != "")
    {
      // door are closed
      Firebase.set("/" + DOOR + "/Log/" + logId + "/closed", data);
      CheckIsFireBaseFailed("Firebase error: set");
    }
  }
  valSwitchOld = valSwitch;
    
  delay(500);
}