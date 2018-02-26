#include <LiquidCrystal_I2C.h>
#include <Eventually.h>

#define BACKLIGHT_PIN     3

// Set the LCD I2C address
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, BACKLIGHT_PIN, POSITIVE );

#define DEBUG false
#define DEBOUNCE 100 // milliseconds
#define TIMEOUT 30000  // 30 seconds before I go to sleep
#define ON_PRESS HIGH
#define ON_RELEASE LOW
#define RELAY_OPEN HIGH
#define RELAY_CLOSED LOW
#define NUM_PINS 11
#define NUM_PAGES 7

#define SCROLL_PIN 10  // input pins
#define UP_PIN 11
#define DOWN_PIN 12
#define UNLOCK_PIN 43  // Close momentarily before other relays

#define UP_NUM 0    // Numbers for Up and Down buttons
#define DOWN_NUM 1  // which correspond to lines of display

EvtManager mgr;         // using the "Eventually" framework
int currentPage = 0;     // index of the currently displayed page
bool isAwake = false;    // if true, can operate bed on input
bool btnPressed = false; // Has SCROLL_PIN been activated?

String bedControls[NUM_PINS] =  // Names of the bed functions we can activate
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
{ // tuples of which controls to display and activate per page
  {0, 1}, // bed flat, chair
  {2, 3}, // Head up, down
  {4, 5}, // Knee up, down
  {6, 7}, // Foot extend, retract
  {8, 0}, // Trend., flat
  {9, 0}, // Rev. Trend., flat
  {9, 1}  // Rev. Trend., Chair
};

void setup()
{
  pinMode(SCROLL_PIN, INPUT);
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);

  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(relayPinNumbers[i], OUTPUT);
  };

  lcd.begin(16, 2);              // initialize the lcd
  openAllRelays();
  wakeUp();  // this is more consistent, I think, to be fully working on bootup

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
  startTimeout();  // Go to sleep if pressed too long
};

// Press the unlock button for a bit, then release
void unlockBedControl() { 
  digitalWrite(UNLOCK_PIN, RELAY_CLOSED);
  delay(250);
  digitalWrite(UNLOCK_PIN, RELAY_OPEN);
};

void upDownReleased() {
  openAllRelays(); 
  wakeUp();
};

void openAllRelays() {
  // set every relay to open
  for (int i = 0; i < NUM_PINS; i++) {
    digitalWrite(relayPinNumbers[i], RELAY_OPEN);
  }
};

String getBtnDescription(int btnNum) {
  // return String(btnPinNum(btnNum)) + "-" + bedControls[pages[currentPage][btnNum]];
  return bedControls[pages[currentPage][btnNum]];
};

int btnPinNum(int btnNum) {
  return relayPinNumbers[pages[currentPage][btnNum]];
};

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
  mgr.addListener(new EvtPinListener(SCROLL_PIN, DEBOUNCE, (EvtAction)scrollPressed, ON_PRESS));
};

void addReleaseBtnScrollListener() {
  // Eventually framework has been changed to be able to set target value == LOW, or ON_RELEASE
  mgr.addListener(new EvtPinListener(SCROLL_PIN, DEBOUNCE, (EvtAction)scrollReleased, ON_RELEASE));
};

void addPressBtnUpListener() {
  mgr.addListener(new EvtPinListener(UP_PIN, DEBOUNCE, (EvtAction)upPressed, ON_PRESS));
};

void addPressBtnDownListener() {
  mgr.addListener(new EvtPinListener(DOWN_PIN, DEBOUNCE, (EvtAction)downPressed, ON_PRESS));
};

void addReleaseBtnUpListener() {
  mgr.addListener(new EvtPinListener(UP_PIN, DEBOUNCE, (EvtAction)upDownReleased, ON_RELEASE));
};

void addReleaseBtnDownListener() {
  mgr.addListener(new EvtPinListener(DOWN_PIN, DEBOUNCE, (EvtAction)upDownReleased, ON_RELEASE));
};

USE_EVENTUALLY_LOOP(mgr);
