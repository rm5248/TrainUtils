#include <NmraDcc.h>

// Note: this example is based off of the NmraDcc_ARD_DCCSHieLD-test from
// the NmraDcc library examples

// The Snowball Creek LCC shield uses pin 3 for DCC packet decoding
#define DCC_PIN     3

unsigned long rxDccLedMillis = 0;
unsigned long blinkLedMillis = 0;
int blinkLedVal = 0;
int rxDccBlinkVal = 0;
int nextLedUpdate = 500;
int rxDccCount = 0;
NmraDcc  Dcc;

void notifyDccAccTurnoutOutput (uint16_t Addr, uint8_t Direction, uint8_t OutputPower){
  Serial.print("Accessory: ");
  Serial.print(Addr);
  Serial.print(" dir: ");
  Serial.println(Direction);
}

void setup() {
  Serial.begin(9600);
  uint8_t maxWaitLoops = 255;
  while(!Serial && maxWaitLoops--)
    delay(20);

  
  pinMode (LED_BUILTIN, OUTPUT);

  // Pins 5 and 6 on the LCC shield are used for LEDs
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  
  Serial.println("Snowball Creek LCC Shield DCC decoding example");

  // Setup which External Interrupt, the Pin it's associated with that we're using and enable the Pull-Up
  // Many Arduino Cores now support the digitalPinToInterrupt() function that makes it easier to figure out the
  // Interrupt Number for the Arduino Pin number, which reduces confusion. 
#ifdef digitalPinToInterrupt
  Dcc.pin(DCC_PIN, 1);
#else
  Dcc.pin(0, DCC_PIN, 1);
#endif

  // Call the main DCC Init function to enable the DCC Receiver
  Dcc.init( MAN_ID_DIY, 10, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE, 0 );

  Serial.println("Init Done");
}

void notifyDccMsg( DCC_MSG * Msg)
{
  rxDccCount++;
}

void loop() {
  // You MUST call the NmraDcc.process() method frequently from the Arduino loop() function for correct library operation
  Dcc.process();

  unsigned long now = millis();

  // Blink our builtin LED periodically
  if (now >= blinkLedMillis)
  {
    blinkLedMillis += 1000 ;
    digitalWrite (LED_BUILTIN, blinkLedVal);
    blinkLedVal = !blinkLedVal;
  }

  // As long as we continue to get DCC packets, blink the LEDs on the shield
  if(now >= rxDccLedMillis)
  {
    rxDccLedMillis += 100;

    if(rxDccCount)
    {
      rxDccCount = 0;

      digitalWrite(5, rxDccBlinkVal);
      digitalWrite(6, !rxDccBlinkVal);
      rxDccBlinkVal = !rxDccBlinkVal;
    }
    else
    {
      digitalWrite(5, 0);
      digitalWrite(6, 0);
    }
  }
}
