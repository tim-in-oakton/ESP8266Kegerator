
String  make_v4_signature_key (const char *key, const char *dateStamp, const char *regionName, const char *serviceName , String stringtosign) {
  // OK, this is going to look hack, but given that this is really a cryptographic operation there is ZERO wiggle room
  // or latitude for creative interpretation.  If you do not pass EXACTLY what is expected, how it is expected it will fail.
  //  Design intent - so please color inside the lines to avoid frustration.
  //  the date expected here is YYYYMMDD - 1999112 won't cut it - 19990112 will.
  //  the region here is lower case, like "us-east-1"
  //  Service name is lowercase, like "iam"
  //  be creative at your own risk of migraines-  TM

  int i;
  String sig ="";
  String AWSpluskey = "AWS4";
  AWSpluskey += key;  //key used in V4 sig is "AWS4+sig"
  uint8_t *hash;

  Sha256.initHmac((uint8_t *) AWSpluskey.c_str(), AWSpluskey.length()); // key, and length of key in bytes
  Sha256.print(dateStamp);
  hash = Sha256.resultHmac();

  Sha256.initHmac((uint8_t *) hash, 32); // now hash region
  Sha256.print(regionName);
  hash = Sha256.resultHmac();

  Sha256.initHmac((uint8_t *) hash, 32); // now hash service
  Sha256.print(serviceName);
  hash = Sha256.resultHmac();

  Sha256.initHmac((uint8_t *) hash, 32); // now hash aws4_request
  Sha256.print("aws4_request");
  hash = Sha256.resultHmac();

  Sha256.initHmac((uint8_t *) hash, 32); // now has the string itself
  Sha256.print(stringtosign);
  hash = Sha256.resultHmac();
  for (i = 0; i < 32; i++) {
    if (hash[i] < 16) {
      //single hex digit, stupid lib doesn't print leading zeroes, so this makes 0-F into 00-0F
      sig += "0";
    }
    sig += String(hash[i], HEX);
  }
  return sig;
}



//start with what are Bbase64 charset members - will use this to make a string from our array of sextets
static const char PROGMEM b64chars[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String Hash_base64( uint8_t *in, int hashlength) {
  int i, out;
  char b64[80];  // working byte array for sextets....
  String base64;
  for (i = 0, out = 0 ;; in += 3) { // octets to sextets
    i++;
    b64[out++] =   b64chars[in[0] >> 2];

    if (i >= hashlength ) { // single byte, so pad two times
      b64[out++] = b64chars[((in[0] & 0x03) << 4) ];
      b64[out++] =  '=';
      b64[out++] =  '=';
      break;
    }

    b64[out++] = b64chars[((in[0] & 0x03) << 4) | (in[1] >> 4)];
    i++;
    if (i >= hashlength ) { // two bytes, so we need to pad one time;
      b64[out++] =  b64chars[((in[1] & 0x0f) << 2)] ;
      b64[out++] =  '=';
      break;
    }
    b64[out++] = b64chars[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
    b64[out++] =   b64chars[in[2] & 0x3f];

    i++;
    if (i >= hashlength ) { // three bytes, so we need no pad - wrap it;
      break;
    }
  } // this should make b64 an array of sextets that is "out" in length
  b64[out] = 0;
  base64 = b64;
  return base64;
}


String make_v2_signing_key (const char *key, const char * whattohash) {

  int i;
  String sig;
  char tmpUID[81];

  uint8_t *hash;
  //  Sha256.initHmac((uint8_t *) key, sizeof(key) - 1); // key, and length of key in bytes
  Sha256.initHmac((uint8_t *) key, 40); // key, and length of key in bytes

  Sha256.print(whattohash);
  hash = Sha256.resultHmac();

  //  V2 wants the has to be Base64 encoded (not HEX!!), then URI encoded....
  sig = Hash_base64(hash, 32);
  urlencode(sig.c_str(), tmpUID);
  sig = tmpUID;
  return sig;
}







