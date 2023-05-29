#ifndef GP_MCU2_EVENT_H
#define GP_MCU2_EVENT_H

typedef enum{
    EVENT_NULL = 0,
    EVENT_READ_DATA = 1,
    EVENT_CONTROL_SERVO = 2,
    EVENT_SEND_DATA = 3,
    EVENT_LORA_RECV = 4
} event_t;

#endif //GP_MCU2_EVENT_H
