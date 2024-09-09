#include "heltec.h"
#include "Arduino.h"
#include "WiFi.h"
#include "images.h"
#include "LoRaWan_APP.h"
#include "Wire.h"  
#include "HT_SSD1306Wire.h"

#define trigPin                                     19  //Define pins for AJRS04M                            
#define echoPin                                     26

#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             5        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket1[BUFFER_SIZE];
char txpacket2[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

int distance;
double dbDistance;
double dbPrFull;
double txNumber;

bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

// Initalise dactory_display
SSD1306Wire  factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst


void logo(){
	factory_display.clear();
	factory_display.drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
	factory_display.display();
}


void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
	
    txNumber=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 

  

    //OLED 
    factory_display.init();
    factory_display.clear();
    factory_display.display();
    logo();
    delay(300);
    factory_display.clear();

    //initialize Serial Monitor
    Serial.begin(115200);

    //initialize Trig Echo
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    //initialize LED
    pinMode(LED ,OUTPUT);

}
void loop()
{
  // Clear the trigPin by setting it LOW:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
 
 // Trigger the sensor by setting the trigPin high for 10 microseconds:
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
// Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:
int  duration = pulseIn(echoPin, HIGH);
  
// Calculate the distance:
// distance = duration*0.034/2;
  
distance = (duration/2) / 29.1;

// Print the distance on the Serial Monitor (Ctrl+Shift+M):
int waterLevelPer = map(distance,22,200, 100, 0);
if(waterLevelPer<0)
{
  waterLevelPer=0;
  }
if (waterLevelPer>100)
{
  waterLevelPer=100;
  }
  Serial.print("Distance = ");
  Serial.print(distance);
  Serial.println(" cm");
  Serial.print("% Full = ");
  Serial.print(waterLevelPer);
  Serial.println(" %");
 
    
  if(lora_idle == true)
	{
    delay(1000);
		txNumber += 0.01;
    dbDistance = double(distance);
    dbPrFull = double(waterLevelPer);
		sprintf(txpacket1,"Water Level is %0.1f",dbDistance);  //start a package1
   
	   
		Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket1, strlen(txpacket1));

		Radio.Send( (uint8_t *)txpacket1, strlen(txpacket1) ); //send the package out	
   
    delay(1000);

    sprintf(txpacket2,"Percentage Full is %0.1f",dbPrFull);         //start a package2
    Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket2, strlen(txpacket2));

		Radio.Send( (uint8_t *)txpacket2, strlen(txpacket2) ); //send the package out	
    lora_idle = false;

	  digitalWrite(LED, HIGH); 

    factory_display.drawString(0, 10,"Water Level Sender");
    factory_display.drawString(0, 20,txpacket1);
    factory_display.drawString(0, 30,txpacket2);
    factory_display.display();
    delay(100);
    factory_display.clear();

    digitalWrite(LED, LOW);
	}
  Radio.IrqProcess( );
}

void OnTxDone( void )
{
	Serial.println("TX done......");
	lora_idle = true;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.println("TX Timeout......");
    lora_idle = true;
}