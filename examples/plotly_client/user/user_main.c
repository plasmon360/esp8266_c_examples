/*Client sends data to plotly once in a while with a timestamp. Device will go to deep sleep between wakeups. 
Notes
Timing:
step 1) will init sntp module. This takes some time
step 2) after "sntp_init_time_sec" sec delay, will start getting sntp time and send data to plotly
step 3) after sending data and getting response, will sleep for "sleep_time_min" minutes and then wakeup
cycle continues
So total time for each cycle = "sleep_time_min"*60+sntp_init_time_sec sec
HARDWARE: MAKE SURE XPD is connected to RST, otherwise it wont wake up from deep sleep
*/

#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "os_type.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "driver/sntp.h"
#include "driver/time_functions.h" // Change ../driver/time_functions.c for your time zone and dst_offset_hours
#include "time.h"

// Server Details
char plotly_username[] = "balu";
char plotly_key[] = "xxxx";
char plotly_rest_api_home[] = "/clientresp";
char plotly_host[] = "plot.ly";

// Wifi details
char ur_ssid[] = "ur_ssid";
char ur_ssid_password[] = "ur_wifi_password";

//Other
os_timer_t mytimer; // Structure for a timer
ip_addr_t plotly_server_ip; // An ip address structure for server
struct espconn plotly_conn; // a connection strructure
esp_tcp tcp1; // creates a esp_tcp structure

// structure with curent time details from time_functions.c 
struct tm * current_time_with_dst;

// Sleep parameters
sint16 sleep_time_min = 1;
sint16 sntp_init_time_sec = 5;

// Function that will run after initialization is done, contains information to connect to the WIFI and SNTP settings
LOCAL ICACHE_FLASH_ATTR void initDone()
{
    wifi_set_opmode_current(STATION_MODE);
    struct station_config stationConfig;
    strncpy(stationConfig.ssid, ur_ssid, 32);
    strncpy(stationConfig.password, ur_ssid_password, 64);
    wifi_station_set_config(&stationConfig);
    wifi_station_connect();
    my_sntp_init(); // This will setup the sntp and it seems to take some time.
}


// callback when a connection happens
void ICACHE_FLASH_ATTR connectCB(void *arg)
{
    struct espconn *pNewEspConn = (struct espconn *)arg; // Type casting the argument to espconn structure type. This structure is 		unique for each connection
    os_printf("Made a connection\n");
    char send_data_buffer[2048];
    char text_data[500];
    char date_string[25] ;
    sint16 date_string_min;
    current_time_with_dst = sntp_current_time(0); // This is a function time_functions.h, returns a time.h tm structure
    os_sprintf(date_string, "%4d-%02d-%02d  %02d:%02d:%02d",
               1900+current_time_with_dst->tm_year, current_time_with_dst->tm_mon+1,
               current_time_with_dst->tm_mday,current_time_with_dst->tm_hour,
               current_time_with_dst->tm_min,current_time_with_dst->tm_sec );
	os_printf("Attempting to send %s, %d\n\r", date_string, current_time_with_dst->tm_min);
	os_sprintf(text_data,"un=%s&key=%s&origin=plot&platform=esp8266_c&args=[{\"x\":\"%s\", \"y\":[%d]}]&kwargs={\"filename\": \"test plot from esp-8266\",\"fileopt\": \"extend\"}",plotly_username, plotly_key, date_string, current_time_with_dst->tm_min);
    os_sprintf(send_data_buffer, "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s", plotly_rest_api_home, plotly_host, strlen(text_data), text_data);
    os_printf("Data that was sent is %s\n\r",send_data_buffer);
  espconn_send(pNewEspConn, send_data_buffer, sizeof(send_data_buffer));
}

// callback when a connection breaks
void ICACHE_FLASH_ATTR disconnectCB(void *arg)
{
    struct espconn *pNewEspConn = (struct espconn *)arg; // Type casting the argument to espconn structure type. This structure is 		unique for each connection
    os_printf("Connection with plotly closed");
}


//callback when data is recieved
void ICACHE_FLASH_ATTR recieveCB(void *arg,char *pData,unsigned short len)
{
    struct espconn *pNewEspConn = (struct espconn *)arg;
    os_printf("Response from Plotly received.  Data length = %d\n\r", len);
    os_printf("Plotly IP address was: %d. %d. %d. %d \n\r", IP2STR(&(pNewEspConn->proto.tcp->remote_ip)));
    int i=0;
    for (i=0; i<len; i++)   // Read data by each character and print it on the debug screen
    {
        os_printf("%c", pData[i]);
    }
    os_printf("\n");
    system_deep_sleep(sleep_time_min*60*1000*1000); // go to sleep after recieving data and wake up periodically
}

//callback when data is sent
LOCAL void ICACHE_FLASH_ATTR sentCB(void *arg)
{
    os_printf("Request was successfully sent\n\r");
}


void ICACHE_FLASH_ATTR find_plotly_dns_and_make_connection(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *conn = (struct espconn *)arg;	// get the corresponding connected control block structure and type cast it
    if (ipaddr != NULL)
    {

        os_printf("DNS for %s found and the ip is : %d.%d.%d.%d\n",name,
                  *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
                  *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
        tcp1.local_port = espconn_port(); // An open port
        tcp1.remote_port = 80;
        os_memcpy(&(tcp1.remote_ip),&ipaddr->addr,4); // remote_ip is unit8[4] and ipaddr->addr is uint32, we need memory copy which needs pointers

        conn->proto.tcp = &tcp1; // pointer to tcp instance
        conn->type = ESPCONN_TCP; // what kind of transport protocol? tcp or UDP
        conn->state = ESPCONN_NONE;// Set it to this state
        espconn_regist_connectcb(conn, connectCB); // register a callback when a connection is fornmed
        espconn_regist_disconcb(conn, disconnectCB); // register a callback when a connection is disconected
        espconn_connect(conn);// make a connection
        espconn_regist_recvcb(conn,recieveCB); // register a callback when one recieves data from remote connection
        espconn_regist_sentcb(conn, sentCB); // register a callback to send data to a remote server
    }
    else
    {
        os_printf("DNS not found");
    }
}


void plotly_plot(void *arg)
{
espconn_gethostbyname(&plotly_conn,plotly_host, &plotly_server_ip,find_plotly_dns_and_make_connection);
}

//Set up a timer
void timer()
{
    os_timer_disarm(&mytimer); // disarm any timer
    os_timer_setfn(&mytimer, (os_timer_func_t *)plotly_plot, NULL);	// associate a callback for a timer
    os_timer_arm(&mytimer, sntp_init_time_sec*1000, 0); // Wait some time to set up sntp and then send a request to plotlty
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
        os_printf("Status: %s\n\r", possible_status[wifi_station_get_connect_status()]);
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
	timer();
	break;
    case EVENT_SOFTAPMODE_STACONNECTED:
        os_printf("Event: EVENT_SOFTAPMODE_STACONNECTED\n");
        break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
        os_printf("Event: EVENT_SOFTAPMODE_STADISCONNECTED\n");
        break;
    default:
        os_printf("Unexpected event: %d\r\n", evt->event);
        break;
    }
}

void user_rf_pre_init(void)
{
system_deep_sleep_set_option(2); // No radio calibration after deep-sleep wake up; this reduces the current consumption
}

//Init function. This is where the program enters
void  user_init()
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200); // set the baud rate for UART0 and UART1, I will use UART1 for debugging and UART0 for flashing
    os_printf("Hello !\n\r");
    os_printf("Chip Id: %lu\n\r", system_get_chip_id()); //Prints chip ID
    os_printf("SDK Version: %s\n\r", system_get_sdk_version()); // Gets the sdk version
    system_init_done_cb(initDone);
   wifi_set_event_handler_cb(eventHandler);
}
