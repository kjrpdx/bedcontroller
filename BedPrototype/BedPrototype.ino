#include <LiquidCrystal_I2C.h>
#include <Eventually.h>

#define BACKLIGHT_PIN     3

// Set the LCD I2C address
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, BACKLIGHT_PIN, POSITIVE );

#define DEBUG false
#define DEBOUNCE 100 // milliseconds
#define TIMEOUT 30000
#define ON_PRESS HIGH
#define ON_RELEASE LOW
#define RELAY_OPEN HIGH
#define RELAY_CLOSED LOW
#define NUM_PINS 10
#define NUM_PAGES 7

#define BTN_SCROLL 10
#define BTN_UP 11
#define BTN_DOWN 12

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
  "Rev. Trendelenburg"  // 9
};

int relayPinNumbers[NUM_PINS] =  // Enter actual pin #s to match above bed control order
{
  //  22, 24, 26, 28, 30, 32, 34, 36, 38, 40
  23, 25, 27, 29, 31, 33, 35, 37, 39, 41
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
  lcd.print(getBtnUpDescription());
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print(getBtnDownDescription());
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
  //  Just ignore other button presses?
  //  mgr.addListener(new EvtPinListener(BTN_SCROLL, (EvtAction)sleep));
  //  mgr.addListener(new EvtPinListener(BTN_DOWN, (EvtAction)sleep));
  // turn on appropriate relay
  // *** HERE! ***
  lcd.clear();
  lcd.print(String(btnUpPinNum()) + "-up pressed");
  lcd.setCursor ( 0, 1 );    
  lcd.print(getBtnUpDescription());
  digitalWrite(btnUpPinNum(), RELAY_CLOSED);
  startTimeout();
};

void downPressed() {
  mgr.resetContext();
  addReleaseBtnDownListener();
  //  Just ignore other button presses?
  //  mgr.addListener(new EvtPinListener(BTN_SCROLL, (EvtAction)sleep));
  //  mgr.addListener(new EvtPinListener(BTN_UP, (EvtAction)sleep));
  // turn on appropriate relay
  // *** HERE! ***
  lcd.clear();
  lcd.print(String(btnDownPinNum()) + "-down pressed");
  lcd.setCursor ( 0, 1 );    
  lcd.print(getBtnDownDescription());
  digitalWrite(btnDownPinNum(), RELAY_CLOSED);
  startTimeout();
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
  // set every relay to low
  for (int i = 0; i < NUM_PINS; i++) {
    // Open each relay accessible in relayPinNumbers[]
    // *** HERE! ***
    digitalWrite(relayPinNumbers[i], RELAY_OPEN);
  }
};

String getBtnUpDescription() {
  // return String(btnUpPinNum()) + "-" + bedControls[pages[currentPage][0]];
  return bedControls[pages[currentPage][0]];
};

String getBtnDownDescription() {
  // return String(btnDownPinNum()) + "-" + bedControls[pages[currentPage][1]];
  return bedControls[pages[currentPage][1]];
};

String longBtnUpDescription() {
  return String(btnUpPinNum()) + "-" + bedControls[pages[currentPage][0]];
};

String longBtnDownDescription() {
  return String(btnDownPinNum()) + "-" + bedControls[pages[currentPage][1]];
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

int btnUpPinNum() {
  return relayPinNumbers[pages[currentPage][0]];
};
int btnDownPinNum() {
  return relayPinNumbers[pages[currentPage][1]];
};

USE_EVENTUALLY_LOOP(mgr);
