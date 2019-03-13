#include <ArduinoJson.h> //https://arduinojson.org/

//Send data from MEGA 2560 to Wifi ESP32 through the logic level shifter.
void sendDataToESP32(String status, long eventTimestamp, String error) {
  String json;
  //Bootstrap ESP32
  digitalWrite(relay, HIGH);

  json = serializedDataToESP32(status, (long)lastAlarmTime, eventTimestamp, getCelsiusTemperature(), error, SMS);

  delay(15000);
  Serial.println("Send data...");
  Serial.println(json);
  sw.print(json + "$");
  // Waiting for process handling from ESP32
  delay(20000);
  digitalWrite(relay, LOW);
}

// Serialized Data into json format as expected by the ESP32 module.
String serializedDataToESP32(String status, long lastAlarmTimestamp, long eventTimestamp, String temperature, String error, String SMS) {
  String json;

  const size_t capacity = JSON_OBJECT_SIZE(6);
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.createObject();
  root["status"] = (status != "" ? status : "error");
  root["lastAlarmTimestamp"] = (lastAlarmTimestamp > 100 ? String(lastAlarmTimestamp) : "0");
  root["eventTimestamp"] = (eventTimestamp > 100 ? String(eventTimestamp) : "0");
  root["temperature"] = temperature;
  root["error"] = error;
  root["SMS"] = SMS;

  root.printTo(json);

  return json;
}
