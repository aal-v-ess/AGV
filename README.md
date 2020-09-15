# AGV

Project of a Automated Guided Vehicle made for a college subject.
It consisted on creating the structure (car) from scratch choosing all the components. The components chosen were ATMEGA88, L293D, DC engines, LM7805, batteries, TCRT5000.


In main() is where all the relevant function for AGV operation is called. In our case we have the init() function where the Timers, Ports and ADC are initialized, and already inside the while cycle, the convert() and moverAGV() functions.


The convert() function aims to start the conversion of the analog signal coming from the sensors at the ADC inputs, for this it is necessary to set the ADSC register (Start Conversion) to 1. Only when the conversion is complete, ADSC returns to zero and enters on the respective interruption (ADC_vect).


When the ADC is interrupted (ADC_vect), the ADMUX register is checked, where it is possible to change the input to be read (MUX), for this the switch-case cycle is made in which, depending on the input, the value read is stored in the respective position in the vector s[MAX] and triggering the next MUX to read all inputs. Except in the last case, where the initial MUX is assigned.


The values read by the ADC will be used in the moverAGV() function. This is where the values obtained stored in the vector s[MAX] are compared with a reference value using if conditions. As the ADC reading is only 8 bits, since it is not necessary to use the 10 bits as the conversion precision is not a fundamental part of our project, our maximum reference value would be 255. However, this would mean that the sensors would be providing a 5V potential difference to the ADC input. In our case the sensors have a maximum value of 3.5V which would mean a reference value of 179, but to ensure that all sensors have a good reading and that the black line is always detected, we lowered the reference value to 128 which it is precisely 2.5V. Conditions include: the detection of a straight line, left and right curves, end of line, loop and intersections. If the AGV gets lost or leaves the course, it will execute the movement in order to find the black line again through the last action it performed before getting lost. In addition to the movement conditions, we also have the function called Pulse() and the condition that checks if an object is in the AGV's path and, if this happens, it stops until the object is removed.


The ultrasonic sensor needs to provide a pulse in the order of 10 microseconds on the Trigger pin so that it can emit 8 pulses of 40kHz to be later received on the Echo. The Pulse () function has the objective of sending the pulse, the Trigger pin is 1 and after only 10 microseconds the pin goes to zero.


For the operation of the ultrasonic sensor, the INT0 interrupt has the function of capturing the time interval in which the Echo is at 1, for which TIMER2 is dedicated in Normal Operation that will count it. From a short time it is possible to calculate the distance at which the sensor obstacle encounters using the formula: Distance (cm) = Time interval / 58.


Interruption of Timer1, through comparison by the value of OCR1A, is intended to change the logical value of the pin where the LED is connected.


