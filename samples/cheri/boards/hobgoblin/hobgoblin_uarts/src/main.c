/*
 * Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>


#ifdef CHERI_TEST_UART_INTERRUPTS
/* call back function for testing uart with interrupts */
static void uart_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev)) {
		while (uart_fifo_read(dev, &c, 1)) {
			printk("RX: %c\n", c);
		}
	}
}
#endif

void main(void)
{

#ifndef CHERI_TEST_UART_INTERRUPTS
/* Use polling to read in characters from the uart console and print */
	const struct device *dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	printf("UART test for %s\n", CONFIG_BOARD_TARGET);
	printf("Enter some characters for polling\n\n");

	if (!device_is_ready(dev)) {
		printf("console device not ready\n");
	return;
	}

	while (1) {
		unsigned char c;

		if (uart_poll_in(dev, &c) == 0) {
			printf("RX: %c\n", c);
		}
	}
#endif

#ifdef CHERI_TEST_UART_INTERRUPTS
/* Use interrupts to read in characters from the uart console and print */
	const struct device *dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	printf("UART test for %s\n", CONFIG_BOARD_TARGET);
	printf("Enter some characters using interrupts\n\n");

	if (!device_is_ready(dev)) {
		printk("console device not ready\n");
	return;
	}

	int ret = uart_irq_callback_user_data_set(dev, uart_cb, NULL);

	uart_irq_rx_enable(dev);

	while (1) {
		k_sleep(K_FOREVER);
	}
#endif
}
