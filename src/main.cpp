#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#define RELAY_PIN 2  // GPIO2 on ESP32-C3 Super Mini

// Configuration structure
struct Config {
  char wifi_ssid[32];
  char wifi_password[64];
  float latitude;
  float longitude;
  int sunset_delay_minutes;
  // Turn off times for each day (0=Sunday, 6=Saturday)
  int turnoff_hour[7];    // Hour for each day
  int turnoff_minute[7];  // Minute for each day
  bool configured;
};

Config config;
Preferences preferences;
WebServer server(80);

bool relay_state = false;
time_t sunset_trigger_time = 0;
bool relay_scheduled = false;
String last_sunset_time = "";
unsigned long last_fetch = 0;

// Days of week names
const char* dayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// HTML page with per-day configuration
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Sunset Relay Controller</title><style>
body{font-family:Arial,sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh}
.container{max-width:650px;margin:0 auto;background:white;border-radius:10px;padding:30px;box-shadow:0 10px 30px rgba(0,0,0,0.3)}
h1{color:#333;margin:0 0 10px 0;font-size:24px}
.subtitle{color:#666;font-size:14px;margin-bottom:25px}
.section{margin-bottom:25px;padding-bottom:25px;border-bottom:1px solid #eee}
.section:last-child{border-bottom:none}
h2{color:#667eea;font-size:18px;margin:0 0 15px 0}
label{display:block;color:#555;font-weight:600;margin-bottom:5px;font-size:14px}
input{width:100%;padding:10px;margin-bottom:15px;border:2px solid #e0e0e0;border-radius:5px;font-size:14px;box-sizing:border-box}
input:focus{outline:none;border-color:#667eea}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:15px}
.day-schedule{display:grid;grid-template-columns:120px 1fr 1fr;gap:10px;align-items:center;margin-bottom:10px;padding:10px;background:#f7fafc;border-radius:5px}
.day-label{font-weight:600;color:#4a5568}
.time-input{padding:8px;border:2px solid #e0e0e0;border-radius:5px;font-size:14px}
.btn{width:100%;padding:12px;border:none;border-radius:5px;font-size:16px;font-weight:600;cursor:pointer;transition:all 0.3s}
.btn-primary{background:#667eea;color:white}
.btn-primary:hover{background:#5568d3}
.btn-success{background:#48bb78;color:white}
.btn-success:hover{background:#38a169}
.btn-secondary{background:#718096;color:white;margin-top:10px}
.btn-secondary:hover{background:#4a5568}
.status{padding:15px;border-radius:5px;margin-top:15px;font-size:14px}
.status-success{background:#c6f6d5;color:#22543d;border:1px solid #9ae6b4}
.status-error{background:#fed7d7;color:#742a2a;border:1px solid #fc8181}
.info{background:#f7fafc;padding:15px;border-radius:5px;border-left:4px solid #667eea;margin-top:20px}
.info p{margin:5px 0;font-size:13px;color:#4a5568}
.relay-status{display:flex;align-items:center;justify-content:space-between;padding:15px;background:#f7fafc;border-radius:5px;margin-bottom:15px}
.relay-indicator{width:20px;height:20px;border-radius:50%;margin-left:10px}
.relay-on{background:#48bb78}
.relay-off{background:#cbd5e0}
.note{font-size:12px;color:#718096;font-style:italic;margin-top:5px}
</style></head><body>
<div class='container'>
<h1>Sunset Relay Controller</h1>
<div class='subtitle'>ESP32-C3 Super Mini with Per-Day Scheduling</div>
<div class='relay-status'>
<span><strong>Relay Status:</strong> <span id='relayStatus'>Loading...</span></span>
<div class='relay-indicator' id='relayIndicator'></div>
</div>
<div class='section'>
<h2>WiFi Configuration</h2>
<label>WiFi SSID</label>
<input type='text' id='ssid' placeholder='Enter WiFi network name'>
<label>WiFi Password</label>
<input type='password' id='password' placeholder='Enter WiFi password'>
</div>
<div class='section'>
<h2>Location</h2>
<div class='grid'>
<div><label>Latitude</label><input type='number' step='0.000001' id='lat' placeholder='41.6764'></div>
<div><label>Longitude</label><input type='number' step='0.000001' id='lng' placeholder='-87.9373'></div>
</div>
</div>
<div class='section'>
<h2>Sunset Delay</h2>
<label>Minutes After Sunset to Turn ON</label>
<input type='number' id='delay' value='0' min='0' max='240'>
</div>
<div class='section'>
<h2>Turn OFF Schedule (Per Day)</h2>
<div class='note'>Set different turn-off times for each day of the week</div>
<div style='margin-top:15px'>
<div class='day-schedule'>
<div class='day-label'>Sunday</div>
<input type='number' class='time-input' id='sun_hour' min='0' max='23' placeholder='Hour'>
<input type='number' class='time-input' id='sun_min' min='0' max='59' placeholder='Min'>
</div>
<div class='day-schedule'>
<div class='day-label'>Monday</div>
<input type='number' class='time-input' id='mon_hour' min='0' max='23' placeholder='Hour'>
<input type='number' class='time-input' id='mon_min' min='0' max='59' placeholder='Min'>
</div>
<div class='day-schedule'>
<div class='day-label'>Tuesday</div>
<input type='number' class='time-input' id='tue_hour' min='0' max='23' placeholder='Hour'>
<input type='number' class='time-input' id='tue_min' min='0' max='59' placeholder='Min'>
</div>
<div class='day-schedule'>
<div class='day-label'>Wednesday</div>
<input type='number' class='time-input' id='wed_hour' min='0' max='23' placeholder='Hour'>
<input type='number' class='time-input' id='wed_min' min='0' max='59' placeholder='Min'>
</div>
<div class='day-schedule'>
<div class='day-label'>Thursday</div>
<input type='number' class='time-input' id='thu_hour' min='0' max='23' placeholder='Hour'>
<input type='number' class='time-input' id='thu_min' min='0' max='59' placeholder='Min'>
</div>
<div class='day-schedule'>
<div class='day-label'>Friday</div>
<input type='number' class='time-input' id='fri_hour' min='0' max='23' placeholder='Hour'>
<input type='number' class='time-input' id='fri_min' min='0' max='59' placeholder='Min'>
</div>
<div class='day-schedule'>
<div class='day-label'>Saturday</div>
<input type='number' class='time-input' id='sat_hour' min='0' max='23' placeholder='Hour'>
<input type='number' class='time-input' id='sat_min' min='0' max='59' placeholder='Min'>
</div>
</div>
<button class='btn btn-secondary' onclick='setAllSame()'>Set All Days to Same Time</button>
</div>
<button class='btn btn-success' onclick='testAPI()'>Test Sunset API</button>
<div id='testResult'></div>
<button class='btn btn-primary' onclick='saveConfig()' style='margin-top:15px'>Save Configuration</button>
<div id='saveResult'></div>
<div class='info'>
<p><strong>Current Time:</strong> <span id='currentTime'>--</span></p>
<p><strong>Today:</strong> <span id='today'>--</span></p>
<p><strong>Next Sunset:</strong> <span id='nextSunset'>--</span></p>
<p><strong>Relay ON Time:</strong> <span id='relayOn'>--</span></p>
<p><strong>Relay OFF Time:</strong> <span id='relayOff'>--</span></p>
</div>
</div>
<script>
const days=['sun','mon','tue','wed','thu','fri','sat'];
function updateStatus(){
fetch('/status').then(r=>r.json()).then(d=>{
document.getElementById('relayStatus').textContent=d.relay?'ON':'OFF';
document.getElementById('relayIndicator').className='relay-indicator '+(d.relay?'relay-on':'relay-off');
document.getElementById('currentTime').textContent=d.current_time||'--';
document.getElementById('today').textContent=d.today||'--';
document.getElementById('nextSunset').textContent=d.next_sunset||'--';
document.getElementById('relayOn').textContent=d.relay_on_time||'--';
document.getElementById('relayOff').textContent=d.relay_off_time||'--';
if(d.ssid)document.getElementById('ssid').value=d.ssid;
if(d.lat)document.getElementById('lat').value=d.lat;
if(d.lng)document.getElementById('lng').value=d.lng;
if(d.delay)document.getElementById('delay').value=d.delay;
if(d.schedule){
for(let i=0;i<7;i++){
document.getElementById(days[i]+'_hour').value=d.schedule[i].hour;
document.getElementById(days[i]+'_min').value=d.schedule[i].min;
}
}
}).catch(e=>console.error('Status error:',e));
}
function setAllSame(){
const hour=prompt('Enter hour (0-23) for all days:','20');
const min=prompt('Enter minute (0-59) for all days:','0');
if(hour!==null && min!==null){
for(let day of days){
document.getElementById(day+'_hour').value=hour;
document.getElementById(day+'_min').value=min;
}
}
}
function testAPI(){
const lat=document.getElementById('lat').value;
const lng=document.getElementById('lng').value;
const delay=document.getElementById('delay').value;
if(!lat||!lng){
document.getElementById('testResult').innerHTML=
"<div class='status status-error'>Please enter latitude and longitude</div>";
return;
}
fetch('/test?lat='+lat+'&lng='+lng+'&delay='+delay).then(r=>r.json()).then(d=>{
if(d.success){
document.getElementById('testResult').innerHTML=
"<div class='status status-success'><strong>✓ API Test Successful!</strong><br>"
+"Sunset: "+d.sunset+"<br>Relay ON: "+d.relay_on+"<br>Today's OFF time: "+d.relay_off+"</div>";
}else{
document.getElementById('testResult').innerHTML=
"<div class='status status-error'>API Test Failed: "+d.message+"</div>";
}
}).catch(e=>{
document.getElementById('testResult').innerHTML=
"<div class='status status-error'>Request failed: "+e.message+"</div>";
});
}
function saveConfig(){
const schedule=[];
for(let day of days){
schedule.push({
hour:parseInt(document.getElementById(day+'_hour').value)||0,
min:parseInt(document.getElementById(day+'_min').value)||0
});
}
const data={
ssid:document.getElementById('ssid').value,
password:document.getElementById('password').value,
lat:parseFloat(document.getElementById('lat').value),
lng:parseFloat(document.getElementById('lng').value),
delay:parseInt(document.getElementById('delay').value),
schedule:schedule
};
fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)})
.then(r=>r.json()).then(d=>{
document.getElementById('saveResult').innerHTML=
"<div class='status status-success'>✓ Configuration saved! ESP32 will restart...</div>";
setTimeout(()=>location.reload(),3000);
}).catch(e=>{
document.getElementById('saveResult').innerHTML=
"<div class='status status-error'>Save failed: "+e.message+"</div>";
});
}
updateStatus();
setInterval(updateStatus,5000);
</script></body></html>
)rawliteral";

// Load configuration from preferences
void loadConfig() {
  preferences.begin("relay-config", false);
  
  preferences.getString("wifi_ssid", config.wifi_ssid, sizeof(config.wifi_ssid));
  preferences.getString("wifi_pass", config.wifi_password, sizeof(config.wifi_password));
  config.latitude = preferences.getFloat("latitude", 0.0);
  config.longitude = preferences.getFloat("longitude", 0.0);
  config.sunset_delay_minutes = preferences.getInt("delay", 0);
  config.configured = preferences.getBool("configured", false);
  
  // Load per-day schedule
  for (int i = 0; i < 7; i++) {
    char key_hour[16], key_min[16];
    snprintf(key_hour, sizeof(key_hour), "hour_%d", i);
    snprintf(key_min, sizeof(key_min), "min_%d", i);
    
    if (i == 0) {  // Sunday default
      config.turnoff_hour[i] = preferences.getInt(key_hour, 17);  // 5 PM
    } else {
      config.turnoff_hour[i] = preferences.getInt(key_hour, 20);  // 8 PM
    }
    config.turnoff_minute[i] = preferences.getInt(key_min, 0);
  }
  
  preferences.end();
  
  Serial.println("Configuration loaded from memory");
}

// Save configuration to preferences
void saveConfig() {
  preferences.begin("relay-config", false);
  
  preferences.putString("wifi_ssid", config.wifi_ssid);
  preferences.putString("wifi_pass", config.wifi_password);
  preferences.putFloat("latitude", config.latitude);
  preferences.putFloat("longitude", config.longitude);
  preferences.putInt("delay", config.sunset_delay_minutes);
  preferences.putBool("configured", true);
  
  // Save per-day schedule
  for (int i = 0; i < 7; i++) {
    char key_hour[16], key_min[16];
    snprintf(key_hour, sizeof(key_hour), "hour_%d", i);
    snprintf(key_min, sizeof(key_min), "min_%d", i);
    
    preferences.putInt(key_hour, config.turnoff_hour[i]);
    preferences.putInt(key_min, config.turnoff_minute[i]);
  }
  
  preferences.end();
  
  Serial.println("Configuration saved to memory");
}

// HTTP handler for main page
void handleRoot() {
  server.send_P(200, "text/html", html_page);
}

// HTTP handler for status
void handleStatus() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S CST", &timeinfo);
  
  int day_of_week = timeinfo.tm_wday;  // 0=Sunday, 6=Saturday
  
  char off_time[64];
  snprintf(off_time, sizeof(off_time), "%s at %02d:%02d:00 CST", 
           dayNames[day_of_week],
           config.turnoff_hour[day_of_week], 
           config.turnoff_minute[day_of_week]);
  
  StaticJsonDocument<768> doc;
  doc["relay"] = relay_state;
  doc["current_time"] = time_str;
  doc["today"] = dayNames[day_of_week];
  doc["next_sunset"] = last_sunset_time;
  doc["relay_off_time"] = off_time;
  doc["ssid"] = config.wifi_ssid;
  doc["lat"] = config.latitude;
  doc["lng"] = config.longitude;
  doc["delay"] = config.sunset_delay_minutes;
  
  JsonArray schedule = doc.createNestedArray("schedule");
  for (int i = 0; i < 7; i++) {
    JsonObject day = schedule.createNestedObject();
    day["hour"] = config.turnoff_hour[i];
    day["min"] = config.turnoff_minute[i];
  }
  
  if (relay_scheduled && sunset_trigger_time > 0) {
    struct tm trigger_tm;
    localtime_r(&sunset_trigger_time, &trigger_tm);
    char trigger_str[32];
    strftime(trigger_str, sizeof(trigger_str), "%H:%M:%S CST", &trigger_tm);
    doc["relay_on_time"] = trigger_str;
  } else {
    doc["relay_on_time"] = "Not scheduled";
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// HTTP handler for API test
void handleTest() {
  if (!server.hasArg("lat") || !server.hasArg("lng") || !server.hasArg("delay")) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters\"}");
    return;
  }
  
  String lat = server.arg("lat");
  String lng = server.arg("lng");
  String delay = server.arg("delay");
  
  HTTPClient http;
  String url = "https://api.sunrise-sunset.org/json?lat=" + lat + "&lng=" + lng + "&formatted=0";
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      const char* sunset = doc["results"]["sunset"];
      
      time_t now;
      struct tm timeinfo;
      time(&now);
      localtime_r(&now, &timeinfo);
      int day_of_week = timeinfo.tm_wday;
      
      StaticJsonDocument<256> response;
      response["success"] = true;
      response["sunset"] = sunset;
      response["relay_on"] = "Sunset + " + delay + " min";
      
      char off_time[64];
      snprintf(off_time, sizeof(off_time), "%s: %02d:%02d:00 CST", 
               dayNames[day_of_week],
               config.turnoff_hour[day_of_week], 
               config.turnoff_minute[day_of_week]);
      response["relay_off"] = off_time;
      
      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);
      http.end();
      return;
    }
  }
  
  http.end();
  server.send(500, "application/json", "{\"success\":false,\"message\":\"API request failed\"}");
}

// HTTP handler for saving config
void handleSave() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"success\":false}");
    return;
  }
  
  String body = server.arg("plain");
  StaticJsonDocument<768> doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\":false}");
    return;
  }
  
  if (doc.containsKey("ssid")) strlcpy(config.wifi_ssid, doc["ssid"], sizeof(config.wifi_ssid));
  if (doc.containsKey("password")) strlcpy(config.wifi_password, doc["password"], sizeof(config.wifi_password));
  if (doc.containsKey("lat")) config.latitude = doc["lat"];
  if (doc.containsKey("lng")) config.longitude = doc["lng"];
  if (doc.containsKey("delay")) config.sunset_delay_minutes = doc["delay"];
  
  if (doc.containsKey("schedule")) {
    JsonArray schedule = doc["schedule"];
    for (int i = 0; i < 7 && i < schedule.size(); i++) {
      config.turnoff_hour[i] = schedule[i]["hour"];
      config.turnoff_minute[i] = schedule[i]["min"];
    }
  }
  
  config.configured = true;
  saveConfig();
  
  server.send(200, "application/json", "{\"success\":true}");
  
  delay(1000);
  ESP.restart();
}

// Fetch sunset time from API
void fetchSunsetTime() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot fetch sunset time");
    return;
  }
  
  HTTPClient http;
  String url = "https://api.sunrise-sunset.org/json?lat=" + 
               String(config.latitude, 6) + "&lng=" + 
               String(config.longitude, 6) + "&formatted=0";
  
  Serial.println("Fetching sunset time from API...");
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      const char* sunsetStr = doc["results"]["sunset"];
      Serial.print("Sunset time (UTC): ");
      Serial.println(sunsetStr);
      
      // Parse ISO 8601 time string
      struct tm sunset_tm = {0};
      int year, month, day, hour, min, sec;
      sscanf(sunsetStr, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec);
      
      sunset_tm.tm_year = year - 1900;
      sunset_tm.tm_mon = month - 1;
      sunset_tm.tm_mday = day;
      sunset_tm.tm_hour = hour;
      sunset_tm.tm_min = min;
      sunset_tm.tm_sec = sec;
      
      time_t sunset_utc = mktime(&sunset_tm);
      
      // Convert to CST (UTC-6) and add delay
      sunset_trigger_time = sunset_utc - (6 * 3600) + (config.sunset_delay_minutes * 60);
      relay_scheduled = true;
      
      struct tm trigger_tm;
      localtime_r(&sunset_trigger_time, &trigger_tm);
      
      char trigger_str[64];
      strftime(trigger_str, sizeof(trigger_str), "%H:%M:%S CST", &trigger_tm);
      last_sunset_time = String(trigger_str);
      
      Serial.print("Relay will turn ON at: ");
      Serial.println(trigger_str);
      
      // Show turn-off time for today
      time_t now;
      struct tm timeinfo;
      time(&now);
      localtime_r(&now, &timeinfo);
      int day_of_week = timeinfo.tm_wday;
      
      Serial.printf("Relay will turn OFF at: %02d:%02d:00 CST (%s)\n", 
                    config.turnoff_hour[day_of_week], 
                    config.turnoff_minute[day_of_week],
                    dayNames[day_of_week]);
      
      last_fetch = millis();
    } else {
      Serial.println("JSON parsing failed");
    }
  } else {
    Serial.print("HTTP request failed, error: ");
    Serial.println(httpCode);
  }
  
  http.end();
}

// Initialize WiFi in AP mode
void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("SunsetRelay-Setup", "12345678");
  
  Serial.println("WiFi AP started");
  Serial.println("SSID: SunsetRelay-Setup");
  Serial.println("Password: 12345678");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Connect to http://192.168.4.1");
}

// Initialize WiFi in STA mode
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi_ssid, config.wifi_password);
  
  Serial.print("Connecting to WiFi: ");
  Serial.println(config.wifi_ssid);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Configure time with CST timezone
    configTime(-6 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for NTP time sync...");
    
    // Wait for time to be set
    time_t now = time(nullptr);
    int wait = 0;
    while (now < 1000000000 && wait < 20) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
      wait++;
    }
    Serial.println("\nTime synchronized!");
    
    // Fetch sunset time
    fetchSunsetTime();
  } else {
    Serial.println("\nFailed to connect to WiFi");
    Serial.println("Starting AP mode for configuration");
    startAccessPoint();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=================================");
  Serial.println("Sunset Relay Controller Starting");
  Serial.println("ESP32-C3 Super Mini with Per-Day Schedule");
  Serial.println("=================================\n");
  
  // Initialize relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  relay_state = false;
  
  // Load configuration
  loadConfig();
  
  // Print current schedule
  Serial.println("Current Schedule:");
  for (int i = 0; i < 7; i++) {
    Serial.printf("%s: %02d:%02d\n", dayNames[i], config.turnoff_hour[i], config.turnoff_minute[i]);
  }
  
  // Setup WiFi
  if (!config.configured || strlen(config.wifi_ssid) == 0) {
    Serial.println("No configuration found - starting AP mode");
    startAccessPoint();
  } else {
    Serial.println("Configuration found - connecting to WiFi");
    connectToWiFi();
  }
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/test", handleTest);
  server.on("/save", HTTP_POST, handleSave);
  
  // Start web server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  // Only run relay control if WiFi is connected and configured
  if (WiFi.status() == WL_CONNECTED && config.configured) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    int day_of_week = timeinfo.tm_wday;  // 0=Sunday, 6=Saturday
    
    // Check if it's midnight - fetch new sunset time
    if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_sec < 2) {
      fetchSunsetTime();
      delay(2000); // Prevent multiple calls
    }
    
    // Refetch if we haven't fetched in the last 24 hours
    if (millis() - last_fetch > 86400000) { // 24 hours
      fetchSunsetTime();
    }
    
    // Check if it's time to turn ON relay
    if (!relay_state && relay_scheduled && now >= sunset_trigger_time) {
      digitalWrite(RELAY_PIN, HIGH);
      relay_state = true;
      Serial.println("Relay turned ON (Sunset triggered)");
    }
    
    // Check if it's time to turn OFF relay (using per-day schedule)
    if (relay_state && 
        timeinfo.tm_hour == config.turnoff_hour[day_of_week] && 
        timeinfo.tm_min == config.turnoff_minute[day_of_week] && 
        timeinfo.tm_sec < 2) {
      digitalWrite(RELAY_PIN, LOW);
      relay_state = false;
      relay_scheduled = false;
      Serial.printf("Relay turned OFF (Scheduled turnoff for %s)\n", dayNames[day_of_week]);
      delay(2000); // Prevent multiple triggers
    }
  }
  
  delay(100);
}