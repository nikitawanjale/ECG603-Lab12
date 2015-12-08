/* 
Lab12_T03 Continously display the temperature of the device (internal temperature sensor) on the
hyperterminal or the serialchart.
*/

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"  	// definitions to use ADC driver

void UARTIntHandler();
char* ftos(float);
int UARTPrint_uint32_t(uint32_t);
void UARTdeleteLastEntry(int);

int main(void) {

	char *label = "Temperature: ";
	int i;
	int written = 0;

	uint32_t ui32ADC0Value[4]; 			// ADC FIFO
	volatile uint32_t ui32TempAvg; 		// Store average
	volatile uint32_t ui32TempValueC; 	// Temp in C
	uint32_t ui32TempValueF; 	// Temp in F

	SysCtlClockSet(
			SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
					| SYSCTL_XTAL_16MHZ);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Set up UART
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //enable GPIO port for LED
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2); //enable pin for LED PF2

	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	/*
	 IntMasterEnable(); //enable processor interrupts
	 IntEnable(INT_UART0); //enable the UART interrupt
	 UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts
	 */
	// Set up ADC module
	SysCtlPeripheralEnable( SYSCTL_PERIPH_ADC0); // enable the ADC0 peripheral
	ADCHardwareOversampleConfigure( ADC0_BASE, 64);	// hardware averaging (64 samples)

	// Configure ADC0 sequencer to use sample sequencer 2, and have the processor trigger the sequence.
	ADCSequenceConfigure( ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
	// Configure each step.
	ADCSequenceStepConfigure( ADC0_BASE, 2, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure( ADC0_BASE, 2, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure( ADC0_BASE, 2, 2, ADC_CTL_TS);
	// Sample temp sensor.
	ADCSequenceStepConfigure( ADC0_BASE, 2, 3,
			ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);
	ADCSequenceEnable( ADC0_BASE, 2); // Enable ADC sequencer 2

	UARTCharPut(UART0_BASE, '\n');
	UARTCharPut(UART0_BASE, '\r');
	for (i = 0; label[i] != '\0'; i++) 	// Print prompt at start of program
		UARTCharPut(UART0_BASE, label[i]);
	UARTCharPut(UART0_BASE, ' ');

	while (1) //let interrupt handler do the UART echo function
	{
		ADCProcessorTrigger(ADC0_BASE, 2); 	// Trigger ADC conversion.

		while (!ADCIntStatus(ADC0_BASE, 2, false))
			; 	// wait for conversion to complete.

		ADCSequenceDataGet(ADC0_BASE, 2, ui32ADC0Value); // get converted data.
		// Average read values, and round.
		// Each Value in the array is the result of the mean of 64 samples.
		ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2]
				+ ui32ADC0Value[3] + 2) / 4;
		ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096) / 10; // calc temp in C
		ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;

		UARTdeleteLastEntry(written);
		written = UARTPrint_uint32_t(ui32TempValueF);
		UARTCharPut(UART0_BASE, 'F');
		written++;

		// LED logic
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); //blink LED
		SysCtlDelay(SysCtlClockGet() / (1000 * 3)); //delay ~1 msec
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //turn off LED
		SysCtlDelay(SysCtlClockGet());
	}
}

void UARTdeleteLastEntry(int length) {
	int i;

	for (i = 0; i < length; i++) {
		UARTCharPut(UART0_BASE, '\b');
		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, '\b');
	}
}

int UARTPrint_uint32_t(uint32_t value)
{
	// unexpected crash at variable assignments. could not fix.
	int i = 0; // iterator
	int written = 0;
	uint32_t temp = value;
	char buffer[100];

	if (value == 0) {
		UARTCharPut(UART0_BASE, '0');
		return 1;
	}

	// Convert to string
	while (temp != 0) // count the number of digits
	{
		i++;
		temp /= 10;
	}
	buffer[i] = '\0';
	i--;
	for (; i >= 0; i--) // convert digits to chars, and store in buffer
			{
		buffer[i] = value % 10 + '0';
		value /= 10;
	}
	while (buffer[i] != '\0') {
		UARTCharPut(UART0_BASE, buffer[i]);
		written++;
	}
	return written;
}
/*
 {
 char buffer[100];
 uint32_t temp;
 uint32_t i;
 uint32_t written;

 temp = value;
 i = 0;
 written = 0;
 if (value == 0){
 UARTCharPut(UART0_BASE, '0');
 return 1;
 }

 // Convert to string
 while(temp != 0) // count the number of digits
 {
 i++;
 temp /= 10;
 }
 buffer[i] = '\0';
 i--;
 for( i; i >= 0; i--) // convert digits to chars, and store in buffer
 {
 buffer[i] = value % 10 + '0';
 value /= 10;
 }
 //UARTCharPut(UART0_BASE, sign);
 //for(i = 0; i < sizeof(buffer); i++)  // Loop to print out data string
 while (buffer[i] != '\0')
 {
 //if (buffer[i] == '\0') break;
 UARTCharPut(UART0_BASE, buffer[i]);
 written++;
 }
 return written;
 // UARTCharPut(UART0_BASE, '\n');UARTCharPut(UART0_BASE, '\r');
 }
 */

void UARTIntHandler() {
	uint32_t ui32Status;
	uint32_t ui32ADC0Value[4]; 			// ADC FIFO
	volatile uint32_t ui32TempAvg; 		// Store average
	volatile uint32_t ui32TempValueC; 	// Temp in C
	volatile uint32_t ui32TempValueF; 	// Temp in F

	ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status

	UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts

	ADCIntClear(ADC0_BASE, 2); 			// Clear ADC0 interrupt flag.
	ADCProcessorTrigger(ADC0_BASE, 2); 	// Trigger ADC conversion.

	while (!ADCIntStatus(ADC0_BASE, 2, false))
		; 	// wait for conversion to complete.

	ADCSequenceDataGet(ADC0_BASE, 2, ui32ADC0Value); 	// get converted data.
	// Average read values, and round.
	// Each Value in the array is the result of the mean of 64 samples.
	ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2]
			+ ui32ADC0Value[3] + 2) / 4;
	ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096) / 10; // calc temp in C
	ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;

	//while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
	//{
	//  UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE)); //echo character
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); //blink LED
	SysCtlDelay(SysCtlClockGet() / (1000 * 3)); //delay ~1 msec
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //turn off LED
	//}
}
