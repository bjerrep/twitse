[start page](../README.md)

# Twitse - Hardware

## I2C - DAC

Run

```
i2cdetect -y 1
```

The device at 0x10 is the VCTCXO LTC2607 DAC in twitse default configuration. The DAC has 3 I2C address pins so the address can choosen to be in the range 0x10 to 0x17.

If there is a temperature sensor Si7054 then it will be at 0x40 or 0x70 dependent on hardware version.

```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- -- 
10: 10 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- 49 -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- 73 -- -- -- --
```



A hifiberry DAC2 will have something at 0x60 and something UU at 0x4D.

