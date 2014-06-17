//	|visualizer.c|: Implementation of the Sound Visualizer
//	@author: Joe Gibson
//	@author: Adam Luckenbaugh

//An example using Timer 0 Subtimer A to trigger the ADC Sequence 0
// on ADC CH0. Both Timer 0 and ADC are interrupt driven.

//Includes
#include "utils/ustdlib.h"

#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include "driverlib/timer.h"
#include "inc/lm3s6965.h"

#include "driverlib/adc.h"
#include "driverlib/gpio.h"

#include "drivers/rit128x96x4.h"

//ADC Value
unsigned long Value;
volatile unsigned ValueUpdated;

//Interrupt handler
void ADCIntHandler()
{
    //Clear the ADC interrupt flag
    ADCIntClear(ADC0_BASE, 3);

    ADCSequenceDataGet(ADC0_BASE, 3, &Value);
    ValueUpdated = 1;
}

void Timer0IntHandler()
{
	//Clear Timer 0 interrupt flag
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	//Trigger ADC Sequence 0
	ADCProcessorTrigger(ADC0_BASE, 3);
}

int main(void)
{
	char str[32];
	
	//Set system clock using PLL and a 1/10 divider to get a 20MHz clock from
	// the 8MHz crystal
	SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);

	//Initialize the OLED Display
	RIT128x96x4Init(1000000);
	
	//Print Test String
	RIT128x96x4StringDraw("TEST STRING!", 20, 40, 15);

	//Setup Timer 0 Subtimer A
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	//Set the Timer 0 Load value
	TimerLoadSet(TIMER0_BASE, TIMER_A, (SysCtlClockGet()/16000) - 1);

	//Enable Timer 0 interrups
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	//Setup the ADC on CH0, Sequence 3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	//Disable Sequence 3 for configuration
	ADCSequenceDisable(ADC0_BASE, 3);

	//Configure ADC Sequence 3 for CH0, Interrupt Enable, Last (only) step in sequence
	ADCSequenceConfigure(ADC_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);

	//Enable the Sequence
	ADCSequenceEnable(ADC0_BASE, 3);

	//Enable interrupts for ADC Sequence 3
	ADCIntEnable(ADC0_BASE, 3);

	//Enable Timer 0 Subtimer A
	TimerEnable(TIMER0_BASE, TIMER_A);

	//Enable Interrupts for Timer 0 and ADC Sequence 3
	IntEnable(INT_TIMER0A);
	IntEnable(INT_ADC0SS3);

	//Enable all interrupts
	IntMasterEnable();

	//Clear ADC interrupt
	ADCIntClear(ADC0_BASE, 3);

	//Loop...
	while(1)
	{
		if(ValueUpdated != 0)
		{
			ValueUpdated = 0;	//reset the flag marking if the value was just read
			//Print the value
			usprintf(str, "ADC = %4d", Value);
			RIT128x96x4StringDraw(str, 0, 0, 15);
		}
	}
}