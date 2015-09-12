
//  This is the MVP AWS Kegerator codebase.  It employs a "naked" ESP8266 module to send beer-tap events to the cloud
//  The physical wiring is that 1 or 2 cheap hall-effect flow sensors will be connected to named GPIO pins on the
//  ESP8266 module with a simple zener diode circuit to allow the 5V sensor to drive input pins on a 3.3v module.
//
//  The software is *very* simple - two ISRs are used to simply increment each Tap's pulse count when triggered.
//  Unfortunately some debounce logic was needed as the test-input-switches used were really crunchy
//
//  The main loop simply looks for changes in the flow counters to walk through a simple FSM loop of "waiting" & "Tap Pulled"
//  We'll try to avoid hard coding to a resonable degree to allow future modules to leverage this code.
//
//  Things to add -
//
//  Don't Worry, Be Happy, Have a Homebrew!!
//

//  Import some modules we'll need
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include "sha256.h"
#include "time.h"
#include "Configuration.h"



int status = WL_IDLE_STATUS;


// The ISRs increment these counters, define them as volatile so they ISR can get to 'em
volatile int TapA_Count = 0;
volatile int TapB_Count = 0;
volatile unsigned long TapA_last_micros;
volatile unsigned long TapB_last_micros;

int TapA_LastCount = 0;
int TapA_EndPourCounter = 0;
boolean TapAInput = false;

int TapB_LastCount = 0;
int TapB_EndPourCounter = 0;
boolean TapBInput = false;

//  ISR - INTERRUPT SERVICE ROUTINES -
//  The kegerator flow meter generates a pulse train when peer is pouring, each pulse roughly
// corresponding to a metered amount of beer....  so the Interrupt Service Routines count pulses
void TapA_ISR() {
  if ((long)(micros() - TapA_last_micros) >= InputDebounce_ms * 1000) {
    TapA_Count++ ;
    TapA_last_micros = micros();
  }
}

void TapB_ISR() {
  if ((long)(micros() - TapB_last_micros) >= InputDebounce_ms * 1000) {
    TapB_Count++ ;
    TapB_last_micros = micros();
  }
}

// this simply is used to create the JSON message that will be passed to the "tellSNS" module - change this to update your data record sent
bool SendBeerEvent(int tap_num, const char *event, int value) {
  String json_msg, timestamp;
  timestamp = year() ;
  if (month() < 10) timestamp += "0";
  timestamp = timestamp + month() ;
  if (day() < 10) timestamp += "0";
  timestamp = timestamp + day() + "T";
  //   timestamp += "T";
  if (hour() < 10) timestamp += "0";
  timestamp += hour();
  if (minute() < 10) timestamp += "0";
  timestamp += minute();
  if (second() < 10) timestamp += "0";
  timestamp += second();
  timestamp += "Z";
  json_msg = "{\"sender\":\"" ;
  json_msg = json_msg + PUBLICNAME + "\",\"tap\":" + tap_num + ",\"event\":\"" + event;
  json_msg = json_msg +  "\",\"ticks\":" + value + ",\"time\":\"" + timestamp + "\",\"version\":\"1.0\"}";
  Serial.println(json_msg);
  return (tell_V4_SNS(json_msg));
}



//  SETUP
void setup() {
  EEPROM.begin(EEPROM_size);  //needed by the ESP lib - specify the size of EEPROM space accessible
  Serial.begin(115200);   //  set up the UART for 115200 bps
  pinMode(TapA_Pin, INPUT); // set the input pins to be input with a 10K pullup - flow meter goes here
  pinMode(TapB_Pin, INPUT);
  pinMode(GPIO_0_Pin, INPUT_PULLUP);
  pinMode(BluePin, OUTPUT);  // set the LED driver pins to be output
  pinMode(GreenPin, OUTPUT);
  pinMode(RedPin, OUTPUT);


  SetLED(OFF); // write a simple 3 bit value corresponding to RGB to control LED - see config names

  //Save_System_EEPROM();  //this is a hack to update EEPROM during dev with hard coded credentials - do not use for security
  delay(100);
  Serial.println("");

  //----------------------------------------------------------------------------------------
  //  Experimental code PLACE
  /*
  SetLED(BLUE);
  int i;
  for (i = 1; i < 800; i++) {
    if (digitalRead(TapA_Pin) == LOW) Serial.println("TAPA LOW");
    if (digitalRead(TapA_Pin) == HIGH) Serial.println("TAPA HIGH");
    if (digitalRead(TapB_Pin) == LOW) Serial.println("TAPB LOW");
    if (digitalRead(TapB_Pin) == HIGH) Serial.println("TAPB HIGH");
    Serial.println(i);
    delay(1000);
  }
  SetLED(OFF);
  */
  //----------------------------------------------------------------------------------------


  // We start by trying to connect to a WiFi network UNLESS TAP A is pulled when booting (to force AP mode easily)
  //  If the EEPROM has been programmed (we will look for 42 & 24 being programmed in eeprom) then load fm EEPROM,
  //  else we will use the defaults programmed above (first time boot with fresh module)
  // the config is stored in EEPROM,  if it hasn't been written, Load_WiFi does nothing and the defaults prevail
  // (Note - storing credentials in code is BAD, I understand.  This is a dev hack with very limited credential
  // permissions scope, and once working we won't populate the credential info above.
  SetLED(RED);  // Red to start
  // GET Configured
  if (Is_System_EEPROM_programmed()) {       //load config & try to connect
    Load_System_EEPROM();  //load past config from EEPROM
  }
  else {
    Save_System_EEPROM();    // make sure we don't have crud in the EEPROM - first time boot hack to refresh EEPROM
  }
  //  at this point we should have a sane EEPROM config loaded - let's see if we can get online with it


  do {  //This is the main setup loop - loop here until we get NTP
    SetLED(RED);  // Red to start
    do { // inner loop to get connected to local wifi

#if VERBOSE==1
      Serial.print("Attempting to connect to ");
      Serial.print(WIFISSID);
      Serial.print("  -  ");
      Serial.println(WIFIPW);
#endif

      WiFi.begin(WIFISSID, WIFIPW);

      // try to connect, and wait for good status or timeout - prints dots for progress
      int wifi_timer = 0;
      do {
        SetLED(OFF);
        Serial.print(".");  // print dots while trying to connect to local wifi from EEPROM, blink red
        delay(250);
        SetLED(RED);
        delay(250);
        wifi_timer++;
      } while (WiFi.status() != WL_CONNECTED  && wifi_timer < 30 ) ;

      if (WiFi.status() == WL_CONNECTED) SetLED(GREEN);  // Green LED if we are on wifi

      //  OK - we either connected or timed-out - if we timed out then assume then turn into AP/server and get new info
      //  else, rock on!

      if ((WiFi.status() != WL_CONNECTED) ||  (digitalRead(GPIO_0_Pin) == LOW) ) {  //if we are not on wifi or the GPIO_0 button is low
        //call the AP mode credential asker
        if (digitalRead(TapA_Pin) == LOW)  {
          Serial.println("tap_A input is low at startup, so time to become an AP/web server");
        }
        SetLED(TEAL);  //TEAL means "web server"
        WiFi.disconnect();
        AP_Get_Credentials();  // this is the call to become an AP, turn into a web server,
        // wait for a conection to the APmode_address, get info, save to eeprom, return
      }
    } while (WiFi.status() != WL_CONNECTED) ;

    // OK - we are connected now, and the current working params are in EEPROM for next time
#if VERBOSE==1
    Serial.println("");
    Serial.print("WiFi is connected - ");
    Serial.println(WiFi.localIP());
#endif
    SetLED(GREEN);
    WiFi.hostByName(TimeServer, timeServer);  //  resolve the TimeServer name into an IP address
#if VERBOSE==1
    Serial.print("NTP Server: ");
    Serial.println(timeServer);
#endif

    //Try to get the date and time via NTP - year is 1970 (epoch begin) if NTP is not available
    // NOTE - accurate time is necessary to make signed calls on AWS, no time=no calls to AWS
    //  blinking green here to show NTPing
    int wifi_timer = 0;
    do {
      if (!getTimeAndDate()) Serial.print("X");
      SetLED(OFF);
      delay(250);
      SetLED(GREEN);
      delay(250);
      wifi_timer++;
    } while (year() < 2015 && wifi_timer < 10 ) ;
    clockDisplay();

  } while (year() < 2015);

  // OK - we have a good date, we are online, LED is Green - set up ISR and wait for events
  //  Set up GPIO pins to bind to ISRs  - do this AFTER we are online
  attachInterrupt(TapA_Pin, TapA_ISR, CHANGE);
  attachInterrupt(TapB_Pin, TapB_ISR, CHANGE);

  // SETUP complete - fall to main loop
}



//   MAIN LOOP HERE - does nothing but look for tap pulls, make sure we're connected, and daily NTP's
//  we loop about 10X/second - see 100ms delay at end
void loop() {
  int loopcounterforstatuschecking = 0;

  //  FIRST we check for tap pulls (beer flow events), and handle accordingly

  // TAP A handling code

  int countTapAsingle = 0; // this is the counter for how long we've seen a single pulse
  
/*  Serial.print(TapA_Count);
  Serial.print("  ");
  Serial.print(TapA_LastCount);
  Serial.print("  ");
  Serial.print(countTapAsingle);
  Serial.println("");
*/


  if (TapA_Count == 1) { // this is a little hacky - filter out "single pulses" from vibration
    countTapAsingle++;
    if (countTapAsingle == 5) { //single pulse, count was 1 for 500ms, so pretend it never happend
      TapA_Count = 0;
      countTapAsingle = 0;
//      Serial.print("filtered out singleton ");
    }
  }

  if (TapA_Count == 0) {
    countTapAsingle = 0;  // reset
    TapA_LastCount = 0;
  }
  else if (TapA_LastCount < 2 && TapA_Count > 1  ) {
    // the count just became greater than 1 from having been 0, the tap has been pulled
    SetLED(BLUE);  // BLUE is beer dispensing
    SendBeerEvent ( 1, "Pour_started", 0);
    TapA_LastCount = TapA_Count;
  }
  else if (TapA_LastCount != TapA_Count) {
    TapA_LastCount = TapA_Count;      // beer is flowing, Count has incremented, nothing to do but wait
  }
  else {
    //  this means that a beer was poured, and the tap Count = LastCount - no flow since the last time we looped
    TapA_EndPourCounter ++;
    if (TapA_EndPourCounter == NumberLoopsToDeclareEOB) {
      //  we've seen no flow for NumberLoopsToDeclareEOB loops, so this means the tap is off -
      // call the "tap went off routine, send count
      if (TapA_LastCount != 1) {
        SendBeerEvent ( 1, "Pour_Finished", TapA_Count);      // call the "tap went off routine, send count to cloud
      }
      SetLED(GREEN);
      TapA_Count = 0;
      TapA_LastCount = 0;
      TapA_EndPourCounter = 0;
    }
  }

  // TAP B handling code
  int countTapBsingle = 0; // this is the counter for how long we've seen a single pulse
  if (TapB_Count == 1) { // this is a little hacky - filter out "single pulses" from vibration
    countTapBsingle++;
    if (countTapBsingle == 5) { //single pulse, count was 1 for 500ms, so pretend it never happend
      TapB_Count = 0;
      countTapBsingle = 0;
    }
  }

  if (TapB_Count == 0) {
    countTapBsingle = 0;  // reset
    TapB_LastCount = 0;
  }
  else if (TapB_LastCount < 2 && TapB_Count > 1  ) {
    // the count just became greater than 1 from having been 0, the tap has been pulled
    SetLED(BLUE);  // BLUE is beer dispensing
    SendBeerEvent ( 2, "Pour_started", 0);
    TapB_LastCount = TapB_Count;
  }
  else if (TapB_LastCount != TapB_Count) {
    TapB_LastCount = TapB_Count;      // beer is flowing, Count has incremented, nothing to do but wait
  }
  else {
    //  this means that a beer was poured, and the tap Count = LastCount - no flow since the last time we looped
    TapB_EndPourCounter ++;
    if (TapB_EndPourCounter == NumberLoopsToDeclareEOB) {
      //  we've seen no flow for NumberLoopsToDeclareEOB loops, so this means the tap is off -
      // call the "tap went off routine, send count
      if (TapB_LastCount != 1) {
        SendBeerEvent ( 2, "Pour_Finished", TapB_Count);      // call the "tap went off routine, send count to cloud
      }
      SetLED(GREEN);
      TapB_Count = 0;
      TapB_LastCount = 0;
      TapB_EndPourCounter = 0;
    }
  }

  //  OK - we've handled everything beer related at this point - time for some hygiene
  //  are we online?  Update NTP daily by resetting to avoid clock drift issues.

  //  First we'll check for wireless status every CHECKONLINESTATUSMINUTES - iff offline, reset
  if (CHECKONLINESTATUSMINUTES * 10 * 60 <= loopcounterforstatuschecking) {
    if (WiFi.status() != WL_CONNECTED)  {
      Serial.println("resetting because I am disconnected");
      ESP.reset();
    }
  }
  //  Next we'll reset daily at the RESETHOUR to avoid leaks/resource depletion issues
  if ((hour() == RESETHOUR && minute() == 0 && second() == 0)) {
    Serial.println("resetting because it is that time");
    ESP.reset();
  }
  delay(100);  //wait a tenth of a second and return
  loopcounterforstatuschecking++;
}





