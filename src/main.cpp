#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

DHT viv1LeftSensor;
DHT viv1RightSensor;
DHT viv2LeftSensor;
DHT viv2RightSensor;

float viv1LeftTemp = 0;
float viv1RightTemp = 0;
float viv1LeftHumid = 0;
float viv1RightHumid = 0;

float viv2LeftTemp = 0;
float viv2RightTemp = 0;
float viv2LeftHumid = 0;
float viv2RightHumid = 0;

WiFiServer server(80);

const char *ssid = "SKY8368B";
const char *password = "AWFPXTQV";
const char *configFilename = "/VivariumConfig.json";
const char *webPageFilename = "/index.htm";

// connect to wifi
void connectToWifi()
{
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // start web server
    server.begin();
    Serial.println("http://" + WiFi.localIP());
}

// get sensor readings and set variables
void setSensorReadings()
{
    // Vivarium 1 left sensor
    viv1LeftTemp = viv1LeftSensor.getTemperature();
    Serial.println(viv1LeftTemp);

    viv1LeftHumid = viv1LeftSensor.getHumidity();
    Serial.println(viv1LeftHumid);

    // Vivarium 1 right sensor
    viv1RightTemp = viv1RightSensor.getTemperature();
    Serial.println(viv1RightTemp);

    viv1RightHumid = viv1RightSensor.getHumidity();
    Serial.println(viv1RightHumid);

    // Vivarium 2 left sensor
    viv2LeftTemp = viv2LeftSensor.getTemperature();
    Serial.println(viv2LeftTemp);

    viv2LeftHumid = viv2LeftSensor.getHumidity();
    Serial.println(viv2LeftHumid);

    // Vivarium 3 right sensor
    viv2RightTemp = viv2RightSensor.getTemperature();
    Serial.println(viv2RightTemp);

    viv2RightHumid = viv2RightSensor.getHumidity();
    Serial.println(viv2RightHumid);

    // Additional sensor readings can be added here
}

// print sensor readings to 20x4 LCD screen
// need to replace snake names with config values
void printSensorReadings()
{
    // \337 is the degree symbol
    lcd.setCursor(0, 0);
    lcd.print("Xena  L: " + String(viv1LeftTemp, 1) + "\337C " + String(viv1LeftHumid, 0) + "%");
    lcd.setCursor(0, 1);
    lcd.print("      R: " + String(viv1RightTemp, 1) + "\337C " + String(viv1RightHumid, 0) + "%");
    lcd.setCursor(0, 2);
    lcd.print("Hanzo L: " + String(viv2LeftTemp, 1) + "\337C " + String(viv2LeftHumid, 0) + "%");
    lcd.setCursor(0, 3);
    lcd.print("      R: " + String(viv2RightTemp, 1) + "\337C " + String(viv2RightHumid, 0) + "%");
}

// generate web page and send to client (need to refactor the config out of this)
void sendWebPage(WiFiClient client)
{
    // open the file in read mode
    File configFile = SPIFFS.open(configFilename, "r");
    if (!configFile)
    {
        Serial.println("File not found or is empty");
        return;
    }

    //print filename
    Serial.print("Found ");
    Serial.print(configFile.name());

    // read file and put json in char buffer
    char json[2000];
    configFile.readBytes(json, configFile.size());
    configFile.close();

    // parse json object
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(json);
    if (!root.success())
    {
        Serial.println("parseObject() failed");
        return;
    }
    // print object
    //root.prettyPrintTo(Serial);

    File indexFile = SPIFFS.open(webPageFilename, "r");
    if (!indexFile)
    {
        Serial.println("File not found");
        return;
    }

    //print filename
    Serial.print("Found ");
    Serial.print(indexFile.name());

    // wait for client and then send response
    client.flush();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");

    while (indexFile.available())
    {
        String webString = indexFile.readStringUntil('\n');
        client.println(webString);
        Serial.print(webString);
    }

    indexFile.close();
}

// replace placeholders in page with environment values
String replaceAllPlaceholders(String page, JsonObject values)
{
    page.replace("#Viv1Name#", values["vivariums"][0]["name"]);
    page.replace("#Viv1Type#", values["vivariums"][0]["type"]);
    page.replace("#Viv1Image#", values["vivariums"][0]["image"]);
    page.replace("#Viv1LeftTemp#", String(viv1LeftTemp, 1));
    page.replace("#Viv1RightTemp#", String(viv1RightTemp, 1));
    page.replace("#Viv1MinTemp#", values["vivariums"][0]["tempRange"][0]);
    page.replace("#Viv1MaxTemp#", values["vivariums"][0]["tempRange"][1]);
    page.replace("#Viv1LeftHumid#", String(viv1LeftHumid, 0));
    page.replace("#Viv1RightHumid#", String(viv1RightHumid, 0));
    page.replace("#Viv1MinHumid#", values["vivariums"][0]["humidRange"][0]);
    page.replace("#Viv1MaxHumid#", values["vivariums"][0]["humidRange"][1]);

    page.replace("#Viv2Name#", values["vivariums"][1]["name"]);
    page.replace("#Viv2Type#", values["vivariums"][1]["type"]);
    page.replace("#Viv2Image#", values["vivariums"][1]["image"]);
    page.replace("#Viv2LeftTemp#", String(viv2LeftTemp, 1));
    page.replace("#Viv2RightTemp#", String(viv2RightTemp, 1));
    page.replace("#Viv2MinTemp#", values["vivariums"][1]["tempRange"][0]);
    page.replace("#Viv2MaxTemp#", values["vivariums"][1]["tempRange"][1]);
    page.replace("#Viv2LeftHumid#", String(viv2LeftHumid, 0));
    page.replace("#Viv2RightHumid#", String(viv2RightHumid, 0));
    page.replace("#Viv2MinHumid#", values["vivariums"][1]["humidRange"][0]);
    page.replace("#Viv2MaxHumid#", values["vivariums"][1]["humidRange"][1]);

    // Io not used. Will be included in future version. Return default values
    page.replace("#Viv3Name#", values["vivariums"][2]["name"]);
    page.replace("#Viv3Type#", values["vivariums"][2]["type"]);
    page.replace("#Viv3Image#", values["vivariums"][2]["image"]);
    page.replace("#Viv3LeftTemp#", "-");
    page.replace("#Viv3RightTemp#", "-");
    page.replace("#Viv3MinTemp#", values["vivariums"][2]["tempRange"][0]);
    page.replace("#Viv3MaxTemp#", values["vivariums"][2]["tempRange"][1]);
    page.replace("#Viv3LeftHumid#", "-");
    page.replace("#Viv3RightHumid#", "-");
    page.replace("#Viv3MinHumid#", values["vivariums"][2]["humidRange"][0]);
    page.replace("#Viv3MaxHumid#", values["vivariums"][2]["humidRange"][1]);

    return page;
}

void setup()
{
    Serial.begin(115200);

    // connect to wifi
    Serial.println("Connecting to wifi");
    connectToWifi();

    // turn on LCD
    lcd.init();
    lcd.backlight();

    // mount file system
    SPIFFS.begin();
    //SPIFFS.format();

    // start sensors
    viv1LeftSensor.setup(0);   // D3
    viv1RightSensor.setup(2);  // D4
    viv2LeftSensor.setup(14);  // D5
    viv2RightSensor.setup(13); // D7
}

void loop()
{
    // reconnect to wifi if necessary
    while (WiFi.status() != WL_CONNECTED)
    {
        connectToWifi();
    }

    // get sensor readings and print on LCD - set on timer
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

        // send web page to client
        sendWebPage(client);
    }
}