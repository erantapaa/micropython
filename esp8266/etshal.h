#ifndef _INCLUDED_ETSHAL_H_
#define _INCLUDED_ETSHAL_H_

void ets_isr_unmask();
void ets_install_putc1();
void ets_isr_attach();
void uart_div_modify();
void ets_isr_unmask();
void ets_isr_mask(unsigned);
extern void ets_delay_us(int us);

#endif // _INCLUDED_ETSHAL_H_
