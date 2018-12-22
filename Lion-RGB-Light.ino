#include <Homie.h>
#include <ArduinoJson.h>
#include "config.h"

HomieNode lightNode("light", "light");

// Set buffer for JSON processing
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

//LED Transition Speed
unsigned long transitionSpeed = CONFIG_TRANSITION_SPEED;

// Milliseconds to wait between state announcements
// ie how long to wait to auto call the current state to the MQTT server
const long callStateDelay = CONFIG_CALL_STATE_DELAY;

unsigned long prevStateCallMillis = 0;
unsigned long prevTransitionMillis = 0;

// Define LED Pins
int ledPins[3] = { CONFIG_PIN_RED, CONFIG_PIN_GREEN, CONFIG_PIN_BLUE };

// Current RGB
int currentRGB[3] = {0, 0, 0};

// Target RGB
int targetRGB[3] = {0, 0, 0};

// Define HSB(Hue, Saturation, Brightness)
float HSB[3] = { 160, 100, 255 };

// Restore RGB
int restoreRGB[3];

// Power State
boolean state = false;

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
    return "";
}

int returnLargeThreeN(int n1, int n2, int n3) {
  // Takes in three numbers and returns the larges of the three
  if( n1>=n2 && n1>=n3 ) {
      return n1;
  }
  if( n2>=n1 && n2>=n3 ) {
      return n2;
  }
  if( n3>=n1 && n3>=n2 ) {
      return n3;
  }
}

bool stringSetHSB(String value) {
  int commaIndex = value.indexOf(',');
  int secondCommaIndex = value.indexOf(',', commaIndex + 1);
  
  float hue = value.substring(0, commaIndex).toFloat();
  float sat = value.substring(commaIndex +1).toFloat();
  float bri = value.substring(secondCommaIndex + 1).toFloat();

  // only call setRGB if the values have changed
  if (hue == HSB[0] && sat == HSB[1] && bri == HSB[2]){
    return false;
  }

  HSB[0] = hue;
  HSB[1] = sat;
  setBrightnessValue(map(bri, 0, 100, 0, 255));
  
  return true;
}

void announceStats() {
  Serial.println("Announcing stats to MQTT server");
  
  String hue = String(HSB[0]);
  String sat = String(HSB[1]);
  int bri = round(map(HSB[2], CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS, 1, 255));
  int briPercent = round(map(HSB[2], CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS, 1, 100));
  
  lightNode.setProperty("on").send((state ? "true" : "false"));
  
  lightNode.setProperty("hs").send(hue + "," + sat);
  
  lightNode.setProperty("brightness").send(String(bri));
  
  lightNode.setProperty("brightnesspct").send(String(briPercent));
  
  lightNode.setProperty("json").send(String(returnJSONstate()));

  lightNode.setProperty("hsb").send(hue + "," + sat + "," + String(briPercent));
}

String returnJSONstate() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (state) ? CONFIG_JSON_PAYLOAD_ON : CONFIG_JSON_PAYLOAD_OFF;
  
  JsonObject& color = root.createNestedObject("color");
  color["h"] = HSB[0];
  color["s"] = HSB[1];

  root["brightness"] = map(HSB[2], CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS, 1, 255);

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  return buffer;
}

void setRGB(int transitionTime = 0) {
  // Most of the code in this function is from Brian Neltner - Saiko LED
  // http://blog.saikoled.com/post/43693602826/why-every-led-light-should-be-using-hsi
  
  float H = HSB[0];
  float S = HSB[1];
  float I = mapfloat(HSB[2], 0, 255, 0.01, 1.00);
  
  int r, g, b;
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
    
  // Math! Thanks in part to Kyle Miller.
  if(H < 2.09439) {
    r = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    g = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    b = 255*I/3*(1-S);
  } else if(H < 4.188787) {
    H = H - 2.09439;
    g = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    b = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    r = 255*I/3*(1-S);
  } else {
    H = H - 4.188787;
    b = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    r = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    g = 255*I/3*(1-S);
  }
  
  if ( transitionTime == 0 ) {
    transitionSpeed = CONFIG_TRANSITION_SPEED;
  } else {
    Serial.println("setting transition");
    int diffRed = abs(targetRGB[0] - r);
    int diffGreen = abs(targetRGB[1] - g);
    int diffBlue = abs(targetRGB[2] - b);
    int maxTicks = returnLargeThreeN(diffRed, diffGreen, diffBlue);
    
    transitionSpeed = ((transitionTime * 1000) / (maxTicks > 0 ? maxTicks : 1) );
  }
 
  state = true;
  targetRGB[0]=r;
  targetRGB[1]=g;
  targetRGB[2]=b;
  
  announceStats();
}

bool processJSON(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }
  
  float h = HSB[0];
  float s = HSB[1];
  float b = HSB[2];
  
  if ( root.containsKey("color") ) {
    h = root["color"]["h"];
    s = root["color"]["s"];
  }

  if (root.containsKey("brightness")) {
    b = root["brightness"];
  }
  
    HSB[0] = h;
    HSB[1] = s;
    setBrightnessValue(b);
    
    if ( root.containsKey("transition") ) {
      int transition = root["transition"];
      setRGB(transition);
      Serial.println("RGB updated with transition");
    } else {
      setRGB();
      Serial.println("RGB updtated without transition");
    }
  

  if (root.containsKey("state")) {
    if (strcmp(root["state"], CONFIG_JSON_PAYLOAD_ON) == 0) {
      setState(true);     
    } else if (strcmp(root["state"], CONFIG_JSON_PAYLOAD_OFF) == 0) {
      setState(false);
    }
    
  }

  return true;
}

void setBrightnessValue(float brightness){
  if (brightness < 1) {
      return;
  } else if ( brightness > 255 ) {
      return;
  }
  
  if ( CONFIG_MIN_BRIGHTNESS > 1 || CONFIG_MAX_BRIGHTNESS < 255 ) {
    brightness = map(brightness, 0, 255, CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS);
  }
  
  HSB[2] = brightness;
  Serial.println("Set Brightness to " + String(brightness));
  
}

void setState( bool newState ) { 
  if ( newState == false && state != false ) {
    // copy the targetRGB to restoreRGB
    // so we can restore settings when we turn back on
    // there must be a more efficient way to do this
    transitionSpeed = CONFIG_TRANSITION_SPEED;
    
    restoreRGB[0] = targetRGB[0];
    restoreRGB[1] = targetRGB[1];
    restoreRGB[2] = targetRGB[2];
    
    targetRGB[0] = 0;
    targetRGB[1] = 0;
    targetRGB[2] = 0;
    
    state = false;
    announceStats();
    
    Serial.println("Light has been turned off");
  } else if ( newState == true && state != true ) { 
    targetRGB[0] = restoreRGB[0];
    targetRGB[1] = restoreRGB[1];
    targetRGB[2] = restoreRGB[2];
    state = true;
    announceStats();
    
    Serial.println("Light has been turned on");
  }
}

bool hsbHandler(HomieRange range, String value) {
  stringSetHSB(value);

  if ( stringSetHSB(value) ) {
    setRGB();
  }
  
  return true;
}

bool colorHandler(HomieRange range, String value) {
  int commaIndex = value.indexOf(',');
  float hue = value.substring(0, commaIndex).toFloat();
  float sat = value.substring(commaIndex +1).toFloat();

  // only call setRGB if the values have changed
  if (hue == HSB[0] && sat == HSB[1]){
    Serial.println("Hue & Sat values are identical, no changes made");
    return true;
  }

  HSB[0] = hue;
  HSB[1] = sat;
    
  setRGB();
  
  Serial.println("color changed to " + String(hue) + String(sat));
  return true;
}

bool jsonHandler(HomieRange range, String value) {
  if (!processJSON(string2char(value))) {
    Serial.println("Error processing JSON data!");
    return false;
  }
  
  Serial.println("Successfully processed JSON data!");
  return true;
}

bool stateHandler(HomieRange range, String value) {
  if (value != "true" && value != "false") {
    Serial.println("Invalid state value sent, unable to parse");
    Serial.println("Value sent: " + value);
    return false;
  }

  bool on = (value =="true");
  
  setState(on);
  
  Serial.println("Changed state to " + String(on));
  return true;
}

bool brightnessHandler(HomieRange range, String value) {
  
  int bri = value.toInt();

  state = true;

  setBrightnessValue(bri);

  setRGB();

  Serial.println("Changed brightness to " + String(bri));
  
  return true;
}

bool brightnessPercentHandler(HomieRange range, String value) {
  int pct = value.toInt();

  if (pct < 1) {
    pct = 1;
  } else if ( pct > 100 ) {
    pct = 100;
  }

  int bri = map(pct, 0, 100, 0, 255);

  state = true;

  setBrightnessValue(bri);

  setRGB();

  Serial.println("Changed brightness to " + String(bri));
  return true;
}

void homieSetup() {
  Serial.println("WiFi Connected; Startup Complete!");
  
  HSB[0] = CONFIG_DEFAULT_HUE;
  HSB[1] = CONFIG_DEFAULT_SAT;
  HSB[2] = CONFIG_DEFAULT_BRI;
  
  if ( CONFIG_DEFAULT_STATE == true) {
    setRGB();
  } 
  
}

void homieLoop() {
  unsigned int currentMillis = millis();

  if ( currentMillis - prevStateCallMillis >= callStateDelay ) {
    announceStats();
    prevStateCallMillis = currentMillis;
  }
}

void setup() { 
  if ( CONFIG_DEBUG == true ) {
    Serial.begin(115200);
    Serial.println("Starting...");
  }

  Homie_setBrand("lhl");
  Homie_setFirmware("lion-home-light", "1.0.0");

  lightNode.advertise("on").settable(stateHandler);
  lightNode.advertise("huesat").settable(colorHandler);
  lightNode.advertise("hsb").settable(hsbHandler);
  lightNode.advertise("brightness").settable(brightnessHandler);
  lightNode.advertise("brightnesspct").settable(brightnessPercentHandler);
  lightNode.advertise("json").settable(jsonHandler);
  
  analogWriteFreq(400);
  analogWriteRange(255);

  Homie.setSetupFunction(homieSetup);
  Homie.setLoopFunction(homieLoop);
  Homie.setup();
  
}
 
void loop()  { 
  unsigned long currentMillis = millis();
  
  if (currentMillis - prevTransitionMillis >= transitionSpeed) {
    for (int i = 0; i < 3; i++) {
      if ( currentRGB[i] != targetRGB[i] ) {
          if ( currentRGB[i] > targetRGB[i] ) {
            currentRGB[i]--;
          } else {
            currentRGB[i]++;
          }
          
          if ( CONFIG_INVERT_LOGIC == true ) {
            int inverted = (255 - currentRGB[i]);
            analogWrite(ledPins[i], inverted);
          }else {
            analogWrite(ledPins[i], currentRGB[i]);
          }      
      }
    }
  
    prevTransitionMillis = currentMillis;
 }
 Homie.loop();
}
