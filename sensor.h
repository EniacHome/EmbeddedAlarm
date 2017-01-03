#ifndef SENSOR_H
#define	SENSOR_H

#include <xc.h>

typedef struct sensor_t{
    unsigned char index;
    void (*handler)(struct sensor_t *sensor);
    
    struct debounce_t {
        unsigned char should_debounce     : 1;
        unsigned char previous_state      : 1;
        unsigned char pin                 : 3;
        unsigned char *port;
    } debounce;
} Sensor;

//Debounce
void enable_debounce(Sensor *sensor);
void process_debounce(Sensor *sensor);
#endif

