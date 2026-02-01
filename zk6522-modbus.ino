/*
zk6522-modbus by Steve Walker swalker2 at tuta dot com
released to public domain

Example code to control zk6522 dc/dc converter with arduino 
leonardo / micropro mcu.  It would be easy to make it run 
on another mcu.  I choose the micro pro (leonardo clone) because
it was 5v and I thought zk6522 ran on 5v since it had a 5v pin 
on the serial connector. Looking at that signal on my scope shows a 
3.3v signal coming from the zk6522.  At any rate it seems to work 
just fine with a mixed 3.3/5v setup.

As an example we will set the voltage and current set points 
to random values between (3-9v,0.5-1.5a) and then read and display 
to the serial console x number of registers every 5 seconds.

*** WARNING ***: 

Do not run this program with the output connected to 
anything that could be damaged by these random voltage / current
settings.

Serial Config:

I tried to use SoftwareSerial, but it just throws timeout 
errors.  So this example just uses the Serial1 hardware 
interface.

Hardware hookup:

Connect three wires (ground, rx, tx) between the zk6522's
4 pin "Communicate" socket and the arduino's serial1 port
(pins rx and tx) You have to connect rx->tx and tx->rx.  
The 4 pin socket also has a +5v power line, you don't need 
to connect that.  You do need to connect the ground of both
devices together.

*/

#include <ModbusRTUMaster.h>

/* 
These are the defaults.  They must match the settings of the zk6522.

If you are trying to switch the device to english rotate the knob to
the right 3 times.  You will be on the setting page.  It will be in 
gibberish, but you will see the first settings is 'Y' the second is
'N', the third '001', etc.  Hit the M (top left) button to scroll
through the setting until you are on the settings before '115200'.  
That is the language setting. Rotate the knob to the right and you
should see the settings page turn to english.  Keep pressing the M
button until you exit the edit settings mode and the encoder can 
rotate to other menu pages.

I'm not really sure what serial port settings are truely corrent.
I've used 8N1 and 8N2 and the both seem to work the same.  The zk6522
manual says it uses an 11 bit byte which is 8N2, but setting the
serial port to the more common 8N1 (10 bits) seems to work just fine.
*/

#define MODBUS_BUAD 115200
#define MODBUS_CONFIG SERIAL_8N2
#define MODBUS_UNIT_ID 1

// number of registers to read and display
const uint8_t numHoldingRegisters = 0x1f + 1;  // 0x20 will display all the settings except the data groups

// starting register to read and display
const uint8_t startingHoldingRegister = 0;

ModbusRTUMaster modbus(Serial1, MODBUS_CONFIG);

// memory to hold the current register values
uint16_t holdingRegisters[numHoldingRegisters];

// unsigned long transactionCounter = 0;
// unsigned long errorCounter = 0;

// error -> text table taken from ModBusMaster example code 

const char* errorStrings[] = {
  "success",
  "invalid id",
  "invalid buffer",
  "invalid quantity",
  "response timeout",
  "frame error",
  "crc error",
  "unknown comm error",
  "unexpected id",
  "exception response",
  "unexpected function code",
  "unexpected response length",
  "unexpected byte count",
  "unexpected address",
  "unexpected value",
  "unexpected quantity"
};

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  // wait for serial console to become ready 
  while(!Serial); 
  
  // setup the modbus serial port and init 
  Serial1.begin(MODBUS_BUAD,MODBUS_CONFIG);
  modbus.begin(MODBUS_BUAD,MODBUS_CONFIG);
}

void loop() {  
  uint8_t error;
  char str[100];
  int i;
  uint16_t volts,amps;

  // CHANGE the voltage / current to random values....

  // will set voltage to a random number between 3 and 9 volts;
  volts = 300 + random() % 600;
  error = modbus.writeSingleHoldingRegister(MODBUS_UNIT_ID,0x00, volts);

  if(error) {
    snprintf(str,100,"ERROR: %s",errorStrings[error]);
    Serial.println(str);
  }

  // will set amperge to a random number between 0.5 and 1.5 amps;
  amps = 50 + random() % 100;
  error = modbus.writeSingleHoldingRegister(MODBUS_UNIT_ID,0x01, amps);

  if(error) {
    snprintf(str,100,"ERROR: %s",errorStrings[error]);
    Serial.println(str);
  }

  // READ holding registers (settings) of the zk6522 dc / dc converter

  error = modbus.readHoldingRegisters(MODBUS_UNIT_ID, startingHoldingRegister, &holdingRegisters[0], numHoldingRegisters);

  if(error) {
    snprintf(str,100,"ERROR: %s",errorStrings[error]);
    Serial.println(str);
  }

  for(i=0;i<numHoldingRegisters;i++) {
    snprintf(str,100,"%s = %d\n",reg2str(startingHoldingRegister+i),(int)holdingRegisters[i]);
    Serial.print(str);
  }

  Serial.println("\n");

  delay(5000);
}

/* 
returns a human readable label for each holding register defined below

Most of these registers are read/write so the can use the register numbers
below change them just like how the voltage / current was changed above.

For example, if you want to change the backlight level to 2 you would write
to the backlight register, which is 0x14, like this:

modbus.writeSingleHoldingRegister(MODBUS_UNIT_ID,0x14, 2);

I've listed all of the commonly used registers below.  There are others, such
as the data group stuff, the factory reset, etc.  They are all listed in the manual.

Note: 
All the data values are stored / sent in as a 16 bit interger.  You have to multiply 
/ divide by 10,100,1000 (depending on the register) to get the real floating point
number.  Things that can't fit in a 16 bit number without overflowing are broken
into two 16 bit words (high,low) and you have to but them together yourself.
*/
char *reg2str(int r) {
  char *ptr;
  static char buf[16];

  // add more human readable labels here
  switch(r) {
    case 0x00: ptr = "Voltage Setpoint"; break;
    case 0x01: ptr = "Current Setpoint"; break;
    case 0x02: ptr = "Display Voltage"; break;
    case 0x03: ptr = "Display Current"; break;
    case 0x04: ptr = "Output Power"; break;
    case 0x05: ptr = "Input Voltage"; break;
    case 0x06: ptr = "mAh low byte"; break; // mAmp hours
    case 0x07: ptr = "mAh high byte"; break;
    case 0x08: ptr = "Wh low byte"; break; // watt hours
    case 0x09: ptr = "Wh high byte"; break;
    case 0x0a: ptr = "Hours"; break;
    case 0x0b: ptr = "Minutes"; break;
    case 0x0c: ptr = "Seconds"; break;
    case 0x0d: ptr = "Temp Internal"; break;
    case 0x0e: ptr = "Temp External"; break;  // connect a 10k ntc temp probe on the 2 pin socket next to the display socket to read external temps
    case 0x0f: ptr = "Lock"; break;
    case 0x10: ptr = "Protect"; break;
    case 0x11: ptr = "Constant Current"; break;
    case 0x12: ptr = "Power Switch"; break;
    case 0x13: ptr = "Centigrade/fahrenheit"; break;
    case 0x14: ptr = "Backlight Level"; break;
    case 0x15: ptr = "Sleep time"; break;
    case 0x16: ptr = "Model"; break;
    case 0x17: ptr = "Firmware"; break;
    case 0x18: ptr = "Modbus ID/address"; break;
    case 0x19: ptr = "Baud rate"; break;
    case 0x1a: ptr = "Internal Temp Comp"; break;
    case 0x1b: ptr = "External Temp Comp"; break;
    case 0x1c: ptr = "Buzzer"; break;
    case 0x1d: ptr = "Current data group"; break;    
    case 0x1e: ptr = "Status"; break;
    case 0x1f: ptr = "Voltage Setting"; break;    // unsure why there are more than one
    default: 
      snprintf(buf,16,"0x%02X",r);
      ptr = &buf[0]; 
  }

  return(ptr);
}

