/**
 * @file gnss.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief GNSS functions and task
 * @version 0.1
 * @date 2020-07-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "app.h"

// The GNSS object
SFE_UBLOX_GNSS my_gnss;

/** GNSS polling function */
bool poll_gnss(void);

/** Location data as byte array */
tracker_data_s g_tracker_data;

/** Latitude/Longitude value converter */
latLong_s pos_union;

/** Flag if location was found */
bool last_read_ok = false;

/** Flag if GNSS is serial or I2C */
bool i2c_gnss = false;

/** Switch between GNSS on/off (1) and GNSS power save mode (0)*/
#define GNSS_OFF 0

/**
 * @brief Initialize the GNSS
 * 
 */
bool init_gnss(void)
{
	bool gnss_found = false;
	// // Give the module some time to power up
	// delay(2000);

	// Power on the GNSS module
	// pinMode(WB_IO2, OUTPUT);
	// delay(100);
	digitalWrite(WB_IO2, HIGH);

	// Give the module some time to power up
	delay(500);

	if (!my_gnss.begin())
	{
		MYLOG("GNSS", "UBLOX did not answer on I2C, retry on Serial1");
		if (g_ble_uart_is_connected)
		{
			g_ble_uart.println("UBLOX did not answer on I2C, retry on Serial1");
		}
		i2c_gnss = false;
	}
	else
	{
		MYLOG("GNSS", "UBLOX found on I2C");
		if (g_ble_uart_is_connected)
		{
			g_ble_uart.println("UBLOX found on I2C");
		}
		i2c_gnss = true;
		gnss_found = true;
		my_gnss.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
	}

	if (!i2c_gnss)
	{
		//Assume that the U-Blox GNSS is running at 9600 baud (the default) or at 38400 baud.
		//Loop until we're in sync and then ensure it's at 38400 baud.
		do
		{
			MYLOG("GNSS", "GNSS: trying 38400 baud");
			Serial1.begin(38400);
			while (!Serial1)
				;
			if (my_gnss.begin(Serial1) == true)
			{
				MYLOG("GNSS", "UBLOX found on Serial1 with 38400");
				my_gnss.setUART1Output(COM_TYPE_UBX); //Set the UART port to output UBX only
				if (g_ble_uart_is_connected)
				{
					g_ble_uart.println("UBLOX found on Serial1 with 38400");
				}
				gnss_found = true;

				break;
			}
			delay(100);
			MYLOG("GNSS", "GNSS: trying 9600 baud");
			Serial1.begin(9600);
			while (!Serial1)
				;
			if (my_gnss.begin(Serial1) == true)
			{
				MYLOG("GNSS", "GNSS: connected at 9600 baud, switching to 38400");
				my_gnss.setSerialRate(38400);
				delay(100);
			}
			else
			{
				my_gnss.factoryReset();
				delay(2000); //Wait a bit before trying again to limit the Serial output
			}
		} while (1);
	}

	// my_gnss.setI2COutput(COM_TYPE_UBX);	  //Set the I2C port to output UBX only (turn off NMEA noise)
	// my_gnss.setUART1Output(COM_TYPE_UBX); //Set the UART port to output UBX only
	my_gnss.saveConfiguration(); //Save the current settings to flash and BBR

	my_gnss.setMeasurementRate(500);

	// my_gnss.powerSaveMode(true);

	return gnss_found;
}

/**
 * @brief Check GNSS module for position
 * 
 * @return true Valid position found
 * @return false No valid position
 */
bool poll_gnss(void)
{
	MYLOG("GNSS", "poll_gnss");
	if (g_ble_uart_is_connected)
	{
		g_ble_uart.println("poll_gnss");
	}

#if GNSS_OFF == 1
	// Start connection
	init_gnss();
#endif

	// delay(100);
	time_t time_out = millis();
	bool has_pos = false;
	int64_t latitude = 0;
	int64_t longitude = 0;
	int32_t altitude = 0;

	while ((millis() - time_out) < 60000)
	{
		byte fix_type = my_gnss.getFixType(); // Get the fix type
		char fix_type_str[32] = {0};
		if (fix_type == 0)
			sprintf(fix_type_str, "No Fix");
		else if (fix_type == 1)
			sprintf(fix_type_str, "Dead reckoning");
		else if (fix_type == 2)
			sprintf(fix_type_str, "Fix type 2D");
		else if (fix_type == 3)
			sprintf(fix_type_str, "Fix type 3D");
		else if (fix_type == 4)
			sprintf(fix_type_str, "GNSS fix");
		else if (fix_type == 5)
			sprintf(fix_type_str, "Time fix");

		if (g_ble_uart_is_connected)
		{
			g_ble_uart.printf("Fixtype: %d %s\n", fix_type, fix_type_str);
			g_ble_uart.printf("SIV: %d\n", my_gnss.getSIV());
		}

		// if (my_gnss.getGnssFixOk())
		// if ((fix_type >= 3) && (my_gnss.getSIV() >= 5))
		if (fix_type >= 3)
		{
			digitalWrite(LED_CONN, HIGH);
			has_pos = true;
			last_read_ok = true;
			latitude = my_gnss.getLatitude();
			longitude = my_gnss.getLongitude();
			altitude = my_gnss.getAltitude();

			/// \todo  For testing, an address in Recife, Brazil, which has both latitude and longitude negative
			// latitude = -80487740;
			// longitude = -349021580;
			// altitude = 156024;

			MYLOG("GNSS", "Fixtype: %d %s", my_gnss.getFixType(), fix_type_str);
			MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
			MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);

			if (g_ble_uart_is_connected)
			{
				g_ble_uart.printf("Fixtype: %d %s\n", my_gnss.getFixType(), fix_type_str);
				g_ble_uart.printf("Lat: %.4f Lon: %.4f\n", latitude / 10000000.0, longitude / 10000000.0);
				g_ble_uart.printf("Alt: %.2f\n", altitude / 1000.0);
			}
			pos_union.val32 = latitude / 1000;
			g_tracker_data.lat_1 = pos_union.val8[2];
			g_tracker_data.lat_2 = pos_union.val8[1];
			g_tracker_data.lat_3 = pos_union.val8[0];

			pos_union.val32 = longitude / 1000;
			g_tracker_data.long_1 = pos_union.val8[2];
			g_tracker_data.long_2 = pos_union.val8[1];
			g_tracker_data.long_3 = pos_union.val8[0];

			pos_union.val32 = altitude / 10;
			g_tracker_data.alt_1 = pos_union.val8[2];
			g_tracker_data.alt_2 = pos_union.val8[1];
			g_tracker_data.alt_3 = pos_union.val8[0];

			// Break the while()
			break;
		}
		else
		{
			delay(1000);
		}
	}

#if GNSS_OFF == 1
	// Power down the module
	digitalWrite(WB_IO2, LOW);
	delay(100);
#endif

	if (has_pos)
	{
#if GNSS_OFF == 0
		my_gnss.powerSaveMode(true);
		my_gnss.setMeasurementRate(10000);
#endif
		return true;
	}

	MYLOG("GNSS", "No valid location found");
	if (g_ble_uart_is_connected)
	{
		g_ble_uart.println("\nNo valid location found");
	}
	last_read_ok = false;

	digitalWrite(LED_CONN, LOW);

#if GNSS_OFF == 0
	my_gnss.setMeasurementRate(1000);
#endif
	return false;
}