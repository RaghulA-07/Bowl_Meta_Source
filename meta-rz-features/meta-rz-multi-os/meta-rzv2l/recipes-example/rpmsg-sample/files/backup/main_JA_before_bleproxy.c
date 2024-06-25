/**
 * @file    main.c
 * @brief   Main function and RPMSG example application.
 * @date    2020.10.27
 * @license SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h> // For timestamp
#include "metal/alloc.h"
#include "metal/utilities.h"
#include "openamp/open_amp.h"
#include "platform_info.h"
#include "rsc_table.h"

#define SHUTDOWN_MSG    (0xEF56A55A)
#define TARGET_SIZE     (4096) // Define the specific size to trigger file saving
#define FOLDER_NAME     "received_files" // Define the folder name

#ifndef max
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     (_a > _b) ? _a : _b; })
#endif

#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

/* Payload definition */
struct _payload {
    unsigned long num;
    unsigned long size;
    unsigned char data[];
};

/* Payload information */
struct payload_info {
    int minnum;
    int maxnum;
    int num;
};

/* SensorData definition */
typedef struct
{
    uint8_t   FrameID;
    uint8_t   PIR_ID;
    uint8_t   PIR_value;
    uint8_t   LOAD_ID;
    uint8_t   LoadSensor_status;
    uint8_t   IR_ID;
    uint8_t   IRSensor_status;
    double    Temperature_value; //8bytes
    double    loadcell_Value;   //8bytes 
} SensorData;

/* Globals */
static struct rpmsg_endpoint rp_ept = { 0 };
static struct _payload *i_payload;
static int rnum = 0;
static int err_cnt = 0;
static char *svc_name = NULL;
int force_stop = 0;
pthread_cond_t cond;
pthread_mutex_t mutex, rsc_mutex;
static int file_counter = 1; // Counter for file names
static size_t buffer_size = 0; // Counter for the current buffer size
static char buffer[TARGET_SIZE]; // Buffer to hold data until it reaches TARGET_SIZE

struct comm_arg ids[] = {
    {NULL, 0},
    {NULL, 1},
};

/* External functions */
extern int init_system(void);
extern void cleanup_system(void);

/* Internal functions */
static void rpmsg_service_bind(struct rpmsg_device *rdev, const char *name, uint32_t dest);
static void rpmsg_service_unbind(struct rpmsg_endpoint *ept);
static int rpmsg_service_cb0(struct rpmsg_endpoint *rp_ept, void *data, size_t len, uint32_t src, void *priv);
static int payload_init(struct rpmsg_device *rdev, struct payload_info *pi);
static void register_handler(int signum, void(* handler)(int));
static void stop_handler(int signum);
static void init_cond(void);
static void launch_communicate(int pattern);
static void *communicate(void* arg);
static char* sensor_data_to_string(SensorData *pSensors);

/* Application entry point */
static int app (struct rpmsg_device *rdev, struct remoteproc *priv, unsigned long svcno)
{
    int ret = 0;
    int shutdown_msg = SHUTDOWN_MSG;
    int i;
    int size;
    int expect_rnum = 0;
    struct payload_info pi = { 0 };
    static int sighandled = 0;

    LPRINTF(" 1 - Send data to remote core, retrieve the echo");
    LPRINTF(" and validate its integrity ..");

    /* Initialization of the payload and its related information */
    if ((ret = payload_init(rdev, &pi))) {
        return ret;
    }

    LPRINTF("Remote proc init.");

    /* Create RPMsg endpoint */
    if (svcno == 0) {
        svc_name = (const char *)CFG_RPMSG_SVC_NAME0;
    } else {
        svc_name = (const char *)CFG_RPMSG_SVC_NAME1;
    }
    
    pthread_mutex_lock(&rsc_mutex);
    ret = rpmsg_create_ept(&rp_ept, rdev, svc_name, APP_EPT_ADDR,
                   RPMSG_ADDR_ANY,
                   rpmsg_service_cb0, rpmsg_service_unbind);
    pthread_mutex_unlock(&rsc_mutex);
    if (ret) {
        LPERROR("Failed to create RPMsg endpoint.");
        return ret;
    }
    LPRINTF("RPMSG endpoint has created. rp_ept:%p ", &rp_ept);

    if (!sighandled) {
        sighandled = 1;
        register_handler(SIGINT, stop_handler);
        register_handler(SIGTERM, stop_handler);
    }

    while (!force_stop && !is_rpmsg_ept_ready(&rp_ept))
        platform_poll(priv);

    if (force_stop) {
        LPRINTF("\nForce stopped. ");
        goto error;
    }

    LPRINTF("RPMSG service has created.");
    for (i = 0, size = pi.minnum; i < (int)pi.num; i++, size++) {
        i_payload->num = i;
        i_payload->size = size;
     
        /* Mark the data buffer. */
        memset(&(i_payload->data[0]), 0xA5, size);
     
        LPRINTF("sending payload number %lu of size %lu",
             i_payload->num, (2 * sizeof(unsigned long)) + size);
     
        ret = rpmsg_send(&rp_ept, i_payload,
             (2 * sizeof(unsigned long)) + size);
     
        if (ret < 0) {
            LPRINTF("Error sending data...%d", ret);
            break;
        }
        LPRINTF("echo test: sent : %lu", (2 * sizeof(unsigned long)) + size);
     
        expect_rnum++;
        do {
            platform_poll(priv);
        } while (!force_stop && (rnum < expect_rnum) && !err_cnt);
        usleep(10000);
        if (force_stop) {
            LPRINTF("\nForce stopped. ");
            goto error;
        }
    }

    LPRINTF("************************************");
    LPRINTF(" Test Results: Error count = %d ", err_cnt);
    LPRINTF("************************************");
error:
    /* Send shutdown message to remote */
    rpmsg_send(&rp_ept, &shutdown_msg, sizeof(int));
    sleep(1);
    LPRINTF("Quitting application .. Echo test end");

    metal_free_memory(i_payload);
    return 0;
}

static void rpmsg_service_bind(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
    LPRINTF("new endpoint notification is received.");
    if (strcmp(name, svc_name)) {
        LPERROR("Unexpected name service %s.", name);
    }
    else
        (void)rpmsg_create_ept(&rp_ept, rdev, svc_name,
                       APP_EPT_ADDR, dest,
                       rpmsg_service_cb0,
                       rpmsg_service_unbind);
    return ;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
    (void)ept;
    /* service 0 */
    rpmsg_destroy_ept(&rp_ept);
    memset(&rp_ept, 0x0, sizeof(struct rpmsg_endpoint));
    return ;
}

static int rpmsg_service_cb0(struct rpmsg_endpoint *cb_rp_ept, void *data, size_t len, uint32_t src, void *priv)
{
    (void)cb_rp_ept;
    (void)src;
    (void)priv;
    int ret = 0;
    
    SensorData *pSensors;
    pSensors = (SensorData*) data;

    // Create directory if it doesn't exist
    struct stat st = {0};
    if (stat(FOLDER_NAME, &st) == -1) {
        mkdir(FOLDER_NAME, 0700);
    }

    // Convert SensorData to a comma-separated string
    char *sensor_str = sensor_data_to_string(pSensors);
    size_t sensor_str_size = strlen(sensor_str) + 1; // +1 for newline character

    // Check if buffer can hold the new data, if not, flush the buffer
    if (buffer_size + sensor_str_size > TARGET_SIZE) {
        char filename[256]; // Buffer for filename
        snprintf(filename, sizeof(filename), "%s/file%d.csv", FOLDER_NAME, file_counter);
        FILE *file = fopen(filename, "w");
        if (file) {
            fwrite(buffer, 1, buffer_size, file);
            fclose(file);
            LPRINTF("Data saved to %s", filename);
            file_counter++; // Increment file counter for next file
            buffer_size = 0; // Reset buffer size
        } else {
            LPERROR("Failed to open file for writing.");
        }
    }

    // Add sensor_str to the buffer
    memcpy(buffer + buffer_size, sensor_str, sensor_str_size - 1);
    buffer[buffer_size + sensor_str_size - 1] = '\n'; // Add newline character
    buffer_size += sensor_str_size;

    return ret;
}

static int payload_init(struct rpmsg_device *rdev, struct payload_info *pi) {
    int rpmsg_buf_size = 0;

    /* Get the maximum buffer size of a rpmsg packet */
    if ((rpmsg_buf_size = rpmsg_virtio_get_buffer_size(rdev)) <= 0) {
        return rpmsg_buf_size;
    }

    pi->minnum = 1;
    pi->maxnum = rpmsg_buf_size - 24;
    pi->num = pi->maxnum / pi->minnum;

    i_payload =
        (struct _payload *)metal_allocate_memory(2 * sizeof(unsigned long) +
                      pi->maxnum);
    if (!i_payload) {
        LPERROR("memory allocation failed.");
        return -ENOMEM;
    }

    return 0;
}

static void init_cond(void)
{
#ifdef __linux__
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&rsc_mutex, NULL);
    pthread_cond_init(&cond, NULL);
#endif
}

int main(int argc, char *argv[])
{
    unsigned long proc_id;
    unsigned long rsc_id;
    int i;
    int ret = 0;

    /* Initialize HW system components */
    init_system();
    init_cond();
    
    /* mqtt_init */

    /* Initialize platform */
    for (i = 0; i < ARRAY_SIZE(ids); i++) {
        proc_id = rsc_id = ids[i].channel;
        ret = platform_init(proc_id, rsc_id, &ids[i].platform);
        if (ret) {
            LPERROR("Failed to initialize platform.");
            ret = 1;
            goto error_return;
        }
    }

    /* Launch communication automatically with pattern = 1 */
    launch_communicate(1);

    for (i = 0; i < ARRAY_SIZE(ids); i++) {
        platform_cleanup(ids[i].platform);
        ids[i].platform = NULL;
    }
    cleanup_system();

error_return:
    return ret;
}

/**
 * @fn communicate
 * @brief perform test communication
 * @param arg - test conditions
 */
static void *communicate(void* arg) {
    struct comm_arg *p = (struct comm_arg*)arg;
    struct rpmsg_device *rpdev;
    unsigned long proc_id = p->channel;

    LPRINTF("thread start ");

    pthread_mutex_lock(&rsc_mutex);
    rpdev = platform_create_rpmsg_vdev(p->platform, 0,
                      VIRTIO_DEV_MASTER,
                      NULL,
                      rpmsg_service_bind);
    pthread_mutex_unlock(&rsc_mutex);
    if (!rpdev) {
        LPERROR("Failed to create rpmsg virtio device.");
    } else {
        (void)app(rpdev, p->platform, proc_id);
        platform_release_rpmsg_vdev(p->platform, rpdev);
    }
    LPRINTF("Stopping application...");

    return NULL;
}

/**
 * @fn launch_communicate
 * @brief Launch test threads according to test patterns
 */
static void launch_communicate(int pattern)
{
    pthread_t th = 0;

    pattern--;
    if ((pattern < 0) || (ARRAY_SIZE(ids) <= max(0, pattern - 1))) return;

    pthread_create(&th, NULL, communicate, &ids[pattern]);

    if (th) pthread_join(th, NULL);
}

static void register_handler(int signum, void(* handler)(int)) {
    if (signal(signum, handler) == SIG_ERR) {
        LPRINTF("register sig:%d failed.", signum);
    } else {
        LPRINTF("register sig:%d succeeded.", signum);
    }
}

static void stop_handler(int signum) {
    force_stop = 1;
    (void)signum;

    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

/**
 * @fn sensor_data_to_string
 * @brief Convert SensorData to a comma-separated string with timestamp and four decimal places for double values.
 * @param pSensors - pointer to SensorData
 * @return char* - comma-separated string
 */
static char* sensor_data_to_string(SensorData *pSensors) {
    static char str[256];
    struct timespec ts;
    struct tm *tm_info;
    char timestamp[64];

    // Get the current time with milliseconds
    clock_gettime(CLOCK_REALTIME, &ts);
    tm_info = localtime(&ts.tv_sec);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(timestamp + strlen(timestamp), sizeof(timestamp) - strlen(timestamp), ".%03ld", ts.tv_nsec / 1000000);

    snprintf(str, sizeof(str), "%s,%u,%u,%u,%u,%u,%u,%u,%.4f,%.4f",
             timestamp,
             pSensors->FrameID,
             pSensors->PIR_ID,
             pSensors->PIR_value,
             pSensors->LOAD_ID,
             pSensors->LoadSensor_status,
             pSensors->IR_ID,
             pSensors->IRSensor_status,
             pSensors->Temperature_value,
             pSensors->loadcell_Value);
    return str;
}
