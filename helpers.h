#include <WiFi.h>

void send200(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
}

void send400(WiFiClient client) {
  client.println("HTTP/1.1 400 Bad Request");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
}