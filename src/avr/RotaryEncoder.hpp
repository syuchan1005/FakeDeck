#include <avr/io.h>

#define RE1_PR_DIR_REG DDRA
#define RE1_PR_OUT_REG PORTA
#define RE1_PR_DATA_REG PINA
#define RE1_PR_DIR_REG_S DDRD
#define RE1_PR_OUT_REG_S PORTD
#define RE1_PR_DATA_REG_S PIND
#define RE1_A PA1
#define RE1_B PA0
#define RE1_S PD2

#define RE2_PR_DIR_REG DDRD
#define RE2_PR_OUT_REG PORTD
#define RE2_PR_DATA_REG PIND
#define RE2_A PD3
#define RE2_B PD4
#define RE2_S PD5

#define RE3_PR_DIR_REG DDRB
#define RE3_PR_OUT_REG PORTB
#define RE3_PR_DATA_REG PINB
#define RE3_A PB4
#define RE3_B PB3
#define RE3_S PB2

#define RE4_PR_DIR_REG DDRB
#define RE4_PR_OUT_REG PORTB
#define RE4_PR_DATA_REG PINB
#define RE4_PR_DIR_REG_S DDRD
#define RE4_PR_OUT_REG_S PORTD
#define RE4_PR_DATA_REG_S PIND
#define RE4_A PB1
#define RE4_B PB0
#define RE4_S PD6

void initRotaryEncoders()
{
    // config rotary encoder pins, input_pullup
    RE1_PR_DIR_REG &= ~(1 << RE1_A) | ~(1 << RE1_B);
    RE1_PR_OUT_REG |= (1 << RE1_A) | (1 << RE1_B);
    RE1_PR_DIR_REG_S &= ~(1 << RE1_S);
    RE1_PR_OUT_REG_S |= (1 << RE1_S);

    RE2_PR_DIR_REG &= ~(1 << RE2_A) | ~(1 << RE2_B) | ~(1 << RE2_S);
    RE2_PR_OUT_REG |= (1 << RE2_A) | (1 << RE2_B) | (1 << RE2_S);

    RE3_PR_DIR_REG &= ~(1 << RE3_A) | ~(1 << RE3_B) | ~(1 << RE3_S);
    RE3_PR_OUT_REG |= (1 << RE3_A) | (1 << RE3_B) | (1 << RE3_S);

    RE4_PR_DIR_REG &= ~(1 << RE4_A) | ~(1 << RE4_B);
    RE4_PR_OUT_REG |= (1 << RE4_A) | (1 << RE4_B);
    RE4_PR_DIR_REG_S &= ~(1 << RE4_S);
    RE4_PR_OUT_REG_S |= (1 << RE4_S);
}

void createRotaryEncoderData(
    uint8_t offset,
    uint16_t rePinDataReg, uint8_t aPin, uint8_t bPin,
    uint16_t reSPinDataReg, uint8_t sPin,
    uint16_t *data, uint16_t *oldData, uint16_t *sendData)
{
    // read pin data
    if (rePinDataReg & (1 << aPin))
        *data |= 1 << (0 + offset);
    if (rePinDataReg & (1 << bPin))
        *data |= 1 << (1 + offset);
    if (~reSPinDataReg & (1 << sPin))
        *sendData |= 1 << (2 + offset);

    // old A and old B are both high
    if ((*oldData & (3 << offset)) == (3U << offset))
    {
        if ((*data & (3 << offset)) == (1U << offset))
        {
            // A is high and B is low - Clockwise
            *sendData |= 1 << offset;
        }
        else if ((*data & (3 << offset)) == (2U << offset))
        {
            // A is low and B is high - Counter-clockwise
            *sendData |= 1 << (1 + offset);
        }
    }
}