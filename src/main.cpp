#include <Arduino.h>
#include <ESP8266WiFi.h>

const char *ssid = "SKY8368B";
const char *password = "AWFPXTQV";

int ledPin = 2;
WiFiServer server(80);

void setup()
{
  Serial.begin(115200);
  pinMode(2, OUTPUT);

  // Connect to WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // Start the web server
    server.begin();
    Serial.println("Server started");

    // Print the IP address
    Serial.print("Use this URL : ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
}

void loop()
{
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client)
    {
        return;
    }

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

    // Match the request
    int value = LOW;
    if (request.indexOf("/LED=ON") != -1)
    {
        digitalWrite(2, HIGH);
        value = HIGH;
    }
    if (request.indexOf("/LED=OFF") != -1)
    {
        digitalWrite(2, LOW);
        value = LOW;
    }

    // Return the response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); //  do not forget this one
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");

    client.print("Led pin is now: ");

    if (value == HIGH)
    {
        client.print("Off");
    }
    else
    {
        client.print("On");
    }
    client.println("<br><br>");
    client.println("Click <a href=\"/LED=ON\">here</a> turn the LED OFF<br>");
    client.println("Click <a href=\"/LED=OFF\">here</a> turn the LED ON<br>");
    client.println("</html>");

    delay(1);
    Serial.println("Client disconnected");
    Serial.println("");
}