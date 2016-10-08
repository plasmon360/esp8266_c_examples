// Simple server that sends a response when connected

#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "os_type.h"
#include "user_interface.h"
#include "driver/uart.h"

// Function that will run after initialization is done, contains information to connect to the WIFI
LOCAL void initDone()
{
    wifi_set_opmode_current(STATION_MODE);
    struct station_config stationConfig;
    strncpy(stationConfig.ssid, "ur_ssid", 32);
    strncpy(stationConfig.password, "ur_pwd", 64);
    wifi_station_set_config(&stationConfig);
    wifi_station_connect();
}

// callback when a connection happens
LOCAL void connectCB(void *arg)
{
    struct espconn *pNewEspConn = (struct espconn *)arg; // Type casting the argument to espconn structure type. This structure is 		unique for each connection
    os_printf("Received and accepted a connection from IP address: %d. %d. %d. %d  from port %d \n\r",
              IP2STR(&(pNewEspConn->proto.tcp->remote_ip)), pNewEspConn->proto.tcp->remote_port);
}

//callback when data is recieved
LOCAL void recieveCB(void *arg,char *pData,unsigned short len)
{
    struct espconn *pNewEspConn = (struct espconn *)arg;
    os_printf("Received data!! - length = %d\n\r", len);
    os_printf("from IP address: %d. %d. %d. %d \n\r", IP2STR(&(pNewEspConn->proto.tcp->remote_ip)));
    int i=0;
    for (i=0; i<len; i++) { // Read data by each character and print it on the debug screen
        os_printf("%c", pData[i]);
    }
    os_printf("\n");

    // send a response back to remote server
    char send_data[] ="<!DOCTYPE html> <html> <body><h1>Hello! </h1> <p>How are you? </p><p>-from ESP 8266</p></body></html>"; // make a data butter
    espconn_send(pNewEspConn, &send_data[0], sizeof(send_data)); // send the data to the espconn structure with a buffer pointer and the 		size of the buffer.
}

//callback when data is sent
LOCAL void sentCB(void *arg)
{
    os_printf("Response sent was successful\n\r");
}

struct espconn conn; // creates a espconn structure
esp_tcp tcp1; // creates a pointer of type esp_tcp
// This is the function where tcp server is started
LOCAL void tcpserver_init()
{
    tcp1.local_port = 100; // This is where the server wil listen to
    conn.type = ESPCONN_TCP; // what kind of transport protocol? tcp or UDP
    conn.state = ESPCONN_NONE;// Set it to this state
    conn.proto.tcp = &tcp1; // pointer to tcp instance
    espconn_regist_connectcb(&conn, connectCB); // register a callback when a connection is fornmed
    espconn_accept(&conn);// accept incoming connections
    espconn_regist_recvcb(&conn,recieveCB); // register a callback when one recieves data from remote connection
    espconn_regist_sentcb(&conn, sentCB); // register a callback to send data to a remote connection
}

// This the wifi event handler
LOCAL void eventHandler(System_Event_t *evt)
{
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
        os_printf("Event: EVENT_STAMODE_
        break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
        os_printf("Event: EVENT_STAMODE_AUTHMODE_CHANGE");
        break;
    case EVENT_STAMODE_GOT_IP:
        os_printf("Event: EVENT_STAMODE_GOT_IP");
        struct ip_info info ;
        wifi_get_ip_info(STATION_IF, &info);
        tcpserver_init(); // Start the tcp server
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
    uart_init(BIT_RATE_115200, BIT_RATE_115200); // set the baud rate for UART0 and UART1, I will use UART1 for debugging and UART0 for flashing
    os_printf("Hello !\n\r");
    os_printf("Chip Id: %lu\n\r", system_get_chip_id()); //Prints chip ID
    os_printf("SDK Version: %s\n\r", system_get_sdk_version()); // Gets the sdk version
    system_init_done_cb(initDone);
    wifi_set_event_handler_cb(eventHandler);
}
