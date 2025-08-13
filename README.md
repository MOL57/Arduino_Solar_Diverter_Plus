# Arduino_Solar_Diverter_Plus
## A load manager to optimize solar power utilization in a domestic photovoltaic installation

### solarDiverterPlus V3 - 2025.08.12 - by Xavier Fernandez Molinet - xavierfmol@gmail.com

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

Hardware schematic in file: Scheme Solar-Diverter-Plus-v02-2025.07.16.pdf

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
