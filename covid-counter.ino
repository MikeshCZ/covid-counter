/*
Application note: Covid counter display actual information about confirmed, recovered and deaths in local area
Version: 1.0
Copyright © 2020 Michal Sara
www.michalsara.cz
www.github.com/MikeshCZ/covid-counter

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/ 

// ####################
// ###              ###
// ###     INIT     ###
// ###              ###
// ####################

#include <TM1637Display.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

//pins definitions for TM1637 and can be changed to other pins
#define CLK 0
#define DIO 2

ESP8266WiFiMulti WiFiMulti;
TM1637Display displej(CLK, DIO);

// create your Query URL on https://coronavirus-disasterresponse.hub.arcgis.com/datasets/bbb2e4f589ba40d692fab712ae37b9ac_1/geoservice
const char* url = "https://services1.arcgis.com/0MSEUqKaxRlEPj5g/arcgis/rest/services/Coronavirus_2019_nCoV_Cases/FeatureServer/1/query?where=Country_Region%20%3D%20'CZECHIA'&outFields=Confirmed,Recovered,Deaths,Last_Update&returnGeometry=false&outSR=4326&f=json";

// certificate fingerprint of https://coronavirus-disasterresponse.hub.arcgis.com
const char* fingerprint = "70 58 0E 78 0C 9D 72 75 50 61 9D 3E 4E FD B2 1D 64 D1 E9 1E";

//wifi connection
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWD";

int DATACONFIRMED = 0;
int DATARECOVERED = 0;
int DATADEATHS = 0;

// words for segment preparation
// Segments maps we will write witch segments we want to light up
//
//   -      A
// |   |  F   B
//   -      G
// |   |  E   C
//   -      D 

const uint8_t INIT[] = {
  SEG_B | SEG_C,                  // I
  SEG_E | SEG_G | SEG_C,          // n
  SEG_B | SEG_C,                  // I
  SEG_F | SEG_E | SEG_D | SEG_G   // t
};

const uint8_t DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};

const uint8_t CONF[] = {
  SEG_A | SEG_F | SEG_E | SEG_D,                   // C
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_E | SEG_F | SEG_G                    // F
};

const uint8_t RECO[] = {
  SEG_E | SEG_G,                                   // r
  SEG_A | SEG_F | SEG_G | SEG_E | SEG_D,           // E
  SEG_A | SEG_F | SEG_E | SEG_D,                   // C
  SEG_A | SEG_F | SEG_E | SEG_D | SEG_C | SEG_B    // O
};

const uint8_t DEAD[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,           // E
  SEG_A | SEG_F | SEG_E | SEG_G | SEG_B | SEG_C,   // A
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
};


// #####################
// ###               ###
// ###     SETUP     ###
// ###               ###
// #####################

void setup() {
  
  //serial connections
  Serial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println("[SETUP] START");
  Serial.println("▱▱▱▱");
  
  // segment setup
  displej.setBrightness(10);
  displej.setSegments(INIT);
  
  // wifi connection
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
  
  // delay for serial connection waiting
  delay(500);
  Serial.println("▰▱▱▱");
  delay(500);
  Serial.println("▰▰▱▱");
  delay(500);
  Serial.println("▰▰▰▱");
  delay(500);
  Serial.println("▰▰▰▰");
  Serial.println();
  
  Serial.println("[SETUP] COMPLETE");
  displej.clear();
  displej.setSegments(DONE);
  delay(3000);
  displej.clear();
}


// ####################
// ###              ###
// ###     LOOP     ###
// ###              ###
// ####################

void loop() {
  
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    bool mfln = client->probeMaxFragmentLength(url, 443, 1024);
    Serial.printf("\nConnecting to ");
    Serial.printf(url);
    Serial.printf("\n");
    Serial.printf("Maximum fragment Length negotiation supported: %s\n", mfln ? "yes" : "no");
    if (mfln) {
      client->setBufferSizes(1024, 1024);
    }

    Serial.println("[HTTPS] begin...");

    // configure server and url
    client->setFingerprint(fingerprint);

    HTTPClient https;

    if (https.begin(*client, url)) {
      Serial.println("[HTTPS] GET...");
      
      // start connection and send HTTP header
      int httpCode = https.GET();
      
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {

          String payload = https.getString();
          char charBuf[500];
          payload.toCharArray(charBuf, 500);

          // uncoment for debug
          // Serial.println(payload);

          // JSON deserialize using https://arduinojson.org/v6/assistant/
          const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 3*JSON_OBJECT_SIZE(6) + 2*JSON_OBJECT_SIZE(7) + 690;
          DynamicJsonDocument doc(capacity);
          deserializeJson(doc, payload);
          JsonArray fields = doc["fields"];
          JsonObject features_0_attributes = doc["features"][0]["attributes"];
          DATACONFIRMED = features_0_attributes["Confirmed"]; // 5221
          DATARECOVERED = features_0_attributes["Recovered"]; // 233
          DATADEATHS = features_0_attributes["Deaths"]; // 99
          // long features_0_attributes_Last_Update = features_0_attributes["Last_Update"]; // 1586373421000

          // print out to serial the results for debug
          Serial.printf("Confirmed: %d\n", DATACONFIRMED);
          Serial.printf("Recovered: %d\n", DATARECOVERED);
          Serial.printf("Deaths: %d\n", DATADEATHS);

          Serial.println();
          Serial.println("[HTTPS] connection closed or file end.");
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      
      // close connection
      https.end();
      
    } else {
      // unable to connect
      Serial.println("Unable to connect");
    }
    
    Serial.flush();
    Serial.println("Wait 60s before the next round...");
	
	// shows results for 10 minutes
    for (int i = 0; i <= 10; i++) {
      // write info to display
	  // one loop is 1 minute
      textToSegment();
    }
  }
}


// ###############################
// ###                         ###
// ###     TEXT TO SEGMENT     ###
// ###                         ###
// ###############################

void textToSegment() {
  Serial.println("[DISPLAY] START");

  Serial.println("[DISPLAY] Confirmed");
  displej.clear();
  displej.setSegments(CONF);
  delay(10000);
  
  displej.clear();
  displej.showNumberDec(DATACONFIRMED, false);
  delay(10000);

  Serial.println("[DISPLAY] Recovered");
  displej.clear();
  displej.setSegments(RECO);
  delay(10000);
  
  displej.clear();
  displej.showNumberDec(DATARECOVERED, false);
  delay(10000);

  Serial.println("[DISPLAY] Deaths");
  displej.clear();
  displej.setSegments(DEAD);
  delay(10000);
  
  displej.clear();
  displej.showNumberDec(DATADEATHS, false);
  delay(10000);
  
  displej.clear();
  Serial.println("[DISPLAY] STOP");
}
