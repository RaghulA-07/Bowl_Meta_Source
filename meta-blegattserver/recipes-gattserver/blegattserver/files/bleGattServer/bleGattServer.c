 /*
* @file    bleGattServer.c
* @author  Nestle Firmware Team
* @version V1.0.0
* @date
* @brief   Source file for BLE App  interface
*/

/* Standard Includes --------------------------------------------------------*/


#include <glib.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "adapter.h"
#include "device.h"
#include "logger.h"
#include "agent.h"
#include "application.h"
#include "advertisement.h"
#include "utility.h"
#include <stdint.h>

/* Project Includes ---------------------------------------------------------*/

#include "parser.h"
#include "CommandParser.h"

/* Private typedef ----------------------------------------------------------*/

/* Private define -----------------------------------------------------------*/

#define TAG                         "Main"
#define SERVICE_UUID                "00c74f02-d1bc-11ed-afa1-0242ac120002"
#define NOTIFY_CHARACTERISTIC_UUID  "207bdc30-c3cc-4a14-8b66-56ba8a826640"
#define WRITE_CHARACTERISTIC_UUID   "6765a69d-cd79-4df6-aad5-043df9425556"
#define READ_CHARACTERISTIC_UUID    "b6ab2ce3-a5aa-436a-817a-cc13a45aab76"

#define MAX_MESSAGE_SIZE             100

/* Private macro ------------------------------------------------------------*/

/* Public variables ---------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

struct message {
    long message_type;
    uint8_t message_text[MAX_MESSAGE_SIZE];
};

guint8 ReadArray[100]        = {};
guint8 NotifyArray[100]      = {};

GMainLoop *loop              = NULL;
Adapter *default_adapter     = NULL;
Advertisement *advertisement = NULL;
Application *app             = NULL;

int msgid;


/* Private function prototypes -----------------------------------------------*/


/*******************************************************************************
* @name    
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/
void readCallback(const uint8_t *data, size_t data_length)
 {
    printf("\n BLE_HOOK_INFO :Read Callback: ");
    for (size_t i = 0; i < data_length; ++i) {
        printf("%02X ", data[i]);
        ReadArray[i] = data[i];
    }
    printf("\n");
}

/*******************************************************************************
* @name    
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/
void notifyCallback(const uint8_t *data, size_t data_length)
 {
    printf("\n BLE_HOOK_INFO :Notify Callback: ");
    for (size_t i = 0; i < data_length; ++i) {
        printf("%02X ", data[i]);
        NotifyArray[i] = data[i];
    }
    printf("\n");
}

/*******************************************************************************
* @name    
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/
void on_powered_state_changed(Adapter *adapter, gboolean state)
 {
    log_debug(TAG, "powered '%s' (%s)", state ? "on" : "off", binc_adapter_get_path(adapter));
}


/*******************************************************************************
* @name    
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/
void on_central_state_changed(Adapter *adapter, Device *device) 
{
    char *deviceToString = binc_device_to_string(device);
    log_debug(TAG, deviceToString);
    g_free(deviceToString);

    log_debug(TAG, "remote central %s is %s", binc_device_get_address(device), binc_device_get_connection_state_name(device));
    ConnectionState state = binc_device_get_connection_state(device);
    if (state == BINC_CONNECTED) {
        binc_adapter_stop_advertising(adapter, advertisement);
    } else if (state == BINC_DISCONNECTED) {
        binc_adapter_start_advertising(adapter, advertisement);
    }
}

/*******************************************************************************
* @name    
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/
const char *on_local_char_read(const Application *application, const char *address, const char *service_uuid, const char *char_uuid) {
    if (g_str_equal(service_uuid, SERVICE_UUID) && g_str_equal(char_uuid, READ_CHARACTERISTIC_UUID)) {
        GByteArray *byteArray = g_byte_array_sized_new(ReadArray[1]);
        g_byte_array_append(byteArray, ReadArray, ReadArray[1]);
        binc_application_set_char_value(application, service_uuid, char_uuid, byteArray);
        return NULL;
    }
    return BLUEZ_ERROR_REJECTED;
}

/*******************************************************************************
* @name    
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/

const char *on_local_char_write(const Application *application, const char *address, const char *service_uuid,
                          const char *char_uuid, GByteArray *byteArray) {
    // Log message indicating a write operation
    log_debug(TAG, "Write operation occurred on characteristic: %s", char_uuid);
    
    // Print the data being written to the characteristic
    printf("\n BLE_HOOK_INFO :Data written to characteristic %s: ", char_uuid);
    for (guint i = 0; i < byteArray->len; i++) {
        printf("%02x ", byteArray->data[i]);
    }
    printf("\n");

    int length = byteArray->len;
    
    const uint8_t *data = (const uint8_t *)byteArray->data;

    parseCommand(data, length, readCallback, notifyCallback);

    return NULL;
}

/*******************************************************************************
* @name    
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/

void on_local_char_start_notify(const Application *application, const char *service_uuid, const char *char_uuid) {
    log_debug(TAG, "on start notify");
    if (g_str_equal(service_uuid, SERVICE_UUID) && g_str_equal(char_uuid, NOTIFY_CHARACTERISTIC_UUID)) {
        GByteArray *byteArray = g_byte_array_sized_new(NotifyArray[1]);
        g_byte_array_append(byteArray, NotifyArray, NotifyArray[1]);
        log_debug(TAG, "length: %u", byteArray->len);
        binc_application_notify(application, service_uuid, char_uuid, byteArray);
        g_byte_array_free(byteArray, TRUE);
    }
}

/*******************************************************************************
* @name     
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/

void on_local_char_stop_notify(const Application *application, const char *service_uuid, const char *char_uuid) {
    log_debug(TAG, "on stop notify");
}

/*******************************************************************************
* @name     
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/

gboolean callback(gpointer data) {
    if (app != NULL) {
        binc_adapter_unregister_application(default_adapter, app);
        binc_application_free(app);
        app = NULL;
    }

    if (advertisement != NULL) {
        binc_adapter_stop_advertising(default_adapter, advertisement);
        binc_advertisement_free(advertisement);
    }

    if (default_adapter != NULL) {
        binc_adapter_free(default_adapter);
        default_adapter = NULL;
    }

    g_main_loop_quit((GMainLoop *) data);
    return FALSE;
}

/*******************************************************************************
* @name     
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/
static void cleanup_handler(int signo) {
    if (signo == SIGINT) {
        log_error(TAG, "received SIGINT");
        callback(loop);
    }
}


/*******************************************************************************
* @name     
* @brief  Function handles the   
* @param   
* @retval  
********************************************************************************/

int main(void)
 {
    // Get a DBus connection
    GDBusConnection *dbusConnection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    // Setup handler for CTRL+C
    if (signal(SIGINT, cleanup_handler) == SIG_ERR)
        log_error(TAG, "can't catch SIGINT");

    // Setup mainloop
    loop = g_main_loop_new(NULL, FALSE);

    // Get the default adapter
    default_adapter = binc_adapter_get_default(dbusConnection);

    // Generate unique key
    // key_t key = ftok("BLE_HOOK", 65);

    key_t key = 1234;
    
    // Create message queue and return id
    msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    if (default_adapter != NULL) {
        log_debug(TAG, "using default_adapter '%s'", binc_adapter_get_path(default_adapter));

        // Make sure the adapter is on
        binc_adapter_set_powered_state_cb(default_adapter, &on_powered_state_changed);
        if (!binc_adapter_get_powered_state(default_adapter)) {
            binc_adapter_power_on(default_adapter);
        }

        // Setup remote central connection state callback
        binc_adapter_set_remote_central_cb(default_adapter, &on_central_state_changed);

        // Setup advertisement
        GPtrArray *adv_service_uuids = g_ptr_array_new();
        g_ptr_array_add(adv_service_uuids, SERVICE_UUID);

        advertisement = binc_advertisement_create();
        //binc_advertisement_set_local_name(advertisement, "NDC_DOG_BOWL_DEVICE");
        binc_advertisement_set_services(advertisement, adv_service_uuids);
        g_ptr_array_free(adv_service_uuids, TRUE);
        binc_adapter_start_advertising(default_adapter, advertisement);

        // Start application
        app = binc_create_application(default_adapter);
        binc_application_add_service(app, SERVICE_UUID);
        binc_application_add_characteristic(
                app,
                SERVICE_UUID,
                NOTIFY_CHARACTERISTIC_UUID,
                GATT_CHR_PROP_INDICATE);

        binc_application_add_characteristic(
                app,
                SERVICE_UUID,
                READ_CHARACTERISTIC_UUID,
                GATT_CHR_PROP_READ);

        binc_application_add_characteristic(
                app,
                SERVICE_UUID,
                WRITE_CHARACTERISTIC_UUID,
                GATT_CHR_PROP_WRITE);

        //const guint8 cud[] = "hello there";
        //GByteArray *cudArray = g_byte_array_sized_new(sizeof(cud));
        // g_byte_array_append(cudArray, cud, sizeof(cud));
        //binc_application_set_desc_value(app, SERVICE_UUID, TEMPERATURE_CHAR_UUID, CUD_CHAR, cudArray);
        binc_application_set_char_read_cb(app, &on_local_char_read);
        binc_application_set_char_write_cb(app, &on_local_char_write);
        binc_application_set_char_start_notify_cb(app, &on_local_char_start_notify);
        binc_application_set_char_stop_notify_cb(app, &on_local_char_stop_notify);
        binc_adapter_register_application(default_adapter, app);
    } else {
        log_debug("MAIN", "No default_adapter found");
    }

    // Start the mainloop
    g_main_loop_run(loop);

    // Clean up mainloop
    g_main_loop_unref(loop);

    // Disconnect from DBus
    g_dbus_connection_close_sync(dbusConnection, NULL, NULL);
    g_object_unref(dbusConnection);

    return 0;
}
