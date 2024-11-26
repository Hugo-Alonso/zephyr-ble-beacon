#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

static uint8_t simulate_htm;  // Flag to simulate temperature notifications
static uint8_t indicating;    // Flag to track if an indication is in progress
static struct bt_gatt_indicate_params ind_params;  // Parameters for GATT indication

// Callback to handle the configuration change of the CCCD (Client Characteristic Configuration Descriptor)
static void htmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	// Enable or disable temperature indication simulation based on the subscription to indications
	simulate_htm = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

// Callback for GATT indication result (success or failure)
static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
}

// Callback to indicate when the indication is complete (to reset the indication flag)
static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	printk("Indication complete\n");
	indicating = 0U;
}

/* Health Thermometer Service Declaration */
BT_GATT_SERVICE_DEFINE(hts_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HTS),  // Primary service UUID for Health Thermometer Service
	BT_GATT_CHARACTERISTIC(BT_UUID_HTS_MEASUREMENT, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),  // Temperature measurement characteristic with indication
	BT_GATT_CCC(htmc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),  // CCCD for enabling/disabling indications
);

void hts_init(void)
{
	// This function is used to initialize the simulated temperature indications (no physical sensor is used)
	printk("Thermometer simulation started, no physical sensor in use.\n");
}

void hts_indicate(void)
{
	static uint8_t htm[5];  // Buffer for the temperature measurement data
	static double temperature = 34U;  // Initial simulated temperature (34°C)
	uint32_t mantissa;
	uint8_t exponent;

	// Only send indications if the simulation is enabled
	if (simulate_htm) {
		// Prevent multiple indications from being sent simultaneously
		if (indicating) {
			return;
		}

		// Simulate a gradual increase in temperature
		temperature += 0.1;  // Increase by 0.1°C per cycle
		if (temperature >= 40.0) {
			temperature = 34.0;  // Reset to 34°C once it exceeds 40°C
		}

		// Prepare the temperature data in the format required by GATT
		mantissa = (uint32_t)(temperature * 100);  // Store the temperature in hundredths of a degree (e.g., 20.0°C -> 2000)
		exponent = (uint8_t)-2;  // Exponent to represent the decimal part (e.g., -2 means the value is divided by 100)

		htm[0] = 0;  // Temperature type (0 indicates Celsius)
		sys_put_le24(mantissa, (uint8_t *)&htm[1]);  // Store the mantissa in the correct byte order
		htm[4] = exponent;  // Store the exponent

		// Prepare the indication parameters
		ind_params.attr = &hts_svc.attrs[2];  // The temperature measurement characteristic
		ind_params.func = indicate_cb;  // Callback function for indication result
		ind_params.destroy = indicate_destroy;  // Callback for when the indication is complete
		ind_params.data = &htm;  // Data to be sent in the indication
		ind_params.len = sizeof(htm);  // Length of the data

		// Send the indication, but only if no indication is in progress
		if (bt_gatt_indicate(NULL, &ind_params) == 0) {
			indicating = 1U;  // Set the indicating flag to prevent further indications until the current one is complete
		}

		// Print the simulated temperature for debugging purposes
		printk("Simulated temperature: %g°C\n", temperature);
	}
}
