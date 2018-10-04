/* arch/arm/mach-msm/board-gelato-input.c
 * Copyright (C) 2009 LGE, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/types.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_event.h>
#include <linux/keyreset.h>
#include <linux/synaptics_i2c_rmi.h> // LGE_DEV_PORTING GELATO [edward1.kim@lge.com]_20110317
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/board.h>
#include <mach/board_lge.h>
#include <mach/rpc_server_handset.h>
#include <mach/pmic.h>//hskim0313_pull_down_test

#include "board-gelato.h"
#include "proc_comm.h" //hskim0313_pull_down_test
static int prox_power_set(unsigned char onoff);

/* LGE_S [ynj.kim@lge.com] 2010-05-15 : atcmd virtual device */
static unsigned short atcmd_virtual_keycode[ATCMD_VIRTUAL_KEYPAD_ROW][ATCMD_VIRTUAL_KEYPAD_COL] = {
	{KEY_1, 		KEY_8, 				KEY_Q,  	 KEY_I,          KEY_D,      	KEY_HOME,	KEY_B,          KEY_UP},
	{KEY_2, 		KEY_9, 		  		KEY_W,		 KEY_O,       	 KEY_F,		 	KEY_RIGHTSHIFT, 	KEY_N,			KEY_DOWN},
	{KEY_3, 		KEY_0, 		  		KEY_E,		 KEY_P,          KEY_G,      	KEY_Z,        	KEY_M, 			KEY_UNKNOWN},
	{KEY_4, 		KEY_BACK,  			KEY_R,		 KEY_SEARCH,     KEY_H,			KEY_X,    		KEY_LEFTSHIFT,	KEY_UNKNOWN},
	{KEY_5, 		KEY_BACKSPACE, 		KEY_T,		 KEY_LEFTALT,    KEY_J,      	KEY_C,     		KEY_REPLY,    KEY_CAMERA},
	{KEY_6, 		KEY_ENTER,  		KEY_Y,  	 KEY_A,		     KEY_K,			KEY_V,  	    KEY_RIGHT,     	KEY_CAMERAFOCUS},
	{KEY_7, 		KEY_MENU,	KEY_U,  	 KEY_S,    		 KEY_L, 	    KEY_SPACE,      KEY_LEFT,     	KEY_SEND},
	{KEY_UNKNOWN, 	KEY_UNKNOWN,  		KEY_UNKNOWN, KEY_UNKNOWN, 	 KEY_UNKNOWN,	KEY_UNKNOWN,    KEY_FOLDER_MENU,      	KEY_FOLDER_HOME},

};

static struct atcmd_virtual_platform_data atcmd_virtual_pdata = {
	.keypad_row = ATCMD_VIRTUAL_KEYPAD_ROW,
	.keypad_col = ATCMD_VIRTUAL_KEYPAD_COL,
	.keycode = (unsigned char *)atcmd_virtual_keycode,
};

static struct platform_device atcmd_virtual_device = {
	.name = "atcmd_virtual_kbd",
	.id = -1,
	.dev = {
		.platform_data = &atcmd_virtual_pdata,
	},
};
/* LGE_E [ynj.kim@lge.com] 2010-05-15 : atcmd virtual device */

/* head set device */
static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

static unsigned int keypad_row_gpios[] = {
	38, 37
};

// LGE_DEV_PORTING GELATO_DS_S ks82.jung@lge.com [2011-04-22]
u8 lge_ecom_accel_pwr_state;

#define COMPASS_POWER_STATE		0x02
#define ACCEL_POWER_STATE		0x01

static u8 get_ecom_accel_pwr_state(void)
{
	return lge_ecom_accel_pwr_state;
}
// LGE_DEV_PORTING GELATO_DS_E

#ifdef CONFIG_MSM_SOC_P698_EV_BOARD
static unsigned int keypad_col_gpios[] = {32, 33, 34};
#else
static unsigned int keypad_col_gpios[][3] = {
	[0] = { 35, 34, 33 },
};
#endif

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(keypad_row_gpios) + (row))

static const unsigned short keypad_keymap_gelato[] = {
		[KEYMAP_INDEX(0, 0)] = KEY_SIM_SWITCH,
		[KEYMAP_INDEX(0, 1)] = KEY_HOME,
		[KEYMAP_INDEX(1, 0)] = KEY_MENU,
	[KEYMAP_INDEX(1, 1)] = KEY_BACK,
		[KEYMAP_INDEX(2, 0)] = KEY_VOLUMEUP,
		[KEYMAP_INDEX(2, 1)] = KEY_VOLUMEDOWN,
};

int gelato_matrix_info_wrapper(struct gpio_event_input_devs *input_dev, struct gpio_event_info *info, void **data, int func)
{
        int ret ;
		if (func == GPIO_EVENT_FUNC_RESUME) {
			gpio_tlmm_config(GPIO_CFG(keypad_row_gpios[0], 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(keypad_row_gpios[1], 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		}
		//else if (func == GPIO_EVENT_FUNC_SUSPEND)
		//{
		//        gpio_tlmm_config(GPIO_CFG(keypad_col_gpios[2], 0,
		//				GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA), GPIO_CFG_DISABLE);
		//}

		ret = gpio_event_matrix_func(input_dev,info, data,func);
        return ret ;
}

static int gelato_gpio_matrix_power(
                const struct gpio_event_platform_data *pdata, bool on)
{
	/* this is dummy function to make gpio_event driver register suspend function
	 * 2010-01-29, cleaneye.kim@lge.com
	 * copy from ALOHA code
	 * 2010-04-22 younchan.kim@lge.com
	 */

	return 0;
}

static struct gpio_event_matrix_info gelato_keypad_matrix_info = {
	.info.func	= gelato_matrix_info_wrapper,
	.keymap		= NULL,
	.output_gpios	= NULL,
	.input_gpios	= keypad_row_gpios,
	.noutputs	= 0,
	.ninputs	= ARRAY_SIZE(keypad_row_gpios),
	.settle_time.tv.nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_DRIVE_INACTIVE
};

static void __init gelato_select_keymap(void)
{
	gelato_keypad_matrix_info.keymap = keypad_keymap_gelato;

	gelato_keypad_matrix_info.output_gpios = keypad_col_gpios[0];
	gelato_keypad_matrix_info.noutputs = ARRAY_SIZE(keypad_col_gpios[0]);

	return;
}

static struct gpio_event_info *gelato_keypad_info[] = {
	&gelato_keypad_matrix_info.info,
};

static struct gpio_event_platform_data gelato_keypad_data = {
	.name		= "gelato_keypad",
	.info		= gelato_keypad_info,
	.info_count	= ARRAY_SIZE(gelato_keypad_info),
	.power          = gelato_gpio_matrix_power,
};

struct platform_device keypad_device_gelato = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &gelato_keypad_data,
	},
};

/* keyreset platform device */
static int gelato_reset_keys_up[] = {
	KEY_HOME,
	0
};

static struct keyreset_platform_data gelato_reset_keys_pdata = {
	.keys_up = gelato_reset_keys_up,
	.keys_down = {
		//KEY_BACK,
		KEY_VOLUMEDOWN,
		KEY_SEARCH,
		0
	},
};

struct platform_device gelato_reset_keys_device = {
	.name = KEYRESET_NAME,
	.dev.platform_data = &gelato_reset_keys_pdata,
};

/* input platform device */
static struct platform_device *gelato_input_devices[] __initdata = {
	&hs_device,
	&keypad_device_gelato,
	//&gelato_reset_keys_device,
	&atcmd_virtual_device,
};

/* MCS6000 Touch */
static struct gpio_i2c_pin ts_i2c_pin[] = {
	[0] = {
		.sda_pin	= TS_GPIO_I2C_SDA,
		.scl_pin	= TS_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= TS_GPIO_IRQ,
	},
};

static struct i2c_gpio_platform_data ts_i2c_pdata = {
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay				= 2,
};

static struct platform_device ts_i2c_device = {
	.name	= "i2c-gpio",
	.dev.platform_data = &ts_i2c_pdata,
};

// LGE_DEV_PORTING GELATO [edward1.kim@lge.com]_20110317
static int ts_config_gpio(int config)
{
	if (config)
	{		/* for wake state */
		gpio_tlmm_config(GPIO_CFG(TS_GPIO_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
	else
	{		/* for sleep state */
		gpio_tlmm_config(GPIO_CFG(TS_GPIO_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		gpio_tlmm_config(GPIO_CFG(TS_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(TS_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

	return 0;
}
//hskim0313_pull_down_test
static int ts_set_pulldown(unsigned int onoff)
{
	int rc;
	unsigned id;

	id = PM_VREG_PDOWN_SYNT_ID;
	rc = msm_proc_comm(PCOM_VREG_PULLDOWN, &onoff, &id);
	if (rc != 0) {
		printk("[Touch] vreg_set_pulldown failed\n");
		return -1;
	}
	id = PM_VREG_PDOWN_MMC_ID;
	rc = msm_proc_comm(PCOM_VREG_PULLDOWN, &onoff, &id);
	if (rc != 0) {
		printk("[Touch] vreg_set_pulldown failed\n");
		return -1;
	}
	return 0;
}

static int ts_set_vreg(unsigned char onoff)
{
	struct vreg *vreg_touch = NULL;
	struct vreg *vreg_pullup = NULL;
	int rc;

	printk("[Touch] %s() onoff:%d\n",__FUNCTION__, onoff);

	vreg_touch = vreg_get(0, "synt");
	if (IS_ERR(vreg_touch)) {
		printk("[Touch] vreg_get fail : touch\n");
		return -1;
	}

	vreg_pullup = vreg_get(0, "mmc");
	if (IS_ERR(vreg_pullup)) {
		printk("[Touch] vreg_get fail : touch\n");
		return -1;
	}

	if (onoff) {
		ts_config_gpio(1);

		rc = vreg_set_level(vreg_touch, 3000);
		if (rc != 0) {
			printk("[TOUCH] vreg_set_level failed\n");
			return -1;
		}
		vreg_enable(vreg_touch);

		msleep(20);

		rc = vreg_set_level(vreg_pullup, 2600);
		if (rc != 0) {
			printk("[TOUCH] vreg_set_level failed\n");
			return -1;
		}
		vreg_enable(vreg_pullup);

		msleep(400);	// wait 400ms

		//ts_config_gpio(1);
	} else {
		ts_config_gpio(0);

		vreg_set_level(vreg_pullup, 0);
		vreg_disable(vreg_pullup);

		vreg_set_level(vreg_touch, 0);
		vreg_disable(vreg_touch);
		//ts_config_gpio(0);
	}
	return 0;
}


// LGE_DEV_PORTING GELATO [edward1.kim@lge.com]_20110317
static struct synaptics_i2c_rmi_platform_data ts_pdata_synaptics = {
	.version = 0x0,
	.irqflags = IRQF_TRIGGER_FALLING,
	.use_irq = true,
	.power = ts_set_vreg,
	.pulldown = ts_set_pulldown,//hskim0313_pull_down_test
};

static struct i2c_board_info ts_i2c_bdinfo_synaptics[] = {
	[0] = {
		I2C_BOARD_INFO("synaptics-rmi-ts", TS_I2C_SLAVE_ADDR),
		.type = "synaptics-rmi-ts",
		.platform_data = &ts_pdata_synaptics,
	},
};
// LGE_DEV_PORTING GELATO [edward1.kim@lge.com]_20110317

static void __init gelato_init_i2c_touch(int bus_num)
{
	ts_i2c_device.id = bus_num;
// LGE_DEV_PORTING GELATO [edward1.kim@lge.com]_20110317
	init_gpio_i2c_pin(&ts_i2c_pdata, ts_i2c_pin[0],	&ts_i2c_bdinfo_synaptics[0]);
	i2c_register_board_info(bus_num, &ts_i2c_bdinfo_synaptics[0], 1);
	platform_device_register(&ts_i2c_device);
}


// LGE_DEV_PORTING GELATO_DS_S ks82.jung@lge.com [2011-04-20]
/* acceleration */

/* k3dh */
#if defined(CONFIG_SENSOR_K3DH)
static int k3dh_device_id(void)
{
	return 0x33;
}

static int k3dh_config_gpio(int config)
{
	if (config) {	/* for wake state */// LGE_DEV_PORTING GELATO_DS_[edward1.kim@lge.com]_20110419
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	} else {		/* for sleep state */
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

	return 0;
}

/* kr3dh */
#elif defined(CONFIG_SENSOR_KR3DH)
static int kr3dh_device_id(void)
{
	return 0x33;
}

static int kr3dh_config_gpio(int config)
{
	if (config) {	/* for wake state */// LGE_DEV_PORTING GELATO_DS_[edward1.kim@lge.com]_20110419
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	} else {		/* for sleep state */
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

	return 0;
}


#endif
// LGE_DEV_PORTING GELATO_DS_E ks82.jung@lge.com [2011-04-20]

static int kr_init(void)
{
	return 0;
}

static void kr_exit(void)
{
}

static int accel_power_on(void)
{
	int ret = 0;
	struct vreg *rfrx2_vreg = vreg_get(0, "rfrx2");

	printk("[Accelerometer] %s() : Power On\n",__FUNCTION__);

	// LGE_DEV_PORTING GELATO_DS_S ks82.jung@lge.com [2011-04-22]

/* k3dh */
#if defined(CONFIG_SENSOR_K3DH)
        k3dh_config_gpio(1);
/* kr3dh */
#elif defined(CONFIG_SENSOR_KR3DH)
	kr3dh_config_gpio(1);
#endif

	// compass pwr chek, if compass power off - power state change
	if(!(lge_ecom_accel_pwr_state & COMPASS_POWER_STATE))
	{
	vreg_set_level(rfrx2_vreg, 2600);
	vreg_enable(rfrx2_vreg);
	}

	// check bit update (on)
	lge_ecom_accel_pwr_state |= ACCEL_POWER_STATE;

	return ret;
}

static int accel_power_off(void)
{
	int ret = 0;
	struct vreg *rfrx2_vreg = vreg_get(0, "rfrx2");

	printk("[Accelerometer] %s() : Power Off\n",__FUNCTION__);

	// LGE_DEV_PORTING GELATO_DS_S ks82.jung@lge.com [2011-04-22]

/* k3dh */
#if defined(CONFIG_SENSOR_K3DH)
        k3dh_config_gpio(0);
/* kr3dh */
#elif defined(CONFIG_SENSOR_KR3DH)
	kr3dh_config_gpio(0);
#endif

	// compass pwr chek, if compass power off - power state change
	if(!(lge_ecom_accel_pwr_state & COMPASS_POWER_STATE))
	{
	vreg_set_level(rfrx2_vreg, 0);// LGE_DEV_PORTING GELATO_DS_[edward1.kim@lge.com]_20110419
	vreg_disable(rfrx2_vreg);
	}

	// check bit update (off)
	lge_ecom_accel_pwr_state &= COMPASS_POWER_STATE;

	return ret;
}


// LGE_DEV_PORTING GELATO_DS_S ks82.jung@lge.com [2011-04-20]

/* k3dh */
#if defined(CONFIG_SENSOR_K3DH)

struct k3dh_platform_data k3dh_data = {
	.poll_interval = 10,

	.min_interval = 0,
	.g_range = 0x00,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,

	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,

	.power_on = accel_power_on,
	.power_off = accel_power_off,
	.kr_init = kr_init,
	.kr_exit = kr_exit,
	.gpio_config = k3dh_config_gpio,
	.device_id = k3dh_device_id,

	.ecom_accel_pwr_state=get_ecom_accel_pwr_state,
};

/* kr3dh */
#elif defined(CONFIG_SENSOR_KR3DH)

struct kr3dh_platform_data kr3dh_data = {
	.poll_interval = 100,
	.min_interval = 0,
	.g_range = 0x00,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,

	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,

	.power_on = accel_power_on,
	.power_off = accel_power_off,
	.kr_init = kr_init,
	.kr_exit = kr_exit,
	.gpio_config = kr3dh_config_gpio,
	.device_id = kr3dh_device_id,

	.ecom_accel_pwr_state=get_ecom_accel_pwr_state,
};

#endif
// LGE_DEV_PORTING GELATO_DS_E ks82.jung@lge.com [2011-04-20]

static struct gpio_i2c_pin accel_i2c_pin[] = {
	[0] = {
		.sda_pin	= ACCEL_GPIO_I2C_SDA,
		.scl_pin	= ACCEL_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ACCEL_GPIO_INT,
	},
};

static struct i2c_gpio_platform_data accel_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device accel_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &accel_i2c_pdata,
};


// LGE_DEV_PORTING GELATO_DS_S ks82.jung@lge.com [2011-04-20]

/* k3dh */
#if defined(CONFIG_SENSOR_K3DH)

static struct i2c_board_info accel_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("K3DH", ACCEL_I2C_ADDRESS_H),
		.type = "K3DH",
		.platform_data = &k3dh_data,
	},
};

/* kr3dh */
#elif defined(CONFIG_SENSOR_KR3DH)

static struct i2c_board_info accel_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("KR3DH", ACCEL_I2C_ADDRESS_H),
		.type = "KR3DH",
		.platform_data = &kr3dh_data,
	},
};

#endif
// LGE_DEV_PORTING GELATO_DS_E ks82.jung@lge.com [2011-04-20]


static void __init gelato_init_i2c_acceleration(int bus_num)
{
	accel_i2c_device.id = bus_num;

	init_gpio_i2c_pin(&accel_i2c_pdata, accel_i2c_pin[0], &accel_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, &accel_i2c_bdinfo[0], 1);	/* KR3DH */
	platform_device_register(&accel_i2c_device);
}


/*  ecompass */

static int ecom_power_set(unsigned char onoff)
{
	struct vreg *rfrx2_vreg = NULL;
	int ret = 0;
	rfrx2_vreg = vreg_get(0, "rfrx2");

	printk("[Ecompass] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");
	// LGE_DEV_PORTING GELATO_DS_[edward1.kim@lge.com]_20110420
	if (onoff) {
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		// LGE_DEV_PORTING GELATO_DS_S ks82.jung@lge.com [2011-04-22]
		// accel pwr chek, if accel power off - power state change
		if(!(lge_ecom_accel_pwr_state & ACCEL_POWER_STATE))
		{
		vreg_set_level(rfrx2_vreg, 2600);
		vreg_enable(rfrx2_vreg);
		}

		// check bit update (on)
		lge_ecom_accel_pwr_state |= COMPASS_POWER_STATE;

		msleep(15);	// wait 15ms
	}
	else
	{
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		// LGE_DEV_PORTING GELATO_DS_S ks82.jung@lge.com [2011-04-22]
		// accel pwr chek, if accel power off - power state change
		if(!(lge_ecom_accel_pwr_state & ACCEL_POWER_STATE))
		{
		vreg_set_level(rfrx2_vreg, 0);
		vreg_disable(rfrx2_vreg);
	}

		// check bit update (off)
		lge_ecom_accel_pwr_state &= ACCEL_POWER_STATE;
	}
	return ret;
}

static struct ecom_platform_data ecom_pdata = {
	.pin_int        	= ECOM_GPIO_INT,
	.pin_rst		= 0,
	.power          	= ecom_power_set,

	.ecom_accel_pwr_state=get_ecom_accel_pwr_state,
};

static struct i2c_board_info ecom_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("ami304_sensor", ECOM_I2C_ADDRESS),
		.type = "ami304_sensor",
		.platform_data = &ecom_pdata,
	},
};

static struct gpio_i2c_pin ecom_i2c_pin[] = {
	[0] = {
		.sda_pin	= ECOM_GPIO_I2C_SDA,
		.scl_pin	= ECOM_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ECOM_GPIO_INT,
	},
};

static struct i2c_gpio_platform_data ecom_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device ecom_i2c_device = {
        .name = "i2c-gpio",
        .dev.platform_data = &ecom_i2c_pdata,
};

static void __init gelato_init_i2c_ecom(int bus_num)
{
	ecom_i2c_device.id = bus_num;

	init_gpio_i2c_pin(&ecom_i2c_pdata, ecom_i2c_pin[0], &ecom_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &ecom_i2c_bdinfo[0], 1);
	platform_device_register(&ecom_i2c_device);
}

static int prox_config_gpio(int config)// LGE_DEV_PORTING GELATO_DS_[edward1.kim@lge.com]_20110420
{
	if (config) {	/* for wake state */
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_DOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	} else {		/* for sleep state */
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_DOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

	return 0;
}

/* proximity */
static int prox_power_set(unsigned char onoff)
{
//	static bool init_done = 0;

	int ret = 0;
	struct vreg *gp6_vreg = vreg_get(0, "gp6");

	printk("[Proximity] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");

//	if (init_done == 0 && onoff)
	{
		if (onoff) {
			prox_config_gpio(1);
			vreg_set_level(gp6_vreg, 2800);
			vreg_enable(gp6_vreg);

//		init_done = 1;
		} else {
//			prox_config_gpio(0);
			vreg_set_level(gp6_vreg, 0);
			vreg_disable(gp6_vreg);
		}
	}

	return ret;
}

static struct proximity_platform_data proxi_pdata = {
	.irq_num	= PROXI_GPIO_DOUT,
	.power		= prox_power_set,
	.methods		= 1,
	.operation_mode		= 2,
	.debounce	 = 0,
	.cycle = 0,
};

static struct i2c_board_info prox_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("proximity_gp2ap", PROXI_I2C_ADDRESS),
		.type = "proximity_gp2ap",
		.platform_data = &proxi_pdata,
	},
};

static struct gpio_i2c_pin proxi_i2c_pin[] = {
	[0] = {
		.sda_pin	= PROXI_GPIO_I2C_SDA,
		.scl_pin	= PROXI_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= PROXI_GPIO_DOUT,
	},
};

static struct i2c_gpio_platform_data proxi_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device proxi_i2c_device = {
        .name = "i2c-gpio",
        .dev.platform_data = &proxi_i2c_pdata,
};

static void __init gelato_init_i2c_prox(int bus_num)
{
	proxi_i2c_device.id = bus_num;

	init_gpio_i2c_pin(&proxi_i2c_pdata, proxi_i2c_pin[0], &prox_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &prox_i2c_bdinfo[0], 1);
	platform_device_register(&proxi_i2c_device);
}


/* common function */
void __init lge_add_input_devices(void)
{
	gelato_select_keymap();
	platform_add_devices(gelato_input_devices, ARRAY_SIZE(gelato_input_devices));

	lge_add_gpio_i2c_device(gelato_init_i2c_touch);
	lge_add_gpio_i2c_device(gelato_init_i2c_prox);
	lge_add_gpio_i2c_device(gelato_init_i2c_ecom);
	lge_add_gpio_i2c_device(gelato_init_i2c_acceleration);
}

