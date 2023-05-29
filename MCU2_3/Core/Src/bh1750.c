#include "bh1750.h"
#include "i2c.h"
#include "stdio.h"

void bh1750_init(void){
    uint8_t data;
    data = BHPowOn;
    HAL_I2C_Master_Transmit(&hi2c1, BHAddWrite, &data, 1, 100);
    data = BHModeH1;
    HAL_I2C_Master_Transmit(&hi2c1, BHAddWrite, &data, 1, 100);
//    data = BHReset;
//    HAL_I2C_Master_Transmit(&hi2c1, BHAddWrite, &data, 1, 100);
}
int bh1750_read(void){
    uint8_t data[2];
    if ( HAL_I2C_Master_Receive(&hi2c1, BHAddRead, data, 2, 100) != HAL_OK){
        printf("bh1750 read error\n");
        return -1;
    }
    int lux = (data[0] << 8 | data[1]) / 1.2;
    return lux;
}