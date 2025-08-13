/*
==============================================================
countTime.h
Conts elapsed time, and launches flags to run periodical tasks
==============================================================

*/


class CountTime
{
  public:
    CountTime(void) {};               // contructor
    void begin( int, int, int, int);  // inicializes counters and starts counting time
    void update();                    // must be called once every loop() cycle
    unsigned long lastTime = 0UL;     // holds the absolute time when the previous second elapsed
    int hours = 0;                  
    int minutes = 0;
    int seconds = 0;                  // hours:minutes:seconds is the absolute time elapsed from start
    char hhmmss[15];                  // string with hours, minutes and seconds
    bool flagOneSec = false;          // flag indicating that one second have elapsed
    int decidePeriod_s = 5;           // period in seconds between launches of flagDecide
    int countDecide_s = 5;            // seconds lasting to next flagDecide
    bool flagDecide = false;          // flag indicating that one decide period has elapsed
    int refreshPeriod_s = 30;         // mean period in seconds between launches of flagRefresh
    int varRefreshPeriod_s = 5;       // maximum random deviation (plus or minus) of refresh period
    int countRefresh_s = 30;          // seconds lasting to next flagRefresh
    bool flagRefresh = false;         // flag indicating that one refresh period has elapsed
    unsigned long prevLoopStart_us = 0UL;   // when the previous loop started
    unsigned long loopTime_us = 0UL;        // duration of the previus loop
};


void CountTime::begin( int setDecidePeriod_s, int setRefreshPeriod_s, int setVarRefreshPeriod_s, int seedAnalogIn )
{

  decidePeriod_s = setDecidePeriod_s;           // stores the arguments
  refreshPeriod_s = setRefreshPeriod_s;
  varRefreshPeriod_s = setVarRefreshPeriod_s;  

  countDecide_s = decidePeriod_s;               // inicialize the counters
  countRefresh_s = setRefreshPeriod_s;

  randomSeed(analogRead(seedAnalogIn));         // seed for random number generation
  lastTime = millis();                          // now

  strcpy(hhmmss, "00:00:00");
}

// update()
// Must be called once each loop() cycle
// The loop cycle must last less than one second
// If one second has elapsed since the previous call to update()
// updates all the time counters
// and launches flags when the corresponding count periods have elapsed
                 
void CountTime::update(void)
{
  unsigned long now_us;
  flagOneSec =  false;  // the flags remain true for only one loop() cycle
  flagDecide =  false;
  flagRefresh = false;

  now_us = micros();
  loopTime_us = now_us - prevLoopStart_us;
  prevLoopStart_us = now_us;

    
  if(millis() - lastTime < 1000UL) return;  // waits for the next second
  
  lastTime += 1000UL;         // one second elapsed                
  flagOneSec  = true;            
  seconds++;

  if( seconds >= 60 )        // one minute elapsed
  {
    seconds = 0;
    minutes++;

    if( minutes >= 60 )      // one hour elapsed
    {
      minutes = 0;
      hours++;
    }
  }

  snprintf_P(hhmmss,14,PSTR("%02d:%02d:%02d"), hours, minutes, seconds);

  countDecide_s--;
  if( countDecide_s <= 0 )    // decison period elapsed
  {
    countDecide_s = decidePeriod_s;
    flagDecide = true;
  }

  countRefresh_s--;
  if( countRefresh_s <= 0 )    // refresh period elapsed
  {
    countRefresh_s = refreshPeriod_s + random( -varRefreshPeriod_s , +varRefreshPeriod_s ); //some random variation is added to the next refresh period
    flagRefresh = true;
  }

}




