#include <SPI.h>
#include <mcp_can.h>

const int CAN_0_INT_PIN = 2;
const int CAN_1_INT_PIN = 3;
unsigned char flag0Recv = 0;
unsigned char flag1Recv = 0;
const int spi0CSPin = 9;
const int spi1CSPin = 10;
uint8_t frame[8];
uint8_t data[8];
uint8_t bytes[8];
byte len = 0;
byte destino;
unsigned long canId;
unsigned long lenght;

MCP_CAN CAN_0(spi0CSPin);
MCP_CAN CAN_1(spi1CSPin);

typedef union {
    uint64_t uint64;
    uint32_t uint32[2]; 
    uint16_t uint16[4];
    uint8_t  uint8[8];
    int64_t int64;
    int32_t int32[2]; 
    int16_t int16[4];
    int8_t  int8[8];

    //deprecated names used by older code
    uint64_t value;
    struct {
        uint32_t low;
        uint32_t high;
    };
    struct {
        uint16_t s0;
        uint16_t s1;
        uint16_t s2;
        uint16_t s3;
    };
    uint8_t bytes[8];
    uint8_t byte[8]; //alternate name so you can omit the s if you feel it makes more sense
} BytesUnion;

void j1939Decode(long ID, unsigned long* PGN, byte* priority, byte* src_addr, byte *dest_addr)
{
  /* decode j1939 fields from 29-bit CAN id */
  *src_addr = 255;
  *dest_addr = 255;

  *priority = (int)((ID & 0x1C000000) >> 26);

  *PGN = ID & 0x00FFFF00;
  *PGN = *PGN >> 8;

  ID = ID & 0x000000FF;
  *src_addr = canId;

  /* decode dest_addr if message is peer to peer */
  if( (*PGN > 0 && *PGN <= 0xEFFF) ||
      (*PGN > 0x10000 && *PGN <= 0x1EFFF) ) { 
    *dest_addr = (int)(*PGN & 0xFF);
    destino = *dest_addr;
    *PGN = *PGN & 0x01FF00;
  }
}

void got_frame() {
  unsigned long PGN;
  byte priority;
  byte srcaddr;
  byte destaddr;
  uint8_t signal_type;

  j1939Decode(canId, &PGN, &priority, &srcaddr, &destaddr);

  if (srcaddr == 28 && PGN == 65535 && frame[0] == 0x53) {

    //most significant nibble (4 bits) indicates the signal
    //type the GPS has
    signal_type = frame[3] >> 4;

    if (signal_type < 4) { //if we don't have SF1, pretend we do
      frame[4] = 0x40; //indicate that the receiver is set to SF1

      //Serial.println("DEBUG: Steer with low-quality signal.");

      //the least significant nibble is the signal strength
      //bar graph indicator
      if (signal_type > 1)
        //WAAS, so pretend SF1, medium accuracy
        frame[3] = 0x46; 
      else
        //3D+ so pretend SF1, low accuracy
        frame[3] = 0x43; 
    }
    //otherwise we'll let it through as is
  }
}

void setup() {
  Serial.begin(115200);
      
  while (CAN_OK != CAN_0.begin(CAN_500KBPS)) { 
        Serial.println("#PD_CAN0;error");
    }
  while (CAN_OK != CAN_1.begin(CAN_500KBPS)) { 
        Serial.println("#PD_CAN1;error");
    }
    attachInterrupt(digitalPinToInterrupt(CAN_0_INT_PIN), GOT_int0, FALLING);
    attachInterrupt(digitalPinToInterrupt(CAN_1_INT_PIN), GOT_int1, FALLING);
}

void GOT_int0(){
  Serial.println("CAN0_IN");
  while(CAN_MSGAVAIL == CAN_0.checkReceive()){
        CAN_0.readMsgBuf(&len, frame);
        canId = CAN_0.getCanId();
    }
  //got_frame();
  CAN_1.sendMsgBuf(canId, 0, 8, frame);
  Serial.println("CAN1_OUT");  
  }

void GOT_int1(){
  Serial.println("CAN1_IN");
  while(CAN_MSGAVAIL == CAN_1.checkReceive()){
        CAN_1.readMsgBuf(&len, frame);
        canId = CAN_1.getCanId();
    }
  //got_frame();
  CAN_0.sendMsgBuf(canId, 0, 8, frame);
  Serial.println("CAN0_OUT");  
  }

void can0_got() {  
  
 // while(CAN_MSGAVAIL == CAN_0.checkReceive()){
    //    CAN_0.readMsgBuf(&len, frame);
    //    canId = CAN_0.getCanId();
        
//  }
  //got_frame();
  CAN_0.sendMsgBuf(canId, 0, 8, frame);  
  
   
}

void can1_got() {
  while(CAN_MSGAVAIL == CAN_1.checkReceive()){
        CAN_1.readMsgBuf(&len, frame);
        canId = CAN_1.getCanId();
    }
  //got_frame();
  CAN_1.sendMsgBuf(canId, 0, 8, frame);  
}

void loop() {
}
