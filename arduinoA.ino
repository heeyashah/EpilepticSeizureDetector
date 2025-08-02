// --- Pin Definitions ---
#define INTERRUPT_PIN 2
#define LED_PIN 13          
#define GREEN_LED_PIN 9
#define WHITE_LED_PIN 10
#define RED_LED_PIN 11
#define BLUE_LED_PIN 12
#define LDR_PIN 4
#define Button_1 5
#define Button_2 6

// ---  Flags and Control Variables ---
volatile bool LDRTriggered = false; 
volatile bool Button_1_Triggered = false; 
volatile bool Button_2_Triggered = false; 

volatile bool greenLEDOn = false;
volatile bool whiteLEDOn = false;
volatile bool redLEDOn = false;
volatile bool blueLEDOn = false;
volatile bool emergency = false;

// --- Timer Counters ---
volatile uint16_t LEDCounter = 0;  // Counts timer ticks for blinking LED
volatile uint16_t flashInterval = 8; // Default to low speed flashing interval (8 * 64ms = 512ms)

volatile uint16_t LDRTriggerTime= 0; // LDR active duration (for speed classification)
volatile uint16_t LDRCounter = 0;  // Timer counter 

volatile uint16_t whiteLEDTime = 0; 
volatile uint16_t redLEDTime = 0;

volatile bool redAlarmAcknowledged = false;
volatile bool whiteAlarmAcknowledged = false;

// --- Flash intervals in 64ms steps ---
#define LOW_SPEED_INTERVAL 8   // Low speed (8 * 64ms = 512ms)
#define MEDIUM_SPEED_INTERVAL 4 // Medium speed (4 * 64ms = 256ms)
#define HIGH_SPEED_INTERVAL 2   // High speed (2 * 64ms = 128ms)

// --- LDR Duration Thresholds for Speed Classification (in ms) ---
#define LOW_SPEED_Threshold 640
#define MEDIUM_SPEED_Threshold 512
#define HIGH_SPEED_Threshold 256

// --- Timeout Durations Before Escalation (in Timer Ticks) ---
#define WHITE_TIMEOUT_TICKS 78 //~5 seconds
#define RED_TIMEOUT_TICKS 100  // ~7 seconds 

// --- Speed State Enumeration ---
enum SpeedState {NONE, LOW_SPEED, MEDIUM_SPEED, HIGH_SPEED };
SpeedState currentSpeed;
SpeedState lastSpeed = NONE; 

void setup() {
  // Start serial communication
  Serial.begin(9600);
  
  // Configuration LED, button, and interrupt pins 
  pinMode(LED_PIN, OUTPUT);       
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);   // External interrupt pin 
  pinMode(Button_1, INPUT_PULLUP);
  pinMode(Button_2, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(WHITE_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  // Disable interrupts during timer configuration 
  noInterrupts();                                                     

  // --- Timer1 Configruation ---
  TCCR1A = 0;  // Normal operation (no PWM output)
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);  // CTC (clear timer on compare match) mode, prescaler = 1024
  
  OCR1A = 999; // Set compare match value, Timer1 ticks every 64µs → 1000 ticks = 64ms interval

  TIMSK1 |= (1 << OCIE1A); // Enable timer interrupt to trigger when Timer1 reaches OCR1A value

  // Enable external interrupt on rising edge
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), ISR_handler, RISING); 

  // Re-enable interrupts
  interrupts(); 

}

// External interrupt service routine 
void ISR_handler (){
  
  // If LDR sensor signal is HIGH, record trigger time
  if (digitalRead(LDR_PIN) == HIGH){
    LDRTriggerTime = LDRCounter*64; // Convert timer tiicks to ms
    LDRTriggered = true; // Set flag 
  }

  // If Button 1 is pressed, set its trigger flag
  if (digitalRead(Button_1) == HIGH){
    Button_1_Triggered = true;
  }
  
  // If Button 2 is pressed, set its trigger flag
  if (digitalRead(Button_2) == HIGH){
      Button_2_Triggered = true;
    }
}

// the timer interrupt service routine (runs everytime the OCR value is reached (every 64ms))
ISR(TIMER1_COMPA_vect) {
  //increment the counters (every 64ms)
  LEDCounter++; 
  LDRCounter++;

  // select the flashing interval 
  //flashInterval = LOW_SPEED_INTERVAL;  // Flash every 960ms (slow)
  flashInterval = MEDIUM_SPEED_INTERVAL;  // Flash every 512ms (moderate)
  //flashInterval = HIGH_SPEED_INTERVAL;  // Flash every 256ms (fast)
    
  //Toggle LED when the counter reaches the flash interval
  if (LEDCounter >= flashInterval) { 
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // flip state of LED
      LEDCounter = 0;  // Reset counter
  } 

  //increment counters for escalation logic (to track how long each LED has been on for)
  if (whiteLEDOn) whiteLEDTime++; 
  if (redLEDOn) redLEDTime++;
  
  // Escalate from white to red if it's been on too long
  if (whiteLEDOn && whiteLEDTime >= WHITE_TIMEOUT_TICKS) {
      greenLEDOn = false;
      whiteLEDOn = false;
      redLEDOn = true;
      blueLEDOn = false;
      updateLEDs(); //before calling this function make sure all the flags are what you want the state of the LEDs to be 
      Serial.println("escalate to red");

      whiteLEDTime = 0; //reset white timer becuase it is not off
      redLEDTime = 0; // reset red timer becuase it should start to count after this 
  }

  // Escalate from red to blue if it's been on too long
  if (redLEDOn && redLEDTime >= RED_TIMEOUT_TICKS) {

    redLEDTime = 0; //reset red timer becuase it is now off 
    emergency = true; // turns blue on in this flag 
    Serial.println("escalate to blue");

  }
}
//function to update the state of LEDs, based on the flags if TRUE = HIGH if FALSE = LOW 
void updateLEDs() {
  digitalWrite(GREEN_LED_PIN, greenLEDOn ? HIGH : LOW);
  digitalWrite(WHITE_LED_PIN, whiteLEDOn ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, redLEDOn ? HIGH : LOW);
  digitalWrite(BLUE_LED_PIN, blueLEDOn ? HIGH : LOW);

}

void loop() {
  //define emergency state
  if (emergency){ 
    // if button 1 is pressed, exit emergency state 
    if (digitalRead(Button_1) == HIGH){
      greenLEDOn = false;
      whiteLEDOn = false;
      redLEDOn = false;
      blueLEDOn = false;
      updateLEDs();
      emergency = false; // set emergency to false to exit 
    }
    else {
      //stay stuck in the emergency state with blue LED on 
      sendStructuredMessage("EMER", "EMERGENCY"); // to communicate to Arduino B
      greenLEDOn = false;
      whiteLEDOn = false;
      redLEDOn = false;
      blueLEDOn = true;
      updateLEDs();
    }
  }
  //define what happens when the LDR is triggered
  if (LDRTriggered){
    LDRTriggered = false; //reset so you dont stay in this forever 

    //check the speed that the LDR is flashing at & update the current speed 
    if (LDRTriggerTime >= LOW_SPEED_Threshold){
      sendStructuredMessage("SPD", "LOW"); // to communicate to Arduino B (type, status)
      currentSpeed = LOW_SPEED;
    }
    else if (LDRTriggerTime >= MEDIUM_SPEED_Threshold){
      sendStructuredMessage("SPD", "MED"); // to communicate to Arduino B (type, status)
      currentSpeed = MEDIUM_SPEED;
    }
    else {
      sendStructuredMessage("SPD", "HIGH"); // to communicate to Arduino B (type, status)
      currentSpeed = HIGH_SPEED;
    }

    if (currentSpeed != lastSpeed && !redAlarmAcknowledged && !whiteAlarmAcknowledged){ //if the speed changes and you arent trying to acknowledge alarms
      lastSpeed = currentSpeed; //update last speed

      greenLEDOn = whiteLEDOn = redLEDOn = blueLEDOn = false;
      updateLEDs(); //reset all LED values
      whiteLEDTime = redLEDTime = 0; // reset timer values

      // if low speed & not emergency turn on green LED 
      if(!emergency && (currentSpeed == LOW_SPEED)){ 
        greenLEDOn = true;
        whiteLEDOn = false;
        redLEDOn = false;
        blueLEDOn = false;
        updateLEDs();
      }
      // if medium speed & not emergency turn on white LED 
      if (!emergency && (currentSpeed == MEDIUM_SPEED)){
        greenLEDOn = false;
        whiteLEDOn = true;
        redLEDOn = false;
        blueLEDOn = false;
        updateLEDs();
      }
      // if high speed & not emergency turn on red LED 
      if(!emergency && (currentSpeed == HIGH_SPEED)){
        greenLEDOn = false;
        whiteLEDOn = false;
        redLEDOn = true;
        blueLEDOn = false;
        updateLEDs();
      }
    }
    LDRCounter = 0; // reset counter after trigger is handled 
  }
  if (Button_1_Triggered){ //full reset of the system (all flags, timers, LEDs)
    Button_1_Triggered = false; //reset flag
    emergency = false; //clear the emergency state
    //clear all acknowledge alarm flags for resetting the system 
    redAlarmAcknowledged = false; 
    whiteAlarmAcknowledged = false; 

    greenLEDOn = false;
    whiteLEDOn = false;
    redLEDOn = false;
    blueLEDOn = false;
    updateLEDs();  // Turn off all LEDs
    whiteLEDTime = redLEDTime = 0; //reset timers

    // Reset counters
    LDRCounter = 0;
    LEDCounter = 0;
    //clear speeds 
    currentSpeed = NONE; 
    lastSpeed = NONE;
  }
  if (Button_2_Triggered){ // restarts the timer for the specific LED you are on 
    Button_2_Triggered = false; //reset flag
    if (emergency){
      return; //breaks if you are in the emergency state (cannot leave emergency by pressing button 2)
    }
    if (whiteLEDOn && whiteLEDTime < WHITE_TIMEOUT_TICKS){ // if the white LED is currently on, reset its timer 
      whiteLEDTime = 0;
      whiteAlarmAcknowledged = true; //update flag to prevent it from leaving this state
    }
    if (redLEDOn && redLEDTime < RED_TIMEOUT_TICKS){ //if the red LED is currently on, reset its timer
      redLEDTime = 0;
      redAlarmAcknowledged = true; //update flag to prevent it from leaving this state
  }
  lastSpeed = currentSpeed; //updates the last speed 
  LDRTriggered = false; //reset flag
  } 
}

//sends messages over UART 
void sendStructuredMessage(const String& type, const String& status) {
  Serial.print("@"); // @ means the beginning of a message 
  Serial.print(type); // "type"
  Serial.print("|"); // | is a seperator 
  Serial.println(status); // "status" is parameter
}

