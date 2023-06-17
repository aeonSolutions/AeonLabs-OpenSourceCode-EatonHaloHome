#include <Arduino.h>
#include <WiFiClientSecure.h>

#define HALO_HOME_API_HOST "api.halohome.io"
#define HALO_HOME_API_PORT 443

class HALOHome {
public:
  HALOHome(const char* apiKey, const char* ssid, const char* password);
  void begin();
  void setLightState(int lightId, bool on);
  bool getLightState(int lightId);
  
private:
  WiFiClientSecure client;
  const char* apiKey;
  const char* ssid;
  const char* password;
  String authToken;
  
  String sendRequest(const String& method, const String& path, const String& body = "");
  bool authenticate();
};

HALOHome::HALOHome(const char* apiKey, const char* ssid, const char* password)
  : apiKey(apiKey), ssid(ssid), password(password) {}

void HALOHome::begin() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  if (authenticate()) {
    Serial.println("Authentication successful!");
  } else {
    Serial.println("Authentication failed!");
  }
}

String HALOHome::sendRequest(const String& method, const String& path, const String& body) {
  if (!client.connect(HALO_HOME_API_HOST, HALO_HOME_API_PORT)) {
    return "";
  }
  
  String request = method + " " + path + " HTTP/1.1\r\n" +
                   "Host: " + String(HALO_HOME_API_HOST) + "\r\n" +
                   "Authorization: Bearer " + authToken + "\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + String(body.length()) + "\r\n\r\n" +
                   body;
  
  client.print(request);
  
  String response;
  while (client.connected()) {
    if (client.available()) {
      response += client.readStringUntil('\r');
    }
  }
  
  client.stop();
  
  return response;
}

bool HALOHome::authenticate() {
  String body = "{\"apiKey\": \"" + String(apiKey) + "\"}";
  String response = sendRequest("POST", "/v1/oauth2/token", body);
  
  if (response == "") {
    return false;
  }
  
  int startIndex = response.indexOf("access_token\": \"");
  int endIndex = response.indexOf("\"", startIndex + 16);
  
  if (startIndex == -1 || endIndex == -1) {
    return false;
  }
  
  authToken = response.substring(startIndex + 16, endIndex);
  
  return true;
}

void HALOHome::setLightState(int lightId, bool on) {
  String body = "{\"on\": " + String(on ? "true" : "false") + "}";
  sendRequest("PUT", "/v1/lights/" + String(lightId), body);
}

bool HALOHome::getLightState(int lightId) {
  String response = sendRequest("GET", "/v1/lights/" + String(lightId));
  
  int startIndex = response.indexOf("\"on\": ");
  int endIndex = response.indexOf(",", startIndex + 6);
  
  if (startIndex == -1 || endIndex == -1) {
    return false;
  }
  
  String state = response.substring(startIndex + 6, endIndex);
  
  return (state == "true");
}

