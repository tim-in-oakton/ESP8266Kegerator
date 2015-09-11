/*  This module is called when either the widget can't get wifi connectivity, or the TAP-A input is there at boot
  (we will have a switch in parallel with the tap to avoid beer wastage to force this mode on boot).

  What the module does is turn into an Access Point (AP Mode), start a simple web server, and wait
  for a client.  The webpage served is a form for the widget's metadata - wifi SSID/PW, credentials,
  public name, etc.  This really should be PW protected, I'll implement that once it is stable and
  debugged in the wild.

  Once the form is submitted, the function writes the input to EEPROM, turns back to a client and exits-

  I should move the specific variables used here from the main module for clarity-
  */

boolean AP_Get_Credentials() {

  WiFiServer server(APServerPort);   // this is the port that the server will listen on when in AP/server mode (80)
  char c ; // used to copy the passed URL
  boolean submitted = false;  // we will loop until "submit" and we parse out the commands
  String Client_MSG = "";
  String tmpSTR;
  char decodeworking[81] = "";

  // Here we start - we convert to AP mode, turn on a server, wait for input, and unless the input
  // has the tokens we want we repeat until the universe dies a cold lonely death
  //
  // (and if we get tokens, we write them EEPROM and return)
  //
  Serial.println("turning into AP now");

  WiFi.mode(WIFI_AP);      //set AP mode, some client will connect to us
  WiFi.softAP(AP_ssid);    //broadcast the "I'm confused" SSID
  WiFi.config(APmode_address, APmode_address, {255, 255, 255, 0} );
  // at this point we should be broadcasting as an AP - let's configure a web server now:
  server.begin();
  delay(25);
  Serial.println("Listening for client");
  WiFiClient client = server.available();

  // listen for incoming clients
  while (!submitted ) {  //listen for clients
    client = server.available();
    if (client) {
      Serial.println("new client");
      // an http request ends with a blank line
      boolean currentLineIsBlank = true;

      while (client.connected()) {
        // someone has connected, let's wait for some input to process
        while (!client.available()) {
          delay(50);
        }

        // Read the first line of the request, drop it into Client_MSG String
        //          Client_MSG = client.readStringUntil('\r');
        Client_MSG = client.readString();
        Serial.println(Client_MSG);

        // Now Client_MSG should have the entire user provided String - let's parse for valid tokens
        // Start by looking for "bogus" = the method used when our page is submitted
        if (Client_MSG.indexOf("bogus.php") != -1) {
          // OK, looks like a valid response, so let's populate the data
          Serial.println("Looks like a valid response, parse it");
          int SSID_pos = (Client_MSG.indexOf("SSID=") ); //position of the SSId token start
          int WIFIPW_pos = (Client_MSG.indexOf("WIFIPW=") ); //position of the WIFIPW token start
          int PUBLICNAME_pos = (Client_MSG.indexOf("PUBLICNAME=") ); //position of the PUBLICNAME token start
          int SECRETAWSKEY_pos = (Client_MSG.indexOf("SECRETAWSKEY=") );
          int AWSACCESSKEYID_pos = (Client_MSG.indexOf("AWSACCESSKEYID=") );
          int SNSARN_pos = (Client_MSG.indexOf("SNSARN=") );
          int REGION_pos = (Client_MSG.indexOf("REGION=") );
          int AWSENDPOINT_pos = (Client_MSG.indexOf("AWSENDPOINT=") );
          int ENDPARM_pos = (Client_MSG.indexOf("HTTP/1") );

          if ( (SSID_pos != -1) && (WIFIPW_pos != -1) && (PUBLICNAME_pos != -1) && ( SECRETAWSKEY_pos != -1) && (AWSACCESSKEYID_pos != -1) && (SNSARN_pos != -1) && (REGION_pos != -1) && (AWSENDPOINT_pos != -1) && (ENDPARM_pos != -1) ) {
            // valid reszponse, or at least we found all values posted
            Serial.println(Client_MSG);
            Serial.println("URL Decoded input");
            tmpSTR = Client_MSG.substring(SSID_pos + 5, WIFIPW_pos - 1);
            tmpSTR.toCharArray(decodeworking, 80);
            urldecode2(decodeworking, WIFISSID);
            Serial.println(WIFISSID);

            tmpSTR = Client_MSG.substring(WIFIPW_pos + 7, PUBLICNAME_pos - 1);
            tmpSTR.toCharArray(decodeworking, 80);
            urldecode2(decodeworking, WIFIPW);
            Serial.println(WIFIPW);

            tmpSTR = Client_MSG.substring(PUBLICNAME_pos + 11, SECRETAWSKEY_pos - 1);
            tmpSTR.toCharArray(decodeworking, 80);
            urldecode2(decodeworking, PUBLICNAME);
            Serial.println(PUBLICNAME);

// the secretaccesskey is special, we don't display what we have, so don't write it if blank
            tmpSTR = Client_MSG.substring(SECRETAWSKEY_pos + 13, AWSACCESSKEYID_pos - 1);
            tmpSTR.toCharArray(decodeworking, 80);
            if (sizeof(decodeworking) > 0) {
              urldecode2(decodeworking, SECRETAWSKEY);
              Serial.println(SECRETAWSKEY);
            }

            tmpSTR = Client_MSG.substring(AWSACCESSKEYID_pos + 15, SNSARN_pos - 1);
            tmpSTR.toCharArray(decodeworking, 80);
            urldecode2(decodeworking, AWSACCESSKEYID);
            Serial.println(AWSACCESSKEYID);

            tmpSTR = Client_MSG.substring(SNSARN_pos + 7, REGION_pos - 1);
            tmpSTR.toCharArray(decodeworking, 80);
            urldecode2(decodeworking, SNSARN);
            Serial.println(SNSARN);

            tmpSTR = Client_MSG.substring(REGION_pos + 7, AWSENDPOINT_pos - 1);
            tmpSTR.toCharArray(decodeworking, 80);
            urldecode2(decodeworking, REGION);
            Serial.println(REGION);

            tmpSTR = Client_MSG.substring(AWSENDPOINT_pos + 12, ENDPARM_pos - 1);
            tmpSTR.toCharArray(decodeworking, 80);
            urldecode2(decodeworking, AWSENDPOINT);
            Serial.println(AWSENDPOINT);
            submitted = true ;

            Serial.println("Writing credentials to EEPROM now");
            Save_System_EEPROM();   //write the info to eeprom
            //            break;
          }
          else {
            Serial.print("Didn't see the tokens, resending");
          }
        }

        client.flush();

        // we should have the tokens here or have read the entire line,

        if (!submitted)  {
          // send a standard http response header
          Serial.println("Sending the page to the client");
          client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: 60\r\n<!DOCTYPE HTML>\r\n");
          // The interface is pretty kludgy to the wifi - each println is a distinct packet, send every 2s....

          Serial.println("Starting HTML section");
          client.println("\r\n<html>\r\n<body bgcolor=\"#FFC670\">\r\n<link rel=\"shortcut icon\" href=\"#\" />\r\n<br>\r\n ");
          client.println("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
          client.println("<br>\r\nI am a poor confused Kegerator, whose WiFi won't connect - can you please update me?");
          client.println("<br>\r\nUpdate the following info, and then hit SUBMIT.  I'll drop you, then try to connect. ");
          Serial.println("Starting FORM section");

          client.print("<br>\r\n<form action=\"bogus.php\"method=\"GET\">\r\nSSID:<br>\r\n<input type=\"text\" name=\"SSID\" maxlength=\"EEPROM_WIFISSID_len-1\" value=\"");
          client.println(WIFISSID);

          client.print("\">\r\n<br>WiFi PW:<br><input type=\"text\" name=\"WIFIPW\" maxlength=\"EEPROM_WIFIPW_len-1\"value=\"");
          client.println(WIFIPW);

          client.print("\">\r\n<br>Public Name:<br>\r\n<input type=\"text\" name=\"PUBLICNAME\" maxlength=\"EEPROM_PUBLICNAME_len-1\"value=\"");
          client.println(PUBLICNAME);

          client.print("\">\r\n<br>AWS Secret Key:<br>\r\n<input type=\"text\" name=\"SECRETAWSKEY\" maxlength=\"EEPROM_SECRETAWSKEY_len-1\"");

          client.print("\">\r\n<br>AWS Public Key:<br>\r\n<input type=\"text\" name=\"AWSACCESSKEYID\" maxlength=\"EEPROM_AWSACCESSKEYID_len-1\"value=\"");
          client.println(AWSACCESSKEYID);

          client.print("\">\r\n<br>SNS ARN:<br>\r\n<input type=\"text\" name=\"SNSARN\" maxlength=\"EEPROM_SNSARN_len-1\"value=\"");
          client.println(SNSARN);

          client.print("\">\r\n<br>REGION:<br>\r\n<input type=\"text\" name=\"REGION\" maxlength=\"EEPROM_REGION_len-1\"value=\"");
          client.println(REGION);

          client.print("\">\r\n<br>AWS ENDPOINT:<br>\r\n<input type=\"text\" name=\"AWSENDPOINT\" maxlength=\"EEPROM_AWSENDPOINT_len-1\"value=\"");
          client.println(AWSENDPOINT);

          client.println("\">\r\n<br><input type=\"submit\" value=\"Submit\">\r\n</form>\r\n</html>\r\n");
          Serial.println("Sent the page - DONE");
          delay(100);
          client.stop();

        } else {

          // send a standard http response header
          client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: 60\r\n");

          // the connection will be closed after completion of the response
          // refresh the page automatically every 5 sec
          client.println("<!DOCTYPE HTML>\r\n<br>\r\n<html>\r\nThanks- Restarting as a client.....\r\n<br>\r\n</html>");
          //          client.println("<html>\r\nThanks- Restarting as a client.....\r\n<br>\r\n</html>");
          delay(100);
          client.stop();
        }
      }
      delay(10);
    }
    else {
      // pause a bit, revisit to see if there is a client

      Serial.print("+");
      delay(500);
    }
  }
  Serial.print("Got the parameters, back to station/client operation now");
  WiFi.mode(WIFI_STA);      //set station/client mode and return
  return (true);
}


