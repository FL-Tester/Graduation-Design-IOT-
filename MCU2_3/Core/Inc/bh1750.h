//
// Created by A9201 on 2023/4/12.
//

#ifndef GP_MCU2_BH1750_H
#define GP_MCU2_BH1750_H

#define BHAddWrite     0x46
#define BHAddRead      0x47
#define BHPowDown      0x00
#define BHPowOn        0x01
#define BHReset        0x07
#define BHModeH1       0x10
#define BHModeH2       0x11
#define BHModeL        0x13
#define BHSigModeH     0x20
#define BHSigModeH2    0x21
#define BHSigModeL     0x23

void bh1750_init(void);
int bh1750_read(void);
#endif //GP_MCU2_BH1750_H
