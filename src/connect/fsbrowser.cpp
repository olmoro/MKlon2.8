/* 
  FSWebServer - Example WebServer with SPIFFS backend for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done
  
  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#endif
//Add a below line for AutoConnect.
#include <AutoConnect.h>

#include "fsbrowser.h"
#include <Arduino.h>


float _current;
float _voltage;
float _celsius;
float _ahCharge;
int   _fanPwm;


const char* ssid = "wifi-ssid";
const char* password = "wifi-password";
#if defined(ARDUINO_ARCH_ESP8266)
const char* host = "esp8266fs";

ESP8266WebServer server(80);
#elif defined(ARDUINO_ARCH_ESP32)
const char* host = "esp32fs";

WebServer server(80);
#endif
//Add a below line for AutoConnect.
AutoConnect      portal(server);
//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);   // moro    size_t sent = server.streamFile(file, contentType);

    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  Serial.println("handleFileList: " + path);
#if defined(ARDUINO_ARCH_ESP8266)
  Dir dir = SPIFFS.openDir(path);
#elif defined(ARDUINO_ARCH_ESP32)
  File root = SPIFFS.open(path);
#endif
  path = String();

  String output = "[";
#if defined(ARDUINO_ARCH_ESP8266)
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
#elif defined(ARDUINO_ARCH_ESP32)
  if(root.isDirectory()){
    File file = root.openNextFile();
    while(file){
      if (output != "[") {
        output += ',';
      }
      output += "{\"type\":\"";
      output += (file.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(file.name()).substring(1);
      output += "\"}";
      file = root.openNextFile();
    }
  }
#endif
  
  output += "]";
  server.send(200, "text/json", output);
}


void initFSBrowser(void) {
  Serial.begin(115200);
  Serial.print("\n");
  Serial.setDebugOutput(true);
  SPIFFS.begin();
  {
#if defined(ARDUINO_ARCH_ESP8266)
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
#elif defined(ARDUINO_ARCH_ESP32)
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
      String fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      file = root.openNextFile();
    }
#endif
    Serial.printf("\n");
  }
  

  //WIFI INIT
  Serial.printf("Connecting to %s\n", ssid);

  //Comment out as follows to make AutoConnect recognition.
  // if (String(WiFi.SSID()) != String(ssid)) {
  //  WiFi.mode(WIFI_STA);
  //  WiFi.begin(ssid, password);
  // }
  
  // while (WiFi.status() != WL_CONNECTED) {
  //  delay(500);
  //  Serial.print(".");
  // }
  

Serial.println("moro-269");

  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() { 
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  //called when the url is not defined here
  //use it to load content from SPIFFS
  //Replacement as follows to make AutoConnect recognition.
  //server.onNotFound([]() {
  portal.onNotFound([]() {
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    //json += "\"heap\":"+String(ESP.getFreeHeap());
    json +=   "\"tekI\":"  +String(_current);
    json += ", \"tekU\":"  +String(_voltage);
    json += ", \"tekA\":"  +String(_ahCharge);
    json += ", \"tekT\":"  +String(_celsius);
    json += ", \"tekN\":"  +String(_fanPwm);
#if defined(ARDUINO_ARCH_ESP8266)
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
#elif defined(ARDUINO_ARCH_ESP32)
    json += ", \"gpio\":" +String((uint32_t)(0));             //(digPorts());             //((uint32_t)(0b11111111111111111111111111011111));
#endif
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  //Replacement as follows to make AutoConnect recognition.
  //server.begin();
  portal.begin();
  Serial.println("HTTP server started");
  //Relocation as follows to make AutoConnect recognition.
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  //Relocation as follows to make AutoConnect recognition.
  // MDNS.begin(host);
  // Serial.print("Open http://");
  // Serial.print(host);
  // Serial.println(".local/edit to see the file browser");
  // май 2019
  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("Open http://");
    Serial.print(host);
    Serial.println(".local/edit to see the file browser");
  }
  else {
    Serial.print("mDNS start failed");
  }
}
 
void runFSBrowser( float voltage, float current, float celsius, float ahCharge, int fanPwm ) {
  _voltage  = voltage;
  _current  = current;
  _celsius  = celsius;
  _ahCharge = ahCharge;
  _fanPwm   = fanPwm / 8;   // %%

  //Replacement as follows to make AutoConnect recognition.
  //server.handleClient();
  portal.handleClient(); 
}
