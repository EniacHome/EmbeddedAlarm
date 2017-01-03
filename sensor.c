#include "sensor.h"

#include <xc.h>

char Sensor_get_value(Sensor *sensor){
    return (*sensor->port & (1U << sensor->pin));
}

void Sensor_pre_debounce(Sensor *sensor){
    sensor->debounce.should_debounce = 1;                                       //This needs to be processed after debounce
    sensor->debounce.previous_state = Sensor_get_value(sensor);                     //Save previous value
}

void Sensor_post_debounce(Sensor *sensor){
    if(sensor->debounce.should_debounce &&                                      //If sensor should debounce and debounce successful
            sensor->debounce.previous_state == Sensor_get_value(sensor))
    {
        sensor->handler(sensor);                                                //Handle 
        sensor->debounce.should_debounce = 0;                                   //Mark as handled
    }
}

