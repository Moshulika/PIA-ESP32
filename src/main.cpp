#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <future>

#include <SevSeg.h>

#include <WiFi.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>

#define SERVICE_UUID "ed9fe439-e69f-477c-af32-39de11dddc70"
#define CHARACTERISTIC_UUID "d95a7780-ba9b-408f-b71a-e14046bb02eb"

const String TEAM_ID = "A01";

Adafruit_PCD8544 display = Adafruit_PCD8544(18, 23, 17, 16, 19);
SevSeg sevseg;

const int contrastValue = 60;

bool isScrolling = false;
bool deviceConnected = false;
BLECharacteristic* pCharacteristic = NULL;
BLEServer *pServer = NULL;

std::string receivedValue = "Nimic nou";

/**
 * Callbacks to set the status of the connection
*/
class MyServerCallbacks: public BLEServerCallbacks
{

    void onConnect(BLEServer* pServer)
    {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer)
    {
      deviceConnected = false;
    }

};

/**
 * Initialize all the Bluetooth stuff and starting the service
*/
void initBluetooth()
{

  BLEDevice::init("Edosu's ESP");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |  BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("Nimic nou");

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();

  Serial.println("Bluetooth enabled");

}

/**
 * Try to initialize WiFi
*/
void initWifi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  //Scanam pentru network-uri
  int nr_retele = WiFi.scanNetworks();
  std::string pass = "";

  if(nr_retele == 0)
  {
    Serial.println("Esti in padure"); //No networks were detected
  }
  else
  {

    for(int i = 0; i < nr_retele; i++)
    {

      if(WiFi.status() == WL_CONNECTED) break;

      Serial.print(WiFi.SSID(i) + ", "); //Show available networks

      if(WiFi.SSID(i) == "Grota")
      //if(WiFi.SSID(i) == "B100" || WiFi.encryptionType(i) == WIFI_AUTH_OPEN) //Connect to the desired router (i guess we don't need something more complicated than this) 
      {

        pass = "oPnA1912";

        int x = 0;        
        WiFi.begin(WiFi.SSID(i).c_str(), pass.c_str());

        while(WiFi.status() != WL_CONNECTED && x <= 20)
        {
          Serial.print(".");
          delay(500);
          x++;
        }

      }

    }

    Serial.println(" ");

    if(WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Connection failed");
    }
    else
    {
      Serial.println("Connected to WiFi");
    }

  }



}

/**
 * Fetch data from URL
 * @param url you wish to get the data from
 * @return a string containing the fetched data or a message saying what went wrong
*/
String fetchData(String url)
{

      if(WiFi.status() != WL_CONNECTED) return "Unable to fetch data due to no internet connection";

      HTTPClient c;

      c.begin(url);
      c.setConnectTimeout(30000);
      c.setTimeout(30000);

      int status = c.GET();

      if(status <= 0)
      {
        Serial.println("Error " + status);
        c.end();
        return "Unable to fetch data due to an error";
      }
      else
      {
        String data = c.getString();
        Serial.println(data);
        c.end();
        return data;
      }


}

/**
 * Display default text on the screen and initialize the display
*/
void splashScreen()
{

  display.begin();
  display.setContrast(contrastValue);

  display.clearDisplay();
  display.display();
  delay(1000);

  display.setCursor(0, 1);
  display.setTextColor(BLACK);
  display.println("Mergi ba odata");
  display.display();
  
}

/**
 * Shortly blinks the LED
*/

void blinkLED(int pin, int times)
{

  for(int i = 0; i < times; i++)
  {
    digitalWrite(pin, HIGH);
    delay(300);
    digitalWrite(pin, LOW);
    delay(300);
  }

}

void flashText(String txt)
{

    sevseg.setChars(txt.c_str());
    sevseg.refreshDisplay();
    delay(2000);
    sevseg.setChars("Ok");

}

void initDigits()
{

  byte numDigits = 3;
  byte digitPins[] = {13, 22, 4};
  byte segmentPins[] = {14, 26, 12, 33, 32, 27, 2, 25};
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_CATHODE; // See README.md for options
  bool updateWithDelays = false; // Default 'false' is Recommended
  bool leadingZeros = false; // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false; // Use 'true' if your decimal point doesn't exist or isn't connected
  
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
  updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(90);

  sevseg.setChars("Ok");
  

}

/**
 * Setup the ESP
 * Display the default image on the display
 * Initialize Bluetooth and WiFi
*/
void setup() {

  Serial.begin(115200);
  Serial.println("Sa traiesti, regele meu!");

  pinMode(21, OUTPUT);

  splashScreen();

  initBluetooth();
  initWifi();
  
  blinkLED(21, 3);
  initDigits();

  Serial.println("Setup done");

}

/**
 * Sending the response to the device
 * @param response you wish to send
*/
void sendResponse(std::string response)
{

  pCharacteristic->setValue(response);
  pCharacteristic->notify();

}

/**
 * Callback to change the characteristic when it needs to
*/
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/**
 * Executes commands based on the JSON query it has received from the connected device
 * @param payload the JSON string
 * 
 * This function also handles interaction with the hardware,
 * and at the end sends a JSON encoded response back to the connected device.
*/
void execute(String payload)
{

  StaticJsonDocument<500> doc;
  StaticJsonDocument<500> res;

  DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        sevseg.setChars("Err");
        Serial.print("Invalid JSON: ");
        Serial.println(error.c_str());
        return;
    }

  String action = doc["action"];
  String response;

  if(action == "getLCDs")
  {
    
    res["type"] = "Nokia 5510";
    res["interface"] = "SPI";
    res["resolution"] = "84x48";
    res["id"] = "0";
    res["teamId"] = "A01";

    //flashText("LCD");

  }
  else if(action == "setText")
  {

    display.clearDisplay();
    display.display();

    const char* s;
    
    const ArduinoJson::JsonArray a = doc["text"];

    res["id"] = "0";
    res["teamId"] = "A01";

    JsonArray text = res.createNestedArray("text");

    for (int i = 0; i < a.size(); i++) 
    {
        s = a[i];

        text.add(s);

        display.setCursor(0, i * 10);
        display.println(s);
    }

    display.display();

    //flashText("TXT");

  }
  else if(action == "setImage")
  {

    display.clearDisplay();
    display.display();

    int nr_pixels = 0;

    const char* url = doc["url"];
    String data = fetchData(url);

    DynamicJsonDocument json(24576);

    DeserializationError error = deserializeJson(json, data);

      if (error) {
        sevseg.setChars("Err");
        Serial.print("Invalid JSON: ");
        Serial.println(error.c_str());
        return;
      }    

    int w = json["height"];
    int h = json["width"];

    int x = 0;
    int y = 0;

    for(int i = 0; i < json["data"].size(); i++)
    {
      
      x = json["data"][i]["x"];
      y = json["data"][i]["y"];
 
      display.drawPixel(x, y, BLACK);

      nr_pixels++;

    }

    display.display();

    res["number_pixels"] = nr_pixels;
    res["id"] = 0;
    res["teamId"] = "A01";

    //flashText("IMG");

  }
  else if(action == "scroll")
  {

    const char* text = doc["direction"];
    //flashText("SCR");

    receivedValue = text;

    res["id"] = 0;
    res["direction"] = text;
    res["teamId"] = "A01";

  }
  else if(action == "clearDisplay")
  {
    display.clearDisplay();
    display.display();

    res["id"] = 0;
    res["cleared"] = "true";
    res["teamId"] = "A01";

    //flashText("CLR");
  }
  else return;

  serializeJson(res, response);
  Serial.println(response.c_str());
  sendResponse(response.c_str());

}

/**
 * Function to scroll the screen left, right and to stop scrolling
 * @param dir direction to scroll. Choose from enum('Left', 'Right', 'Off')
*/
void scroll(std::string dir)
{

  if(dir != "Left" && dir != "Right") 
  {
    isScrolling = false;
    return;
  }

  for(int i = 0; i < 16; i++)
  {

    if(dir == "Off") 
    {
      isScrolling = false;
      break;
    }

    if(dir == "Left")
    {
      display.scroll(-1, 0);
    }
    
    if(dir == "Right")
    {
      display.scroll(1, 0);
    }

    display.display();
    isScrolling = true;
    delay(100);
    

  }

}

void loop() {

    if(deviceConnected)
    {

      sevseg.setChars("CON");

      if(pCharacteristic->getValue().find("action") != -1)
      {

      if(receivedValue != pCharacteristic->getValue())
      {

        receivedValue = pCharacteristic->getValue();

        if(receivedValue.length() > 0)
        {
          
          blinkLED(21, 3);

          Serial.println(receivedValue.c_str());

          execute(receivedValue.c_str());

        }

      }

      }

      scroll(receivedValue);

    }

    if(isScrolling)
    {
      sevseg.blank();
    }
 
    sevseg.refreshDisplay();
    

}
