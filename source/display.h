/*
==================================================================
display.h
Sends formatted information to a LCD display connected by I2C
Several screens can be sequentially displayed by pressing a button
==================================================================
*/

/*
NOTES:

Only 2 loads are displayed on the screen, 
formatting code must be changed to accomodate for more loads

Writing all the display screen takes a lot of time
Thus, it is refreshed only every 1 second
and only one display line is refreshed each loop cycle
so that it takes 4 loop cycles to write one screen

There are three screens:
- the electric magnitudes and the total time spent from start of the program
- the powers balance, the status of the loads, and the time to next decision
- the program credits (source file name and date/hour of compilation) 
*/



const int DISPLAY_I2C_ADDRESS = 0x27;   // I2C address of the display
const int DISPLAY_COLS = 20;            // number of columns
const int DISPLAY_ROWS = 4;             // number of rows
const int DISPLAY_SCREENS = 3;          // number of screens to display


class Display
{
  public:
    Display(void) {};                                                     // constructor
    void begin(int);                                                      // inicialization
    void show(Credits *, CountTime *, Values *, Simul *, Loads *);        // refreshing the display
    LiquidCrystal_I2C lcd = LiquidCrystal_I2C(DISPLAY_I2C_ADDRESS, DISPLAY_COLS, DISPLAY_ROWS);    // display lcd object
    int buttonGpio;                                                       // input gpio where the button to change screen is connected
    int button;                                                           // status of the button (LOW = pressed)
    int prevButton = HIGH;                                                // previous status on the button (at the previous loop cycle)
    int line = -1;                                                        // next display line to be refreshed, -1 if waiting the 1 second period
    int screen = 1;                                                       // which screen must be displayed
    unsigned long displayTimeUs;                                          // time spent while executing the show function, it lasts 32 ms approx
};

void Display::begin( int buttonGpio_arg )
{
  lcd.init();                                 // inicialize LCD
  lcd.backlight();
  lcd.clear();

  buttonGpio = buttonGpio_arg;
  pinMode(buttonGpio, INPUT_PULLUP);          // inicialize button input
}

void Display::show( Credits *pCR, CountTime *pCT, Values *pCV, Simul *pSM, Loads *pLD )  // show the measures on the display, one line at a time, and manages the change screen button
{
  unsigned long startUs;                       // measures the time spent in the function, it lasts 32 ms approx
  int i;
  
  startUs = micros();

  button = digitalRead(buttonGpio);
  if( (prevButton == HIGH) && (button == LOW) ) // screen is changed on the falling edge of the button input, no debuncing is required because it is sampled once each cycle loop  (20ms or more)
  { 
    screen = (screen+1) % DISPLAY_SCREENS;      
    line = 0;  
  }
  prevButton = button;
  
  if(line==-1)                                  // only if the screen have been fully refreshed (no line pending)
  {
    if( !pCT->flagOneSec)   return;             // only after 1 second elapsed
    line = 0;
  }
  
  switch(screen)
  {
    case 0:                                     // SCREEN WITH THE ELECTRIC MEASURES

      switch(line)                              // only one line is refreshed each loop cycle
      {
        case 0:
          snprintf_P(buffer,21,PSTR("Gen: % 5dW %2d.%1dA %2d          "), (int) round(pCV->Pg), (int) (pCV->IgEff),( (int) (10.0*pCV->IgEff) )%10, min( 99,(int) round (100.0*pCV->PFg) ) );
          lcd.setCursor(0, 0); lcd.print(buffer);
          line++;
          break;
        case 1:
          snprintf_P(buffer,21,PSTR("Cons:% 5dW %2d.%1dA %2d          "),(int) round(pCV->Pc), (int) (pCV->IcEff), ( (int) (10.0*pCV->IcEff) )%10, min( 99,(int) round(100.0*pCV->PFc) ) );
          lcd.setCursor(0, 1); lcd.print(buffer);
          line++;
          break;
        case 2:
          snprintf_P(buffer,21,PSTR("Exc: % 5dW  %3dV                 "),(int) round(pCV->Pn), (int) round(pCV->VxEff) );
          lcd.setCursor(0, 2); lcd.print(buffer);
          line++;
          break;
        case 3:
          snprintf_P(buffer,21,PSTR("  %s     %s     "), pCT->hhmmss, ( pSM->mode == Simul::NO_SIMUL ) ? "    " : ( ( pSM->mode == Simul::SIMUL_ANALOG ) ? "SimA" : "SimP" ) );
          lcd.setCursor(0, 3); lcd.print(buffer);
          line = -1;  //
          break;
        default:
          lcd.clear();
          line = -1;
      }
      break;

    case 1:                                     // SCREEN WITH THE POWERS AND THE LOADS DATA

      switch(line)                              // only one line is refreshed each loop cycle
      {
        case 0:
          snprintf_P(buffer,21,PSTR("%dW-%dW=%dW                      "), (int) round(pCV->PgFilt), abs( (int) round(pCV->PcFilt) ), (int) round(pCV->PnFilt) );
          lcd.setCursor(0, 0); lcd.print(buffer);
          line++;
          break;
        case 1:
          snprintf_P(buffer,21,PSTR("%s %dW %s %s%3ds                     "), pLD->name[0], (int) round(pLD->powerW[0]), ( pLD->on[0]?"On ":"Off" ), ( pLD->solarMode[0]?"S":"M" ), pLD->lockSec[0] );
          lcd.setCursor(0, 1); lcd.print(buffer);
          line++;
          break;
        case 2:
          if(pLD->nLoads>1)
            snprintf_P(buffer,21,PSTR("%s %dW %s %s%3ds                     "), pLD->name[1], (int) round(pLD->powerW[1]), ( pLD->on[1]?"On ":"Off" ), ( pLD->solarMode[1]?"S":"M" ), pLD->lockSec[1] );
          else
            snprintf_P(buffer,21,PSTR("                                 "));
          lcd.setCursor(0, 2); lcd.print(buffer);
          line++;
          break;
        case 3:
          snprintf_P(buffer,21,PSTR("%2ds marg:%4dW %s     "), pCT->countDecide_s, (int) round(pCV->Margin), ( pSM->mode == Simul::NO_SIMUL ) ? "    " : ( ( pSM->mode == Simul::SIMUL_ANALOG ) ? "SimA" : "SimP" ) );
          lcd.setCursor(0, 3); lcd.print(buffer);
          line = -1;  //
          break;
        default:
          lcd.clear();
          line = -1;
      }
      break;

    case 2:                                     // SCREEN WITH THE CREDITS OF THE PROGRAM

      switch(line)                              // only one line is refreshed each loop cycle
      {
        case 0:
          snprintf_P(buffer,21,PSTR("%-20s"), pCR->fileName);
          lcd.setCursor(0, 0); lcd.print(buffer);
          line++;
          break;
        case 1:
          if( strlen(pCR->fileName) < 20 )
            snprintf_P(buffer, 21, PSTR("                       "));
          else
            snprintf_P(buffer, 21, PSTR("%-20s"), 20 + pCR->fileName);
          lcd.setCursor(0, 1); lcd.print(buffer);
          line++;
          break;
        case 2:
          snprintf_P(buffer, 21, PSTR("                       "));
          lcd.setCursor(0, 2); lcd.print(buffer);
          line++;
          break;
        case 3:
          snprintf_P(buffer,21,PSTR("%-20s"), pCR->fileDateTime);
          lcd.setCursor(0, 3); lcd.print(buffer);
          line = -1;  //
          break;
        default:
          lcd.clear();
          line = -1;
      }
      break;
    
    default:
      screen = 0;
      break;
  }
  
  displayTimeUs = micros() - startUs;
}


