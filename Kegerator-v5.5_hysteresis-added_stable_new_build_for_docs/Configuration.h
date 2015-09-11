// GLOBAL DEFINITIONS AND VARIABLES
//
//#define VERBOSE 1  // comment out this line normally, uncomment for verbose serial output
#define TapA_Pin 14 // input from flow meter - 
#define TapB_Pin 13
#define GPIO_0_Pin 0 // GPIO pin - low to boot from Serial, if blinking red push to turn into webserver

//Stupid Color Stuff for the LED
// NOTE - at 3.3v, the red LED is much brighter, so we really only get B/G/R/Teal in practice unless you dim the red
#define BluePin 16 // output pin for LEDs - look at picture of the module to get a pinout
#define GreenPin 12 //  NOTE - GPIO16 cannot be used as an interrupt driving input, so I had to swap w 14
#define RedPin 15

#define BLUE 1
#define GREEN 2
#define RED  4
#define TEAL 3
#define PURPLE 5
#define ORANGE 6
#define WHITE 7
#define OFF 0

#define NumberLoopsToDeclareEOB 6  // EOB = End of Beer - how many main loops without a pulse
#define InputDebounce_ms 50  // debounce time in ms - cruncy button for testing
#define APServerPort 80   // this is the port we'll listen to if we turn into an AP/web server to get config info
#define RESETHOUR 5  // reboot at this hour, UMT - we have some memory leaks, easier to boot-to-the-head
#define CHECKONLINESTATUSMINUTES 5 //  check online status every this many minutes - done to avoid a leak in  wireless lib

// EEPROM specs and map for persisting wireless stuff - mainly strings (need null termination, so 20 char string = 21 bytes)
// it was a pain to keep up with arithmetic, so I used a length constant
#define EEPROM_size 512

#define EEPROM_PROGRAMMED1 0
#define EEPROM_PROGRAMMED2 EEPROM_PROGRAMMED1+1

#define EEPROM_WIFISSID_start EEPROM_PROGRAMMED2+1
#define EEPROM_WIFISSID_len 33
#define EEPROM_WIFISSID_end EEPROM_WIFISSID_start+EEPROM_WIFISSID_len

#define EEPROM_WIFIPW_start EEPROM_WIFISSID_end+1
#define EEPROM_WIFIPW_len 33
#define EEPROM_WIFIPW_end EEPROM_WIFIPW_start+EEPROM_WIFIPW_len

#define EEPROM_PUBLICNAME_start EEPROM_WIFIPW_end+1
#define EEPROM_PUBLICNAME_len 33
#define EEPROM_PUBLICNAME_end EEPROM_PUBLICNAME_start+EEPROM_PUBLICNAME_len

#define EEPROM_SECRETAWSKEY_start EEPROM_PUBLICNAME_end+1
#define EEPROM_SECRETAWSKEY_len 41
#define EEPROM_SECRETAWSKEY_end EEPROM_SECRETAWSKEY_start+EEPROM_SECRETAWSKEY_len

#define EEPROM_AWSACCESSKEYID_start EEPROM_SECRETAWSKEY_end+1
#define EEPROM_AWSACCESSKEYID_len 21
#define EEPROM_AWSACCESSKEYID_end EEPROM_AWSACCESSKEYID_start+EEPROM_AWSACCESSKEYID_len

#define EEPROM_SNSARN_start EEPROM_AWSACCESSKEYID_end+1
#define EEPROM_SNSARN_len 81   // not sure how long an ARN we should support - do 80 chars for now
#define EEPROM_SNSARN_end EEPROM_SNSARN_start+EEPROM_SNSARN_len

#define EEPROM_REGION_start EEPROM_SNSARN_end+1
#define EEPROM_REGION_len 12   // not sure how long an ARN we should support - do 80 chars for now
#define EEPROM_REGION_end EEPROM_REGION_start+EEPROM_REGION_len

#define EEPROM_AWSENDPOINT_start EEPROM_REGION_end+1
#define EEPROM_AWSENDPOINT_len 60   // not sure how long an ARN we should support - do 60 chars for now
#define EEPROM_AWSENDPOINT_end EEPROM_AWSENDPOINT_start+EEPROM_AWSENDPOINT_len




//Data structures to hold the stuff persisted in EEPROM -
//  NOTE - these variables will get loaded from EEPROM, or you will be prompted to enter it
char WIFISSID[EEPROM_WIFISSID_len] = "knights who say ni";
char WIFIPW[EEPROM_WIFIPW_len] = "not really my wifi PW";

char PUBLICNAME[EEPROM_PUBLICNAME_len] = "BrewmonkeyIAD";

//  YES - putting credentials in code is BAD - this user has minimal permissions, will be defaulted to bogus safe info
char SECRETAWSKEY[EEPROM_SECRETAWSKEY_len] = "1adBOGUSHf9Y1EWSz+Fx/geh2uvTJH48wTSYvx";
char AWSACCESSKEYID[EEPROM_AWSACCESSKEYID_len] = "AKIBOGUSIAYGWH4ULKA";
// Location of the SNS topic.
char SNSARN[EEPROM_SNSARN_len] = "arn:aws:sns:us-east-1:193129100670:arduino-topic";
char REGION[EEPROM_REGION_len] = "us-east-1";
char AWSENDPOINT[EEPROM_AWSENDPOINT_len] = "sns.us-east-1.amazonaws.com";
#define AWSendpointPort 80

//******** NTP Server Settings ********
// us.pool.ntp.org NTP server
//   (Set to your time server of choice)
//const char* TimeServer = "time-c.nist.gov";
const char* TimeServer = "us.pool.ntp.org";
IPAddress timeServer ;
unsigned int localNTPPort = 64001;

//******** ACCESS POINT & WEB SERVER Settings ********
// This is what we broadcast when we can't connect
const char* AP_ssid = "Confused Kegerator needs Love";
const int httpPort = 80;  //port we use when we are a server getting credentials
IPAddress APmode_address = { 192, 168, 4, 1 };  // web server address we broadcast
