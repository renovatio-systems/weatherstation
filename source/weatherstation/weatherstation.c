#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include <mosquitto.h>
#include <libconfig.h>

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

static void mqtt_setup( void );
static void mosq_log_callback(struct mosquitto *mosq, void *userdata, int32_t level, const char *str);
static int32_t mqtt_send(int32_t field, char *msg);
static bool read_configuration_str(config_t *cfg, const char *key, char *value);
static bool read_configuration_int(config_t *cfg, const char *key, int *value);

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

    config_destroy(&cfg);

    return true;
}

int main(int argc, char *argv[])
{
    char *buf = malloc(64);
    int32_t i = 500;

    /* Setup Weather Station Configuration */
    initialize_weatherstation_configuration();

    /* Setup MQTT */
    mqtt_setup();

    while(1)
    {
        i+=100;
        sprintf(buf,"%i",i);
        int32_t snd = mqtt_send(3, buf);

        if(snd != 0)
            printf("mqtt_send error=%i\n", snd);
        
        printf("Sending %s\r\n", buf);
        sleep(30);
    }
}