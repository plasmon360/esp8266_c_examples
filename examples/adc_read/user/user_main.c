#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "driver/uart.h"

/* Hygrometer reads 3.3 V when open and ~0 V when shorted. Built a potential divider with three 
resistor of equal value ( Higher resistance preferred like 1Mohm to avoid potential drop). Output of hygrometer is scaled to 1/3 or 1.1V by 
potential divider and fed to ADC of esp8266
*/


 

LOCAL uint16 voltage;

LOCAL void ICACHE_FLASH_ATTR adc_read(void *arg)
{
	uint16 raw_read = system_adc_read();
	os_printf("Raw read: %d\n\r",raw_read);	
	// ADC pins sees a maximum of 1.030 V when the hygrometer is open and reads 0.008 when saturated. These are measured using DVM
	float voltage = 0.008+raw_read*(1.030-0.008)/(1024-0);
	
	// Because we cannot print float numbers using osprintf. We have to do the following.
	uint16 voltage_int = (uint16) (voltage); // contains the integer part
	uint16 voltage_decimal;

	if (voltage_int>=1)
	voltage_decimal = (uint16) ((voltage-1)*1000); // calculate decimal part *1000
	else 
	voltage_decimal = (uint16) ((voltage)*1000);

	if(voltage_decimal >= 100)
		os_printf("%d.%dV \n\r", voltage_int, voltage_decimal );
	else if (10<=voltage_decimal<100)
		os_printf("%d.0%dV \n\r", voltage_int, voltage_decimal );
	else if (0<=voltage_decimal<10)
		os_printf("%d.00%dV \n\r", voltage_int, voltage_decimal );
}


os_timer_t mytimer;
void my_adc_read_every_1s()
{
os_timer_disarm (&mytimer); // Disarm the timee
os_timer_setfn(&mytimer, (os_timer_func_t *) adc_read, NULL); // set the callback fnction of the timer. the function will start when timer starts
os_timer_arm (&mytimer, 1000, 1); // Arm the timer. duration of timer will be 1000 microsec and it will keep repeating
}


//Init function 
void ICACHE_FLASH_ATTR

user_init()
{
 
uart_init(BIT_RATE_115200, BIT_RATE_115200); // Set the baud rate for two uarts
os_printf("Hello !\n\r"); 
os_printf("Chip Id: %lu\n\r", system_get_chip_id());
os_printf("SDK Version: %s\n\r", system_get_sdk_version());
my_adc_read_every_1s();
}
