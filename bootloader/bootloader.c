/*
 * Copyright (C) 2023 GP Orcullo <kinsamanka@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "hw.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
 
#ifndef FLASH_BOOTLDR_SIZE
#define FLASH_BOOTLDR_SIZE                  0x1000
#endif
#define APP_ADDR                            (FLASH_BASE + FLASH_BOOTLDR_SIZE)

#ifdef RUN_LED
#ifdef ERR_LED
const struct leds led = ERR_LED;
#else
const struct leds led = RUN_LED;
#endif
#endif

#define SEND_ACK(p)     do { \
                            usart_send_blocking(p, 0x79); \
                        } while(0)

#define SEND_NACK(p)    do { \
                            usart_send_blocking(p, 0x1F); \
                        } while(0)

void sys_tick_handler(void)
{
#ifdef RUN_LED
    gpio_toggle(led.gpio.port, led.gpio.pin);
#endif
}

static void systick_setup(void)
{
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_frequency(20, rcc_ahb_frequency);
    systick_counter_enable();
    systick_interrupt_enable();
}

void clock_setup(void)
{
    rcc_clock_setup_in_hsi_out_48mhz();

    /* Enable Periperal clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_GPIOE);
#ifdef RCC_GPIOF
    rcc_periph_clock_enable(RCC_GPIOF);
#endif
#ifdef RCC_GPIOG
    rcc_periph_clock_enable(RCC_GPIOG);
#endif
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_AFIO);
}

void gpio_setup(void)
{
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO9);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO10);
#ifdef RUN_LED
    gpio_set_mode(led.gpio.port, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, led.gpio.pin);
#endif
}

static void usart_setup(void)
{
    /* Setup USART parameters. */
    usart_set_baudrate(USART1, UPLOAD_SPEED);
    usart_set_databits(USART1, 9);
    usart_set_parity(USART1, USART_PARITY_EVEN);
    usart_set_stopbits(USART1, USART_CR2_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);
}

static void send(uint32_t usart, char *src, size_t count)
{
    char *srcb = src;
    while (count--)
        usart_send_blocking(usart, *srcb++);
}

static int get_addr(uint32_t * addr)
{
    char cs = 0;

    for (int i = 3; i >= 0; i--)
        cs ^= ((char *)addr)[i] = usart_recv_blocking(USART1);

    /* checksum */
    cs ^= usart_recv_blocking(USART1);

    return cs;
}

static void run_app(void)
{
    /* Set vector table base address */
    SCB_VTOR = APP_ADDR & 0xFFFF;

    /* Initialise master stack pointer. */
    asm volatile ("msr msp, %0"::"g" (*(volatile uint32_t *)APP_ADDR));
    /* Jump to application. */
    (*(void (**)())(APP_ADDR + 4)) ();
}

enum task_state {
    STATE_WAIT,
    STATE_GET,
    STATE_GV,
    STATE_GID,
    STATE_RM,
    STATE_GO,
    STATE_WM,
    STATE_ER,
    STATE_MAGIC,
};

static const uint32_t *prog_size = (uint32_t *) (APP_ADDR + 0x20);
uint32_t flash_size, end_addr;
extern volatile uint32_t _BOOTFLAG;

/* This will ensure that the previous loaded program will be erased
 * when using stlink
 */
const uint32_t codeVersion[8] __attribute__ ((section(".eeprom"))) =
    { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

int main(void)
{
    flash_size = (*(uint16_t*)DESIG_FLASH_SIZE_BASE) << 10;
    end_addr = FLASH_BASE + flash_size;

    (void) codeVersion;

    if ((_BOOTFLAG != 0xDEADBEEF)
            && (*prog_size < (flash_size - FLASH_BOOTLDR_SIZE))
            && ((*(volatile uint32_t *)APP_ADDR & 0x2FFE0000) == 0x20000000)) {

        /* calculate checksum */
        uint32_t cksum = 0;
        const uint32_t *const base_addr = (uint32_t *) APP_ADDR;
        for (unsigned i = 0; i < *prog_size; i++)
            cksum ^= base_addr[i];

        if (cksum == 0) {
            run_app();
        }
    }

    _BOOTFLAG = 0;

    clock_setup();
    gpio_setup();
    systick_setup();

    usart_setup();

    enum task_state state = STATE_WAIT;
    char c, d;
    char buf[256];

    for (;;) {
        switch (state) {

        case STATE_WAIT:
            c = usart_recv_blocking(USART1);

            if (c == 0x7F) {
                SEND_ACK(USART1);
                break;
            }

            c = (usart_recv_blocking(USART1) & 0xFF) - c;

            if (c == 0xFF) {            /* CMD 0x00FF */
                state = STATE_GET;
            } else if (c == 0xFD) {     /* CMD 0x01FE */
                state = STATE_GV;
            } else if (c == 0xFB) {     /* CMD 0x02FD */
                state = STATE_GID;
            } else if (c == 0xDD) {     /* CMD 0x11EE */
                state = STATE_RM;
            } else if (c == 0xBD) {     /* CMD 0x21DE */
                state = STATE_GO;
            } else if (c == 0x9D) {     /* CMD 0x31CE */
                state = STATE_WM;
            } else if (c == 0x79) {     /* CMD 0x43BC */
                state = STATE_ER;
            } else if (c == 0x00) {     /* CMD \xff\xffBOOTLOADER\xff\xff */
                state = STATE_MAGIC;
            } else {
                SEND_NACK(USART1);
                break;
            }
            break;

        case STATE_GET:
            SEND_ACK(USART1);
            send(USART1, "\x07\x10\x00\x01\x02\x11\x21\x31\x43", 9);
            SEND_ACK(USART1);
            state = STATE_WAIT;
            break;

        case STATE_GV:
            SEND_ACK(USART1);
            send(USART1, "\x10\x00\x00", 3);
            SEND_ACK(USART1);
            state = STATE_WAIT;
            break;

        case STATE_GID:
            SEND_ACK(USART1);
            send(USART1, "\x01\x04\x10", 3);
            SEND_ACK(USART1);
            state = STATE_WAIT;
            break;

        case STATE_RM:
            SEND_ACK(USART1);

            uint32_t address = 0;

            if (get_addr(&address) != 0) {
                SEND_NACK(USART1);
                state = STATE_WAIT;
                break;
            } else {
                SEND_ACK(USART1);
            }

            c = usart_recv_blocking(USART1);

            /* ignore checksum */
            d = usart_recv_blocking(USART1);

            SEND_ACK(USART1);

            volatile char *a = (char *)address;

            for (int i = 0; i <= c; i++)
                usart_send_blocking(USART1, a[i]);

            state = STATE_WAIT;
            break;

        case STATE_GO:
            SEND_ACK(USART1);

            address = 0;
            get_addr(&address);

            /* ignore checksum */
            SEND_ACK(USART1);

            /* wait until tx complete */
            while (usart_get_flag(USART1, USART_FLAG_TC) == 0) ;

            scb_reset_system();
            break;

        case STATE_WM:
            SEND_ACK(USART1);

            address = 0;

            if (get_addr(&address) != 0) {
                SEND_NACK(USART1);
                state = STATE_WAIT;
                break;
            } else {
                SEND_ACK(USART1);
            }

            c = usart_recv_blocking(USART1);
            d = c;
            for (int i = 0; i <= c; i++)
                d ^= buf[i] = usart_recv_blocking(USART1);

            /* checksum */
            d ^= usart_recv_blocking(USART1);

            if (d != 0) {
                SEND_NACK(USART1);
                state = STATE_WAIT;
                break;
            }

            flash_unlock();
            for (int i = 0; i < ((c >> 1) + 1); i++)
                if (address >= APP_ADDR && address < end_addr) {

                    flash_program_half_word(address,
                                            *(uint16_t *) & buf[i << 1]);
                    address += 2;
                }
            flash_lock();

            SEND_ACK(USART1);
            state = STATE_WAIT;
            break;

        case STATE_ER:
            SEND_ACK(USART1);

            c = usart_recv_blocking(USART1);

            if (c == 0xFF) {

                c = usart_recv_blocking(USART1);

            } else {
                d = buf[0] = c;
                for (int i = 1; i < c + 3; i++) {
                    d ^= buf[i] = usart_recv_blocking(USART1);
                }

                /* checksum */
                if (d != 0) {
                    SEND_NACK(USART1);
                    state = STATE_WAIT;
                    break;
                }
            }

            static int erase_done = 0;

            /* always perform full flash erase */
            if (!erase_done) {

                flash_unlock();

                for (uint32_t addr = APP_ADDR; addr < end_addr; addr += 1024)
                    flash_erase_page(addr);

                flash_lock();

                erase_done = 1;
            }

            SEND_ACK(USART1);
            state = STATE_WAIT;
            break;

        case STATE_MAGIC:
            /* ignore magic string */
            for (int i = 0; i < 12; i++)
                c = usart_recv_blocking(USART1);

            state = STATE_WAIT;
            break;
        }
    }

    return 0;
}
