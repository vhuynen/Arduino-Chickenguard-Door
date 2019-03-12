//###################### Vincent HUYNEN ########################/
//################## vincent.huynen@gmail.com ##################/
//######################### March 2019 #########################/
//######################## Version 1.0.0 #######################/

/*
  Adding Statistics to Google Sheet
  Send SMS notification
  All secrets have been hidden into on another file
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* access_token;
String bearer;

void setup()
{

  Serial.begin(115200);

  Serial.print("Connecting to WiFi..");
  delay(1000);
  WiFi.begin(getSSID(), getWifiPassword());
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // Retrieving Access Token for Google Sheet API from Refresh Token
  // https://developers.google.com/identity/protocols/OAuth2WebServer
  // Refreshing an access token (offline access)
  if ((WiFi.status() == WL_CONNECTED)) {

    String request = "{\"grant_type\":\"refresh_token\",\"refresh_token\":\"" + getRefreshToken() +
                     "\",\"client_id\":\"" + getClientId() + "\",\"client_secret\":\"" + getClientSecret() + "\"}";
    HTTPClient http;
    http.setTimeout(10000);
    //Retreived Access Token from Refresh Token
    http.begin("https://www.googleapis.com/oauth2/v4/token");
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(request);
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        const size_t capacity = JSON_OBJECT_SIZE(4) + 302;
        DynamicJsonBuffer jsonBuffer(capacity);
        JsonObject& root = jsonBuffer.parseObject(*http.getStreamPtr());
        // Access Token
        access_token = root["access_token"];
        // Set Authorization Bearer
        bearer = "Bearer " + String(access_token);
      }
    }
    http.end(); //Free the resources
  }
}

void loop()
{
  // Retreive json data from Arduino Mega 2560
  if (Serial.available() > 0) {
    char json[501];
    memset(json, 0, 501);
    Serial.readBytesUntil( '$', json, 500);
    handleJson(json);
  }
}

// Handle the json message retrieved from the pin RX
void handleJson(char * json) {

  // Generated from https://arduinojson.org/v5/assistant/
  const size_t capacity = JSON_OBJECT_SIZE(6) + 170;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.parseObject(json);

  const char* status = root["status"]; // "open"
  const char* lastAlarmTimestamp = root["lastAlarmTimestamp"]; // "1551727177"
  const char* eventTimestamp = root["eventTimestamp"]; // "1551727177"
  const char* temperature = root["temperature"]; // "10"
  const char* error = root["error"]; // "Error Message"
  const char* SMS = root["SMS"]; // "true"

  // Send SMS if error thrown or notification enabled
  if (String(status) == "error" || String(SMS) == "true") {
    sendSMS(String(status), String(eventTimestamp), String(temperature), String(error));
  }
  // Write statisctics into Google Sheet
  if (String(status) == "open" || String(status) == "close") {
    addRowToGoogleSheet(String(status), String(lastAlarmTimestamp), String(eventTimestamp), String(temperature));
  }
}
//https://developers.google.com/sheets/api/samples/writing
boolean addRowToGoogleSheet(String status, String lastAlarmTimestamp, String eventTimestamp, String temperature) {

  // Adding row to GoogleSheet Chickenguard statistics
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    String request = "{\"range\": \"" + status + "!A1:D1\",\"majorDimension\": \"ROWS\", \"values\": [[\"=" + lastAlarmTimestamp +
                     "/86400+date(1970,1,1)\",\"=" + eventTimestamp + "/86400+date(1970,1,1)\",\"" + temperature + "\"]]}";

    // https://sheets.googleapis.com/v4/spreadsheets/$spreadsheetId$/values/open/values/open!A1:D1:append?valueInputOption=USER_ENTERED
    String url = getGoogleSheetURL() + status + "!A1:D1:append?valueInputOption=USER_ENTERED";
    http.setTimeout(10000);
    //Add row to Google spreadsheets
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    // Set Authorization Bearer
    http.addHeader("Authorization", bearer);
    int httpCode = http.POST(request);
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        http.end(); //Free the resources
        return true;
      }
    }
    http.end(); //Free the resources
    return false;
  }
}

// Send SMS
boolean sendSMS(String status, String eventTimestamp, String temperature, String error) {
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    String msg;
    if (status == "error") {
      msg = "Status : " + status + " !!\nError : " + error + "\nEvent Date : " + getLiteralDate(eventTimestamp) + "\nSomething Wrong Today !\nChickenGuard";
    } else {
      msg = "Status : " + status + "\nEvent Date : " + getLiteralDate(eventTimestamp) + "\nTemperature : " + temperature + "\nChickenGuard";
    }

    HTTPClient http;
    // Call free SMS API from "FREE" provider : https://smsapi.free-mobile.fr/sendmsg
    http.begin(getURLFreeSMS() + urlencode(msg));
    int httpCode = http.GET();

    if (httpCode > 0) { //Check for the returning code
      http.end(); //Free the resources
      return true;
    }
    http.end(); //Free the resources
    return false;
  }
}

/*
 * Samples json
 * 
  {
  "status" : "open",
  "lastAlarmTimestamp" :"1551727177",
  "eventTimestamp" : "1551729238",
  "temperature" : "10",
  "error" : "",
  "SMS" : "true"
  }
  
  {
  "status" : "error",
  "lastAlarmTimestamp" :"",
  "eventTimestamp" : "1551729238",
  "temperature" : "10",
  "error" : "Error Message",
  "SMS" : "true"
  }
  
 */
