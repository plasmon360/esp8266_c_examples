#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"


LOCAL void initDone() {
wifi_set_opmode_current(STATION_MODE);
struct station_config stationConfig;
strncpy(stationConfig.ssid, "ur_ssid", 32);
strncpy(stationConfig.password, "ur_password", 64);
wifi_station_set_config(&stationConfig);
wifi_station_connect();

} 

LOCAL void eventHandler(System_Event_t *evt) {
switch(evt->event) {
case EVENT_STAMODE_CONNECTED:
os_printf("Event: EVENT_STAMODE_CONNECTED");
            os_printf("connect to ssid %s, channel %d\n",
                        evt->event_info.connected.ssid,
                        evt->event_info.connected.channel);
const char* possible_status[] = {
"STATION_IDLE" ,
"STATION_CONNECTING",
"STATION_WRONG_PASSWORD",
"STATION_NO_AP_FOUND",
"STATION_CONNECT_FAIL",
"STATION_GOT_IP"
};
os_printf("Status: %s\n\r", possible_status[wifi_station_get_connect_status()]);

break;
case EVENT_STAMODE_DISCONNECTED:
os_printf("Event: EVENT_STAMODE_DISCONNECTED");

break;
case EVENT_STAMODE_AUTHMODE_CHANGE:
os_printf("Event: EVENT_STAMODE_AUTHMODE_CHANGE");
break;
case EVENT_STAMODE_GOT_IP:
os_printf("Event: EVENT_STAMODE_GOT_IP");
struct ip_info info ;
wifi_get_ip_info(STATION_IF, &info);
os_printf("Bala: IP address: %d, %d, %d, %d", IP2STR(&(info.ip.addr)));
/*
os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
IP2STR(&(evt->event_info.got_ip.ip)),
IP2STR(&evt->event_info.got_ip.mask),
IP2STR(&evt->event_info.got_ip.gw));
os_printf("\n");
*/
break;
case EVENT_SOFTAPMODE_STACONNECTED:
os_printf("Event: EVENT_SOFTAPMODE_STACONNECTED");
struct ip_info ipconfig;
wifi_get_ip_info(STATION_IF, &ipconfig);
break;
case EVENT_SOFTAPMODE_STADISCONNECTED:
os_printf("Event: EVENT_SOFTAPMODE_STADISCONNECTED");
break;
default:
os_printf("Unexpected event: %d\r\n", evt->event);
break;
}
}

LOCAL uint8_t led_state=0;


LOCAL void ICACHE_FLASH_ATTR blinky_cb(void *arg)
{
led_state = !led_state;
GPIO_OUTPUT_SET(4, led_state);
}
os_timer_t mytimer;
void blink_led()
{

gpio_init();
PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
gpio_output_set(0,BIT4,BIT4,0);

os_timer_disarm (&mytimer);
os_timer_setfn(&mytimer, (os_timer_func_t *) blinky_cb, NULL);
os_timer_arm (&mytimer, 1000, 1);
}



//Init function 
void ICACHE_FLASH_ATTR

user_init()
{
//uart_div_modify(0, UART_CLK_FREQ / 115200);
system_set_os_print(1);
uart_init(BIT_RATE_115200, BIT_RATE_115200);
os_printf("Hello !\n\r"); 
os_printf("Chip Id: %lu\n\r", system_get_chip_id());
os_printf("SDK Version: %s\n\r", system_get_sdk_version());
system_init_done_cb(initDone);
wifi_set_event_handler_cb(eventHandler);
}
