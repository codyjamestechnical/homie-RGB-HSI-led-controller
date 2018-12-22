// how long between color steps in milliseconds
// default value, a different value will be 
// automatically calculated if a transition time
// is supplied over MQTT JSON
#define CONFIG_TRANSITION_SPEED 3

// time between calling current state to server
#define CONFIG_CALL_STATE_DELAY 5000

// LED pin assignments
#define CONFIG_PIN_RED 2
#define CONFIG_PIN_GREEN 4
#define CONFIG_PIN_BLUE 5

// set on/off values for MQTT JSON commands
#define CONFIG_JSON_PAYLOAD_ON "ON"
#define CONFIG_JSON_PAYLOAD_OFF "OFF"

// Reverse the LED logic
// false: 0 (off) - 255 (bright)
// true: 255 (off) - 0 (bright)
#define CONFIG_INVERT_LOGIC true

// Enables Serial and Print Statements
#define CONFIG_DEBUG true

// this is required, and is the minimum/maximum brightness allowed
// 0 - 255 are acceptable values
// this is useful if you have LEDs that don't look like they're on below a certain value
// or are too bright above certain values
#define CONFIG_MIN_BRIGHTNESS 5
#define CONFIG_MAX_BRIGHTNESS 255

#define CONFIG_DEFAULT_HUE 160
#define CONFIG_DEFAULT_SAT 1
#define CONFIG_DEFAULT_BRI 255
#define CONFIG_DEFAULT_STATE false

