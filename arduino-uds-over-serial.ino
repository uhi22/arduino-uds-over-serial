/* arduino-uds-over-serial */
/* receives UDS commands via serial line and sends response back */

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        /* temporary array for use when parsing */

/* variables to hold the parsed data */
uint8_t udsRequestBufferLen;
#define UDS_REQUEST_BUFFER_SIZE 10
uint8_t udsRequestBuffer[UDS_REQUEST_BUFFER_SIZE];
String strUdsResponse;

/* The registers which can be written and read */
#define NUMBER_OF_REGISTERS 10
uint8_t registerArray[NUMBER_OF_REGISTERS];

boolean newData = false;


/*======================*/
/* Helper functions */

String twoCharHex(uint8_t x) {
  String s;
  s = String(x, HEX);
  if (s.length()<2) s="0"+s;
  return s;
}

/* Serial reception functions */
void recvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = 0x0d; /* carriage return */
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        //Serial.print(rc, HEX);
        if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
        } else {
                receivedChars[ndx] = '\0'; /* terminate the string */
                ndx = 0;
                newData = true;
        }
    }
}

/*======================*/
/* parsing of the data which was received on serial line */
void parseData(void) { 
    char * strtokIndx; /* this is used by strtok() as an index */
    uint8_t i;
    udsRequestBufferLen = 0;
    strtokIndx = strtok(tempChars," ");
    if (strtokIndx==NULL) return;
    udsRequestBufferLen++;
    udsRequestBuffer[0] = strtol(strtokIndx, NULL, 16); /* convert string  to an integer */
    for (i=1; i<UDS_REQUEST_BUFFER_SIZE; i++) {
      strtokIndx = strtok(NULL, " "); /* this continues where the previous call left off */
      if (strtokIndx==NULL) break;
      udsRequestBufferLen++;
      udsRequestBuffer[i]  = strtol(strtokIndx, NULL, 16);
    }
}

/*======================*/
/* The UDS functions */
void handleReadDataByIdentifier(void) {
  //Serial.println("You wanted to read something.");
  uint16_t did;
  if (udsRequestBufferLen!=3) { /* one byte service ID and two byte DID */
    strUdsResponse = "7F 22 13"; /* incorrect message length */
  } else {
    did=(udsRequestBuffer[1]<<8) + udsRequestBuffer[2];
    if (did==0xF1AB) {
      strUdsResponse="62 F1 AB AF FE";
    } else if (did<NUMBER_OF_REGISTERS) {
      strUdsResponse = "62 " + twoCharHex(did>>8) + " " + twoCharHex(did) + " " + twoCharHex(registerArray[did]);
    } else {
      strUdsResponse="7F 22 31"; /* Request out of range */
    }
  }
  Serial.println(strUdsResponse);
}

void handleWriteDataByIdentifier(void) {
  //Serial.println("You wanted to write something.");
  uint16_t did;
  if (udsRequestBufferLen!=4) { /* one byte service ID and two byte DID and one byte data */
    strUdsResponse = "7F 2E 13"; /* incorrect message length */
  } else {
    did=(udsRequestBuffer[1]<<8) + udsRequestBuffer[2];
    if (did<NUMBER_OF_REGISTERS) {
      registerArray[did]=udsRequestBuffer[3];
      strUdsResponse = "6E " + twoCharHex(did>>8) + " " + twoCharHex(did);
    } else {
      strUdsResponse = "7F 2E 31"; /* Request out of range */
    }
  }
  Serial.println(strUdsResponse);
}

/*======================*/

void processReceivedData() {
  uint8_t i;
  //Serial.print("You requested ");
  //for (i=0; i<requestBufferLen; i++) {
  //  Serial.print(twoCharHex(requestBuffer[i]));
  //  Serial.print(" ");
  //}
  if (udsRequestBuffer[0]==0x22) { handleReadDataByIdentifier(); }
  else if (udsRequestBuffer[0]==0x2E) { handleWriteDataByIdentifier(); }
  else Serial.println("7F "+ twoCharHex(udsRequestBuffer[0]) + " 11"); /* service not supported */
}

/*=========================================================================*/
/* The Arduino standard functions */
void setup() {
    Serial.begin(115200);
    Serial.println("This demo expects space separated hex values.");
    Serial.println("Enter data in this style 22 F1 AB");
    Serial.println();
}

void loop() {
    recvWithEndMarker();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        parseData();
        processReceivedData();
        newData = false;
    }
}
