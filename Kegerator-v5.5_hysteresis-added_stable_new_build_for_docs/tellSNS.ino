//  tellSNS - this module is called by the main event handling loop, and it
//  makes a signed call on the AWS SNS service using credentials from EEPROM

// Two "tell_SNS" functions - the second  uses our V2 signing process which is older, simpler but not recommended
// you should really use the V4 signature module, called tell_V4_SNS, instead


boolean tell_V4_SNS (const String message)  {
  //  I pulled all the strings forward as there  is not a lot of heap in these devices, will optimize string usage
  char encodetmp[192] = "";
  String sig, Datestamp, amzdate, timestamp, CanonicalHeaders, SignedHeaders;
  String CanonicalQuery, stringtosign, credential_scope, CanonicalRequest, requeststring, line;
  uint8_t *payloadhash, SECOND;
  int i;
  boolean SNS_Success = 0;

  // get the connection established
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(AWSENDPOINT, AWSendpointPort)) {
    Serial.println("AWS connection failed");
    return (SNS_Success);
  }

  // OK - first thing's first - let's URLencode the string to send, or it will bite us !!!


  //************* TASK 1: CREATE A CANONICAL REQUEST *************
  Datestamp = year() ;
  if (month() < 10) Datestamp += "0";
  Datestamp = Datestamp + month() ;
  if (day() < 10) Datestamp += "0";
  Datestamp = Datestamp + day();

  timestamp = year();
  timestamp += "-";
  if (month() < 10) timestamp += "0";
  timestamp = timestamp + month() + "-";
  if (day() < 10) timestamp += "0";
  timestamp = timestamp + day() + "T";
  if (hour() < 10) timestamp += "0";
  timestamp = timestamp +  hour() + ":";
  if (minute() < 10) timestamp += "0";
  timestamp = timestamp + minute() + ":";
  SECOND = second(); // this was an interesting corner case - failures when seconds did not line up
  // between calculating timestamp & amzdate...  sneaky intermittent behavior
  // so we snap seconds ONCE and use that...  all the printlns didn't help ;-)
  if (SECOND < 10) timestamp += "0";
  timestamp += SECOND;

  delay(10);
  amzdate = Datestamp + "T";
  if (hour() < 10) amzdate += "0";
  amzdate += hour();
  if (minute() < 10) amzdate += "0";
  amzdate += minute();
  if (SECOND < 10) amzdate += "0";
  amzdate += SECOND;
  amzdate += "Z";



  // V4 - Make the string to be signed (NOTE - not the same as the URL....)
  // NOTE also - the process is that AWS will reconstruct this string from what is passed
  // and then hash it to compare against the HMAC hash you provided.  THIS IS VERY INTOLERANT!!
  //  By design - so the order is important, format is unforgiving...  add a single extra ";"
  // and it will fail - so be careful messing with this - again, this is a security signature
  // scheme, so don't color outside the lines here!!

  CanonicalHeaders = "host:";
  CanonicalHeaders = CanonicalHeaders + AWSENDPOINT + "\n" ;
  SignedHeaders = "host";

  credential_scope = Datestamp;
  credential_scope = credential_scope + "/" + REGION + "/sns/aws4_request";

  CanonicalQuery = "AWSAccessKeyId=";
  CanonicalQuery = CanonicalQuery + AWSACCESSKEYID + "&Action=Publish&Message=";
  urlencode(message.c_str(), encodetmp);
  CanonicalQuery += encodetmp;
  CanonicalQuery += "&SignatureMethod=HmacSHA256&SignatureVersion=4&Timestamp=";
  urlencode(timestamp.c_str(), encodetmp);
  CanonicalQuery += encodetmp;
  CanonicalQuery += "&TopicArn=";
  urlencode(SNSARN, encodetmp);
  CanonicalQuery += encodetmp;
  CanonicalQuery += "&X-Amz-Algorithm=AWS4-HMAC-SHA256";
  CanonicalQuery += "&X-Amz-Credential=";
  CanonicalQuery += AWSACCESSKEYID;
  CanonicalQuery += "%2F" + Datestamp + "%2F" + REGION + "%2Fsns%2Faws4_request";
  CanonicalQuery += "&X-Amz-Date=" + amzdate;
  CanonicalQuery += "&X-Amz-Expires=30";
  CanonicalQuery += "&X-Amz-SignedHeaders=" + SignedHeaders;

  Sha256.init();
  Sha256.print("");
  payloadhash = Sha256.result();
  // local hash has the hash of the empty string passed.  NOTE - the payload hash in this step is just 32 hex-represented bytes - like ab12FF00  etc.
  // payload hash here is a pointer to 32 bytes, we'll stringify it into Hex in task 2

  CanonicalRequest = "GET\n";
  CanonicalRequest = CanonicalRequest + "/" + "\n" + CanonicalQuery + "\n" + CanonicalHeaders + "\n" + SignedHeaders + "\n" ;
  // now we print out the SHA256 hash from above-
  for (i = 0; i < 32; i++) {
    if (payloadhash[i] < 16) CanonicalRequest += "0";    //single hex digit, lib doesn't print leading zeroes - turn "F" into "0F"
    CanonicalRequest += String(payloadhash[i], HEX);
  }



  // ************* TASK 2: CREATE THE STRING TO SIGN*************
  stringtosign =  "AWS4-HMAC-SHA256";
  stringtosign = stringtosign + "\n" + amzdate + "\n" + credential_scope + "\n"  ;
  Sha256.init();
  Sha256.print(CanonicalRequest);
  payloadhash = Sha256.result();
  for (i = 0; i < 32; i++) {
    if (payloadhash[i] < 16) stringtosign += "0";    //single hex digit, lib doesn't print leading zeroes - turn "F" into "0F"
    stringtosign += String(payloadhash[i], HEX);
    //  !!!  AHA - failed sigs have 65 char issue here......
  }

  // ************* TASK 3: CALCULATE THE SIGNATURE *************
  // this is going to return the HMAC key based on the string built above & our secret key
  delay(1);
  sig = make_v4_signature_key (SECRETAWSKEY, Datestamp.c_str(), REGION, "sns", stringtosign);

  CanonicalQuery += "&X-Amz-Signature=";
  CanonicalQuery += sig;

  //************* TASK 4: ADD SIGNING INFORMATION TO THE REQUEST *************
  requeststring =  "GET /?";
  requeststring +=  CanonicalQuery;
  requeststring += " HTTP/1.1\r\nHost: ";
  requeststring += AWSENDPOINT;
  requeststring += "\r\nConnection: keep-alive\r\n\r\n";

#if VERBOSE==1
  Serial.print("Datestamp = ");
  Serial.println(Datestamp);
  Serial.print("timestamp = ");
  Serial.println(timestamp);
  Serial.print("amzdate = ");
  Serial.println(amzdate);
  Serial.print("msg for SNS = ");
  Serial.println(message);
  Serial.print("CanonicalHeaders= ");
  Serial.println(CanonicalHeaders);
  Serial.println("CanonicalQuery= ");
  Serial.println(CanonicalQuery);
  Serial.println("CanonicalRequest=");
  Serial.println(CanonicalRequest);
  Serial.println();
  Serial.println("stringtosign= ");
  Serial.println(stringtosign);
  Serial.print("sig = ");
  Serial.println(sig);
  Serial.println("Request String =:");
  Serial.println(requeststring);
#endif
  delay(10);
  //****************SEND THE REQUEST
  Serial.println("sending");
  client.println(requeststring);

  delay(250);  //  we need to wait for a reply, this is a bit of a hack to swag 1/4 second

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    line = client.readStringUntil('\r');
    if (line == "HTTP/1.1 200 OK") SNS_Success = true;
#if VERBOSE==1
    Serial.println(line);
#endif
  }

#if VERBOSE==1
  Serial.println("closing connection");
  Serial.println();
#endif
  client.stop();
  if (SNS_Success) Serial.println("successfull SNS publish");
  else Serial.println("BORKED publish");
  return (SNS_Success);
}



void tell_V2_SNS (String message)  {
  char encodetmp[EEPROM_SNSARN_len] = "";
  String sig;

  Serial.print("connecting to ");
  Serial.println(AWSENDPOINT);
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(AWSENDPOINT, AWSendpointPort)) {
    Serial.println("AWS connection failed");
    return;
  }

  String timestamp = "";
  timestamp = timestamp + year() + "-";
  if (month() < 10) timestamp += "0";
  timestamp = timestamp + month() + "-";
  if (day() < 10) timestamp += "0";
  timestamp = timestamp + day() + "T";
  if (hour() < 10) timestamp += "0";
  timestamp = timestamp +  hour() + ":";
  if (minute() < 10) timestamp += "0";
  timestamp = timestamp + minute() + ":";
  if (second() < 10) timestamp += "0";
  timestamp += second();

  Serial.println(message); // this is what will be posted to the SNS topic at SNSARN

  // Make the string to be signed (NOTE - not the same as the URL....)
  // NOTE also - the process is that AWS will reconstruct this string from what is passed
  // and then hash it to compare against the HMAC hash you provided.  THIS IS VERY INTOLERANT!!
  //  By design - so the order is important, format is unforgiving...  add a single extra ";"
  // and it will fail - so be careful messing with this - again, this is a security signature
  // scheme, so don't color outside the lines here!!
  String stringtosign;
  stringtosign += "GET\n";
  stringtosign += AWSENDPOINT;
  stringtosign += "\n/\n";

  String CanonicalQuery = "AWSAccessKeyId=";
  CanonicalQuery = CanonicalQuery + AWSACCESSKEYID + "&Action=Publish&Message=";
  urlencode(message.c_str(), encodetmp);
  CanonicalQuery += encodetmp;
  CanonicalQuery += "&SignatureMethod=HmacSHA256&SignatureVersion=2&Timestamp=";
  urlencode(timestamp.c_str(), encodetmp);
  CanonicalQuery += encodetmp;
  CanonicalQuery += "&TopicArn=";
  urlencode(SNSARN, encodetmp);
  CanonicalQuery += encodetmp;

  stringtosign += CanonicalQuery;

  // Serial.println("stringtosign = ");
  // Serial.println(stringtosign);

  // this is going to return the HMAC key based on the string built above & our secret key
  sig = make_v2_signing_key (SECRETAWSKEY, stringtosign.c_str());

  //  Serial.print("made signing key - ");
  //  Serial.println(sig);

  CanonicalQuery += "&Signature=" + sig;

  String requeststring =   "GET /?";
  requeststring +=  CanonicalQuery;
  requeststring += " HTTP/1.1\r\nHost: ";
  requeststring += AWSENDPOINT;
  requeststring += "\r\nConnection: keep-alive\r\n\r\n";
  Serial.println(requeststring);
  client.println(requeststring);
  delay(200);
  Serial.println("AWS response = ");

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.println(line);
  }
  Serial.println("closing connection");
  client.stop();
}










