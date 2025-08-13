/*
=======================================================================================
values.h
Computes the electrical magnitudes (RMS voltage and currents, powers and power factors)
from the values sampled from the last grid cycle, stored at measure.h
=======================================================================================

*/

/*
NOTES:

This task is performed once for every grid cycle measured

Hardware conditioning of Vx, Ig and Ic analog inputs must ensure a floating ground of 2.5V 
and a maximum amplitude of 2 V, in order not to exceeed the 0 to 5V ADC input range

The sign convention for the powers is:
- positive for generated solar power and for excedents exported to the grid
- negative for consumed power and for deficits imported from the grid
*/


const float V0_REF_V = 2.5;           // DC reference voltage at the V0 analog input, which acts as an offset (floating ground) for the other three analog inputs (grid voltage, solar generated current and consumed current)
const float MAX_AMPL_V = 2.0;         // Largest expected amplitude at the three analog inputs (grid voltage, solar generated current and consumed current), reached when their nominal RMS voltage or current is achieved
const float TIME_CONSTANT_US = 1.0e6; // Filtering  time constant for the powers in microseconds (default 1 second)

class Values 
{
  public:
  Values(void) {};
  void begin( float VxNomVeff, float IgNomAeff, float IcNomAeff, float MaxConsumpt_arg);
  void compute(Simul *pSM, Measure *pCM);
  float V0RefV = V0_REF_V;            // Offset voltage of the inputs (floating ground), also used as a voltage reference to compute volts per count ratio of the ADC
  float MaxAmplV = MAX_AMPL_V;        // Maximum expected amplitude in the analog inputs
  float VxRatio;                      // Ratio between the grid RMS voltage and the amplitude at the corresponding analog input
  float IgRatio;                      // Ratio between the solar generated RMS current and the amplitude at the corresponding analog input  
  float IcRatio;                      // Ratio between the consumed RMS current and the amplitude at the corresponding analog input  
  float V0Avg;                        // Average of the offset voltage samples of one grid cycle
  float VoltsPerCount;                // Computed ratio between analog input volts and the measured ADC counts
  float VxEff;                        // Computed RMS voltage of the grid
  float IgEff;                        // Computed RMS intensity of the solar generation
  float IcEff;                        // Computed RMS intensity of the consumption
  float Pg;                           // Computed solar generated power (always positive)
  float Pc;                           // Computed consumed power (always negative)
  float Pn;                           // Computed net power balance (positive if excedents exported to grid, negative if deficits imported from grid)
  float PFg;                          // Computed power factor (cos phi) of the solar generated power
  float PFc;                          // Computed power factor (cos phi) of the consumed power
  float TimeConst = TIME_CONSTANT_US; // Time constant (microseconds) for filtering the powers
  float PgFilt;                       // Filtered solar generated power
  float PcFilt;                       // Filtered consumed power
  float PnFilt;                       // Filtered net power
  float MaxConsumpt;                  // Maximum allowed consumed power, if exceeded can trip grid protections
  float Margin;                       // Difference between the maximum allowed consumed power, and the actual consumed power
  unsigned long interval;             // Time between the previous grid cycle mesurement and the current measurement (0 if there is no previous
  unsigned long startUs;              // When the computation started
  unsigned long endUs;                // When the computation finished
  unsigned long samplingTimeAvg_us;   // Average sampling time of the 4 analog inputs
};

void Values::begin(float VxNomVeff, float IgNomAeff, float IcNomAeff, float MaxConsumpt_arg )
{
  VxRatio = VxNomVeff * 1.4142 / MaxAmplV;
  IgRatio = IgNomAeff * 1.4142 / MaxAmplV;
  IcRatio = IcNomAeff * 1.4142 / MaxAmplV;
  MaxConsumpt = MaxConsumpt_arg;

}



void Values::compute(Simul *pSM, Measure *pCM)  // computes RMS voltage and currents, powers and power factors, takes 2ms approx.
{
  int i;
  long offset, value, sum;

  startUs = micros();

  // average sampling time
  for( i=0, sum=0; i<pCM->numSamples; i++ )
    sum += pCM->samplingUs[i];
  samplingTimeAvg_us = sum / pCM->numSamples;

  // offset value and conversion scaling factor
  sum = 0L; 
  for(i=0; i<pCM->numSamples; i++)
    sum += (long) pCM->V0[i];
  offset = sum / ((long) pCM->numSamples);  // averages offset
  V0Avg = (float) offset;
  VoltsPerCount = V0RefV / max(1.0,V0Avg); // conversion scaling factor, avoids division by 0

  // RMS grid voltage
  sum = 0L; 
  for(i=0; i<pCM->numSamples; i++)
  {
    value = ((long) pCM->Vx[i]) - offset;  // substracts offset
    sum += (value * value);
  }
  VxEff = sqrt(((float)sum) / ((float) pCM->numSamples)) * (VoltsPerCount * VxRatio); 

  // solar generated RMS intensity
  sum = 0L; 
  for(i=0; i<pCM->numSamples; i++)
  {
    value = ((long) pCM->Ig[i]) - offset;  // substracts offset
    sum += (value * value);
  }
  IgEff = sqrt(((float)sum) / ((float) pCM->numSamples)) * (VoltsPerCount * IgRatio);
  
  // solar generated power and its power factor
  sum = 0L; 
  for(i=0; i<pCM->numSamples; i++)
  {
    value = (((long) pCM->Ig[i]) - offset) * (((long) pCM->Vx[i]) - offset); // substracts offset
    sum += value;  
  }  
  Pg = (((float)sum) / ((float) pCM->numSamples)) * (VoltsPerCount * IgRatio) * (VoltsPerCount * VxRatio);
  Pg = abs(Pg); // solar generated power is always positive (don't care wiring polarity)
  PFg = Pg / max( 1.0, VxEff * IgEff); // power factor, avoid division by 0
  
  // consumed RMS intensity
  sum = 0L; 
  for(i=0; i<pCM->numSamples; i++)
  {
    value = ((long) pCM->Ic[i]) - offset;  // substracts offset
    sum += value * value;
  }
  IcEff = sqrt(((float)sum) / ((float) pCM->numSamples)) * (VoltsPerCount * IcRatio);
  
  // consumed power and its power factor
  sum = 0L; 
  for(i=0; i<pCM->numSamples; i++)
  {
    value = (((long) pCM->Ic[i]) - offset) * (((long) pCM->Vx[i]) - offset); // substracts offset
    sum += value;
  }
  Pc = (((float)sum) / ((float) pCM->numSamples)) * (VoltsPerCount * IcRatio) * (VoltsPerCount * VxRatio);
  Pc = - abs(Pc); // consumed power is always negative (don't care wiring polarity)
  PFc = - Pc / max( 1.0, VxEff * IcEff); // power factor, avoid division by 0

  if(pSM->mode == Simul::SIMUL_POWER)   // if simulating powers
  {
    Pg = (float) pSM->Pg;
    Pc = - (float) pSM->Pc;
  }

  // net power (exported if >0, imported if <0)
  Pn = Pg + Pc;

  // clipping extreme values (when failing analog inputs), to avoid overflow when converting to int

  VxEff = constrain( VxEff,  0.0,  999.0 );
  IgEff = constrain( IgEff,  0.0,   99.0 );
  IcEff = constrain( IcEff,  0.0,   99.0 );              
  Pg =    constrain( Pg,     0.0, 9999.0 );            
  Pc =    constrain( Pc, -9999.0,    0.0 );                   
  Pn =    constrain( Pn, -9999.0, 9999.0 );      
  PFg =   constrain( PFg,    0.0,   0.99 ); 
  PFc =   constrain( PFc,    0.0,   0.99 ); 

  // Filtering (smoothing) of the powers, to avoid instability of the activation of the loads

  float alpha;  // weight in the filter formula of the most recent grid cycle measure

  if(interval ==0L)  // no filtering if ther are no previous measurements
  {
    PgFilt = Pg;
    PcFilt = Pc;
    PnFilt = Pn;    
  }
  else
  {
    alpha = min(1.0, ((float) interval) /TimeConst); // the weight of the new cycle measure computed as the time between successive grid cycle measures divided by the time constant
    PgFilt = PgFilt + alpha * (Pg - PgFilt);
    PcFilt = PcFilt + alpha * (Pc - PcFilt);
    PnFilt = PgFilt + PcFilt;
 }

 Margin = MaxConsumpt - (-PcFilt);  // remaining power margin until the maximum allowed consumption (Pc is negative)

  // Time between the previous grid cycle measure and the current one
  if(pCM->prevCycleStartUs==0L)
    interval = 0L;
  else
    interval = pCM->cycleStartUs-pCM->prevCycleStartUs;
 
  endUs = micros();
}
  

