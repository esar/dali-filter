#include <avr/io.h>
#include <avr/interrupt.h>

#define GET_BIT(value, bit)    ((value) & (1 << (bit)))
#define SET_BIT(value, bit)    ((value) |= 1 << (bit))
#define CLR_BIT(value, bit)    ((value) &= ~(1 << (bit)))
#define BIT(bit)               (1 << (bit))


volatile unsigned long g_data;
volatile unsigned char g_dataLength;

volatile unsigned long g_transmitData;
volatile unsigned char g_transmitBitsLeft;
volatile unsigned long g_transmitClock;

volatile unsigned long g_lastData = 0;


void transmitTimerStart()
{
	TCNT0L = 0;
	TCNT0H = 0;
	SET_BIT(TIFR, OCF0A);
	SET_BIT(TIMSK, OCIE0A);
}

void transmitTimerStop()
{
	CLR_BIT(TIMSK, OCIE0A);
}

void transmit(unsigned long value, unsigned char length)
{
	g_transmitBitsLeft = 0;
	g_transmitData = 0;
	g_transmitClock = 0;

	while(length--)
	{
		g_transmitData <<= 1;
		g_transmitData |= value & 1;
		value >>= 1;
		g_transmitBitsLeft += 1;
	}

	//g_transmitData <<= 1;
	//g_transmitData |= 1;
	//g_transmitBitsLeft += 1;

	transmitTimerStart();
}

ISR(TIMER0_COMPA_vect)
{
	TCNT0L = 0;
	TCNT0H = 0;

	if(!g_transmitBitsLeft)
	{
		CLR_BIT(PORTB, PORTB3);
		transmitTimerStop();
		return;
	}

	++g_transmitClock;

	if((g_transmitData & 1) ^ (g_transmitClock & 1))
		CLR_BIT(PORTB, PORTB3);
	else
		SET_BIT(PORTB, PORTB3);

	if((g_transmitClock & 1) == 0)
	{
		g_transmitData >>= 1;
		g_transmitBitsLeft -= 1;
	}
}

char receiveTimerIsRunning()
{
	return GET_BIT(TIMSK, TOIE1);
}

void receiveTimerStart()
{
	TCNT1 = 0;
	TC1H = 0;
	SET_BIT(TIFR, TOV1);
	SET_BIT(TIMSK, TOIE1);
}

void receiveTimerStop()
{
	CLR_BIT(TIMSK, TOIE1);
	CLR_BIT(TIMSK, OCIE1A);
}

void receiveTimerClear()
{
	TCNT1 = 0;
	TC1H = 0;
}

char receiveTimerHasPassedHalfBitTime()
{
	return TCNT1 > 75;
}

ISR(TIMER1_COMPA_vect)
{
}

ISR(TIMER1_OVF_vect)
{
	receiveTimerStop();
	if(g_data != g_lastData)
	{
		g_lastData = g_data;
		transmit(g_data, g_dataLength);
	}
}

ISR(INT0_vect)
{
	if(!receiveTimerIsRunning())
	{
		g_data = 0;
		g_dataLength = 0;
		receiveTimerStart();
		return;
	}

	if(g_dataLength > 0 && !receiveTimerHasPassedHalfBitTime())
		return;
	receiveTimerClear();

	g_data <<= 1;
	g_data |= GET_BIT(PINB, PINB6) ? 1 : 0;
	g_dataLength += 1;
}


int main()
{
	SET_BIT(PORTA, PA7);
	SET_BIT(DDRA, DDA7);
	SET_BIT(PORTA, PA1);
	SET_BIT(DDRA, DDA1);

	SET_BIT(PORTB, PB3);
	SET_BIT(DDRB, DDB3);

	// Set timer 0 to clk/64
	SET_BIT(TCCR0B, CS00);
	SET_BIT(TCCR0B, CS01);
	// Set timer0 compare A to 25 ticks
	OCR0A = 25;

	// Set timer 1 to clk/32
	TCCR1B = BIT(CS11) | BIT(CS12);
	// Set timer1 compare A to 600uS
	//OCR1A = 75;
	// Set timer1 compare C to 1600uS
	OCR1C = 200;

	SET_BIT(GIMSK, INT0);
	SET_BIT(MCUCR, ISC00);
	sei();


	for(;;)
		;
}
