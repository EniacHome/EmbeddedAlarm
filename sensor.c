#include "sensor.h"

#include <xc.h>

char get_port_pin(unsigned char *port, unsigned char pin){
    return (*port & (1U << pin));
}

void enable_debounce(Sensor *sensor){
    sensor->debounce.should_debounce = 1;                                        //This needs to be processed after debounce
    sensor->debounce.previous_state = get_port_pin(sensor->debounce.port, sensor->debounce.pin);  //Save previous value
}

void process_debounce(Sensor *sensor){
    if(sensor->debounce.should_debounce &&                                       //If sensor should debounce and debounce successful
            sensor->debounce.previous_state == get_port_pin(sensor->debounce.port, sensor->debounce.pin))
    {
        sensor->handler(sensor);                                                      //Handle 
        sensor->debounce.should_debounce = 0;                                    //Mark as handled
    }
}

