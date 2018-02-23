#include <LiquidCrystal_I2C.h>
#include <Eventually.h>

#define BACKLIGHT_PIN     3

// Set the LCD I2C address
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, BACKLIGHT_PIN, POSITIVE );

#define DEBUG false
#define DEBOUNCE 100 // milliseconds
#define TIMEOUT 30000  // 30 seconds
#define ON_PRESS HIGH
#define ON_RELEASE LOW
#define RELAY_OPEN HIGH
#define RELAY_CLOSED LOW
#define KEY_PIN 43
#define NUM_PINS 11
#define NUM_PAGES 7

#define BTN_SCROLL 10  // input pins
#define BTN_UP 11
#define BTN_DOWN 12
#define UP_NUM 0
#define DOWN_NUM 1

EvtManager mgr;
int currentPage = 0;
bool isAwake = false;
bool btnPressed = false;

String bedControls[NUM_PINS] =
{ // index
  "Bed Flat",           // 0
  "Chair",              // 1
  "Head Up",            // 2
  "Head Down",          // 3
  "Knee Up",            // 4
  "Knee Down",          // 5
  "Foot Extend",        // 6
  "Foot Retract",       // 7
  "Trendelenburg",      // 8
  "Rev. Trendelenburg", // 9
  "Unlock"              // 10   
};

int relayPinNumbers[NUM_PINS] =  // Enter actual pin #s to match above bed control order
{
  //  22, 24, 26, 28, 30, 32, 34, 36, 38, 40
  23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43
  //  0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};

int pages[NUM_PAGES][2] =
{ // tuples of what control to display and activate per page
  {0, 1}, // bed flat, chair
  {2, 3}, // Head up, down
  {4, 5}, // Knee up, down
  {6, 7}, // Foot extend, retract
  {8, 0}, // Trend., flat
  {9, 0}, // Rev. Trend., flat
  {9, 1}  // Chair, Rev. Trend.
};

void setup()
{
  pinMode(BTN_SCROLL, INPUT);
  pinMode(BTN_UP, INPUT);
  pinMode(BTN_DOWN, INPUT);

  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(relayPinNumbers[i], OUTPUT);
  };

  lcd.begin(16, 2);              // initialize the lcd
  displayPage();
  debugAddNextPageTimer();
  addPressBtnScrollListener();
  openAllRelays();
}

void displayPage()
{
  lcd.home();
  lcd.clear();
  lcd.print(getBtnDescription(UP_NUM));
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print(getBtnDescription(DOWN_NUM));
  delay(50); // give LCD time to finish displaying text
  startTimeout();
};

void wakeUp() {
  mgr.resetContext();
  isAwake = true;
  btnPressed = false;
  addPressBtnScrollListener();
  addPressBtnUpListener();
  addPressBtnDownListener();
  displayPage();
};

void sleep() { // stop and reset everything!
  mgr.resetContext();
  openAllRelays();
  btnPressed = false;
  isAwake = false;
  lightOff();
  addPressBtnScrollListener();
};

void nextPage() {
  currentPage = currentPage + 1;
  if (currentPage >= NUM_PAGES)
    currentPage = 0;
  wakeUp();
  debugAddNextPageTimer();
};

void scrollPressed() {
  mgr.resetContext();
  btnPressed = true;
  addReleaseBtnScrollListener(); // ignores other buttons
  lightOn();
  delay(10);
  startTimeout(); // goes to sleep if not released in time
  // ignore all other button events
};

void scrollReleased() {
  if ((isAwake == true) & (btnPressed == true)) {
    btnPressed = false;
    nextPage();
  }
  else if ((isAwake == false) & (btnPressed == true)) {
    wakeUp();
  }
  else {
    sleep();
  };
};

void upPressed() {
  mgr.resetContext();
  addReleaseBtnUpListener();
  pressButton(UP_NUM);
};

void downPressed() {
  mgr.resetContext();
  addReleaseBtnDownListener();
  pressButton(DOWN_NUM);
};

void pressButton(int btnNum) {
  int pin = btnPinNum(btnNum);
  lcd.clear();
  lcd.print(String(pin) + "-" + getBtnDescription(btnNum));
  lcd.setCursor ( 0, 1 );
  lcd.print(" pressed");
  unlockBedControl();
  digitalWrite(pin, RELAY_CLOSED);
  startTimeout();
};

// Press the unlock button
void unlockBedControl() { 
  digitalWrite(KEY_PIN, RELAY_CLOSED);
  delay(250);
  digitalWrite(KEY_PIN, RELAY_OPEN);
};
  
void upReleased() {
  openAllRelays(); // should I just open the one relay?
  //    digitalWrite(relayPinNumbers[btnUpPinNum()], LOW);
  wakeUp();
};

void downReleased() {
  openAllRelays(); // should I just open the one relay?
  //    digitalWrite(relayPinNumbers[btnDownPinNum()], LOW);
  wakeUp();
};

void openAllRelays() {
  // set every relay to open
  for (int i = 0; i < NUM_PINS; i++) {
    digitalWrite(relayPinNumbers[i], RELAY_OPEN);
  }
};

String getBtnDescription(int num) {
  // return String(btnPinNum(num)) + "-" + bedControls[pages[currentPage][num]];
  return bedControls[pages[currentPage][num]];
};

int btnPinNum(int num) {
  return relayPinNumbers[pages[currentPage][num]];
};

/*
 String getBtnUpDescription() {
  return String(btnPinNum(0)) + "-" + bedControls[pages[currentPage][0]];
  // return bedControls[pages[currentPage][0]];
};

String getBtnDownDescription() {
  return String(btnPinNum(1)) + "-" + bedControls[pages[currentPage][1]];
  // return bedControls[pages[currentPage][1]];
};
*/

void lightOn() {
  lcd.setBacklightPin( BACKLIGHT_PIN, HIGH );
}

void lightOff() {
  lcd.clear();
  lcd.setBacklightPin( BACKLIGHT_PIN, LOW );
}

void startTimeout() {
  mgr.addListener(new EvtTimeListener(TIMEOUT, false, (EvtAction)sleep));
}

void addPressBtnScrollListener() {
  mgr.addListener(new EvtPinListener(BTN_SCROLL, DEBOUNCE, (EvtAction)scrollPressed, HIGH));
};

void addReleaseBtnScrollListener() {
  // Eventually framework has been changed to be able to set target value == LOW, or ON_RELEASE
  mgr.addListener(new EvtPinListener(BTN_SCROLL, DEBOUNCE, (EvtAction)scrollReleased, LOW));
};

void addPressBtnUpListener() {
  mgr.addListener(new EvtPinListener(BTN_UP, DEBOUNCE, (EvtAction)upPressed, HIGH));
};

void addPressBtnDownListener() {
  mgr.addListener(new EvtPinListener(BTN_DOWN, DEBOUNCE, (EvtAction)downPressed, HIGH));
};

void addReleaseBtnUpListener() {
  mgr.addListener(new EvtPinListener(BTN_UP, DEBOUNCE, (EvtAction)upReleased, LOW));
};

void addReleaseBtnDownListener() {
  mgr.addListener(new EvtPinListener(BTN_DOWN, DEBOUNCE, (EvtAction)downReleased, LOW));
};

void debugAddNextPageTimer() {
  if (DEBUG == true)
    mgr.addListener(new EvtTimeListener(1500, false, (EvtAction)nextPage));
};

USE_EVENTUALLY_LOOP(mgr);
