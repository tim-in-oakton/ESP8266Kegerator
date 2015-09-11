/*

This module uses code from Brandon Beardon's Reef Controller project

 */



// Clock display of the time and date (Basic)
void clockDisplay() {
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}

// Utility function for clock display: prints preceding colon and leading 0
void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}



// Set this to the offset (in seconds) to your local time
const long timeZoneOffset = 0L;

// ALTER THESE VARIABLES AT YOUR OWN RISK
// local port to listen for UDP packets


// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE = 48;
// Buffer to hold incoming and outgoing packets
byte packetBuffer[NTP_PACKET_SIZE];

// A UDP instance to let us send and receive packets over UDP


// Keeps track of how long ago we updated the NTP server
unsigned long ntpLastUpdate = 0;
WiFiUDP Udp_s;


int getTimeAndDate() {
  //  Note - this only sets the time IF it gets online

  int flag = 0;

  Udp_s.begin(localNTPPort);
  sendNTPpacket(timeServer);

  int waitloop = 0;
  do {
    waitloop++;
    delay(200);
    if (Udp_s.parsePacket() ) {
      Udp_s.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      unsigned long highWord, lowWord, epoch;
      highWord = word(packetBuffer[40], packetBuffer[41]);
      lowWord = word(packetBuffer[42], packetBuffer[43]);
      epoch = highWord << 16 | lowWord;
      epoch = epoch - 2208988800 + timeZoneOffset;
      flag = 1;
      setTime(epoch);
      ntpLastUpdate = now();
    }
  } while (flag = 0 && waitloop < 21);
  Udp_s.stop();
  return flag;
}

// Do not alter this function, it is used by the system
unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11101011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp_s.beginPacket(address, 123);
  Udp_s.write(packetBuffer, NTP_PACKET_SIZE);
  Udp_s.endPacket();
}





