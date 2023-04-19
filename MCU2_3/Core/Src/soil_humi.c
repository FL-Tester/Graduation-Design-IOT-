#include "soil_humi.h"

uint16_t soil_humi_read(void){
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint16_t adc_value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    adc_value = -0.03968253968253968*adc_value + 159.52380952380952;
    if (adc_value < 0)
      adc_value = 0;
    if (adc_value > 100)
      adc_value = 100;
    return adc_value;
}