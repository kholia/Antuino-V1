#ifndef SI5351_H
#define SI5351_H

void si5351aOutputOff(uint8_t clk);
void si5351aSetFrequency_clk0(uint32_t frequency);
void si5351aSetFrequency_clk1(uint32_t frequency);
void si5351aSetFrequency_clk2(uint32_t frequency);

#endif /* SI5351_H */
