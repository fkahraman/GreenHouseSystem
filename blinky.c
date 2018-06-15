#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*

HAVA KALITE SENSORU = E1
TOPRAN NEM SENSÖRÜ = D3

FAN PINI = C5
KETTLE PINI = C4

HMI RX = B0
HMI TX = B1

ARDUINO RX = D6
ARDUINO TX = D7

*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/


/*			ADC KANAL AKTIVASYONU			*/

#define SENSOR_ADC_BASE           		ADC0_BASE
#define SENSOR_ADC_SYSCTL_PERIPH  		SYSCTL_PERIPH_ADC0

/*			MQ135 HAVA KALITE SENSÖRÜ KONFIGÜRASYONU			*/

#define MQ135_PORT_SYSTEM_PERIPH		SYSCTL_PERIPH_GPIOE
#define	MQ135_PORT						GPIO_PORTE_BASE
#define	MQ135_PIN						GPIO_PIN_1
#define	MQ135_ADC_CH					ADC_CTL_CH2

/*			TOPRAK NEM SENSÖRÜ KONFIGÜRASYONU			*/

#define EARTH_HUMIDITY_PORT_SYSTEM_PERIPH		SYSCTL_PERIPH_GPIOD
#define	EARTH_HUMIDITY_PORT						GPIO_PORTD_BASE
#define	EARTH_HUMIDITY_PIN						GPIO_PIN_3
#define	EARTH_HUMIDITY_ADC_CH					ADC_CTL_CH4

/*			FAN ICIN ROLE KONFIGURASYONU			*/

#define	FAN_PORT_SYSTEM_PERIPH			SYSCTL_PERIPH_GPIOC
#define	FAN_PORT						GPIO_PORTC_BASE
#define FAN_PIN							GPIO_PIN_4

/*			KETTLE ICIN ROLE KONFIGURASYONU			*/

#define ISITICI_PORT_SYSTEM_PERIPH		SYSCTL_PERIPH_GPIOC
#define ISITICI_PORT					GPIO_PORTC_BASE
#define ISITICI_PIN						GPIO_PIN_5

/*			HABERLESME PINLERI VE BASELERI			*/

#define HMI_BASE						UART1_BASE
#define USB_BASE						UART0_BASE

/*			L298N KONFIGURASYONU			*/

#define L298N_PORT_SYSTEM_PERIPH		SYSCTL_PERIPH_GPIOA
#define L298N_PORT						GPIO_PORTA_BASE
#define L298N_PIN_1						GPIO_PIN_5
#define L298N_PIN_2						GPIO_PIN_6

#define More							true
#define Less							false

/*			UART VE INT ILE ILGILI TANIMLAMALAR			*/

char rxDataUSB[32];
char rxDataHMI[32];
char rxDataUNO[32];

int rxIndexUSB = 0;
int rxIndexHMI = 0;
int rxIndexUNO = 0;

bool rxFlagUSB = false;
bool rxFlagHMI = false;
bool rxFlagUNO = false;

bool FlagTemp = false;
bool FlagHum = false;
bool FlagEarth = false;
bool FlagQua = false;

bool FlagTempG = false;
bool FlagHumG = false;
bool FlagEarthG = false;
bool FlagQuaG = false;

/*			PANEL TANIMLAMALARI			*/

uint8_t temp = 27;
uint8_t hum = 75;
uint8_t earth = 5;
uint8_t qua = 30;

uint8_t tempMin;
uint8_t tempMax;
uint8_t humMin;
uint8_t humMax;
uint8_t earthMin;
uint8_t earthMax;

/*			FANUS TANIMLAMALARI			*/

uint8_t hum_ui;
uint8_t temp_ui;
uint32_t earth_ui;
uint32_t qua_ui;

char temp_str[32];
char hum_str[32];
char earth_str[32];
char qua_str[32];

char Mq135_str[32];
char Toprak_str[32];
char humidity[32];
char temperature[32];

bool counter = 0;

uint32_t sysClk ;

char data_zero;
char data_one;

bool isitici;
bool fan;

char temp_hmi[32];
char hum_hmi[32];

char *isiticiState;
char *fanState;

bool FlagT;
bool FlagH;
bool FlagE;
bool FlagQ;

bool TempState;
bool HumState;
bool EarthState;

uint32_t boudrate = 9600;

uint32_t gasDegeri;
uint32_t toprakDegeri;

/*			USB ICIN UART KESMESI			*/
void UARTIntHandlerUSB(void)
{
	char ch;	
	uint32_t ui32Status;

	ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
	UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts

	ch = UARTCharGet(UART0_BASE);
	
	if(ch != '\r')		// 0x0D 
	{
		rxDataUSB[rxIndexUSB++] = ch;		
	}
	else
	{	
		
		rxDataUSB[rxIndexUSB] = '\0';
		rxIndexUSB = 0;
		rxFlagUSB = true;
	}
}

/*			HMI ICIN UART KESMESI			*/
void UARTIntHandlerHMI(void)
{
	uint32_t ui32Status;
	char rcv_ch;
	
	ui32Status = UARTIntStatus(UART1_BASE, true);
	UARTIntClear(UART1_BASE, ui32Status);
	
	while (UARTCharsAvail(UART1_BASE))
	{
	rcv_ch=UARTCharGetNonBlocking(UART1_BASE);	
		
	if (rcv_ch != '\0')	
	{
		rxDataHMI[rxIndexHMI] = rcv_ch;
		
		if(rxDataHMI[rxIndexHMI-1] == '#' && rcv_ch == '#')
		{
			
			rxDataHMI[rxIndexHMI+1] = '\0';
			
			if( strstr(rxDataHMI, "#Temp##") != NULL)
			{
				FlagTempG = true;
				FlagHumG = false;
				FlagEarthG = false;
				FlagQuaG = false;
				
			}
			
			else if( strstr(rxDataHMI, "#Hum##") != NULL)
			{
				FlagTempG = false;
				FlagHumG = true;
				FlagEarthG = false;
				FlagQuaG = false;
			}
			
			else if( strstr(rxDataHMI, "#Earth##") != NULL)
			{
				FlagTempG = false;
				FlagHumG = false;
				FlagEarthG = true;
				FlagQuaG = false;
			}
			
			else if( strstr(rxDataHMI, "#Qua##") != NULL)
			{
				FlagTempG = false;
				FlagHumG = false;
				FlagEarthG = false;
				FlagQuaG = true;
			}
			
			else if( strstr(rxDataHMI, "@@Stop##") != NULL)
			{
				boudrate = 0;
			}
			
			else if( strstr(rxDataHMI, "@@Start##") != NULL)
			{
				boudrate = 9600;
			}
			
			else
			{
				if(FlagHumG == true)
				{
					data_zero = rxDataHMI[rxIndexHMI-3];
					data_one = rxDataHMI[rxIndexHMI-2];
					
					FlagHum = true;
					FlagHumG = false;
				}
					
				else if(FlagTempG == true)
				{
					data_zero = rxDataHMI[rxIndexHMI-3];
					data_one = rxDataHMI[rxIndexHMI-2];
					
					FlagTemp = true;
					FlagTempG = false;
				}
					
				else if(FlagQuaG == true)
				{
					data_zero = rxDataHMI[rxIndexHMI-3];
					data_one = rxDataHMI[rxIndexHMI-2];
					
					FlagQua = true;
					FlagQuaG = false;
				}
					
				else if(FlagEarthG == true)
				{
					data_zero = rxDataHMI[rxIndexHMI-3];
					data_one = rxDataHMI[rxIndexHMI-2];
					
					FlagEarth = true;
					FlagEarthG = false;
				}
				
			}
				rxIndexHMI=0;
				//return;
		}
		rxIndexHMI++;
		if(rxIndexHMI>=100)
			rxIndexHMI=0;
		}
	}
}
/*			ARDUINO-TIVA ICIN VERI KESMESI			*/
void UARTIntHandlerUNO(void)
{
	
	char ch;
	uint32_t ui32Status;
	//int i;

	ui32Status = UARTIntStatus(UART2_BASE, true); 
	UARTIntClear(UART2_BASE, ui32Status); 

	ch = UARTCharGet(UART2_BASE);
	
	if(ch != '\r')		// 0x0D 
	{
		rxDataUNO[rxIndexUNO++] = ch;
	}
	else
	{	
		
		if(counter == 0)
		{
			for(int i=0; i < rxIndexUNO; i++)
			{
				humidity[i] = rxDataUNO[i];
				
			}

		}
		
		
		
		if(counter == 1)
		{
			for(int i=0; i < rxIndexUNO; i++)
			{
				temperature[i] = rxDataUNO[i];
			}	
		}
			
		
		if(counter == 1)
			counter = 0;
		
		else
			counter = 1;
		
		rxDataUNO[rxIndexUNO] = '\0';
		rxIndexUNO = 0;
		rxFlagUNO = true;

	}

}

/*			FAN ISITICI VE L298N KONFIGÜRASYONU			*/
void RELAY_SETUP(void)
{
	SysCtlPeripheralEnable(FAN_PORT_SYSTEM_PERIPH);
	SysCtlPeripheralEnable(ISITICI_PORT_SYSTEM_PERIPH);
	SysCtlPeripheralEnable(L298N_PORT_SYSTEM_PERIPH);
	
	GPIOPinTypeGPIOOutput(FAN_PORT, FAN_PIN);
	GPIOPinTypeGPIOOutput(ISITICI_PORT, ISITICI_PIN);
	
	GPIOPinTypeGPIOOutput(L298N_PORT, L298N_PIN_1);
	GPIOPinTypeGPIOOutput(L298N_PORT, L298N_PIN_2);
	
	GPIOPinWrite(L298N_PORT, L298N_PIN_1, 0x00);
}

/*			MILISANIYE CINSINDEN BEKLEME SÜRESI			*/
void delayMS(unsigned int ms)
{
	SysCtlDelay(SysCtlClockGet()/3000 *ms);
}

/*			MIKROSANIYE CINSINDEN BEKLEME SÜRESI			*/
void delayUS(unsigned int us)
{
	SysCtlDelay(SysCtlClockGet()/3000000 *us);
}

/*			Numerik Panelden Gelen Verinin Islenmesi			*/
int NumericPanel(char dataOne, char dataTwo)
{
	if(dataOne == '0')
	{
		if(dataTwo == '1')
			return 1;
		
		else if(dataTwo == '2')
			return 2;
		
		else if(dataTwo == '3')
			return 3;
		
		else if(dataTwo == '4')
			return 4;
		
		else if(dataTwo == '5')
			return 5;
		
		else if(dataTwo == '6')
			return 6;
		
		else if(dataTwo == '7')
			return 7;
		
		else if(dataTwo == '8')
			return 8;
		
		else if(dataTwo == '9')
			return 9;
		
		else
			return 0;
	}
	
	else if(dataOne == '1')
	{
		if(dataTwo == '1')
			return 11;
		
		else if(dataTwo == '2')
			return 12;
		
		else if(dataTwo == '3')
			return 13;
		
		else if(dataTwo == '4')
			return 14;
		
		else if(dataTwo == '5')
			return 15;
		
		else if(dataTwo == '6')
			return 16;
		
		else if(dataTwo == '7')
			return 17;
		
		else if(dataTwo == '8')
			return 18;
		
		else if(dataTwo == '9')
			return 19;
		
		else
			return 10;
	}
	
	else if(dataOne == '2')
	{
		if(dataTwo == '1')
			return 21;
		
		else if(dataTwo == '2')
			return 22;
		
		else if(dataTwo == '3')
			return 23;
		
		else if(dataTwo == '4')
			return 24;
		
		else if(dataTwo == '5')
			return 25;
		
		else if(dataTwo == '6')
			return 26;
		
		else if(dataTwo == '7')
			return 27;
		
		else if(dataTwo == '8')
			return 28;
		
		else if(dataTwo == '9')
			return 29;
		
		else
			return 20;
	}
	
	else if(dataOne == '3')
	{
		if(dataTwo == '1')
			return 31;
		
		else if(dataTwo == '2')
			return 32;
		
		else if(dataTwo == '3')
			return 33;
		
		else if(dataTwo == '4')
			return 34;
		
		else if(dataTwo == '5')
			return 35;
		
		else if(dataTwo == '6')
			return 36;
		
		else if(dataTwo == '7')
			return 37;
		
		else if(dataTwo == '8')
			return 38;
		
		else if(dataTwo == '9')
			return 39;
		
		else
			return 30;
	}
	
	else if(dataOne == '4')
	{
		if(dataTwo == '1')
			return 41;
		
		else if(dataTwo == '2')
			return 42;
		
		else if(dataTwo == '3')
			return 43;
		
		else if(dataTwo == '4')
			return 44;
		
		else if(dataTwo == '5')
			return 45;
		
		else if(dataTwo == '6')
			return 46;
		
		else if(dataTwo == '7')
			return 47;
		
		else if(dataTwo == '8')
			return 48;
		
		else if(dataTwo == '9')
			return 49;
		
		else
			return 40;
	}
	
	else if(dataOne == '5')
	{
		if(dataTwo == '1')
			return 51;
		
		else if(dataTwo == '2')
			return 52;
		
		else if(dataTwo == '3')
			return 53;
		
		else if(dataTwo == '4')
			return 54;
		
		else if(dataTwo == '5')
			return 55;
		
		else if(dataTwo == '6')
			return 56;
		
		else if(dataTwo == '7')
			return 57;
		
		else if(dataTwo == '8')
			return 58;
		
		else if(dataTwo == '9')
			return 59;
		
		else
			return 50;
	}
	
	else if(dataOne == '6')
	{
		if(dataTwo == '1')
			return 61;
		
		else if(dataTwo == '2')
			return 62;
		
		else if(dataTwo == '3')
			return 63;
		
		else if(dataTwo == '4')
			return 64;
		
		else if(dataTwo == '5')
			return 65;
		
		else if(dataTwo == '6')
			return 66;
		
		else if(dataTwo == '7')
			return 67;
		
		else if(dataTwo == '8')
			return 68;
		
		else if(dataTwo == '9')
			return 69;
		
		else
			return 60;
	}
	
	else if(dataOne == '7')
	{
		if(dataTwo == '1')
			return 71;
		
		else if(dataTwo == '2')
			return 72;
		
		else if(dataTwo == '3')
			return 73;
		
		else if(dataTwo == '4')
			return 74;
		
		else if(dataTwo == '5')
			return 75;
		
		else if(dataTwo == '6')
			return 76;
		
		else if(dataTwo == '7')
			return 77;
		
		else if(dataTwo == '8')
			return 78;
		
		else if(dataTwo == '9')
			return 79;
		
		else
			return 70;
	}
	
	else if(dataOne == '8')
	{
		if(dataTwo == '1')
			return 81;
		
		else if(dataTwo == '2')
			return 82;
		
		else if(dataTwo == '3')
			return 83;
		
		else if(dataTwo == '4')
			return 84;
		
		else if(dataTwo == '5')
			return 85;
		
		else if(dataTwo == '6')
			return 86;
		
		else if(dataTwo == '7')
			return 87;
		
		else if(dataTwo == '8')
			return 88;
		
		else if(dataTwo == '9')
			return 89;
		
		else
			return 80;
	}
	
	else if(dataOne == '9')
	{
		
		if(dataTwo == '1')
			return 91;
		
		else if(dataTwo == '2')
			return 92;
		
		else if(dataTwo == '3')
			return 93;
		
		else if(dataTwo == '4')
			return 94;
		
		else if(dataTwo == '5')
			return 95;
		
		else if(dataTwo == '6')
			return 96;
		
		else if(dataTwo == '7')
			return 97;
		
		else if(dataTwo == '8')
			return 98;
		
		else if(dataTwo == '9')
			return 99;
		
		else
			return 90;
	}
	
	else
	{
		if(dataTwo == '1')
			return 1;
		
		else if(dataTwo == '2')
			return 2;
		
		else if(dataTwo == '3')
			return 3;
		
		else if(dataTwo == '4')
			return 4;
		
		else if(dataTwo == '5')
			return 5;
		
		else if(dataTwo == '6')
			return 6;
		
		else if(dataTwo == '7')
			return 7;
		
		else if(dataTwo == '8')
			return 8;
		
		else if(dataTwo == '9')
			return 9;
		
		else if(dataTwo == '0')
			return 0;
		
		else
		{
			if(FlagTemp == true)
				return temp;
			
			else if(FlagHum == true)
				return hum;
			
			else if(FlagEarth == true)
				return earth;
			
			else 
				return qua;
		}
	}
}
/*			FAN IÇIN AÇMA KAPAMA FONKSIYONU			*/
bool FAN_SET(bool state)
{	
	SysCtlPeripheralEnable(FAN_PORT_SYSTEM_PERIPH);
	
	if(state == true)
		GPIOPinWrite(FAN_PORT, FAN_PIN, 0x00);
	
	else
		GPIOPinWrite(FAN_PORT, FAN_PIN, FAN_PIN);
	
	return state;
}

/*			KETTILE IÇIN AÇMA KAPAMA FONKSIYONU			*/
bool ISITICI_SET(bool state)
{	
	SysCtlPeripheralEnable(ISITICI_PORT_SYSTEM_PERIPH);
	
	if(state == true)
		GPIOPinWrite(ISITICI_PORT, ISITICI_PIN, 0x00);
	
	else
		GPIOPinWrite(ISITICI_PORT, ISITICI_PIN, ISITICI_PIN);
	
	return state;
}

/*			GAZ SENSORU ICIN KONFIGURASYON			*/
void MQ135_CONF_SETUP(void)
{	
	//fifo 1
    SysCtlPeripheralEnable(MQ135_PORT_SYSTEM_PERIPH);
    SysCtlPeripheralEnable(SENSOR_ADC_SYSCTL_PERIPH);
    GPIOPinTypeADC(MQ135_PORT, MQ135_PIN);	
	
	ADCSequenceConfigure(SENSOR_ADC_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(SENSOR_ADC_BASE, 1, 0, ADC_CTL_IE | ADC_CTL_END | MQ135_ADC_CH);
	ADCSequenceEnable(SENSOR_ADC_BASE, 1);
}
/*			TOPRAK SENSORU ICIN KONFIGURASYON			*/
void ToprakSensoru_Conf_Setup(void)
{	
	//fifo 2
	SysCtlPeripheralEnable(EARTH_HUMIDITY_PORT_SYSTEM_PERIPH);
    SysCtlPeripheralEnable(SENSOR_ADC_SYSCTL_PERIPH);
    GPIOPinTypeADC(EARTH_HUMIDITY_PORT, EARTH_HUMIDITY_PIN);	
	
	ADCSequenceConfigure(SENSOR_ADC_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(SENSOR_ADC_BASE, 2, 0, ADC_CTL_IE | ADC_CTL_END | EARTH_HUMIDITY_ADC_CH);
	ADCSequenceEnable(SENSOR_ADC_BASE, 2);
}
/*			TOPRAK SENSORU ICIN ANALOG OKUMA FONKSIYONU			*/
uint32_t AnalogReadEarth(void)
{
	uint32_t analogValue = 0;
	
	ADCProcessorTrigger(SENSOR_ADC_BASE, 2);	
	
	while(!ADCIntStatus(SENSOR_ADC_BASE, 2, false));
	
	ADCSequenceDataGet(SENSOR_ADC_BASE, 2, &analogValue);
	
	return analogValue;
}
/*			GAZ SENSORU ICIN ANALOG OKUMA			*/
uint32_t AnalogReadGas(void)
{
	uint32_t analogValue = 0;
	
	ADCProcessorTrigger(SENSOR_ADC_BASE, 1);	
	
	while(!ADCIntStatus(SENSOR_ADC_BASE, 1, false));
	
	ADCSequenceDataGet(SENSOR_ADC_BASE, 1, &analogValue);
	
	return analogValue;
}

/*			UART KANALLAR ICIN MESAJ ILETIM FONKSIYONU			*/
void SendUartString(uint32_t nBase, char *nData)
{
	while(*nData!='\0')
	{
		if(UARTSpaceAvail(nBase))
		{
			while(!UARTCharPutNonBlocking(nBase, *nData));
			nData++;
		}
	}
}


/*			USB UART KONFIGURASYONU			*/
void USB_UART_CONFIGURE_SETUP(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

/*			USB ICIN UART KESMESI KONFIGURASYONU			*/
void USB_UART_INT_CONFIGURE_SETUP(void)
{	
	//UARTEnable(UART1_BASE);
	IntMasterEnable(); //enable processor interrupts
	IntEnable(INT_UART0); //enable the UART interrupt
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts
	
}
/*			HAVA KALITE FILTRE			*/
uint32_t QualityFilter(uint32_t value)
{
	uint32_t valueG;
	valueG = 3800 - value;
	valueG = valueG / 15;
	
	if(value > 3800)
		return 0;
	
	else if(value < 2300)
		return 100;
	
	return (valueG);
}
/*			TOPRAK NEM FILTRE			*/
uint32_t EarthHumFilter(uint32_t valueE)
{
	uint32_t valueEG;
	valueEG = 3900 - valueE;
	valueEG = valueEG / 20;
	
	if (valueE > 3900)
		return 1;
	
	else if(valueE < 1900)
		return 100;
	
	return valueEG;
	
	
}
/*			MOTOR SURUCU			*/
void MOTOR_SET(bool state)
{	
	if(state == true)
		GPIOPinWrite(L298N_PORT, L298N_PIN_2, L298N_PIN_2);
	
	else
		GPIOPinWrite(L298N_PORT, L298N_PIN_2, 0x00);
}

/*			HMI ICIN UART KONFIGURASYON			*/
void HMI_UART_CONFIGURE_SETUP(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	
	GPIOPinConfigure(GPIO_PB0_U1RX);
	GPIOPinConfigure(GPIO_PB1_U1TX);
	GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	
	UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

/*			HMI ICIN UART KESMESI KONFIGURASYONU			*/
void HMI_UART_INT_CONFIGURE_SETUP(void)
{	
	UARTEnable(UART1_BASE);
	IntMasterEnable(); //enable processor interrupts
	IntEnable(INT_UART1); //enable the UART interrupt
	UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts
	
}
/*			ARDUINO-TIVA UART KONFIGURASYON			*/
void UNO_TIVA_UART_CONFIGURE_SETUP(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	
	GPIOPinConfigure(GPIO_PD6_U2RX);
	GPIOPinConfigure(GPIO_PD7_U2TX);
	GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
	
	UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), boudrate, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}
/*			ARDUINO-TIVA UART KESME KONFIGURASYONU			*/
void UNO_TIVA_INT_CONFIGURE_SETUP(void)
{
	UARTEnable(UART2_BASE);
	IntMasterEnable(); //enable processor interrupts
	IntEnable(INT_UART2); //enable the UART interrupt
	UARTIntEnable(UART2_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts
}
/*			ANA FONKSIYON			*/
int main(void) 
{	

	
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // 80 MHz *max speed
	
	sysClk = SysCtlClockGet();
	
	USB_UART_CONFIGURE_SETUP();
	USB_UART_INT_CONFIGURE_SETUP();
	
	UNO_TIVA_UART_CONFIGURE_SETUP();
	UNO_TIVA_INT_CONFIGURE_SETUP();
	
	HMI_UART_CONFIGURE_SETUP();
	HMI_UART_INT_CONFIGURE_SETUP();
	
	ToprakSensoru_Conf_Setup();	
	MQ135_CONF_SETUP();
	
	RELAY_SETUP();
	
	while (1) 
	{	
		tempMin = temp - 2;
		tempMax = temp + 2;
		
		humMin = hum - 7;
		humMax = hum + 7;
		
		earthMin = earth - 9;
		earthMax = earth + 9;
		
		gasDegeri = AnalogReadGas();
		toprakDegeri = AnalogReadEarth();
		
		// sprintf(STR, "%d", gasDegeri);
		
		// SendUartString(UART0_BASE, STR);
		// SendUartString(UART0_BASE, "\n");
		
		qua_ui = QualityFilter(gasDegeri);
		earth_ui = EarthHumFilter(toprakDegeri);
		
		
		sprintf(Mq135_str, "%% %d", qua_ui);	
		sprintf(Toprak_str, "%% %d", earth_ui);
		
		
		//ekran verileri
		
		SendUartString(HMI_BASE, "t0.txt=\"");
		SendUartString(HMI_BASE, Mq135_str);
		SendUartString(HMI_BASE, "\"ÿÿÿ");
		
		if(isitici == false)
			isiticiState = "KAPALI";
		
		else
			isiticiState = "ACIK";
		
		SendUartString(HMI_BASE, "t1.txt=\"");
		SendUartString(HMI_BASE, isiticiState);
		SendUartString(HMI_BASE, "\"ÿÿÿ");
		
		SendUartString(HMI_BASE, "t2.txt=\"");
		SendUartString(HMI_BASE, temp_hmi);
		SendUartString(HMI_BASE, "\"ÿÿÿ");	
		
		SendUartString(HMI_BASE, "t3.txt=\"");
		SendUartString(HMI_BASE, hum_hmi);
		SendUartString(HMI_BASE, "\"ÿÿÿ");
		
		if(fan == false)
			fanState = "KAPALI";
		
		else
			fanState = "ACIK";
		
		SendUartString(HMI_BASE, "t4.txt=\"");
		SendUartString(HMI_BASE, fanState);
		SendUartString(HMI_BASE, "\"ÿÿÿ");
		
		SendUartString(HMI_BASE, "t5.txt=\"");
		SendUartString(HMI_BASE, Toprak_str);
		SendUartString(HMI_BASE, "\"ÿÿÿ");			
		
		//numeric panel islemesi
		
		if(FlagTemp == true)
		{
			temp = NumericPanel(data_zero, data_one);
			FlagTemp = false;
		}
	
		if(FlagHum == true)
		{
			hum = NumericPanel(data_zero, data_one);
			FlagHum = false;
		}
		
		if(FlagEarth == true)
		{
			earth = NumericPanel(data_zero, data_one);
			FlagEarth = false;
		}
		
		if(FlagQua == true)
		{
			qua = NumericPanel(data_zero, data_one);
			FlagQua = false;
		}
		
		//panel verileri
		
		sprintf(temp_str, "%d °C", temp);
		sprintf(hum_str, "%% %d", hum);
		sprintf(earth_str,"%% %d", earth);
		sprintf(qua_str, "%% %d", qua);
		
		SendUartString(HMI_BASE, "b2.txt=\"");
		SendUartString(HMI_BASE, temp_str);
		SendUartString(HMI_BASE, "\"ÿÿÿ");
		
		SendUartString(HMI_BASE, "b3.txt=\"");
		SendUartString(HMI_BASE, hum_str);
		SendUartString(HMI_BASE, "\"ÿÿÿ");
		
		SendUartString(HMI_BASE, "b4.txt=\"");
		SendUartString(HMI_BASE, earth_str);
		SendUartString(HMI_BASE, "\"ÿÿÿ");
		
		SendUartString(HMI_BASE, "b5.txt=\"");
		SendUartString(HMI_BASE, qua_str);
		SendUartString(HMI_BASE, "\"ÿÿÿ");				
				

		if(rxFlagUNO == true)
		{
			if (counter == 0)
			{
				hum_ui = NumericPanel(humidity[1], humidity[2]);
				
				sprintf(hum_hmi, "%% %d", hum_ui);
				
					SendUartString(UART0_BASE, rxDataUNO);
					SendUartString(UART0_BASE, "\n");
				
				rxFlagUNO = false;
			}
			
			else
			{
				temp_ui = NumericPanel(temperature[1], temperature[2]);
				
				sprintf(temp_hmi, "%d °C", temp_ui);
				
					SendUartString(UART0_BASE, rxDataUNO);
					SendUartString(UART0_BASE, "\n");
				
				rxFlagUNO = false;
			}
		}
		
		
		if((tempMin <= temp_ui) && (temp_ui <= tempMax))
			FlagT = true;
		
		else
		{
			FlagT = false;
			
			if (temp_ui < temp)
				TempState = Less;
			
			else
				TempState = More;
		}
		
		if((humMin <= hum_ui) && (hum_ui <= humMax))
			FlagH = true;
		
		else
		{
			FlagH = false;
			
			if (hum_ui < hum)
				HumState = Less;
			
			else
				HumState = More;
		}
		
		if((earthMin <= earth_ui) && (earth_ui <= earthMax))
			FlagE = true;
		
		else
		{
			FlagE = false;
			
			if (earth_ui < earth)
				EarthState = Less;
			
			else
				EarthState = More;
		}
		
		if(qua_ui > qua)
			FlagQ = true;
		
		else
			FlagQ = false;
		
				

		
		if((FlagT == false) && (FlagH == false))
		{
			if((TempState == Less) && (HumState == Less))
			{
				isitici = ISITICI_SET(true);
				fan = FAN_SET(false);
			}
			
			else if((TempState == More) && (HumState == Less))
			{
				isitici = ISITICI_SET(false);
				fan = FAN_SET(true);
			}
			
			else if((TempState == Less) && (HumState == More))
			{
				isitici = ISITICI_SET(true);
				fan = FAN_SET(true);
			}
			
			else if((TempState == Less) && (HumState == More))
			{
				isitici = ISITICI_SET(true);
				fan = FAN_SET(true);
			}
		}
		
		else if((FlagT == false) && (FlagH == true))
		{
			if(TempState == Less)
			{
				isitici = ISITICI_SET(true);
				fan = FAN_SET(true);
			}
			
			else
			{
				isitici = ISITICI_SET(false);
				fan = FAN_SET(true);
			}
		}
		
		
		else if((FlagT == true) && (FlagH == false))
		{
			if(HumState == Less)
			{
				isitici = ISITICI_SET(true);
				fan = FAN_SET(true);
			}
			
			else
			{
				isitici = ISITICI_SET(false);
				fan = FAN_SET(true);
			}
		}	
		
		
		else if((FlagT == true) && (FlagH == true))
		{
			isitici = ISITICI_SET(false);
			fan = FAN_SET(false);
		}
		
		if(FlagE == true)
		{
			if(EarthState == Less)
				MOTOR_SET(true);
			
			else
				MOTOR_SET(false);
		}
		
		//MOTOR_SET(true);
		if(FlagQ == false)
			fan = FAN_SET(true);
			
			/*
			fan = FAN_SET(true);
			isitici = ISITICI_SET(false);
			MOTOR_SET(false);
			*/
	
	}		
}
