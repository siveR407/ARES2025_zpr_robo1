/*
 * Copyright (c) 2021-2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* Devicetree */
#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)
static void can_tx_callback(const struct device *dev, int error, void *user_data)
{
	struct k_sem *tx_queue_sem = user_data;

	k_sem_give(tx_queue_sem);
}

void rx_callback_function(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct my_rx_data *rx_data = (struct my_rx_data *)user_data;
	printf("1");
         
}
struct my_rx_data {
    int device_id;
    uint8_t rx_buffer[64];
    // 其他需要的数据字段...
};
int main(void)
{

	const struct device *dev = DEVICE_DT_GET(CANBUS_NODE);
	struct k_sem tx_queue_sem;
	struct can_frame frame = {0};
	const struct can_filter my_filter = {
        .flags = 0U,
        .id = 0x201,
        .mask = CAN_STD_ID_MASK
    };
	 
	int filter_id;
	int err;
    
	k_sem_init(&tx_queue_sem, CONFIG_SAMPLE_CAN_BABBLING_TX_QUEUE_SIZE,
		   CONFIG_SAMPLE_CAN_BABBLING_TX_QUEUE_SIZE);

	if (!device_is_ready(dev)) {
		printk("CAN device not ready");
		return 0;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_CAN_BABBLING_FD_MODE)) {
		err = can_set_mode(dev, CAN_MODE_FD);
		if (err != 0) {
			printk("Error setting CAN FD mode (err %d)", err);
			return 0;
		}
	}

	err = can_start(dev);
	if (err != 0) {
		printk("Error starting CAN controller (err %d)", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_CAN_BABBLING_EXT_ID)) {
		frame.flags |= CAN_FRAME_IDE;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_CAN_BABBLING_RTR)) {
		frame.flags |= CAN_FRAME_RTR;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_CAN_BABBLING_FD_MODE)) {
		frame.flags |= CAN_FRAME_FDF;
	}
	
	frame.id = 0x200;
	frame.data[0]=2000>>8;
	frame.data[1]=2000&0xFF;
	

	struct my_rx_data callback_arg = {
    .device_id = 123,
    .rx_buffer = {0}  // 初始化缓冲区
};

	filter_id = can_add_rx_filter(dev, rx_callback_function, (void*)&callback_arg, &my_filter);
	while (true) {
		if (k_sem_take(&tx_queue_sem, K_MSEC(100)) == 0) {
			err = can_send(dev, &frame, K_NO_WAIT, can_tx_callback, &tx_queue_sem);
		}



	}
}

