//  This controls LED, not sure if it deserves to be in a module or not
//  Simple 3 bit input , corresponding to:
//  R  G  B
//  4  2  1
void SetLED(uint8_t color) {
  digitalWrite(BluePin, (color & 1) ==  1 ? HIGH : LOW );   // sets the blue LED on if ANDed with 1
  digitalWrite(GreenPin, (color & 2) == 2 ? HIGH : LOW  );   // sets the LED on if ANDed w 2
  digitalWrite(RedPin, (color & 4) == 4 ? HIGH : LOW  );   // sets the LED onif ANDed w 4
}

