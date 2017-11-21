#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal_I2C.h>

#define DHTTYPE DHT22

DHT_Unified viv1LeftSensor(D5, DHTTYPE);
DHT_Unified viv1RightSensor(D6, DHTTYPE);
DHT_Unified viv2LeftSensor(D7, DHTTYPE);
DHT_Unified viv2RightSensor(D8, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 20, 4);

const char *ssid = "SKY8368B";
const char *password = "AWFPXTQV";

WiFiServer server(80);

// this will be refactored out
float xenaLeftTemp = 0;
float xenaRightTemp = 0;
float xenaLeftHumid = 0;
float xenaRightHumid = 0;

float hanzoLeftTemp = 0;
float hanzoRightTemp = 0;
float hanzoLeftHumid = 0;
float hanzoRightHumid = 0;

const char *configFilename = "/VivariumConfig.json";
const char *webPageFilename = "/index.htm";

// connect to wifi
void connectToWifi()
{
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    server.begin();

    // Print the IP address
    Serial.print("http://");
    Serial.print(WiFi.localIP());
}

// get sensor readings and set variables (To be refactored)
void setSensorReadings()
{
    // create reusable sensor event
    sensors_event_t event;
    Serial.println("Sensor Readings");

    viv1LeftSensor.temperature().getEvent(&event);
    xenaLeftTemp = event.temperature;
    Serial.print(xenaLeftTemp, 1);
    Serial.print("C ");

    viv1LeftSensor.humidity().getEvent(&event);
    xenaLeftHumid = event.relative_humidity;
    Serial.print(xenaLeftHumid, 0);
    Serial.print("% ");

    // Xena right sensor
    viv1RightSensor.temperature().getEvent(&event);
    xenaRightTemp = event.temperature;
    Serial.print(xenaRightTemp, 1);
    Serial.print("C ");

    viv1RightSensor.humidity().getEvent(&event);
    xenaRightHumid = event.relative_humidity;
    Serial.print(xenaRightHumid, 0);
    Serial.print("% ");

    // Hanzo left sensor
    viv2LeftSensor.temperature().getEvent(&event);
    hanzoLeftTemp = event.temperature;
    Serial.print(hanzoLeftTemp, 1);
    Serial.print("C ");

    viv2LeftSensor.humidity().getEvent(&event);
    hanzoLeftHumid = event.relative_humidity;
    Serial.print(hanzoLeftHumid, 0);
    Serial.print("% ");

    // Hanzo right sensor
    viv2RightSensor.temperature().getEvent(&event);
    hanzoRightTemp = event.temperature;
    Serial.print(hanzoRightTemp, 1);
    Serial.print("C ");

    viv2RightSensor.humidity().getEvent(&event);
    hanzoRightHumid = event.relative_humidity;
    Serial.print(hanzoRightHumid, 0);
    Serial.println("%\n");

    // No Io sensors. These will be called from another web server in future version
}

// print sensor readings to 20x4 LCD screen (To be refactored)
void printSensorReadings()
{
    // degree symbol is 0xdf or 223 decimal or 337 octal or 377 octal
    String line1 = "Xena  L: " + String(xenaLeftTemp, 1) + "\337C " + String(xenaLeftHumid, 0) + "%";
    String line2 = "      R: " + String(xenaRightTemp, 1) + "\337C " + String(xenaRightHumid, 0) + "%";
    String line3 = "Hanzo L: " + String(hanzoLeftTemp, 1) + "\337C " + String(hanzoLeftHumid, 0) + "%";
    String line4 = "      R: " + String(hanzoRightTemp, 1) + "\337C " + String(hanzoRightHumid, 0) + "%";

    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    lcd.setCursor(0, 2);
    lcd.print(line3);
    lcd.setCursor(0, 3);
    lcd.print(line4);
}

String getWebPage()
{
    String webString = "";

    // open the file in read mode
    File file = SPIFFS.open(configFilename, "r");
    int fileSize = file.size();

    // create empty object to return
    StaticJsonBuffer<1> jsonBuffer;
    JsonObject &empty = jsonBuffer.createObject();

    if (!file || file.size() == 0)
    {
        Serial.println("File not found or is empty");
        return "";
    }
    else
    {
        //print filename and size
        Serial.print("Found ");
        Serial.print(file.name());
        Serial.print(" (");
        Serial.print(fileSize);
        Serial.println(" bytes)\n");

        // read file and put json in char buffer
        char json[2000];
        file.readBytes(json, fileSize);

        // parse json object
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(json);
        if (!root.success())
        {
            Serial.println("parseObject() failed");
            return "";
        }

        file.close();

        // print object
        Serial.println("JSON Object");
        root.prettyPrintTo(Serial);
        Serial.println(" ");

        File indexFile = SPIFFS.open(webPageFilename, "r");
        if (!indexFile)
        {
            Serial.println("File not found");
            return "";
        }
        else
        {
            //print filename and size
            Serial.print("Found ");
            Serial.print(indexFile.name());
            Serial.print(" (");
            Serial.print(indexFile.size());
            Serial.println(" bytes)\n");
        }

        webString += "HTTP/1.1 200 OK";
        webString += '\n';
        webString += "Content-Type: text/html";
        webString += '\n';

        while (indexFile.available())
        {
            webString += indexFile.readStringUntil('\n');
            webString += '\n';
        }

        indexFile.close();

        //Replace all placeholders
        webString.replace("#XenaName#", root["vivariums"][0]["name"]);
        webString.replace("#XenaType#", root["vivariums"][0]["type"]);
        webString.replace("#XenaImage#", root["vivariums"][0]["image"]);
        webString.replace("#XenaLeftTemp#", String(xenaLeftTemp, 1));
        webString.replace("#XenaRightTemp#", String(xenaRightTemp, 1));
        webString.replace("#XenaMinTemp#", root["vivariums"][0]["tempRange"][0]);
        webString.replace("#XenaMaxTemp#", root["vivariums"][0]["tempRange"][1]);
        webString.replace("#XenaLeftHumid#", String(xenaLeftHumid, 0));
        webString.replace("#XenaRightHumid#", String(xenaRightHumid, 0));
        webString.replace("#XenaMinHumid#", root["vivariums"][0]["humidRange"][0]);
        webString.replace("#XenaMaxHumid#", root["vivariums"][0]["humidRange"][1]);

        webString.replace("#HanzoName#", root["vivariums"][1]["name"]);
        webString.replace("#HanzoType#", root["vivariums"][1]["type"]);
        webString.replace("#HanzoImage#", root["vivariums"][1]["image"]);
        webString.replace("#HanzoLeftTemp#", String(hanzoLeftTemp, 1));
        webString.replace("#HanzoRightTemp#", String(hanzoRightTemp, 1));
        webString.replace("#HanzoMinTemp#", root["vivariums"][1]["tempRange"][0]);
        webString.replace("#HanzoMaxTemp#", root["vivariums"][1]["tempRange"][1]);
        webString.replace("#HanzoLeftHumid#", String(hanzoLeftHumid, 0));
        webString.replace("#HanzoRightHumid#", String(hanzoRightHumid, 0));
        webString.replace("#HanzoMinHumid#", root["vivariums"][1]["humidRange"][0]);
        webString.replace("#HanzoMaxHumid#", root["vivariums"][1]["humidRange"][1]);

        // Io not used. Will be included in future version
        webString.replace("#IoName#", root["vivariums"][2]["name"]);
        webString.replace("#IoType#", root["vivariums"][2]["type"]);
        webString.replace("#IoImage#", root["vivariums"][2]["image"]);
        webString.replace("#IoLeftTemp#", "-");
        webString.replace("#IoRightTemp#", "-");
        webString.replace("#IoMinTemp#", root["vivariums"][2]["tempRange"][0]);
        webString.replace("#IoMaxTemp#", root["vivariums"][2]["tempRange"][1]);
        webString.replace("#IoLeftHumid#", "-");
        webString.replace("#IoRightHumid#", "-");
        webString.replace("#IoMinHumid#", root["vivariums"][2]["humidRange"][0]);
        webString.replace("#IoMaxHumid#", root["vivariums"][2]["humidRange"][1]);
    }

    return webString;
}

void setup()
{
    Serial.begin(115200);

    Serial.println("Connecting to wifi");
    WiFi.begin(ssid, password);
    connectToWifi();
    Serial.println("Connected");

    // turn on LCD
    lcd.init();
    lcd.backlight();

    // get sensor readings and print on LCD
    //setSensorReadings(); // need sensors!
    printSensorReadings();

    // mount file system
    SPIFFS.begin();
    //SPIFFS.format();

    // start sensors
    viv1LeftSensor.begin();
    viv1RightSensor.begin();
    viv2LeftSensor.begin();
    viv2RightSensor.begin();
}

void loop()
{
    // get sensor readings and print on LCD
    //setSensorReadings();
    printSensorReadings();

    // Check if a client has connected
    WiFiClient client = server.available();
    if (client)
    {
        // Wait until the client sends some data
        Serial.println("new client");

        while (!client.available())
        {
            delay(1);
        }

        // Read the first line of the request
        String request = client.readStringUntil('\r');
        Serial.println(request);
        client.flush();

        // get web page and return
        String pageContent = getWebPage();
        client.print(pageContent);

        /* this last part returns only part of the page
         * the client.print method has a limit on the string
         * that is passed to it
         */
    }
}