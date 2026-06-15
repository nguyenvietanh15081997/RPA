#include "i2c_master.h"


#define SLAVE_ADDR_1  		0x2D // địa chỉ slave
#define SLAVE_ADDR_2  		0x2E // địa chỉ slave

enum{
    ADDR_CTRL_POWER_POS = 0x01,
    ADDR_STT_BUTTON = 0x06,
    ADDR_STT_GPIO_POS0 = 0x07,
    ADDR_STT_GPIO_POS1 = 0x08,
    ADDR_STT_GPIO_POS2 = 0x09,
    ADDR_MAX = 0xFF
};




uint8_t GetSttGroupRelayPos(uint8_t pos)
{
    return i2c_read_register(ADDR_STT_GPIO_POS0 + pos, SLAVE_ADDR_1);
}

void SetPowerOnPos(uint8_t pos)
{
    ESP_LOGI("RelayI2C","Set power on pos %d", pos);
    uint8_t power_stt_pos[3] = {0};
    power_stt_pos[pos] = 1;
    uint8_t data = 0;
    data |= power_stt_pos[0] << 0;
    data |= power_stt_pos[1] << 1;
    data |= power_stt_pos[2] << 2;

    uint8_t data_send[2] = {ADDR_CTRL_POWER_POS, data};
    i2c_send_bytes(&data_send[0],2, SLAVE_ADDR_1);
}


