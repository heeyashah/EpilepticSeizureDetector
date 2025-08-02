#include <LiquidCrystal.h>

const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7; //define pins used by the LCD screen 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); //creating instance of lcd as Liquid Crystal

String incomingLine = ""; // empty string to store received message

void setup() {
  lcd.begin(16, 2); // (col, row) of LCD screen
  Serial.begin(9600); //set baud rate (# of signal changes or symbols sent per second)
  lcd.print("Waiting..."); 
}

void loop() {
  while (Serial.available() > 0) { //while you have incoming data 
    char c = Serial.read(); //read each character by character
    if (c == '\n' || c == '\r') { //if character is new line or return 
      incomingLine.trim(); //delete the space

      if (incomingLine.startsWith("@")) { //if the line is the beginning of the message
        int typeSep = incomingLine.indexOf('|'); //defines what seperates the words between type & state
        if (typeSep != -1) {
          // seperate the message into type and status 
          String type = incomingLine.substring(1, typeSep);
          String status = incomingLine.substring(typeSep + 1);

          //formatting LCD screen 
          lcd.clear();
          lcd.setCursor(0, 0);

          // alerts for the type
          if (type == "SPD") {
            if (status == "LOW") {
              lcd.print("All good :)");
              lcd.setCursor(0, 1);
              lcd.print("Low speed");
            } else if (status == "MED") {
              lcd.print("Warning!");
              lcd.setCursor(0, 1);
              lcd.print("Medium speed");
            } else if (status == "HIGH") {
              lcd.print("Alert!");
              lcd.setCursor(0, 1);
              lcd.print("High speed");
            }
          } else if (type == "EMER") {
            lcd.print("Emergency!");
            lcd.setCursor(0, 1);
            lcd.print("Call 911");
          } 
        }
      }

      incomingLine = ""; //clearing incoming line 
    } else {
      incomingLine += c; //keep reading to the next character of the message
    }
  }
}