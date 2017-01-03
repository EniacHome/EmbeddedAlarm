#ifndef SENSOR_H
#define	SENSOR_H

#include <xc.h>

typedef struct sensor_t{
    unsigned char index;
    unsigned char pin;
    unsigned char *port;
    void (*handler)(struct sensor_t *sensor);
    
    struct debounce_t {
        unsigned char should_debounce       : 1;
        unsigned char previous_state        : 1;
    } debounce;
} Sensor;

unsigned char Sensor_get_value(Sensor *sensor);

//Debounce
void Sensor_pre_debounce(Sensor *sensor);
void Sensor_post_debounce(Sensor *sensor);
#endif

