#define LED_PIN 2
#define OUT_PIN 1
#define BTN_PIN 0

bool power_on = false;
bool powering_off = false;
unsigned long power_off_start_time = 0;
unsigned long led_time = 0;
bool led_state = true;

unsigned long last_debounce_time = 0;
int last_button_state = LOW;
int button_state;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);

  digitalWrite(LED_PIN, power_on ? HIGH : LOW);
  digitalWrite(OUT_PIN, power_on ? HIGH : LOW);
}

bool check_button() {
  bool result = false;
  int reading = digitalRead(BTN_PIN);

  if (reading != last_button_state) 
    last_debounce_time = millis();

  if (millis() - last_debounce_time > 50UL) {
    if (reading != button_state) {
      button_state = reading;
      if (button_state == LOW) 
        result = true;
    }
  }

  last_button_state = reading;
  return result;
}

void loop() {
  if (check_button()) {
    if (powering_off) {
      powering_off = false;
      digitalWrite(LED_PIN, LOW);
    } else if (!power_on) {
      power_on = true;
      digitalWrite(OUT_PIN, HIGH);
    } else if (power_on) {
      powering_off = true;
      power_off_start_time = millis();
      digitalWrite(LED_PIN, HIGH);
    }
  }

  if (powering_off) {
    if ((millis() - power_off_start_time) > 5000) {
      power_on = false;
      powering_off = false;
      digitalWrite(OUT_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
    } else {
      unsigned long new_time = millis();
      if (new_time - led_time >= 500) {
          led_time = new_time;
          led_state = !led_state;
      }
    }
  } else {
    led_state = power_on;
  }

  digitalWrite(LED_PIN, led_state ? HIGH : LOW);
}
