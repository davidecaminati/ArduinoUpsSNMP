/*
Macasoft SNMP Agent Library For Old (but gold) UPS
 
Released under MIT License

Copyright (c) 2019 Davide Caminati

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Streaming.h>
#include <Ethernet.h>
#include <SPI.h>
#include <MemoryFree.h>
#include <Agentuino.h>
#include <Flash.h>

// network
static byte mac[] = { 0xDE, 0xAD, 0xAE, 0xEF, 0xFE, 0xED };
static byte ip[] = { 192, 168, 200, 64 };
static byte gateway[] = { 192, 168, 200, 1 };
static byte subnet[] = { 255, 255, 255, 0 };
//pin 
const int PinAnalogicSensor = A0;

static const char sysOIDModel[] PROGMEM = "1.3.6.1.4.1.318.1.1.1.1.1.1.0";  // Model name
static char locOIDModel[20] = "Maca UPS";                                   
static int  ActualState = 2;                                                // 2 = normal, 3 = Power Fail
static int FailState = 3;                                                   // Fail 

char oid[SNMP_MAX_OID_LEN];
SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;

void pduReceived()
{
  SNMP_PDU pdu;
  api_status = Agentuino.requestPdu(&pdu);

  if (pdu.type == SNMP_PDU_GET || pdu.type == SNMP_PDU_GET_NEXT || pdu.type == SNMP_PDU_SET
    && pdu.error == SNMP_ERR_NO_ERROR && api_status == SNMP_API_STAT_SUCCESS) {
    pdu.OID.toString(oid);

    if (strcmp_P(oid, sysOIDModel) == 0 ) {
      if (pdu.type == SNMP_PDU_SET) {
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locOIDModel);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else {
      //SNMP_SYNTAX_INT
      //SNMP_SYNTAX_OCTETS
      //SNMP_SYNTAX_TIME_TICKS
      status = pdu.VALUE.encode(SNMP_SYNTAX_INT, ActualState);
      pdu.type = SNMP_PDU_RESPONSE;
      pdu.error = status;
    }
    Agentuino.responsePdu(&pdu);
  }
  Agentuino.freePdu(&pdu);
}

void setup()
{
  pinMode(PinAnalogicSensor,INPUT_PULLUP);
  Ethernet.begin(mac, ip);
  api_status = Agentuino.begin();
  
  if (api_status == SNMP_API_STAT_SUCCESS) {
    Agentuino.onPduReceive(pduReceived);
    delay(10);
    return;
  }
  delay(10);
}

void loop()
{
  // listen/handle for incoming SNMP requests
  Agentuino.listen();
  
  if (analogRead(0) < 200) ActualState = FailState;   // need reset for restore
}
