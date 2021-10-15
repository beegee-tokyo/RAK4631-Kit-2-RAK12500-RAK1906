/**
 * @file app.h
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief For application specific includes and definitions
 *        Will be included from main.h
 * @version 0.1
 * @date 2021-04-23
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef APP_H
#define APP_H

#include <Arduino.h>
/** Add you required includes after Arduino.h */
#include <Wire.h>

/** Include the SX126x-API */
#include <WisBlock-API.h> // Click to install library: http://librarymanager/All#WisBlock-API

// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 0
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)           \
	do                            \
	{                             \
		if (tag)                  \
			PRINTF("[%s] ", tag); \
		PRINTF(__VA_ARGS__);      \
		PRINTF("\n");             \
	} while (0)
#else
#define MYLOG(...)
#endif

/** Application function definitions */
void setup_app(void);
bool init_app(void);
void app_event_handler(void);
void ble_data_handler(void) __attribute__((weak));
void lora_data_handler(void);

/** Examples for application events */
#define GNSS_FIN      0b0100000000000000
#define N_GNSS_FIN    0b1011111111111111

/** Application stuff */
extern BaseType_t g_higher_priority_task_woken;

/** Temperature + Humidity stuff */
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
bool init_bme(void);
bool read_bme(void);
void start_bme(void);

// GNSS functions
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
bool init_gnss(void);
bool poll_gnss(void);
void gnss_task(void *pvParameters);
extern SemaphoreHandle_t g_gnss_sem;
extern TaskHandle_t gnss_task_handle;

// LoRaWan functions
struct tracker_data_s
{
	uint8_t data_flag1 = 0x01;	// 1
	uint8_t data_flag2 = 0x88;	// 2
	uint8_t lat_1 = 0;			// 3
	uint8_t lat_2 = 0;			// 4
	uint8_t lat_3 = 0;			// 5
	uint8_t lat_4 = 0;			// 6
	uint8_t long_1 = 0;			// 7
	uint8_t long_2 = 0;			// 8
	uint8_t long_3 = 0;			// 9
	uint8_t long_4 = 0;			// 10
	uint8_t alt_1 = 0;			// 11
	uint8_t alt_2 = 0;			// 12
	uint8_t alt_3 = 0;			// 13
	uint8_t alt_4 = 0;			// 14
	uint8_t data_flag3 = 0x08;	// 15
	uint8_t data_flag4 = 0x02;	// 16
	uint8_t batt_1 = 0;			// 17
	uint8_t batt_2 = 0;			// 18
	uint8_t data_flag5 = 0x07;	// 19
	uint8_t data_flag6 = 0x68;	// 20
	uint8_t humid_1 = 0;		// 21
	uint8_t data_flag7 = 0x02;	// 22
	uint8_t data_flag8 = 0x67;	// 23
	uint8_t temp_1 = 0;			// 24
	uint8_t temp_2 = 0;			// 25
	uint8_t data_flag9 = 0x06;	// 26
	uint8_t data_flag10 = 0x73; // 27
	uint8_t press_1 = 0;		// 28
	uint8_t press_2 = 0;		// 29
	uint8_t data_flag11 = 0x04; // 30
	uint8_t data_flag12 = 0x02; // 31
	uint8_t gas_1 = 0;			// 32
	uint8_t gas_2 = 0;			// 33
};
extern tracker_data_s g_tracker_data;
#define TRACKER_DATA_LEN 33 // sizeof(g_tracker_data)

/** Battery level uinion */
union batt_s
{
	uint16_t batt16 = 0;
	uint8_t batt8[2];
};
/** Latitude/Longitude value union */
union latLong_s
{
	uint32_t val32;
	uint8_t val8[4];
};

#endif