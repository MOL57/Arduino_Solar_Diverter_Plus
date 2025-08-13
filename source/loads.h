/*
============================================================
loads.h
Defines the characteristics of the loads
Decides whether to switch On or Off each load
according to the available power excedent and margin
Activates or deactivates each load according to the decision
Periodically refreshes the activation status of the loads
============================================================
*/

const int N_LOADS_MAX = 3;                  // Maximum number of loads to be managed
const float POWER_REDUCTION_FACTOR = 0.85;  // To solve a priority inversion, the nominal power of the load to be set to off is reduced by this factor, 
                                            // in order not to compute a too optimistic available power

class Loads
{ 
  public:
    Loads(void) {};                                             // constructor
    int add(char *,float,int,int,int,int,Radio::RadioHW, int);  // adds and inicializes a new load
    void decide(CountTime *, Values *);                         // decides which loads are activated or deactivated according to the powers
    void activate(CountTime *, Radio *, Values * );             // executes the activation and deactivation of the loads according to the decision, and also periodically refreshes the activation status
    void print( int, CountTime *, Values * );                   // prints the change of status of load
    void printRefr( int, CountTime * );                         // prints the periodical refresh of load
    int nLoadsMax = N_LOADS_MAX;                                // maximum number of loads to be managed
    int nLoads = 0;                                             // actual number of loads which are managed
    char *cause = "program start";                              // reason for the most recent change on loads

    // configuration data
    char *name[N_LOADS_MAX];                                    // name of the load, to be displayed (max 5 characters)
    float powerW[N_LOADS_MAX];                                  // nominal power of the load (Watts)
    int lockOnSec[N_LOADS_MAX];                                 // time in seconds which the activated load is prevented to be switched off 
    int lockOffSec[N_LOADS_MAX];                                // time in seconds which the deactivated load is prevented to be switched on 
    int gpioOut[N_LOADS_MAX];                                   // number of the digital output gpio which activates-deactivates the load (-1 if no output is assigned) 
    int gpioMode[N_LOADS_MAX];                                  // number of the digital input gpio where the manual/solar switch of the load is connected
    Radio::RadioHW radioModel[N_LOADS_MAX];                     // type of radio model/protocol used by the remote switch which controls the load
    int channel[N_LOADS_MAX];                                   // radio channel number used by the remote switch which controls the load

    // Status data
    bool solarMode[N_LOADS_MAX];                                // load mode, false = manual, true = solar
    bool flag[N_LOADS_MAX];                                     // true if the ativation status is pending to be changed, false if not
    bool on[N_LOADS_MAX];                                       // true if the load is On, false if Off
    int lockSec[N_LOADS_MAX];                                   // remaining time in seconds until a change in the activation status of the load will be allowed 
};

int Loads::add( char * name_arg, float powerW_arg, int lockOnSec_arg, int lockOffSec_arg, int gpioOut_arg, int gpioMode_arg, Radio::RadioHW radioModel_arg, int channel_arg )
{
  if (nLoads >= nLoadsMax ) return(-1);       // ERROR: no space for another load

  name[nLoads] =        name_arg;
  powerW[nLoads] =      powerW_arg;
  lockOnSec[nLoads] =   lockOnSec_arg;
  lockOffSec[nLoads] =  lockOffSec_arg;
  gpioOut[nLoads] =     gpioOut_arg;
  gpioMode[nLoads] =    gpioMode_arg;
  radioModel[nLoads] =  radioModel_arg;
  channel[nLoads] =     channel_arg;
  flag[nLoads] =        true;
  on[nLoads] =          false;
  lockSec[nLoads]  =    0;

  if( gpioOut[nLoads] != -1 )                 // initializes load output to Off, if there is a pin assigned
  {
      pinMode( gpioOut[nLoads], OUTPUT );
      digitalWrite( gpioOut[nLoads], LOW );
  }

  if( gpioMode[nLoads] != -1 )                 // initializes and reads the input from the manual/solar switch, if there is a pin assigned
  {
    pinMode(gpioMode[nLoads], INPUT_PULLUP); 
    solarMode[nLoads] = digitalRead(gpioMode[nLoads]);
  }
  else
  {
    solarMode[nLoads] = true;                  // if no pin assigned, assumes that mode is solar
  }
   
  nLoads++;

  return(0);
}

void Loads::decide(CountTime *pCT, Values *pCV) //decides whether every load must be activated or deactivated according to consumption margin and solar excedent
{
  int i, j;
    
  if( pCT->flagOneSec )                               // task every second
  {
    for(i=0; i< nLoads; i++)
    {
        solarMode[i] = digitalRead(gpioMode[i]);      // updates mode solar/manual according to switch input
        if(lockSec[i] > 0)   lockSec[i]--;            // decreases lock time counter
    }
  }

  if( pCT->flagDecide )                               // decision tasks are run every decide period, period should be >= 6 * filtering time constant (for stability)
  {
    // IF NO CONSUMPTION MARGIN, DEACTIVATE THE ACTIVE LOAD WITH LEAST PRIORITY
    // DISREGARD LOCK TIME COUNTER, DEACTIVATION MUST BE IMMEDIATE TO AVOID GRID PROTECTION TO TRIP

    if( pCV->Margin <= 0 )
    {
      for( i=nLoads-1; i>=0; i-- )                    // from less to more priority
      {
        if(on[i])
        {
          on[i] = false;  
          flag[i] = true; 
          lockSec[i] = lockOffSec[i];
          cause = "no margin";
          return;                                     // no more tasks are performed until next decide period
        }
      }
    }

    // IF SOLAR EXCEDENT NEGATIVE, DEACTIVATE THE ACTIVE LOAD IN SOLAR MODE WITH LEAST PRIORITY

    if( pCV->PnFilt <= 0.0)
    {
      for( i=nLoads-1; i>=0; i-- )                    // from less to more priority
      {
        if( solarMode[i] && on[i] && (lockSec[i] == 0) ) 
        {
          on[i] = false;  
          flag[i] = true; 
          lockSec[i] = lockOffSec[i];
          cause = "no excedent";
          return;                                      // no more tasks are performed until next decide period
        }                         
      }
    }

    // AVOIDING PRIORITY INVERSION
    // When a load  with higher priority is Off, and a load with lower priority is On,  
    // and the (reduced) nominal power of the lower priority load plus the solar excedent would suffice to supply the load with higher priority
    // in such a case, set to Off the load with lower priority,
    // so that in the next decision period, if there is actually enough excedent, the higher priority load will be set to On 

    for( i = 0; i < nLoads; i++ )
    {
      if( ( !on[i] ) && ( solarMode[i] ) && ( lockSec[i] == 0 ) )                   // a higher priority load in off condition, solar mode, and ready to change status
      {
        for( j = i+1; j < nLoads; j++ )
        {
          if( ( on[j] ) && ( solarMode[j] ) && ( lockSec[j] == 0 ) &&               // a lower priority load in on condition, solar mode, and ready to change status
              ( powerW[j] * POWER_REDUCTION_FACTOR + pCV->PnFilt >= powerW[i] ) )   // the (reduced) power of the lower priority load plus the excedent would suffice to supply the higher prority load
          {                                                                                
            on[j] = false;                                                          // put to Off the lower priority load
            flag[j] = true; 
            lockSec[j] = lockOffSec[j];
            cause = "priority inversion";
            return;                                                                 // no more tasks are performed until next decide period
          }
        }
      }
    }

    // ACTIVATE THE MOST PRIORITY LOAD IF THERE IS ENOUGH CONSUMPTION MARGIN FOR ITS NOMINAL POWER
    // AND, IF THE LOAD IS IN SOLAR MODE, IF THERE IS ENOUGH SOLAR EXCEDENT FOR ITS NOMINAL POWER

    for( i=0; i < nLoads; i++)                                      // from more to less priority
    {
      if( ( !on[i] ) && ( lockSec[i] == 0 ) && ( powerW[i] < pCV->Margin ) && ( ( !solarMode[i] ) || ( powerW[i] < pCV->PnFilt ) ) )
      {
          on[i] = true;  
          flag[i] = true; 
          lockSec[i] = lockOnSec[i]; 
          if( !solarMode[i] ) cause = "enough margin";
          else                cause = "enough excedent and margin";
          return;                                                   // no more tasks are performed until next decide period
      }
    }
  }
}


void Loads::activate(CountTime *pCT, Radio *pRD, Values *pCV)  // manages the load output and sends the load radio messsage to the remote switch
                                                               // to activate/deactivate the loads which have changed (according to their flag)
                                                               // and also to periodically refresh their status (to cope with radio interferences which prevented receiving previous messages by the remote switches)
{
  int i;

  for( i=0; i < nLoads; i++)
  {
    if( flag[i] || pCT->flagRefresh )
    {
      if( flag[i] ) print( i, pCT, pCV);
      else          printRefr( i, pCT );
    
      flag[i] = false;
      
      if( gpioOut[i] != -1 )
        digitalWrite( gpioOut[i], on[i] ? HIGH : LOW );
      
      if( radioModel[i] != Radio::NO_RADIO )                    // only if there is a radio model assigned
        pRD->send( radioModel[i], channel[i], on[i] );
    }
  }
}

void Loads::print( int iLoad, CountTime *pCT, Values *pCV )     // prints the change on load status which has been decided
{
  snprintf_P(buffer,149,PSTR("%s Load \"%s\" set to %s \tPg_W:%d \tPc_W:%d \texcedent_W:%d \tmargin_W:%d \tcause: %s\n"),
                            pCT->hhmmss, name[iLoad], on[iLoad]?"On ":"Off", 
                            (int) round( pCV->PgFilt ), (int) round( pCV->PcFilt ),
                            (int) round( pCV->PgFilt + pCV->PcFilt ), (int) round( pCV->Margin ), cause );
  Serial.print(buffer);
}

void Loads::printRefr( int iLoad, CountTime *pCT )              // prints the refreshed load status
{
  snprintf_P(buffer,99,PSTR("%s Load \"%s\" refreshed (%s)\n"),pCT->hhmmss, name[iLoad], on[iLoad]?"On":"Off" );
  Serial.print(buffer);
}
