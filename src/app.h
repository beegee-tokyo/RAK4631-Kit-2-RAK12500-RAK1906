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

/** Application function definitions */
void setup_app(void);
bool init_app(void);
void app_event_handler(void);
void ble_data_handler(void) __attribute__((weak));
void lora_data_handler(void);

/** Examples for application events */
#define ACC_TRIGGER 0b1000000000000000
#define N_ACC_TRIGGER 0b0111111111111111

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

// LoRaWan functions
struct tracker_data_s
{
	uint8_t data_flag1 = 0x01;	// 1
	uint8_t data_flag2 = 0x88;	// 2
	uint8_t lat_1 = 0;			// 3
	uint8_t lat_2 = 0;			// 4
	uint8_t lat_3 = 0;			// 5
	uint8_t long_1 = 0;			// 6
	uint8_t long_2 = 0;			// 7
	uint8_t long_3 = 0;			// 8
	uint8_t alt_1 = 0;			// 9
	uint8_t alt_2 = 0;			// 10
	uint8_t alt_3 = 0;			// 11
	uint8_t data_flag3 = 0x08;	// 12
	uint8_t data_flag4 = 0x02;	// 13
	uint8_t batt_1 = 0;			// 14
	uint8_t batt_2 = 0;			// 15
	uint8_t data_flag5 = 0x07;	// 16
	uint8_t data_flag6 = 0x68;	// 17
	uint8_t humid_1 = 0;		// 18
	uint8_t data_flag7 = 0x02;	// 19
	uint8_t data_flag8 = 0x67;	// 20
	uint8_t temp_1 = 0;			// 21
	uint8_t temp_2 = 0;			// 22
	uint8_t data_flag9 = 0x06;	// 23
	uint8_t data_flag10 = 0x73; // 24
	uint8_t press_1 = 0;		// 25
	uint8_t press_2 = 0;		// 26
	uint8_t data_flag11 = 0x04; // 27
	uint8_t data_flag12 = 0x02; // 28
	uint8_t gas_1 = 0;			// 29
	uint8_t gas_2 = 0;			// 30
};
extern tracker_data_s g_tracker_data;
#define TRACKER_DATA_LEN 30 // sizeof(g_tracker_data)

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