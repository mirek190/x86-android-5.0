/* drivers/input/touchscreen/ft5x06_ts.c
 *
 * FocalTech ft5x0x TouchScreen driver.
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
#include <linux/lnw_gpio.h>
#include <linux/acpi.h>
#include <linux/acpi_gpio.h>

//#define FTS_CTL_IIC
//#define FTS_APK_DEBUG
//#define FT_SYSFS_DEBUG
//#define FTS_AUTO_UPGRADEG
//#define CONFIG_TOUCHSCREEN_FT5406

#ifdef FTS_CTL_IIC
#include "focaltech_ctl.h"
#endif
#ifdef FT_SYSFS_DEBUG
#include "ft5x06_ex_fun.h"
#endif

#include "ft5x06_ts.h"

static void ft5x0x_ts_suspend(struct early_suspend *handler);
static void ft5x0x_ts_resume(struct early_suspend *handler);


struct ts_event {
	u16 au16_x[CFG_MAX_TOUCH_POINTS];	/*x coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];	/*y coordinate */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];	/*touch event:
					0 -- down; 1-- contact; 2 -- contact */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];	/*touch ID */
	u16 pressure;
	u8 touch_point;
};

struct ft5x0x_ts_data {
	unsigned int irq;
	unsigned int x_max;
	unsigned int y_max;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct ts_event event;
	struct ft5x0x_platform_data *pdata;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};
#define ANDROID_INPUT_PROTOCOL_B
#define FT5X0X_I2C_ADDR  (0x70>>1)
#define FT5X0X_SELF_REGIST  1  /* we don't use ACPI enumeration for test purpose*/

struct ft5x0x_platform_data ft5x_data = {
//#ifdef CONFIG_TOUCHSCREEN_FT5406
#if 0
	.x_max = 1920,
	.y_max = 1080,
#else
	.x_max = 800, //1280
	.y_max = 1280, //800
#endif


//	.irqflags = IRQF_TRIGGER_FALLING|IRQF_TRIGGER_LOW,// RVP
        .irqflags = IRQF_TRIGGER_RISING,// CR
	.irq_gpio  = 133,//133 on CR,142 for RVP
	.reset = 128,//128 on CR, 60 on RVP
};

/*
*ft5x0x_i2c_Read-read data and write data by i2c
*@client: handle of i2c
*@writebuf: Data that will be written to the slave
*@writelen: How many bytes to write
*@readbuf: Where to store data read from slave
*@readlen: How many bytes to read
*
*Returns negative errno, else the number of messages executed
*
*
*/
int ft5x0x_i2c_Read(struct i2c_client *client, char *writebuf,
		    int writelen, char *readbuf, int readlen)
{
	int ret;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "f%s: i2c read error.\n",
				__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}
/*write data by i2c*/
int ft5x0x_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s i2c write error.\n", __func__);

	return ret;
}
int ft5x0x_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};
	buf[0] = regaddr;
	buf[1] = regvalue;

	return ft5x0x_i2c_Write(client, buf, sizeof(buf));
}
int ft5x0x_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{
	return ft5x0x_i2c_Read(client, &regaddr, 1, regvalue, 1);
}
/*release the point*/
static void ft5x0x_ts_release(struct ft5x0x_ts_data *data)
{
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	input_sync(data->input_dev);
}

/*Read touch point information when the interrupt  is asserted.*/
static int ft5x0x_read_Touchdata(struct ft5x0x_ts_data *data)
{
	struct ts_event *event = &data->event;
	u8 buf[POINT_READ_BUF] = { 0 };
	int ret = -1;
	int i = 0;
	u8 pointid = FT_MAX_ID;

	ret = ft5x0x_i2c_Read(data->client, buf, 1, buf, POINT_READ_BUF);

	if (ret < 0) {
		dev_err(&data->client->dev, "%s read touchdata failed.\n",
			__func__);
		pr_info("ft5x reset value:%d",
			gpio_get_value(data->pdata->irq_gpio));
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));

	event->touch_point = 0;

    {
        int num_of_tps = buf[2];
        //pr_info("ft5x number of tps:%d\n", num_of_tps);
        if (!num_of_tps) {
            for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++)  {
                input_mt_slot(data->input_dev, i);
                input_mt_report_slot_state(data->input_dev,
                        MT_TOOL_FINGER,
                        false);
            }
        }
    }


	for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++) {
		pointid = (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		if (pointid >= FT_MAX_ID)
			break;
		else
			event->touch_point++;
			
			
#if 1
		event->au16_x[i] =
		    (s16) (buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i];
		event->au16_y[i] =
//		    ft5x_data.y_max-((s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
//		    8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i]);
                    (s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
                    8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i];
#else
		event->au16_y[i] =
                    (s16) (buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
                    8 | (s16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i];
                event->au16_x[i] =
//                  ft5x_data.y_max-((s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
//                  8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i]);
                    (s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
                    8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i];

#endif
		event->au8_touch_event[i] =
		    buf[FT_TOUCH_EVENT_POS + FT_TOUCH_STEP * i] >> 6;
		event->au8_finger_id[i] =
		    (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;

	/*	pr_info("wangdong tsv2:id=%d event=%d x=%d y=%d\n",
			event->au8_finger_id[i],
			event->au8_touch_event[i],
			event->au16_x[i], event->au16_y[i]);*/
	}


	event->pressure = FT_PRESS;

	return 0;
}

/*
*report the point information
*/
static void ft5x0x_report_value(struct ft5x0x_ts_data *data)
{
	struct ts_event *event = &data->event;
	int i,j;
	int uppoint = 0;

	/*protocol B*/
	for (i = 0; i < event->touch_point; i++) {
		input_mt_slot(data->input_dev, event->au8_finger_id[i]);

		if (event->au8_touch_event[i] == 0
			|| event->au8_touch_event[i] == 2) {
			input_mt_report_slot_state(data->input_dev,
				MT_TOOL_FINGER,
				true);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
					event->pressure);

			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
					event->au16_x[i]);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
					event->au16_y[i]);				
		} else {
			uppoint++;
			input_mt_report_slot_state(data->input_dev,
				MT_TOOL_FINGER,
				false);
		}
	}

#if 0
	for (i; i < CFG_MAX_TOUCH_POINTS; i++) {
		input_mt_slot(data->input_dev, i);
		input_mt_report_slot_state(data->input_dev,
				MT_TOOL_FINGER,
				false);
	}
#endif

	if (event->touch_point == uppoint)
		input_report_key(data->input_dev, BTN_TOUCH, 0);
	else
		input_report_key(data->input_dev, BTN_TOUCH,
			event->touch_point > 0);
	input_sync(data->input_dev);


}

/*The ft5x0x device will signal the host about TRIGGER_FALLING.
*Processed when the interrupt is asserted.
*/
static irqreturn_t ft5x0x_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x0x_ts_data *ft5x0x_ts = dev_id;
	int ret = 0;
	disable_irq_nosync(ft5x0x_ts->irq);

	ret = ft5x0x_read_Touchdata(ft5x0x_ts);
	if (ret == 0)
		ft5x0x_report_value(ft5x0x_ts);

	enable_irq(ft5x0x_ts->irq);
	/*pr_info("ft5x0x_ts_interrupt\n"); */
	return IRQ_HANDLED;
}


void ft5x0x_reset_tp(int HighOrLow)
{
    pr_info("set tp reset pin to %d\n",HighOrLow);
//    gpio_set_value(GPIO_TP_RESET, HighOrLow);
	gpio_set_value(ft5x_data.reset, HighOrLow);
}

static int ft5x0x_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	 /*struct ft5x0x_platform_data *pdata =
	    (struct ft5x0x_platform_data *)client->dev.platform_data; */
	   struct ft5x0x_platform_data *pdata;
	    
	struct ft5x0x_ts_data *ft5x0x_ts;
	struct input_dev *input_dev;
	struct acpi_gpio_info gpio_info;
	
	int err = 0;
	int counter = 0;
	unsigned char uc_reg_value;
	unsigned char uc_reg_addr;
	/* + debug begin */
        pr_info("ft5x0x_ts_probe enter\n");
        pr_info("client name is: %s\n", client->name);
        pdata = &ft5x_data;
        /* - debug end */      
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ft5x0x_ts = kzalloc(sizeof(struct ft5x0x_ts_data), GFP_KERNEL);

	if (!ft5x0x_ts) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, ft5x0x_ts);

	client->irq = gpio_to_irq(pdata->irq_gpio);
	ft5x0x_ts->irq = client->irq;
	ft5x0x_ts->client = client;
	ft5x0x_ts->pdata = pdata;
	ft5x0x_ts->x_max = pdata->x_max - 1;
	ft5x0x_ts->y_max = pdata->y_max - 1;
	pr_info("ft5x0x irq:%d from GPIO:%d; reset:%d\n",
			client->irq, pdata->irq_gpio, pdata->reset);

	dev_warn(&client->dev, "ft5x0x_ts_probe: %d\n",
				pdata->irq_gpio);
	err = gpio_request(pdata->reset, "ft5x0x reset");
	if (err < 0) {
		dev_err(&client->dev, "%s:failed to set gpio reset.\n",
			__func__);
		goto exit_request_reset;
	}

	err = gpio_direction_output(pdata->reset, 0);
	if (err) {
		printk(KERN_ERR "set reset direction error=%d\n", err);
		gpio_free(pdata->reset);
	}

	
	err = gpio_direction_output(pdata->reset, 0);
	if (err) {
		printk(KERN_ERR "set reset direction error=%d\n", err);
		gpio_free(pdata->reset);
	}


	/*init INTERRUPT pin*/
	err = gpio_request(pdata->irq_gpio, "ft5x0x irq");
	if (err < 0)
		printk(KERN_ERR "Failed to request GPIO%d (ft5x0x int) err=%d\n",
			pdata->irq_gpio, err);

	err = gpio_direction_input(pdata->irq_gpio);
	if (err) {
		printk(KERN_ERR "set int direction err=%d\n", err);
		gpio_free(pdata->irq_gpio);
	}
	
	pr_info("ft5x0x ft5x0x_ts irq:%d, irqflags:%d,client->name: %s, client->dev.driver->name: %s \n",client->irq, (pdata->irqflags | IRQF_ONESHOT),client->name, client->dev.driver->name);
	err = request_threaded_irq(client->irq, NULL, ft5x0x_ts_interrupt,
				   pdata->irqflags | IRQF_ONESHOT, client->dev.driver->name,
				   ft5x0x_ts);
	if (err < 0) {
		dev_err(&client->dev, "ft5x0x_probe: request irq failed err = %d\n",err);
		goto exit_irq_request_failed;
	}
	disable_irq(client->irq);
	pr_info("ft5x0x ft5x0x_ts irq:%d, irqflags:%d",		client->irq, pdata->irqflags);
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

        mdelay(50);
  
        gpio_direction_output(pdata->reset, 0);
	msleep(150);
	gpio_direction_output(pdata->reset, 1);

	ft5x0x_ts->input_dev = input_dev;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);

	input_mt_init_slots(input_dev, CFG_MAX_TOUCH_POINTS,0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			     0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, ft5x0x_ts->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, ft5x0x_ts->y_max, 0, 0);

	input_dev->name = FT5X0X_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"ft5x0x_ts_probe: failed to register input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	/*make sure CTP already finish startup process */
	msleep(150);

		/*get some register information */
	uc_reg_addr = FT5x0x_REG_FW_VER;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	pr_info("[FTS] Firmware version = 0x%x, counter=%d\n", uc_reg_value, counter);

	uc_reg_addr = FT5x0x_REG_POINT_RATE;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	pr_info("[FTS] report rate is %dHz.\n",
		uc_reg_value * 10);

	uc_reg_addr = FT5X0X_REG_THGROUP;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	pr_info("[FTS] touch threshold is %d.\n",
		uc_reg_value * 4);
	
	ft5x0x_write_reg(client, &uc_reg_addr,25);
	msleep(10);
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
        pr_info("[FTS] WangDong touch threshold is %d.\n",
        uc_reg_value * 4);

	
#ifdef FTS_AUTO_UPGRADEG
        fts_ctpm_auto_upgrade(client);
#endif
#ifdef FT_SYSFS_DEBUG
	ft5x0x_create_sysfs(client);
#endif
#ifdef FTS_APK_DEBUG
	ft5x0x_create_apk_debug_channel(client);
#endif

#ifdef FTS_CTL_IIC
	if (ft_rw_iic_drv_init(client) < 0)
		dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n",
				__func__);
#endif
	enable_irq(client->irq);
#ifdef CONFIG_HAS_EARLYSUSPEND
	ft5x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft5x0x_ts->early_suspend.suspend = ft5x0x_ts_suspend;
	ft5x0x_ts->early_suspend.resume = ft5x0x_ts_resume;
	register_early_suspend(&ft5x0x_ts->early_suspend);
#endif

	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free_irq(client->irq, ft5x0x_ts);

exit_request_reset:
	gpio_free(ft5x0x_ts->pdata->reset);


exit_irq_request_failed:
	i2c_set_clientdata(client, NULL);
	kfree(ft5x0x_ts);

exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}


void ft5x0x_ts_suspend(struct early_suspend *handler)
{
	struct ft5x0x_ts_data *ts = container_of(handler, struct ft5x0x_ts_data,
						early_suspend);
	unsigned char uc_reg_value;
	unsigned char uc_reg_addr;
	pr_info("[FTS]ft5x0x suspend\n");
	ft5x0x_write_reg(ts->client, 0xA5, 3);

	disable_irq(ts->irq);
}

void ft5x0x_ts_resume(struct early_suspend *handler)
{
	struct ft5x0x_ts_data *ts = container_of(handler, struct ft5x0x_ts_data,
						early_suspend);

	pr_info("[FTS]ft5x0x resume.\n");
	gpio_set_value(ts->pdata->reset, 0);
	msleep(20);
	gpio_set_value(ts->pdata->reset, 1);
	enable_irq(ts->irq);
}


static int ft5x0x_ts_remove(struct i2c_client *client)
{
	struct ft5x0x_ts_data *ft5x0x_ts;
	ft5x0x_ts = i2c_get_clientdata(client);
	input_unregister_device(ft5x0x_ts->input_dev);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ft5x0x_ts->early_suspend);
	gpio_free(ft5x0x_ts->pdata->reset);
#endif
	#ifdef FTS_APK_DEBUG
	ft5x0x_release_apk_debug_channel();
	#endif
	#ifdef FT_SYSFS_DEBUG
	ft5x0x_release_sysfs(client);
	#endif
	#ifdef FTS_CTL_IIC
	ft_rw_iic_drv_exit();
	#endif
	free_irq(client->irq, ft5x0x_ts);
	kfree(ft5x0x_ts);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id ft5x0x_ts_id[] = {
	{FT5X0X_NAME, 0},
	{}
};

static const struct acpi_device_id ft5x_acpi_match[] = {

	{FT5X0X_ACPI_NAME, 0},
	{ },
};

MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);

MODULE_DEVICE_TABLE(acpi, ft5x_acpi_match);

static struct i2c_driver ft5x0x_ts_driver = {
	.probe = ft5x0x_ts_probe,
	.remove = ft5x0x_ts_remove,
	.id_table = ft5x0x_ts_id,

	.driver = {
		   .name = FT5X0X_NAME,
		   .owner = THIS_MODULE,
		   },
};

#if FT5X0X_SELF_REGIST
static int __init ft5x0x_platform_init(void)
{

	/* schematic's i2c bus start at 0, but ACPI start at 1 */
	int i2c_busnum = 4;/*4 on CR, 6 on Baytrail RVP */

	printk(KERN_INFO "%s:\n", __func__);

	static struct i2c_board_info __initdata fx5x0x_i2c_device = {
		I2C_BOARD_INFO(FT5X0X_NAME, FT5X0X_I2C_ADDR),
		.platform_data = &ft5x_data,
	};

	fx5x0x_i2c_device.irq = gpio_to_irq(ft5x_data.irq_gpio);

	return i2c_register_board_info(i2c_busnum, &fx5x0x_i2c_device, 1);
}
#endif

static int __init ft5x0x_ts_init(void)
{
	int ret;
	ret = i2c_add_driver(&ft5x0x_ts_driver);
	if (ret) {
		printk(KERN_WARNING "Adding ft5x0x driver failed " \
		       "(errno = %d)\n", ret);
	} else {
		pr_info("Successfully added driver %s\n",
			ft5x0x_ts_driver.driver.name);
	}
	return ret;
}

static void __exit ft5x0x_ts_exit(void)
{
	i2c_del_driver(&ft5x0x_ts_driver);
}

#if FT5X0X_SELF_REGIST
fs_initcall(ft5x0x_platform_init);
#endif

module_init(ft5x0x_ts_init);
module_exit(ft5x0x_ts_exit);


MODULE_AUTHOR("<luowj>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");
