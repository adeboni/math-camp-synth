#define LED_LINK_PIN  5
#define LED_1_PIN     19
#define LED_2_PIN     16
#define BUTTON_1_PIN  18
#define BUTTON_2_PIN  17
#define RELAY_1_PIN   15
#define RELAY_2_PIN   32
#define EXT_LED_PIN   33
#define EXT_BTN_PIN   14
#define ADC_PIN       34
#define HLWBL_SEL_PIN 25
#define HLWBL_CF_PIN  26
#define BL0937_CF_PIN 27

bool power_on[2] = {false, false};
bool powering_off = false;
unsigned long power_off_start_time = 0;
unsigned long ext_led_time = 0;
bool ext_led_state = true;

int relays[2] = {RELAY_1_PIN, RELAY_2_PIN};
int leds[3] = {LED_1_PIN, LED_2_PIN, EXT_LED_PIN};
int buttons[3] = {BUTTON_1_PIN, BUTTON_2_PIN, EXT_BTN_PIN};
int target_state[3] = {HIGH, HIGH, LOW};
bool button_state[3] = {false, false, true};
bool last_button_state[3] = {false, false, true};
unsigned long last_debounce_time[3] = {0, 0, 0};

void set_relay(int index, bool state) {
  power_on[index] = state;
  digitalWrite(relays[index], state);
  digitalWrite(leds[index], !state);
}

void setup() {
  pinMode(LED_LINK_PIN, OUTPUT);
  pinMode(LED_1_PIN, OUTPUT);
  pinMode(LED_2_PIN, OUTPUT);
  pinMode(EXT_LED_PIN, OUTPUT);
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(BUTTON_1_PIN, INPUT_PULLDOWN);
  pinMode(BUTTON_2_PIN, INPUT_PULLDOWN);
  pinMode(EXT_BTN_PIN, INPUT_PULLUP);
  pinMode(ADC_PIN, INPUT);

  digitalWrite(LED_LINK_PIN, LOW);
  set_relay(0, false);
  set_relay(1, false);
}

bool check_button(int index) { 
  int reading = digitalRead(buttons[index]);
  unsigned long new_time = millis();

  if (reading != last_button_state[index]) {
    last_debounce_time[index] = new_time;
    last_button_state[index] = reading;
  }

  if (new_time - last_debounce_time[index] > 50UL) {
    if (reading != button_state[index]) {
      button_state[index] = reading;
      if (button_state[index] == target_state[index]) {
        if (index < 2) set_relay(index, !power_on[index]);
        return true;
      }
    }
  }
  
  return false;
}

void loop() {
  check_button(0);
  check_button(1);
  
  if (check_button(2)) {
    if (powering_off) {
      powering_off = false;
    } else if (power_on[0] || power_on[1]) {
      powering_off = true;
      power_off_start_time = millis();
    } else {
      set_relay(0, true);
      set_relay(1, true);
    }
  }

  if (powering_off) {
    unsigned long new_time = millis();
    if ((new_time - power_off_start_time) > 5000) {
      powering_off = false;
      set_relay(0, false);
      set_relay(1, false);
      ext_led_state = false;
    } else {
      if (new_time - ext_led_time >= 500) {
          ext_led_time = new_time;
          ext_led_state = !ext_led_state;
      }
    }
  } else {
    ext_led_state = power_on[0] || power_on[1];
  }

  digitalWrite(EXT_LED_PIN, ext_led_state);
}
