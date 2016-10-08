// Get Time from a sntp server every 5 sec and print it. Daylights saving time is taken into consideration.

#include "osapi.h"
#include "ip_addr.h"
#include "os_type.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "driver/sntp.h"
#include "driver/time_functions.h" // Change time_functions.c for your time zone and dst_offset_hours

// Wifi details
char ur_ssid[] = "ur_ssid";
char ur_ssid_password[] = "ur_wifi_password";

//Timer
os_timer_t mytimer; // Structure for a timer
LOCAL void initDone()
{
    wifi_set_opmode_current(STATION_MODE);
    struct station_config stationConfig;
    strncpy(stationConfig.ssid, ur_ssid, 32);
    strncpy(stationConfig.password, ur_ssid_password, 64);
    wifi_station_set_config(&stationConfig);
    wifi_station_connect();
    my_sntp_init(); // This will setup the sntp and adjust it for timezone and daylightoffset
    
}


//Set up a timer
void timer()
{
    os_timer_disarm(&mytimer); // disarm any timer
    os_timer_setfn(&mytimer, (os_timer_func_t *)sntp_current_time, NULL);	// associate a callback for a timer
    os_timer_arm(&mytimer, 5000, 1); // arm the timer. every 5sec, the callback function will be called.
}

// This the wifi event handler
void eventHandler(System_Event_t *evt)
{
    switch(evt->event)
    {
    case EVENT_STAMODE_CONNECTED:
        os_printf("Event: EVENT_STAMODE_CONNECTED");
        os_printf("connect to ssid %s, channel %d\n",
                  evt->event_info.connected.ssid,
                  evt->event_info.connected.channel);
        const char* possible_status[] =
        {
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
        timer(); 	// Once we get an IP, start the timer
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
//Init function. This is where the program enters
void ICACHE_FLASH_ATTR user_init()
{
    uart_init(BIT_RATE_115200,BIT_RATE_115200); // set the baud rate for UART0 and UART1, I will use UART1 for debugging and UART0 for flashing
    os_printf("Hello !\n\r");
    os_printf("Chip Id: %lu\n\r", system_get_chip_id()); //Prints chip ID
    os_printf("SDK Version: %s\n\r", system_get_sdk_version()); // Gets the sdk version
    system_init_done_cb(initDone);
    wifi_set_event_handler_cb(eventHandler);
}
