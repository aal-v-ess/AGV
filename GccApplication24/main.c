#define F_CPU 1000000UL	// Define micro controller's frequency at 1 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define OCR_ESQ_F OCR0A=150;	//Sets OCR value for TIMER0A, controls left motors speed for foward motion
#define	OCR_DIR_F OCR0B=150;	//Sets OCR value for TIMER0B, controls right motors speed for foward motion
#define	OCR_ESQ_CE OCR0A=200;	//Sets OCR value for TIMER0A, controls left motors speed for left curve
#define	OCR_ESQ_CD OCR0A=180;	//Sets OCR value for TIMER0A, controls left motors speed for right curve
#define	OCR_DIR_CE OCR0B=180;	//Sets OCR value for TIMER0B, controls right motors speed for left curve
#define	OCR_DIR_CD OCR0B=200;	//Sets OCR value for TIMER0B, controls right motors speed for right curve
#define Trigger PORTB7	//Sets PINB7 OF PORTB as sonar's trigger pin
#define Echo PORTB6	//Sets PINB6 OF PORTB as sonar's echo pin
#define MAX 4		//Size of the array which contains the ADC values
#define th 128		//Reference value for ADC (black/white separation)

int s[MAX]={0,0,0,0};	//Initialization of the array holding the ADC values
unsigned char sentido_esq;	//Global variables
unsigned char sentido_dir;
volatile uint8_t button_flag = 0;
unsigned int resultado=0;
unsigned char c;
void init(void);	//External functions initialization
void moverAGV();
void Pulso();


//Input/Output definition
void init(void){
	DDRB |= (1 << PINB0) | (1<<PINB3) | (1<<PINB1) | (1<<PINB7);	//pin B0 (LED) / pin B1 (L293D) / pin B7 (Trigger)
	DDRD |= (1 << PIND6) | (1 << PIND0) | (1 << PIND1) | (1<<PIND5) | (1<<PIND3) | (1<<PIND4) | (1<<PIND7);
	DDRD&=~(1<<PIND2);	// Pin INT0 as input
	//PORTC Pin's which control the rotation of the motors
	PORTD|=(1<<PIND3);
	PORTB|=(1<<PINB1);

	//ADC
	ADMUX |=(1 << REFS0) | (1 << ADLAR); //External tension with capacitor (aligned left)
	ADCSRA |= (1<< ADEN) | (1 << ADPS1) | (1 << ADPS0) |  (1 << ADIE);

	//Motor Timer's
	TCCR0A |= (1<<COM0A1) | (1<<WGM00) | (1<<COM0B1);	//Phase Correct
	TCCR0B|=(1<<CS01);	//Prescaler
	OCR0A = 0;	//PWM control (motor speed)
	OCR0B = 0;
	
	//Led blinking at 1Hz
	TCCR1A|=0;  //Timer1 16 bits, which means a maximum number ((2^16) -1)=65535 // CTC
	TCCR1B|= (1 << WGM12) | (1 << CS12) | (1 << CS10); //Prescaler 1024/ timer startup
	TIMSK1=(1 << OCIE1A); // enable interruption
	OCR1A=487; //487
	
	//INT0 interruption
	EICRA|=(1<<ISC00);
	EIMSK|=(1<<INT0);
	EIFR|=(1<<INTF0);
	

	//SREG |= 0x80;
	sei();  //enable interruption
}

ISR(TIMER1_COMPA_vect){
	PORTB ^= (1 << PINB0);	//toggle pin logical level
}

ISR(ADC_vect)	//line following sensors
{
	switch (ADMUX)
	{
		case (0x60) :	//ADC selection
		s[0]=ADCH;	//ADC read and value storage
		ADMUX = 0x61;	//ADC switch
		break;
		
		case (0x61):
		s[1]=ADCH;
		ADMUX = 0x62;
		break;
		
		case (0x62):
		s[2]=ADCH;
		ADMUX = 0x63;
		break;
		
		case (0x63):
		s[3]=ADCH;
		ADMUX = 0x64;
		break;
		
		case (0x64):
		s[4]=ADCH;
		ADMUX = 0x60;
		break;
		
	}
	ADCSRA|= (1<<ADSC);	//value conversion initiates
	
}

ISR(INT0_vect){
	_delay_ms(60);
	uint16_t Ultra();
	
}

uint16_t Ultra() {

	uint16_t i,result;			//local variables
	
	//Echo
	for(i=0;i<600000;i++)		//Test to determine when echo's logical level is 1
	{
		if(!(PIND&(1<<PD2)))	//If logical value equals 0, continues processing
		continue;
		else					//If equals 1 exits for loop
		break;
	}
	//Timer2
	TCCR2A|=0x00;				//Normal mode operation
	TCCR2B|=(1<<CS20);			//Without Prescaler (=1)
	TCNT2|=0x00;				//Counter initialization

	for(i=0;i<600000;i++)		//Test to determine when echo's logical level is 0
	{
		if(PIND&(1<<PD2))
		{
			if(TCNT2>60000)		//Once superior limit is hit, loop breaks
			break;
			else				//Else loop continues
			continue;
		}

		else
		break;
	}
	result=TCNT2;				//Stores value in local variable (result)
	//Stop Timer
	TCCR2B=0x00;				//Reset
	//Convetr to CM
	//result/=58;					//Convertion to centimeters
	return (result>>1);
	

}

void Pulso(){
	
	// 		PORTB&=~(1<<PB7);
	PORTB|=(1<<PB7);	//Sends pulse
	_delay_us(10);	//Waits 10 us
	PORTB&=~(1<<PB7);	//receives pulse
	
}

int converter(void){
	
	ADCSRA|=(1<<ADSC); //Conversion begins
	while((ADCSRA & (1<<ADSC)) != 0);	//Waits until conversion is finished
	return (s[MAX]);
}

void moverAGV(void){

	char c=0;	//local variable which aids in movement control once black line is lost
	
	Pulso();
	if(Ultra() <10) //1160 = 20 x 58 para 20 CM
	PORTD|=(1<<PIND0);
	else

	PORTD&=~(1<<PIND0);
	

	if(s[0]>th && s[1]<th && s[2]>th && s[3]<th) //Forward
	{
		c=1;
		PORTD|=(1<<PIND3); PORTD &= ~(1<<PIND4);	//Left side motors rotate clockwise
		PORTB|=(1<<PINB1); PORTD &= ~(1<<PIND7);	//Right side motors rotate clockwise
		OCR_ESQ_F;	//Dictates left motor's speed
		OCR_DIR_F;	//Dictates right motor's speed
	}

	else if(s[0]<th && s[1]>th && s[2]>th && s[3]<th) //Right curve
	{
		c=2;
		do	//curves until front sensor detects black
		{
			PORTD|=(1<<PIND3); PORTD &= ~(1<<PIND4);	//Left side motors rotate clockwise
			PORTD|=(1<<PIND7); PORTB &= ~(1<<PINB1);	//Right side motors rotate anti-clockwise
			OCR_ESQ_CD;	//Sets left motors speed for right curve
			OCR_DIR_CD;	//Sets right motors speed for right curve
			
		}	 while (s[0]<th && s[1]>th);
		//Sets both motors rotation direction to clockwise once the curve is finished
		PORTD|=(1<<PIND3); PORTD &= ~(1<<PIND4);
		PORTB|=(1<<PINB1); PORTD &= ~(1<<PIND7);
	}
	
	else if(s[0]<th && s[1]<th && s[2]>th && s[3]>th) //Left curve
	{
		c=3;
		do	//curves until front sensor detects black
		{
			PORTB|=(1<<PINB1); PORTD &= ~(1<<PIND7);	//Left side motors rotate anti-clockwise
			PORTD|=(1<<PIND4); PORTD &= ~(1<<PIND3);	//Right side motors rotate clockwise
			OCR_ESQ_CE;	//Sets left motors speed for left curve
			OCR_DIR_CE;	//Sets right motors speed for left curve
		}	while (s[0]<th);
		//Sets both motors rotation direction to clockwise once the curve is finished
		PORTD|=(1<<PIND3); PORTD &= ~(1<<PINB4);
		PORTB|=(1<<PINB1); PORTD &= ~(1<<PIND7);
	}
	
	else if(s[0]<th && s[1]<th && s[2]<th && s[3]<th)	//Black line lost
	{
		//If black line is lost the AGV will continue to do the same it was doing before hence the reason for the existence of variable 'c'
		//This variable saves the previous position
		switch(c){
			case 1:
			PORTD|=(1<<PIND4); PORTD &= ~(1<<PIND3);
			PORTD|=(1<<PIND7); PORTD &= ~(1<<PINB1);
			OCR_ESQ_F;
			OCR_ESQ_F;
			break;
			
			case 2:
			do
			{
				PORTD|=(1<<PIND3); PORTD &= ~(1<<PIND4);
				PORTD|=(1<<PIND7); PORTB &= ~(1<<PINB1);
				OCR0A=100;
				OCR0B=120;

			}	 while (s[0]<th);
			
			PORTD|=(1<<PIND3); PORTD &= ~(1<<PIND4);
			PORTB|=(1<<PINB1); PORTD &= ~(1<<PIND7);
			break;
			
			case 3:
			do
			{
				PORTB|=(1<<PINB1); PORTD &= ~(1<<PIND7);
				PORTD|=(1<<PIND4); PORTD &= ~(1<<PIND3);
				OCR_ESQ_CE;
				OCR_DIR_CE;
				
			}	while (s[0]<th);
			PORTD|=(1<<PIND3); PORTD &= ~(1<<PINB4);
			PORTB|=(1<<PINB1); PORTD &= ~(1<<PIND7);
			break;
			
		}
	}
}



int main(void)
{
	init();	//I/O, Timer and ADMUX initializations
	while(1){
		//do{
		//PORTD |= (1<<PIND1);
		converter();	//Converts sensors analogic values to digital ones
		moverAGV();	//Controls the AGV's movement according to the values the sensors read
		//}while(button_flag==1);
		// 			do{
		// 				PORTD&=~(1<<PIND3); PORTD &= ~(1<<PIND4);
		// 				PORTB&=~(1<<PINB1); PORTD &= ~(1<<PIND7);
		// 				PORTD &= ~(1<<PIND1);
		// 				button_flag=0;
		// 				}while(button_flag==2);

		//

		

	}
}
