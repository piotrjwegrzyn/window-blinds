void setup() {
  Serial.begin(9600);
}

uint8_t get_ph_r_val_perc() {
  return static_cast<uint8_t>((analogRead(A0) * 100) / 1023);
}

void loop() {
  delay(3000);
  Serial.printf("Current voltage is: %d\n", get_ph_r_val_perc());
}
