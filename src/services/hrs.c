#include <zephyr/types.h>
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(hrs, CONFIG_BT_HRS_LOG_LEVEL);

static uint8_t simulate_hr;  // Flag to simulate heart rate notifications

// Callback to handle the configuration change of the CCCD (Client Characteristic Configuration Descriptor)
static void hrmc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    simulate_hr = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
    LOG_INF("Heart Rate notifications %s", simulate_hr ? "enabled" : "disabled");
}

// Health Rate Measurement characteristic, configured to notify
BT_GATT_SERVICE_DEFINE(hrs_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),  // Heart Rate Service UUID
    BT_GATT_CHARACTERISTIC(BT_UUID_HRS_MEASUREMENT, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),  // Heart rate measurement characteristic
    BT_GATT_CCC(hrmc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)  // CCCD for notifications
);

// Function to send heart rate notifications
int bt_hrs_notify(uint16_t heartrate)
{
    int rc;
    static uint8_t hrm[2];

    // Prepare the heart rate measurement data
    hrm[0] = 0x06; // Sensor contact status (e.g., 0x06 means "sensor in contact")
    hrm[1] = (uint8_t)heartrate;  // Heart rate value (single byte for simplicity)

    // Send the notification (only if notifications are enabled)
    if (simulate_hr) {
        rc = bt_gatt_notify(NULL, &hrs_svc.attrs[1], &hrm, sizeof(hrm));
        if (rc == 0) {
            LOG_INF("Heart Rate Notification sent: %u", heartrate);
        } else {
            LOG_ERR("Failed to send Heart Rate Notification, error %d", rc);
        }
        return rc;
    }

    return 0;
}

