// Simple server that sends a response when connected

#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "os_type.h"
#include "user_interface.h"
#include "gpio.h"
#include "driver/uart.h"

// Wifi details
char ur_ssid[] = "ur_ssid";
char ur_ssid_password[] = "ur_wifi_password";

// Function that will run after initialization is done, contains information to connect to the WIFI and SNTP settings
LOCAL ICACHE_FLASH_ATTR void initDone()
{
    gpio_init();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
    gpio_output_set(0,0,0,BIT4); // Set GPIO4 as input
    wifi_set_opmode_current(STATION_MODE);
    struct station_config stationConfig;
    strncpy(stationConfig.ssid, ur_ssid, 32);
    strncpy(stationConfig.password, ur_ssid_password, 64);
    wifi_station_set_config(&stationConfig);
    wifi_station_connect();
}


// callback when a connection happens
LOCAL ICACHE_FLASH_ATTR void connectCB(void *arg)
{
    struct espconn *pNewEspConn = (struct espconn *)arg; // Type casting the argument to espconn structure type. This structure is 		unique for each connection
    os_printf("Received and accepted a connection from IP address: %d. %d. %d. %d  from port %d \n\r",
              IP2STR(&(pNewEspConn->proto.tcp->remote_ip)), pNewEspConn->proto.tcp->remote_port);
}

//callback when data is recieved
LOCAL ICACHE_FLASH_ATTR void recieveCB(void *arg,char *pData,unsigned short len)
{  struct espconn *pNewEspConn = (struct espconn *)arg;
   os_printf("Received data!! - length = %d\n\r", len);
   os_printf("from IP address: %d. %d. %d. %d \n\r", IP2STR(&(pNewEspConn->proto.tcp->remote_ip)));
   int i=0;
   for (i=0; i<len; i++) { // Read data by each character and print it on the debug screen
        os_printf("%c", pData[i]);
   }
   os_printf("\n");
   uint32 stove_on_or_off = GPIO_INPUT_GET(4); // read the status of GPIO 4
     // send a response back to remote server
   char send_data_buffer[200];
   char status[10];
   if (stove_on_or_off == 1)
	os_sprintf(status, "ON");
   else if(stove_on_or_off == 0)
	os_sprintf(status, "OFF");
   else
	os_sprintf(status, "UNKNOWN");
    os_sprintf(send_data_buffer,"<!DOCTYPE html> <html> <body><h1><center>Honey..Did I turn off the stove?</center></h1> <center>Stove status is <font color=\"red\">%s!</font> </center><center>-from ESP 8266</center></body></html>", status);
  os_printf("Sending : %s\n", send_data_buffer);
    espconn_send(pNewEspConn, send_data_buffer, sizeof(send_data_buffer)); // send the data to the espconn structure with a buffer pointer and the 		size of the buffer.
espconn_disconnect(pNewEspConn); // disconnect after sending data.
}

//callback when connection is disconnected
LOCAL ICACHE_FLASH_ATTR void disconnectCB(void *arg)
{
    os_printf("Connection was disconnected.\n\r");
}

//callback when data is sent
LOCAL ICACHE_FLASH_ATTR void sentCB(void *arg)
{
    os_printf("Response sent was successful\n\r");
}

struct espconn conn; // creates a espconn structure
esp_tcp tcp1; // creates a pointer of type esp_tcp
// This is the function where tcp server is started
LOCAL ICACHE_FLASH_ATTR void tcpserver_init()
{

    tcp1.local_port = 9800; // This is where the server wil listen to
    conn.type = ESPCONN_TCP; // what kind of transport protocol? tcp or UDP
    conn.state = ESPCONN_NONE;// Set it to this state
    conn.proto.tcp = &tcp1; // pointer to tcp instance
    espconn_regist_connectcb(&conn, connectCB); // register a callback when a connection is fornmed
    espconn_accept(&conn);// accept incoming connections
    espconn_regist_recvcb(&conn,recieveCB); // register a callback when one recieves data from remote connection
    espconn_regist_sentcb(&conn, sentCB); // register a callback to send data to a remote connection
    espconn_regist_disconcb(&conn, disconnectCB); // register a callback when a remote connection is disconnected
}

// This the wifi event handler
void ICACHE_FLASH_ATTR eventHandler(System_Event_t *evt)
{
    switch(evt->event)
    {
    case EVENT_STAMODE_CONNECTED:
        os_printf("Event: EVENT_STAMODE_CONNECTED\n");
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
        os_printf("Status: %s\n", possible_status[wifi_station_get_connect_status()]);
        break;
    case EVENT_STAMODE_DISCONNECTED:
        os_printf("Event: EVENT_STAMODE_DISCONNECTED\n");
        break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
        os_printf("Event: EVENT_STAMODE_AUTHMODE_CHANGE\n");
        break;
    case EVENT_STAMODE_GOT_IP:
        os_printf("Event: EVENT_STAMODE_GOT_IP\n");
        struct ip_info info ;
        wifi_get_ip_info(STATION_IF, &info);
	tcpserver_init();
	break;
    case EVENT_SOFTAPMODE_STACONNECTED:
        os_printf("Event: EVENT_SOFTAPMODE_STACONNECTED\n");
        break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
        os_printf("Event: EVENT_SOFTAPMODE_STADISCONNECTED\n");
        break;
    default:
        os_printf("Unexpected event: %d\n", evt->event);
        break;
    }
}

//Init function. This is where the program enters
void  user_init()
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200); // set the baud rate for UART0 and UART1, I will use UART1 for debugging and UART0 for flashing
    os_printf("Hello !\n");
    os_printf("Chip Id: %lu\n", system_get_chip_id()); //Prints chip ID
    os_printf("SDK Version: %s\n", system_get_sdk_version()); // Gets the sdk version
    system_init_done_cb(initDone);
   wifi_set_event_handler_cb(eventHandler);
}
