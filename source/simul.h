/*
======================================================
simul.h
Reads from serial the simulation or print commands
and the corresponding simulation values
======================================================
*/

class Simul
{
  public:
    Simul(void) {};
    ~Simul(void) {free(sine1000);}
    int begin(int sineTableSize);
    void printHelp(void);             // prints help about the simulation and print commands
    void receiveValues(void);         // reads from serial the simulation commands and the printing commands
    enum SimulMode { NO_SIMUL, SIMUL_ANALOG, SIMUL_POWER }; // modes of simulation: either analog inputs or powers
    enum SimulMode mode = NO_SIMUL;   // mode de simulacio
    long AmplIg = 200L;               // amplitude of the simulated sinoidal wave corresponding to the solar generated current, in ADC resolution units
    long AmplIc = 100L;               // amplitude of the simulated sinoidal wave corresponding to the sconsumed current, in ADC resolution units
    long AmplVx = 410L;               // amplitude of the simulated sinoidal wave corresponding to the grid voltage, in ADC resolution units
    int ShiftIg = 0;                  // phase shift of the simulated sinoidal wave corresponding to the solar generated current, in sampling periods units (must be always >= 0)
    int ShiftIc = 0;                  // phase shift of the simulated sinoidal wave corresponding to the consumed current, in sampling periods units (must be always >= 0)
    long ValV0  = 512L;               // value of the offset-reference voltage, in ADC resolution units
    int Pg = 0;                       // simulated solar generated power (W)
    int Pc = 0;                       // simulated consumed power (W)
    long *sine1000;                   // pointer to the table of sinus values (multiplied by 1000)
    int tableSize = 40;               // size of the table of sinus values
    char printCode = '0';             // code character corresponding to the print command
};

int Simul::begin(int sineTableSize)
{
  int i;

  tableSize = sineTableSize;

  sine1000 = calloc(tableSize, sizeof(long));   // allocates memory for the sinus table

  if(sine1000 == NULL)
  {
    Serial.println("ERROR: sine table could not be allocated");
    return(-1);
  }
 
  for(i=0; i<tableSize; i++)                    // fills the sinus table
    sine1000[i] = (long) roundf(1000.0 * sin(2.0 * PI * ((float) i) / ((float) tableSize)));

  
  Serial.println("\nPress \? to get help about serial commands for simulation and printing\n\n");
  
  return(0);
}

void Simul::printHelp(void)         // prints help for the serail commands for simulation and printing
{
  Serial.println(F("SIMULATION MODES\n"));
  Serial.println(F("To simulate powers, enter:   P gggg, cccc"));
  Serial.println(F("where \n  gggg: generated power (W) \n  cccc: consumed power (W)"));
  Serial.println(F("\nTo simulate analog inputs, enter:   A iii, jjj, vvv, rr, ss, ooo"));
  Serial.println(F("where \n  iii: generated current amplitude (ADC counts)\n"
                    "  jjj: consumed current amplitude (ADC counts)\n"
                    "  vvv:  mains voltage amplitude (ADC counts)\n"
                    "  rr:  generated current phase (samples)\n"
                    "  ss:  consumed current phase (samples)\n"
                    "  ooo: reference voltage (ADC counts)"));
  Serial.println(F("\nTo end simulation, enter:   X"));
  Serial.println(F("Less values than specified can be entered, some trailing values can be omitted"));
  Serial.println(F("\nPRINTING MODES"));
  Serial.println(F("\nTo print every second some variables, enter a single digit:"));
  Serial.println(F("   1: times, 2: measures, 3: computed values, 4: filtered values, 0: no print\n"));
}

void Simul::receiveValues(void)   // Reads from serial all or part of the characters of a serial command
                                  // If first character is 'A': simulate analog inputs: AmplIg, AmplIc, AmplVx, ShiftIg, ShiftIc, ValV0
                                  // If first character is 'P': simulate powers: Pg, Pc
                                  // If first character is 'X': stop simulation
                                  // Otherwise, assume it is a print command, and store the first character
                                  // Non-blocking function, returns immediatelly if no new character has been received, or the line has not been completed
{
  const size_t buffSize = 61;
  static char RxBuffer[buffSize] = {0};   // buffer to hold the received characters
  static int i = 0; 
  char rc;
  int n;
  int m;

  do
  { 
    if(Serial.available()==0) return(0);  // no new characters received
    rc = Serial.read();
    if( i == buffSize - 1 ) rc = '\n';    // if too much characters, truncate the line
    if( rc == '\n') rc = 0;               // if line finished, close the buffer
    RxBuffer[i++] = rc;                   // store the received character in the buffer
  }
  while( rc != 0 );                       // wait for line completion

  n = i-1;                                // number of received characters 

  // This point is only reached after receiving a complete line
  // Now the buffer contains the complete line, and n the number of characters

  // The values in the line in buffer are interpreted and applied to the fields of the simulation object according to the simulation mode indicated by the first character

  if(n>0) //si la línia no és buida
  {
    Serial.println("");
    //Serial.print("Received ");Serial.print(n); Serial.print(" chars: "); Serial.print(RxBuffer);

    if(toupper(RxBuffer[0]) == 'X')       // no simulation
    {
      mode = NO_SIMUL;
      Serial.println("No simulation\n");
    }
    else if(toupper(RxBuffer[0]) == 'A')   // simulation of analog inputs
    {
      mode = SIMUL_ANALOG;
      Serial.print("Simulating Analog Inputs:");   

      RxBuffer[0] = ' ';  
      for(i=0;i<strlen(RxBuffer);i++) if(!isdigit(RxBuffer[i]))  RxBuffer[i]=' ';                               // all non-digits set to spaces
      m = sscanf ( RxBuffer, "%ld %ld %ld %d %d %ld", &AmplIg, &AmplIc, &AmplVx, &ShiftIg, &ShiftIc, &ValV0);   // read values and store in the fields
      
      //Serial.print(m); Serial.print(":"); 
      Serial.print("\tAmplIg: "); Serial.print(AmplIg);  
      Serial.print("\tAmplIc: ");Serial.print(AmplIc);  
      Serial.print("\tAmplVx: ");Serial.print(AmplVx);  
      Serial.print("\tShiftIg: ");Serial.print(ShiftIg); 
      Serial.print("\tShiftIc: ");Serial.print(ShiftIc); 
      Serial.print("\tValV0: ");Serial.print(ValV0);
      Serial.println("\n");
    }
    else if(toupper(RxBuffer[0]) == 'P')   // simulation of powers
    {
      mode = SIMUL_POWER;
      Serial.print("Simulating Powers:");   

      RxBuffer[0] = ' '; 
      for(i=0;i<strlen(RxBuffer);i++) if(!isdigit(RxBuffer[i]))  RxBuffer[i]=' ';   // all non-digits set to spaces
      m = sscanf ( RxBuffer, "%d %d", &Pg, &Pc);                                    // read values and store in the fields
      
      //Serial.print(m); Serial.print(":"); 
      Serial.print("\tPg: "); Serial.print(Pg);  
      Serial.print("\tPc: ");Serial.print(Pc);  
      Serial.println("\n");
    }
    else if( RxBuffer[0] == '?' )             // command to print help about simulation and printing commands
      printHelp();
    else printCode = toupper(RxBuffer[0]);    // is a printing command, store it
  }
  i = 0; 
  RxBuffer[0] = 0;
}


