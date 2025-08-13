/*
===========================
solarDiverterPlus V3
2025.08.12
by Xavier Fernandez Molinet
xavierfmol@gmail.com
===========================

Solar Diverter Plus is a project to optimize a domestic photovoltaic installation, 
which measures: 
- the instantaneous solar power generation
- the instantaneous power consumption of the home appliances
then computes:
- the solar power excedent between generation and consumption
- the power margin between the maximum allowable consumption and the actual consumption
and consequently switches on or of certain (possibly remotely managed) critical loads in order to
- consume the most solar power excedent
- consume the least imported mains power
- avoid exceeding the maximum allowable power consumption (which might trip the mains supply circuit breakers)
A serial interface for simulation of electric variables is included (for testing without real electric circuit)
and to print values on demand

The hardware is composed by:
- An Arduino Mega 2560 board
- a liquid crystal display of 4 lines x 20 characters with I2C interface
- a push button to change the screen contents of the display 
- two current transformers to measure intensities of solar generation and home consumption
- one voltage transformer to measure mains voltage
- a voltage reference of 2.5 volts, acting as a floating ground for the measured AC grid voltage and currents
- several discrete components to atenuate the measured voltage and currents, in order to fit into the ADC input range
- one radio transmitter on the 433 Mhz band to actuate on remote switches to manage the loads
- several external radio-controlled switches to remotelly manage the loads
- some digital outputs with LEDs to indicate the status of the manage loads (and optially to manage them through wires instead of radio)
- some switches to select the control of the loads either as solar-automatic or as manual

Hardware schematic in file: Scheme Solar-Diverter-Plus-v02-2025.07.16

Adaptation to the remote switches used:
- Radio codes for On and Off of each switch (channel), and their PWM times and parameters, must be cloned and included in file radio.h
- Alternatively, loads can be managed in a wired fashion through their correspondig digital outputs, thus avoiding radio cloning any remote switches

Compatibility:
This software + hardware has been designed and tested under the following conditions:
- Plattform: Arduino Mega 2560
- Grid voltage: 230V through a voltage transformer of ratio 230V / 17V
- Grid frequency: 50Hz.
- Maximum solar power intensity: 25A through an intensity transformer of ratio: 100A / 50mA
- Maximum power consumption intensity: 25A through an intensity transformer of ratio: 100A / 50mA

To port this software to other processors, take into account:
- In measure.h, the ADC prescaler must be modified according to the particular ADC registers
  and the conversion time must be lower than 1/4 of the designed sampling interval
- The assignation of the digital and analog gpios must be revised according to the disponibility of the board
- RAM and FLASH memory size must be enough. In SW v3 we have: RAM usage: 2442 bytes, FLASH usage: 26778 bytes
- The schematic must be revised to cope with ADC input ranges other than 0 to 5V

To use different electrical parameters:
- Modify values of electrical components in the schematic, if required.
- Modify electrical constants in source files (main file and measure.h file)
*/

/*
Changes in v3:
- Added a change cause to the text printed when a load chages activation status
- Credits of the code (file name and compilation date and hour) are printed on start and on a third display screen
- Help text about simulation and printing commands is printed on demand by the order '?'

Changes in v2:
- Avoid priority inversion of loads, in Loads::decide()
- Changed decide period to 6 sec (to be 6 times the filtering time constant)
- Changed refresh period to 60 sec (to quickly restore status of the remote switches that were offline when a On command was launched)
*/

#include <Wire.h>               // I2C communications
#include <LiquidCrystal_I2C.h>  // LCD display via I2C
#include <avr/wdt.h>            // Watchdog

char buffer[300];               // shared buffer to assemble formatted text, only for immediate use in functions

#include "credits.h"            // source file name and compilation date-time
#include "countTime.h"          // time counting and timed launch of tasks
#include "radio.h"              // transmission of radio codes for remote switches activating loads
#include "simul.h"              // simulation of the analog inputs (AC currents, AC tension and DC reference), and processing received  print commands
#include "measure.h"            // sampling and storing the analog inputs (AC currents, AC tension and DC reference) during one grid cycle
#include "values.h"             // computing the electrical values from the stored analog input measures
#include "loads.h"              // managing the loads according to the powers from the computed electrical values
#include "display.h"            // managing the LCD display

// there is also the file "print.ino" containing auxiliary printing functions

// TIME SETTINGS

const int DECIDE_PERIOD_S = 6;        // time in seconds between successive load activation decisions, recommended 6 times the time constant of filtering powers in values.h
const int REFRESH_PERIOD_S = 60;      // time in seconds between successive load status refreshes
const int VAR_REFRESH_PERIOD_S = 5;   // màximum random variation (+ or -) of load refresh period
const int RANDOM_SEED_ANALOG_IN = A0; // analog input whose instantaneous value is used as a seed to initialize random values generation

// RADIO SETTINGS

const int RADIO_OUT = 8;              // output to PWM modulate the 433.92 MHz radio transmitting module

// MAINS CYCLE MEASURE SETTINGS

const int V0_IN = A0;                 // analog input for the reference voltage (and floating-ground input voltage)
const int VX_IN = A1;                 // analog input for the grid voltage (scaled-down)
const int IG_IN = A2;                 // analog input for the solar generated current (scaled-down and converted to voltage)
const int IC_IN = A3;                 // analog input for the consumed current (scaled-down and converted to voltage) 

// ELECTRICAL VALUES SETTINGS

const float VX_NOM_VEFF = 230.0;      // Nominal RMS voltage of the grid, for which the maximum amplitude MAX_AMPL_V (default 2V) is read at the corresponding analog input
const float IG_NOM_AEFF = 25.0;       // Nominal RMS intensity of the solar generated power, for which the maximum amplitude MAX_AMPL_V (default 2V) is read at the corresponding analog input
const float IC_NOM_AEFF = 25.0;       // Nominal RMS intensity of the consumed power, for which the maximum amplitude MAX_AMPL_V (default 2V) is read at the corresponding analog input
const float VX_CAL = 1.012;           // Calibration factor of the grid voltage (to be fine-tuned to cope with hardware components inaccuracies)
const float IG_CAL = 1.0;             // Calibration factor of the solar generated power (to be fine-tuned to cope with hardware components inaccuracies)
const float IC_CAL = 1.0;             // Calibration factor of the consumed power (to be fine-tuned to cope with hardware components inaccuracies)
const float MAX_CONSUMPTION = 4600.0; // Maximum allowed power consumption in watts (to prevent grid protections to trip)

// LOADS SETTINGS, do not excceed the max number of loads N_LOADS_MAX set in loads.h

// highest priority load
const char *LOAD0_NAME =          "Pisc";   // Name of the load to be displayed
const float LOAD0_POWER_W =       1500.0;   // Nominal power of the load
const int   LOAD0_LOCK_ON_SEC =   60.0;     // Waiting time after an activation to On of the load, until a deactivation to Off is permitted
const int   LOAD0_LOCK_OFF_SEC =  60.0;     // Waiting time after a deactivation to Off of the load, until an activation to On is permitted
const int   LOAD0_ON_OUT =        10;       // Digital out gpio to signal the activation status of the load, and optionally to manage the load in a wired fashion
const int   LOAD0_MODE_IN =       A7;       // Digital in gpio (here analog input is used as a digital input), to receive the load mode switch (manual or solar)
const Radio::RadioHW LOAD0_RADIO_MODEL = Radio::RADIO_GMOMXEN;  // Type of radio used by the remote switches, as defined in radio.h
const int   LOAD0_CHANNEL =       2;        // Radio channel to which the load remote switch is responding, as defined in radio.h

// lowest priority load
const char *LOAD1_NAME =          "Term";   // Name of the load to be displayed
const float LOAD1_POWER_W =       1000.0;   // Nominal power of the load
const int   LOAD1_LOCK_ON_SEC =   60.0;     // Waiting time after an activation to On of the load, until a deactivation to Off is permitted
const int   LOAD1_LOCK_OFF_SEC =  60.0;     // Waiting time after a deactivation to Off of the load, until an activation to On is permitted
const int   LOAD1_ON_OUT =        11;       // Digital out gpio to signal the activation status of the load, and optionally to manage the load in a wired fashion
const int   LOAD1_MODE_IN =       A6;       // Digital in gpio (here analog input is used as a digital input), to receive the load mode switch (manual or solar)
const Radio::RadioHW LOAD1_RADIO_MODEL = Radio::RADIO_GMOMXEN;  // Type of radio used by the remote switches, as defined in radio.h
const int   LOAD1_CHANNEL =       3;        // Radio channel to which the load remote switch is responding, as defined in radio.h

// DISPLAY CONSTANTS

const int BUTTON_IN = 12;  // Digital input gpio where the button to change screen is connected (connects to GND when pressed)

// GLOBAL OBJECTS

class Credits CR;     // compilation info
class CountTime CT;   // count time object
class Radio RD;       // radio object
class Simul SM;       // simulation object
class Measure CM;     // measure analog inputs object
class Values CV;      // compute electrical values object
class Loads LD;       // manage loads object
class Display DS;     // manage display object

//PROGRAM BODY

void setup() 
{
  wdt_disable();
  wdt_enable(WDTO_8S);                    // watchdog time set to 8 segons
  
  Serial.begin(115200);                   // serial port is inicialized
  Serial.println("\n\n\n\n\n\n\n\n\n");   // clears the terminal screen

  CR.begin(__FILE__, __DATE__, __TIME__); // info of source file compilation

  RD.begin(RADIO_OUT);                    // set-up of the radio object

  wdt_reset();                            // resets watchdog counter

  CT.begin( DECIDE_PERIOD_S, REFRESH_PERIOD_S, VAR_REFRESH_PERIOD_S, RANDOM_SEED_ANALOG_IN );   // starts time counting

  CM.begin(V0_IN,VX_IN,IG_IN,IC_IN);      // starts sampling the electric values during a grid cycle

  SM.begin(CM.numSamples);                // set-up of the simulation

  CV.begin( VX_CAL * VX_NOM_VEFF, IG_CAL * IG_NOM_AEFF, IC_CAL * IC_NOM_AEFF, MAX_CONSUMPTION );                                            // initiates computing of electric values 

  LD.add( LOAD0_NAME, LOAD0_POWER_W, LOAD0_LOCK_ON_SEC, LOAD0_LOCK_OFF_SEC, LOAD0_ON_OUT, LOAD0_MODE_IN, LOAD0_RADIO_MODEL, LOAD0_CHANNEL); // initializes the highest-priority load
  LD.add( LOAD1_NAME, LOAD1_POWER_W, LOAD1_LOCK_ON_SEC, LOAD1_LOCK_OFF_SEC, LOAD1_ON_OUT, LOAD1_MODE_IN, LOAD1_RADIO_MODEL, LOAD1_CHANNEL); // initializes the lowest-priority load

  wdt_reset();                            // resets watchdog counter

  DS.begin( BUTTON_IN );                  // set-up of the display

  wdt_reset();                            // resets watchdog counter
}


void loop() 
{
  wdt_reset();                            // resets watchdog counter
  //if(CT.minutes>1)  while(1);           // testing watchdog

  CT.update();                            // update time counting   
  SM.receiveValues();                     // receive optional serial commands for simulation and printing of values
  CM.getCycle( &SM );                     // samples electrical inputs during a grid cycle
  CV.compute( &SM, &CM );                 // computes the electrical magnitudes from the sampled values
  LD.decide( &CT, &CV );                  // decides whether activate or de-activate the loads
  LD.activate( &CT, &RD, &CV );           // executes the activation/de-activation of the loads
  DS.show( &CR, &CT, &CV, &SM, &LD );     // refreshes the display
  
  if(CT.flagOneSec) 
  {
    switch(SM.printCode)                  // if a print command character has been received, print the corresponding values to serial
    {
      case '1': printTimes();             // prints the elapsed time and the time consumed by each program task
                break;
      case '2': printOneCycle();          // prints the values of the analog inputs sampled during a grid cycle
                break;
      case '3': printValues();            // prints the electric magnitudes computed from the sampled values
                break;
      case '4': printFilteredValues();    // prints the powers after being filtered (for time-smoothing)
                break;
      case '0':
      case ' ':
      default:  break;
    }
  }
}


