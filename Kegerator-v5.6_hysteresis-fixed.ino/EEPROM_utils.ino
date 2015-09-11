#include <EEPROM.h>

bool Is_System_EEPROM_programmed () {
  return ((EEPROM.read(EEPROM_PROGRAMMED1) == 42) && (EEPROM.read(EEPROM_PROGRAMMED2) == 24));
}

void Load_System_EEPROM() {
  if ((EEPROM.read(EEPROM_PROGRAMMED1) == 42) && (EEPROM.read(EEPROM_PROGRAMMED2) == 24)) {
    EEPROM_load_string ( (byte*) WIFISSID, EEPROM_WIFISSID_start, EEPROM_WIFISSID_end);
    EEPROM_load_string ( (byte*) WIFIPW, EEPROM_WIFIPW_start, EEPROM_WIFIPW_end);
    EEPROM_load_string ( (byte*) PUBLICNAME, EEPROM_PUBLICNAME_start, EEPROM_PUBLICNAME_end);
    EEPROM_load_string ( (byte*) SECRETAWSKEY, EEPROM_SECRETAWSKEY_start, EEPROM_SECRETAWSKEY_end);
    EEPROM_load_string ( (byte*) AWSACCESSKEYID, EEPROM_AWSACCESSKEYID_start, EEPROM_AWSACCESSKEYID_end);
    EEPROM_load_string ( (byte*) REGION, EEPROM_REGION_start, EEPROM_REGION_end);
    EEPROM_load_string ( (byte*) SNSARN, EEPROM_SNSARN_start, EEPROM_SNSARN_end);
    EEPROM_load_string ( (byte*) AWSENDPOINT, EEPROM_AWSENDPOINT_start, EEPROM_AWSENDPOINT_end);
  }
}

void Save_System_EEPROM() {
  // Note - Secretkey not written if field is blank
  EEPROM_save_string ( (byte*) WIFISSID, EEPROM_WIFISSID_start, EEPROM_WIFISSID_end);
  EEPROM_save_string ( (byte*) WIFIPW, EEPROM_WIFIPW_start, EEPROM_WIFIPW_end);
  EEPROM_save_string ( (byte*) PUBLICNAME, EEPROM_PUBLICNAME_start, EEPROM_PUBLICNAME_end);
  if (SECRETAWSKEY != "") {
    EEPROM_save_string ( (byte*) SECRETAWSKEY, EEPROM_SECRETAWSKEY_start, EEPROM_SECRETAWSKEY_end);
  }
  EEPROM_save_string ( (byte*) AWSACCESSKEYID, EEPROM_AWSACCESSKEYID_start, EEPROM_AWSACCESSKEYID_end);
  EEPROM_save_string ( (byte*) SNSARN, EEPROM_SNSARN_start, EEPROM_SNSARN_end);
  EEPROM_save_string ( (byte*) REGION, EEPROM_REGION_start, EEPROM_REGION_end);
  EEPROM_save_string ( (byte*) AWSENDPOINT, EEPROM_AWSENDPOINT_start, EEPROM_AWSENDPOINT_end);
  EEPROM.write(EEPROM_PROGRAMMED1, 42);  // easy but hack way to see if EEPROM was initialized, must be a smarter approach
  EEPROM.write(EEPROM_PROGRAMMED2, 24);  //42 24 - thanks Douglas Adams!
  EEPROM.commit();
}

void EEPROM_load_string ( byte string[] , int str_start, int str_end ) {
  int i;
  for ( i = str_start; i < str_end ; i++) {
    string[i - str_start] = EEPROM.read(i);
  }
}


void EEPROM_save_string ( byte string[] , int str_start, int str_end ) {
  int i;
  for ( i = str_start; i < str_end ; i++) {
    EEPROM.write(i, string[i - str_start]);
  }
}






