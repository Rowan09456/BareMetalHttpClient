# Bare Metal HTTP Client
This is an illustrative example that can be used in your Group project using the Raspberry Pico 2W.
This example project implements an Asynchronous HTTP Client, and features use of multicore API.
The idea behind this is that one of the cores is used for sensing/actuating/processing functionality application.
The other core is dedicated to making the HTTP request to the server. 
You create a URL which encodes the readings you wish to report to the HTTP Server, and ask the other core to do this.

## secrets.h
You need to create a secrets.h file with the following defines. This file is not included in the git repo.
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
#define HTTP_SERVER "192.168.88.100" //The IP Address where your Flask Web server is running
