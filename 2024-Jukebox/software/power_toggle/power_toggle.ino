#define LED_PIN 2
#define OUT_PIN 1
#define BTN_PIN 0

bool power_on = false;
bool powering_off = false;
bool button_pressed = false;
unsigned long power_off_start_time = 0;
unsigned long led_time = 0;
bool led_state = true;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);

  digitalWrite(LED_PIN, power_on ? HIGH : LOW);
  digitalWrite(OUT_PIN, power_on ? HIGH : LOW);
}

bool check_button() {
  if (digitalRead(BTN_PIN) == LOW && !button_pressed) {
    button_pressed = true;
  } else if (digitalRead(BTN_PIN) == HIGH && button_pressed) {
    button_pressed = false;
    return true;
  }
  return false;
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
