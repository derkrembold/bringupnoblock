#ifndef SLAVE__HPP_
#define SLAVE__HPP_

// LED Pins
#define BLUE_LED_PIN PB0
#define GREEN_LED_PIN PB1
#define RED_LED_PIN PB2

// Head Pins
#define HEAD_0_PIN PC0
#define HEAD_1_PIN PC1


uint8_t iam();
void led(uint8_t, uint8_t);
void writeBitC(uint8_t, uint8_t);

#endif
