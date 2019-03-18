#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include <mosquitto.h>
#include <libconfig.h>

#include <opt3001.h>
#include <ccs811.h>
#include <sht31.h>

#define WEATHERSTATION_CONFIG_FILE  "/etc/weatherstation.cfg"
#define MAX_STRING_SIZE             128

static struct mosquitto *mosq = NULL;
static char topic[1024] = {0};

static int32_t global_port = 1883;
static char global_host[MAX_STRING_SIZE] = "mqtt.thingspeak.com";
static char global_username[MAX_STRING_SIZE];
static char global_password[MAX_STRING_SIZE];
static char global_identity[MAX_STRING_SIZE];
static char global_channel[MAX_STRING_SIZE];
static char global_writeapikey[MAX_STRING_SIZE];

static int32_t global_opt3001_bus = 1;
static int32_t global_opt3001_addr = 0x46;

static int32_t global_sht31_bus = 1;
static int32_t global_sht31_addr = 0x44;

static int32_t global_ccs811_bus = 1;
static int32_t global_ccs811_addr = 0x5A;

static int32_t global_sampling_interval = 30;
static int32_t global_sht31_handle = 0;

static void mqtt_setup( void );
static void mosq_log_callback(struct mosquitto *mosq, void *userdata, int32_t level, const char *str);
static int32_t mqtt_send(int32_t field, char *msg);
static bool read_configuration_str(config_t *cfg, const char *key, char *value);
static bool read_configuration_int(config_t *cfg, const char *key, int *value);
static void weatherstation(bool opt3001_detected, bool sht31_detected, bool ccs811_detected);
static void read_opt3001(void);
static void read_sht31(void);
static void read_ccs811(void);
static void weatherstation(bool opt3001_detected, bool sht31_detected, bool ccs811_detected);

static bool read_configuration_str(config_t *cfg, const char *key, char *value)
{
    const char *str;

    /* Get the store name. */
    if(config_lookup_string(cfg, key, &str))
    {
        snprintf(value, MAX_STRING_SIZE, "%s", str);
        return true;
    }
    else
    {
        fprintf(stderr, "No %s setting in configuration file.\n", key);
    }
    return false;
}

static bool read_configuration_int(config_t *cfg, const char *key, int *value)
{
    int param;

    /* Get the store name. */
    if(config_lookup_int(cfg, key, &param))
    {
        *value = param;
        return true;
    }
    else
    {
        fprintf(stderr, "No %s setting in configuration file.\n", key);
    }
    return false;
}

static void mosq_log_callback(struct mosquitto *mosq, void *userdata, int32_t level, const char *str)
{
    /* Pring all log messages regardless of level. */
    switch(level)
    {
    //case MOSQ_LOG_DEBUG:
    //case MOSQ_LOG_INFO:
    //case MOSQ_LOG_NOTICE:
    case MOSQ_LOG_WARNING:
    case MOSQ_LOG_ERR:
    {
        printf("%i:%s\n", level, str);
    }
    }
}

static void mqtt_setup( void )
{

    int32_t keepalive = 60;
    bool clean_session = true;

    printf("%s@%s:%d\r\n", global_username, global_host, global_port);
    printf("Identity %s\r\n", global_identity);
    printf("Channel %s\r\n", global_channel);
    printf("Write API Key %s\r\n", global_writeapikey);

    mosquitto_lib_init();

    mosq = mosquitto_new(global_identity, clean_session, NULL);
    if(!mosq)
    {
        fprintf(stderr, "Error: Out of memory.\n");
        exit(1);
    }

    mosquitto_username_pw_set(mosq, global_username, global_password);

    mosquitto_log_callback_set(mosq, mosq_log_callback);

    if(mosquitto_connect(mosq, global_host, global_port, keepalive))
    {
        fprintf(stderr, "Unable to connect.\n");
        exit(1);
    }
    int32_t loop = mosquitto_loop_start(mosq);
    if(loop != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "Unable to start loop: %i\n", loop);
        exit(1);
    }


}

static int32_t mqtt_send(int32_t field, char *msg)
{

    /* Setup Topic */
    snprintf(topic, sizeof(topic), "channels/%s/publish/fields/field%d/%s",
            global_channel,
            field,
            global_writeapikey
        );

    return mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, 0);
}

static bool initialize_weatherstation_configuration(void)
{
    config_t cfg;

    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if(! config_read_file(&cfg, WEATHERSTATION_CONFIG_FILE) )
    {
        fprintf(stderr, "%s:%d _ %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return false;
    }

    read_configuration_str(&cfg, "host", global_host);
    read_configuration_int(&cfg, "port", &global_port);
    read_configuration_str(&cfg, "username", global_username);
    read_configuration_str(&cfg, "password", global_password);
    read_configuration_str(&cfg, "identity", global_identity);
    read_configuration_str(&cfg, "channel", global_channel);
    read_configuration_str(&cfg, "writeapikey", global_writeapikey);
    read_configuration_int(&cfg, "opt3001_bus", &global_opt3001_bus);
    read_configuration_int(&cfg, "opt3001_addr", &global_opt3001_addr);
    read_configuration_int(&cfg, "sht31_bus", &global_sht31_bus);
    read_configuration_int(&cfg, "sht31_addr", &global_sht31_addr);
    read_configuration_int(&cfg, "ccs811_bus", &global_ccs811_bus);
    read_configuration_int(&cfg, "ccs811_addr", &global_ccs811_addr);
    config_destroy(&cfg);

    return true;
}

int main(int argc, char *argv[])
{
    int32_t status;
    char i2c_dev[32];
    bool opt3001_detected = true;
    bool sht31_detected = true;
    bool ccs811_detected = true;

    /* Setup Weather Station Configuration */
    initialize_weatherstation_configuration();

    /* Setup Sensors */
    snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", global_opt3001_bus);
    status = opt3001_init(i2c_dev, global_opt3001_addr);
    if(status != 0)
    {
        printf("Cannot setup Digital ambient light sensor (ALS) opt3001\r\n");
        opt3001_detected = false;
    }

    snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", global_sht31_bus);
    global_sht31_handle = initializeSensor(i2c_dev, global_sht31_addr);
    if(global_sht31_handle < 0)
    {
        printf("Cannot setup Digital Humidity Sensor SHT3x (RH/T)\r\n");
        sht31_detected = false;        
    }

    status = ccs811Init(global_ccs811_bus, global_ccs811_bus);
    if(status != 0)
    {
        printf("Cannot setup Digital Gas Sensor for Monitoring Indoor Air Quality\r\n");
        ccs811_detected = false;
    }

    /* Setup MQTT */
    mqtt_setup();

    /* Weather Station Loop */
    weatherstation(opt3001_detected, sht31_detected, ccs811_detected);

}

static void read_opt3001(void)
{
    int32_t status = 0;
    char buf[64];
    float lux;

    status = opt3001_read(&lux);
    if(status != 0)
    {
        printf("opt3001 opt3001_read error=%i\r\n", status);
        return;        
    }

    snprintf(buf, sizeof(buf), "%.4f", lux);
    status = mqtt_send(1, buf);
    if(status != 0)
        printf("opt3001 mqtt_send error=%i\r\n", status);
    else
        printf("opt3001 sensor read & send success\r\n");      
}

static void read_sht31(void)
{
    int32_t status = 0;
    struct sensorOutput result;
    char buf[64];

    result = parseEnvData(global_sht31_handle);

    if ( (result.temperature == SENSOR_NA) || (result.humidity == SENSOR_NA) )
    {
        printf("sht31 parseEnvData error\r\n");
        return;
    }

    snprintf(buf, sizeof(buf), "%.1f", result.temperature);
    status = mqtt_send(2, buf);
    if(status != 0)
        printf("sht31 (T) mqtt_send error=%i\r\n", status);            
    else
        printf("sht31 (T) sensor read & send success\r\n");      

    sleep(5);

    snprintf(buf, sizeof(buf), "%.1f", result.humidity);
    status = mqtt_send(3, buf);
    if(status != 0)
        printf("sht31 (H) mqtt_send error=%i\r\n", status);   
    else
        printf("sht31 (H) sensor read & send success\r\n");      


}

static void read_ccs811(void)
{
    int32_t status = 0;
    int32_t eCO2 = 0, TVOC = 0;
    char buf[64];
    
    status = ccs811ReadValues(&eCO2, &TVOC);
    if(status)
    {
        snprintf(buf, sizeof(buf), "%d", eCO2);
        status = mqtt_send(4, buf);
        if(status != 0)
            printf("ccs811 (eCO2) mqtt_send error=%i\r\n", status);            
        else
            printf("ccs811 (eCO2) sensor read & send success\r\n");      

        sleep(5);

        snprintf(buf, sizeof(buf), "%d", TVOC);
        status = mqtt_send(5, buf);
        if(status != 0)
            printf("ccs811 (TVOC) mqtt_send error=%i\r\n", status);            
        else
            printf("ccs811 (TVOC) sensor read & send success\r\n");      

    }
    else
    {
        printf("ccs811 data not ready\r\n");
    }
}

static void weatherstation(bool opt3001_detected, bool sht31_detected, bool ccs811_detected)
{

    while(1)
    {
        if(opt3001_detected)
        {
            read_opt3001();
        }

        if(sht31_detected)
        {
            read_sht31();
        }

        if(ccs811_detected)
        {
            read_ccs811();
        }

        
        sleep(global_sampling_interval);
    }

}