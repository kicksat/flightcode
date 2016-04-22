#define NO_PORTB_PINCHANGES
#define NO_PORTC_PINCHANGES

#include <EasyTransfer.h>
#include <LowPower.h>
#include <PinChangeInt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// Pins and Constants
#define RESET_ENB       12
#define LED             13
#define WD_PIN          1   // labeled TX0
#define SW_PIN           0   
#define TEST_LED        10
#define WD_MULT         1
#define SERIAL_RX_TIME  2500


unsigned long last_updated = 0;  // seconds
unsigned long now = 0;           // milliseconds
volatile char reset_flag = 0;    // for hysteresis
volatile uint8_t wd_count = 0;
int led_flip = 1;
volatile int wd_mode = 1;    // serial mode if 0
const char ack_buff[4] = {'K', 'S', 'A', '\n'};

// Structs for serial communication

typedef struct info {
  uint64_t seconds;
  uint64_t x_ecef;
  uint64_t y_ecef;
  uint64_t z_ecef;
  int32_t x_vel;
  int32_t y_vel;
  int32_t z_vel;
  uint8_t burnwires;
  char padding[3];
} info;

EasyTransfer ET;

info satInfo;

// Called when rising pin change detected on pin 0
void heartbeat() {
  reset_flag = 0;
  digitalWrite(LED, LOW);
  wdt_reset();
}

void switchToSerial() {
  wd_mode = 0;
  
}

void serialMode() {
  // TODO: Implement switching modes
  detachPinChangeInterrupt(WD_PIN);
  detachPinChangeInterrupt(SW_PIN);
  Serial.begin(9600);
  ET.begin(details(satInfo), &Serial);
  
//  pinMode(LED, OUTPUT);
//  ET_data.begin(details(sat_info), &Serial);
//  reset_flag = 0;
//  digitalWrite(TEST_LED, LOW);
//  wdt_reset();
}

void watchdogMode() {
  wd_mode = 1;
  pinMode(WD_PIN, INPUT);
  pinMode(SW_PIN, INPUT);
  attachPinChangeInterrupt(WD_PIN, heartbeat, RISING);
  attachPinChangeInterrupt(SW_PIN, switchToSerial, RISING);
  
}

void setup() {
  pinMode(RESET_ENB, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(WD_PIN, INPUT);
  pinMode(SW_PIN, INPUT);
  pinMode(TEST_LED, OUTPUT);

  attachPinChangeInterrupt(WD_PIN, heartbeat, RISING);
  attachPinChangeInterrupt(SW_PIN, switchToSerial, RISING);
  
//  ET_data.begin(details(sat_info), &Serial);

  for (int k = 0; k < 3; k++) {
    digitalWrite(LED, HIGH);
    delay(250);
    digitalWrite(LED, LOW);
    delay(250);
   }
  delay(1000);

//  for(int i = (sizeof(satInfo)-40); i>0; i--){
//    digitalWrite(13, HIGH);
//    delay(250);
//    digitalWrite(13, LOW);
//    delay(250);
//  }
  
}

void loop() {
//  digitalWrite(TEST_LED, HIGH);
  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);

//  digitalWrite(TEST_LED, HIGH);
//  delay(2000);
//  digitalWrite(TEST_LED, LOW);
  // CHECK MODE AND THEN DO SERIAL STUFF
//  wd_mode = 0;
  satInfo.x_vel = 2;
  if (wd_mode == 0) {
//    (led_flip) ? digitalWrite(LED, HIGH) : digitalWrite(LED, LOW);
//    led_flip = !led_flip;
    serialMode();
//    ET.begin(details(satInfo), &Serial);
    now = millis();
    
    while (millis() - now < SERIAL_RX_TIME ) {
      if (ET.receiveData()) {
        digitalWrite(TEST_LED, HIGH);
        Serial.write(ack_buff, 4);
        break;
//       Just for fun, we will blink it out too
//       for (int i = satInfo.x_vel; i>0; i--) {
//          digitalWrite(LED, HIGH);
//          delay(300);
//          digitalWrite(LED, LOW);
//          delay(300);
//        }
      }
      delay(100);  //TODO: REMOVE?
      digitalWrite(TEST_LED, LOW);
    }
//    digitalWrite(TEST_LED, LOW);
    Serial.end();
    watchdogMode();
//    (led_flip) ? digitalWrite(TEST_LED, HIGH) : digitalWrite(TEST_LED, LOW);
//    led_flip = !led_flip;
//    digitalWrite(TEST_LED, LOW);
  }

//  delay(1000);
  
  // TESTING STUFF
//  digitalWrite(TEST_LED, HIGH);
//  delay(1000);
//  digitalWrite(TEST_LED, LOW);

}



void wdt_setup() {

  wdt_reset();
  // disable interrupts
  cli();
  // Enter Watchdog Configuration mode:
  WDTCSR |= (1<<WDCE) | (1<<WDE);   
  // Set Watchdog settings:
  //        enb int     no reset
  // WDTCSR = (1<<WDIE) | (0<<WDE) ;
   WDTCSR = 0b01100000;   // last bit goes between 4 and 8 secs
   sei();
}

ISR(WDT_vect) {
  wd_count++;
  wdt_reset();
//  (led_flip) ? digitalWrite(TEST_LED, HIGH) : digitalWrite(TEST_LED, LOW);
//  led_flip = !led_flip;
  if (wd_count > WD_MULT - 1) {
    digitalWrite(LED, HIGH);
  //  digitalWrite(TEST_LED, HIGH);
  //  (led_flip) ? digitalWrite(LED, HIGH) : digitalWrite(LED, LOW);
  //  led_flip = !led_flip;
  //  if (reset_flag) {
  //    digitalWrite(LED,HIGH);
  //  }
    reset_flag = 1;
    wd_count = 0;
  } 
}

//ISR(PCINT2_vect) {
////  if (digitalRead(WD_PIN)) {
//  if(PIND & (1 << PD1)){
//    reset_flag = 0;
//    digitalWrite(LED, LOW);
//    wdt_reset();
////    (led_flip) ? digitalWrite(LED, HIGH) : digitalWrite(LED, LOW);
////    led_flip = !led_flip;
//  }
//}

void sleepNow() {

    attachInterrupt(0, pinInterrupt, LOW);
    delay(50);
    // Choose our preferred sleep mode:
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
 
    // Set sleep enable (SE) bit:
    sleep_enable();
 
    // Put the device to sleep:
    sleep_mode();
 
    // Upon waking up, sketch continues from this point.
    sleep_disable();
}

void pinSetup() {
  cli();
  PCICR |= 0b00000100;    // turn on port d
  PCMSK2 |= 0b00000010;    // ,PCINT17, physical pin tx0
  sei();
}

void pinInterrupt(void)
{
    detachInterrupt(0);
}

// TODO: fix this
void resetBoard() {
  if (reset_flag == 0) {
    reset_flag = 1;
    Serial.println("reset flag set");
    // send signal to main board?
  } else if (reset_flag == 1) {
    // actually reset
    digitalWrite(LED, LOW);
    Serial.println("resetting");
    reset_flag = 0;
    delay(100);
  }
}
  

