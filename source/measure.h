/*
=============================================================================
measure.h
Samples the analog inputs during one grid cycle
Stores the sampled values for the future computation of electrical magnitudes
=============================================================================
*/

/*
NOTES: 

Hardware conditioning of Vx, Ig and Ic analog inputs must ensure a floating ground of 2.5V 
and a maximum amplitude of 2 V, in order not to exceeed the 0 to 5V ADC input range

The values of grid frequency, samples per cycle and ADC prescaler must ensure
that the resulting ADC conversion duration (samplingUs) is lower than the sampling period (samplingPeriodUs)
The default values supplied in the program fulfill this requirement for an Arduino Mega 2560

A complete grid cycle is measured but the phase at which the cycle sampling starts is not fixed

The getCycle method is blocking during one grid cycle period
*/


// grid frequency and samples per grid cycle
const float MAINS_FREQ_HZ = 50.0;       // this value must match the nominal frequency of the grid
const long SAMPLES_PER_CYCLE = 40L;     // number of samples that are taken froma whole grid cycle

// ADC configuration
const int ADC_PRESCALER = 32;           // ADC prescaler value, for prescaler = 32 a conversion time of 34.5us is achieved
const int ADC_RESOLUTION_STEPS = 1024;  // must coincide with the resoluciton of the ADC, maximum 4096 (12 bits) in order to avoid long int overflow during calculations

// REQUIRES PREVIOUS DECLARATION OF CLASS Simul

class Measure
{
  public:
    Measure(void)  {};
    void begin(int v0Gpio, int vxGpio, int igGpio, int icGpio);
    void setADCprescaler(int prescalerValue);
    void getCycle(class Simul *pSM);
    int numSamples = SAMPLES_PER_CYCLE;
    float samplingPeriodUs = ( 1000000.0 / MAINS_FREQ_HZ ) / ( (float) SAMPLES_PER_CYCLE );
    int resolution = ADC_RESOLUTION_STEPS;
    int v0In;
    int vxIn;
    int igIn;
    int icIn;
    int V0[SAMPLES_PER_CYCLE];            // array of samples from the analog input corresponding to reference/offset
    int Vx[SAMPLES_PER_CYCLE];            // array of samples from the analog input corresponding to the grid voltage
    int Ig[SAMPLES_PER_CYCLE];            // array of samples from the analog input corresponding to the solar generation current
    int Ic[SAMPLES_PER_CYCLE];            // array of samples from the analog input corresponding to the consumption current
    int samplingUs[SAMPLES_PER_CYCLE];    // time that lasted the sampling and conversion of all four analog inputs
                                          // With prescaler = 32 and 40 samples/grid cycle, this uses to be 150us 
                                          // so that there are 350us left until the 500us of samplig period (= 20000us / 40)
    unsigned long prevCycleStartUs = 0UL; // Time when the previous grid cycle sampling started
    unsigned long cycleStartUs = 0UL;     // Time when the current grid cycle sampling started 
    unsigned long cycleEndUs;             // Time when the current grid cycle samplig has finished
};

void Measure::begin(int v0Gpio, int vxGpio, int igGpio, int icGpio)
{
  v0In = v0Gpio;
  vxIn = vxGpio;
  igIn = igGpio;
  icIn = icGpio;

  setADCprescaler(ADC_PRESCALER);
}


void Measure::setADCprescaler(int prescalerValue) 
{
  //Serial.print("prescaler: "); Serial.println(prescalerValue);
  
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2)); // clear prescaler bits
  
  switch(prescalerValue)
  {
    case 2:
      ADCSRA |= bit (ADPS0);                               //   2 
      return(0);
    case 4:
      ADCSRA |= bit (ADPS1);                               //   4
      return(0);
    case 8:  
      ADCSRA |= bit (ADPS0) | bit (ADPS1);                 //   8
      return(0); 
    case 16:
      ADCSRA |= bit (ADPS2);                               //  16 
      return(0);
    case 32:
      ADCSRA |= bit (ADPS0) | bit (ADPS2);                 //  32
      return(0); 
    case 64:
      ADCSRA |= bit (ADPS1) | bit (ADPS2);                 //  64 
      return(0);
    case 128:
      ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);   // 128
      return(0);
    default:
      return(-1);  
  }
}

void Measure::getCycle(Simul *pSM)    // During one grid cycle reads and stores the values of the analog inputs at each sampling period. Blocking method during one grid cycle
{
  prevCycleStartUs = cycleStartUs;    
  cycleStartUs = micros();        

  unsigned long prevUs = micros();

  unsigned long samplingStartUs;
  for(int i=0; i<numSamples; i++)
  {
    samplingStartUs = micros();

    if(pSM->mode == Simul::SIMUL_ANALOG)   // simulation of sine wave at analog inputs
    {
      V0[i] = (int) pSM->ValV0;
      Ig[i] = constrain( pSM->ValV0  + (int) ((pSM->AmplIg) * pSM->sine1000[ (i+pSM->ShiftIg) % numSamples ] / 1000L), 0, resolution);
      Vx[i] = constrain( pSM->ValV0  + (int) ((pSM->AmplVx) * pSM->sine1000[ (i+0           ) % numSamples ] / 1000L), 0, resolution);
      Ic[i] = constrain( pSM->ValV0  + (int) ((pSM->AmplIc) * pSM->sine1000[ (i+pSM->ShiftIc) % numSamples ] / 1000L), 0, resolution);
    }
    else                                  // reading the actual analog inpunts
    {
      V0[i]=analogRead(v0In);
      Ig[i]=analogRead(igIn);
      Vx[i]=analogRead(vxIn);              // the grid voltage is read between both currents, in order to minimize phase delay between voltage and current
      Ic[i]=analogRead(icIn);
    }
     
    samplingUs[i]=(int)(micros()-samplingStartUs);
        
    while( micros() - prevUs < ((unsigned long) samplingPeriodUs) );  // waiting for the next sampling period
    prevUs += ((unsigned long) samplingPeriodUs); 
  }
  cycleEndUs = micros();
  return(0);
}
