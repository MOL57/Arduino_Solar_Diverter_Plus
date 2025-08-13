/*
============================================================================
radio.h
Sends the cloned radio codes to manage the remoteswitches
Inplemented the radio protocol for remote switches of brand GMOMXSEN 
https://www.amazon.es/dp/B09T3C86GY?psc=1&ref=ppx_yo2ov_dt_b_product_details
============================================================================
*/

/*
NOTES:
Adaptation to the remote switches used:
- Radio codes for On and Off of each switch, and their PWM times and parameters must be cloned and included in file radio.h
- Alternatively, loads can be managed in a wired fashion through their correspondig digital outputs, thus avoiding radio cloning the remote switches
*/

class Radio
{
  public:
    Radio(void) {};
    void begin(int pinRadio);
    enum RadioHW { NO_RADIO, RADIO_GMOMXEN };                 // enum of the diverse radio types, here only GMOMXSEN brand is implemented
    int send(RadioHW radioType, int channel, bool setToOn);
  private:
    int sendGmomxen(int channel, bool setToOn);
    int gpioRadio;      // the gpio digital out to PWM modulate the radio transmitter
};


void Radio::begin(int pinRadio) 
{
  gpioRadio = pinRadio;
  pinMode(gpioRadio, OUTPUT);
  digitalWrite(gpioRadio,LOW);
}

int Radio::send(RadioHW radioType, int channel, bool setToOn)   // sends an activation or de-activation code to the remote switch in the channel, using the radio protocol radioType
{
  int ret = 0;

  switch( radioType )
  {
    case RADIO_GMOMXEN:
      ret = sendGmomxen(channel, setToOn);
    default:
      ret=-1;
  }

  return ret;
}

int Radio::sendGmomxen(int channel, bool setToOn)   // sends an activation or de-activation code to the remote switch in the channel, using the GMOMXSEN radio protocol
{
  // constants for radio Gmomxen

  // pulse duration for bit = 0
  static const unsigned long HIGH_SHORT_US = 591UL;
  static const unsigned long LOW_LONG_US = 1263UL;

  // pulse duration for bit = 1
  static const unsigned long HIGH_LONG_US = 1190UL;
  static const unsigned long LOW_SHORT_US = 665UL;

  // repetitions of the code
  static const unsigned long WAIT_REPEAT_US = 7000UL;
  static const int NUM_REPEATS = 5;

  // cloned codes

  static const int N_CHANNELS = 3;
                                              // Off code                          // On code
  static const char *codes[N_CHANNELS][2] = { {"100000011011010000110100000000000","100011101011010000110100000000000"},    // Channel 1
                                              {"101011101011010000110100000000000","101001101011010000110100000000000"},    // Channel 2
                                              {"100111101011010000110100000000000","100101101011010000110100000000000"} };  // Channel 3
 

  // sends a code with its repetitons

  int rep, i;        
  unsigned long start;  // when the repetition  waiting time started
  char *bitsCode;
  int setOn;

  setOn = setToOn? 1 : 0;
  
  if( (channel <1) || (channel > N_CHANNELS))  return -2;
  channel -= 1;

  bitsCode = codes[channel][setOn];

  int nbits = strlen(bitsCode);     //code length

  for(rep=0; rep<NUM_REPEATS; rep++)
  {
    // waiting time between repetitions
    start = micros();
    while( micros() - start < WAIT_REPEAT_US );

    // sends the bits of the code
    for(i=0; i<nbits; i++)
    {
      if(bitsCode[i]=='0') //bit = 0
      {
        // pulse HIGH
        start = micros();
        digitalWrite(gpioRadio,HIGH);
        while(micros() - start < HIGH_SHORT_US );

        // pulse LOW
        start = micros();
        digitalWrite(gpioRadio,LOW);
        while(micros() - start < LOW_LONG_US );
      }
      else  //bit = 1
      {
        // pulse HIGH
        start = micros();
        digitalWrite(gpioRadio,HIGH);
        while(micros() - start < HIGH_LONG_US );

        // pulse LOW
        start = micros();
        digitalWrite(gpioRadio,LOW);
        while(micros() - start < LOW_SHORT_US );
      } 
    }
  }
}
