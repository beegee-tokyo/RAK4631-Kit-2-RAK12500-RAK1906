/**
 * @file app.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Application specific functions. Mandatory to have init_app(), 
 *        app_event_handler(), ble_data_handler(), lora_data_handler()
 *        and lora_tx_finished()
 * @version 0.1
 * @date 2021-04-23
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "app.h"

/** Set the device name, max length is 10 characters */
char g_ble_dev_name[10] = "RAK-GNSS";

/** Flag showing if TX cycle is ongoing */
bool lora_busy = false;

/** Timer since last position message was sent */
time_t last_pos_send = 0;
/** Timer for delayed sending to keep duty cycle */
SoftwareTimer delayed_sending;
/** Required for give semaphore from ISR */
BaseType_t g_higher_priority_task_woken = pdTRUE;

/** Battery level uinion */
batt_s batt_level;

/** Flag if delayed sending is already activated */
bool delayed_active = false;

/** Minimum delay between sending new locations, set to 45 seconds */
time_t min_delay = 45000;

// Forward declaration
void send_delayed(TimerHandle_t unused);

/** Send Fail counter **/
uint8_t send_fail = 0;

/** Flag for low battery protection */
bool low_batt_protection = false;

/**
 * @brief Application specific setup functions
 * 
 */
void setup_app(void)
{
	// Enable BLE
	g_enable_ble = true;
}

/**
 * @brief Application specific initializations
 * 
 * @return true Initialization success
 * @return false Initialization failure
 */
bool init_app(void)
{
	bool init_result = true;
	MYLOG("APP", "init_app");

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

	// Start the I2C bus
	Wire.begin();
	Wire.setClock(400000);

	// Initialize GNSS module
	init_result = init_gnss();

	// Initialize Temperature sensor
	// init_result |= init_th();
	init_result |= init_bme();

	if (g_lorawan_settings.send_repeat_time != 0)
	{
		// Set delay for sending to 1/2 of scheduled sending
		min_delay = g_lorawan_settings.send_repeat_time / 2;
	}
	else
	{
		// Send repeat time is 0, set delay to 30 seconds
		min_delay = 30000;
	}
	// Set to 1/2 of programmed send interval or 30 seconds
	delayed_sending.begin(min_delay, send_delayed, NULL, false);

	return init_result;
}

/**
 * @brief Application specific event handler
 *        Requires as minimum the handling of STATUS event
 *        Here you handle as well your application specific events
 */
void app_event_handler(void)
{
	// Timer triggered event
	if ((g_task_event_type & STATUS) == STATUS)
	{
		g_task_event_type &= N_STATUS;
		MYLOG("APP", "Timer wakeup");

		// If BLE is enabled, restart Advertising
		if (g_enable_ble)
		{
			restart_advertising(15);
		}

		if (!low_batt_protection)
		{
			// Wake up the temperature sensor and start measurements
			start_bme();

			// Start the GNSS location tracking
			xSemaphoreGive(g_gnss_sem);
		}

		// Get battery level
		batt_level.batt16 = read_batt() / 10;
		g_tracker_data.batt_1 = batt_level.batt8[1];
		g_tracker_data.batt_2 = batt_level.batt8[0];

		// Protection against battery drain
		if (batt_level.batt16 < 290)
		{
			// Battery is very low, change send time to 1 hour to protect battery
			low_batt_protection = true;						   // Set low_batt_protection active
			api_timer_restart(1 * 60 * 60 * 1000); // Set send time to one hour
			MYLOG("APP", "Battery protection activated");
		}
		else if ((batt_level.batt16 > 410) && low_batt_protection)
		{
			// Battery is higher than 4V, change send time back to original setting
			low_batt_protection = false;
			api_timer_restart(g_lorawan_settings.send_repeat_time);
			MYLOG("APP", "Battery protection deactivated");
		}

		if (low_batt_protection)
		{
			lmh_error_status result = send_lora_packet((uint8_t *)&g_tracker_data, TRACKER_DATA_LEN);
			switch (result)
			{
			case LMH_SUCCESS:
				MYLOG("APP", "Packet enqueued");
				lora_busy = true;
				if (g_ble_uart_is_connected)
				{
					g_ble_uart.println("Packet enqueued");
				}
				break;
			case LMH_BUSY:
				MYLOG("APP", "LoRa transceiver is busy");
				lora_busy = true;
				if (g_ble_uart_is_connected)
				{
					g_ble_uart.println("LoRa transceiver is busy");
				}
				break;
			case LMH_ERROR:
				MYLOG("APP", "Packet error, too big to send with current DR");
				if (g_ble_uart_is_connected)
				{
					g_ble_uart.println("Packet error, too big to send with current DR");
				}
				break;
			}
		}
	}

	// GNSS location search finished
	if ((g_task_event_type & GNSS_FIN) == GNSS_FIN)
	{
		g_task_event_type &= N_GNSS_FIN;

		// Get temperature and humidity data
		// read_th();
		read_bme();

		// Remember last time sending
		last_pos_send = millis();
		// Just in case
		delayed_active = false;

#if MY_DEBUG == 1
		uint8_t *packet = &g_tracker_data.data_flag1;
		for (int idx = 0; idx < TRACKER_DATA_LEN; idx++)
		{
			Serial.printf("%02X", packet[idx]);
		}
		Serial.println("");
		if (g_ble_uart_is_connected)
		{
			for (int idx = 0; idx < TRACKER_DATA_LEN; idx++)
			{
				g_ble_uart.printf("%02X", packet[idx]);
			}
			g_ble_uart.println("");
		}
#endif

		lmh_error_status result = send_lora_packet((uint8_t *)&g_tracker_data, TRACKER_DATA_LEN);
		switch (result)
		{
		case LMH_SUCCESS:
			MYLOG("APP", "Packet enqueued");
			/// \todo set a flag that TX cycle is running
			lora_busy = true;
			if (g_ble_uart_is_connected)
			{
				g_ble_uart.println("Packet enqueued");
			}
			break;
		case LMH_BUSY:
			MYLOG("APP", "LoRa transceiver is busy");
			if (g_ble_uart_is_connected)
			{
				g_ble_uart.println("LoRa transceiver is busy");
			}
			break;
		case LMH_ERROR:
			MYLOG("APP", "Packet error, too big to send with current DR");
			if (g_ble_uart_is_connected)
			{
				g_ble_uart.println("Packet error, too big to send with current DR");
			}
			break;
		}
	}
}

/**
 * @brief Handle BLE UART data
 * 
 */
void ble_data_handler(void)
{
	if (g_enable_ble)
	{
		// BLE UART data handling
		if ((g_task_event_type & BLE_DATA) == BLE_DATA)
		{
			MYLOG("AT", "RECEIVED BLE");
			/** BLE UART data arrived */
			g_task_event_type &= N_BLE_DATA;

			while (g_ble_uart.available() > 0)
			{
				at_serial_input(uint8_t(g_ble_uart.read()));
				delay(5);
			}
			at_serial_input(uint8_t('\n'));
		}
	}
}

/**
 * @brief Handle received LoRa Data
 * 
 */
void lora_data_handler(void)
{
	// LoRa data handling
	if ((g_task_event_type & LORA_DATA) == LORA_DATA)
	{
		/**************************************************************/
		/**************************************************************/
		/// \todo LoRa data arrived
		/// \todo parse them here
		/**************************************************************/
		/**************************************************************/
		g_task_event_type &= N_LORA_DATA;
		MYLOG("APP", "Received package over LoRa");
		char log_buff[g_rx_data_len * 3] = {0};
		uint8_t log_idx = 0;
		for (int idx = 0; idx < g_rx_data_len; idx++)
		{
			sprintf(&log_buff[log_idx], "%02X ", g_rx_lora_data[idx]);
			log_idx += 3;
		}
		lora_busy = false;
		MYLOG("APP", "%s", log_buff);

		if (g_ble_uart_is_connected && g_enable_ble)
		{
			for (int idx = 0; idx < g_rx_data_len; idx++)
			{
				g_ble_uart.printf("%02X ", g_rx_lora_data[idx]);
			}
			g_ble_uart.println("");
		}
	}

	// LoRa TX finished handling
	if ((g_task_event_type & LORA_TX_FIN) == LORA_TX_FIN)
	{
		g_task_event_type &= N_LORA_TX_FIN;

		MYLOG("APP", "LPWAN TX cycle %s", g_rx_fin_result ? "finished ACK" : "failed NAK");
		if (g_ble_uart_is_connected)
		{
			g_ble_uart.printf("LPWAN TX cycle %s", g_rx_fin_result ? "finished ACK" : "failed NAK");
		}

		if (!g_rx_fin_result)
		{
			// Increase fail send counter
			send_fail++;

			if (send_fail == 10)
			{
				// Too many failed sendings, reset node and try to rejoin
				delay(100);
				sd_nvic_SystemReset();
			}
		}
		/// \todo reset flag that TX cycle is running
		lora_busy = false;
	}

	// LoRa Join finished handling
	if ((g_task_event_type & LORA_JOIN_FIN) == LORA_JOIN_FIN)
	{
		g_task_event_type &= N_LORA_JOIN_FIN;
		if (g_join_result)
		{
			MYLOG("APP", "Successfully joined network");

			// Prepare GNSS task
			// Create the GNSS event semaphore
			g_gnss_sem = xSemaphoreCreateBinary();
			// Initialize semaphore
			xSemaphoreGive(g_gnss_sem);
			// Take semaphore
			xSemaphoreTake(g_gnss_sem, 10);
			if (!xTaskCreate(gnss_task, "LORA", 4096, NULL, TASK_PRIO_LOW, &gnss_task_handle))
			{
				MYLOG("APP", "Failed to start GNSS task");
			}
		}
		else
		{
			MYLOG("APP", "Join network failed");
			/// \todo here join could be restarted.
			// lmh_join();
		}
	}
}

/**
 * @brief Timer function used to avoid sending packages too often.
 * 			Delays the next package by 10 seconds
 * 
 * @param unused 
 * 			Timer handle, not used
 */
void send_delayed(TimerHandle_t unused)
{
	g_task_event_type |= STATUS;
	xSemaphoreGiveFromISR(g_task_sem, &g_higher_priority_task_woken);
}
