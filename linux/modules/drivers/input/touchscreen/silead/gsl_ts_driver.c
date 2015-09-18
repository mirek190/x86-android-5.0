/* drivers/input/touchscreen/gslX68X.h
 * 
 * 2010 - 2013 SLIEAD Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the SLIEAD's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 */
  
 #include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

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

#include <linux/kthread.h>


#include <linux/ctype.h>//
#include <linux/err.h>//

#include <linux/byteorder/generic.h>//
#include <linux/jiffies.h>//
#include <linux/irq.h>//
#include <linux/platform_device.h>//
#include <linux/earlysuspend.h>//
#include <linux/firmware.h>//
#include <linux/proc_fs.h>//

#ifdef CONFIG_TOUCHSCREEN_GSLX68X_FIRMWARE_A785
#include <linux/input/gslX68X_a785.h>
#elif defined(CONFIG_TOUCHSCREEN_GSLX68X_FIRMWARE_A8HD)
#include <linux/input/gslX68X_a8hd.h>
#else
#include "gslX68X.h"
#endif


//#define GSL_3670_D_EDITION
#define GSL_ALG_ID

static void gsl_ts_suspend(struct early_suspend *handler);
static void gsl_ts_resume(struct early_suspend *handler);
static int gsl_identify_tp(struct i2c_client *client);


struct gsl_ts_data{
	unsigned int x_max;
	unsigned int y_max;
	struct i2c_client		*client;
	struct input_dev		*idev;
	struct work_struct		work;
	struct workqueue_struct *wq;	
	struct gsl_touch_info	*cinfo;
	struct gsl_platform_data	*pdata;
	u32 gsl_halt_flag;			//0 normal;1 the machine is suspend;
	u32 gsl_sw_flag;			//0 normal;1 download the firmware;
	u32 gsl_up_flag;			//0 normal;1 have one up event;
	u32 gsl_point_state;		//the point down and up of state	

#ifdef GSL_TIMER
	struct delayed_work		timer_work;
	struct workqueue_struct	*timer_wq;
	volatile int gsl_timer_flag;	//0:first test	1:second test 2:doing gsl_load_fw
	unsigned int gsl_timer_data;		
#endif

#ifdef GSL_HAVE_TOUCH_KEY
	int gsl_key_state;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
  struct early_suspend	pm;
#endif
};

#ifdef GSL_IDENTY_TP
static int gsl_tp_type = 0;
static unsigned int *gsl_config_data_id = gsl_config_data_id_tp_7_0;
#endif


//---------------------------------------------------------------
//---------------------------------------------------------------
#define GPIO_TP_RESET 	128
#define GPIO_TP_IRQ		133

#define GSL_IRQ_GPIO_NUM GPIO_TP_IRQ

#define GSL
#ifdef GSL

//#define TPD_PROC_DEBUG
#ifdef TPD_PROC_DEBUG
static struct proc_dir_entry *gsl_config_proc = NULL;
#define GSL_CONFIG_PROC_FILE "gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag = 0;
#endif


/*mutex lock*/
static struct mutex gsl_i2c_lock;

/*debug information*/
#ifdef GSL_DEBUG 
#define print_info(fmt, args...)   \
        do{                              \
                printk("[tp-gsl][%s]"fmt,__func__, ##args);     \
        }while(0)
#else
#define print_info(fmt, args...)   //
#endif

/*define golbal variable*/
static struct gsl_ts_data *ddata = NULL;
static unsigned int GSL_MAX_X_ADAPTION = GSL_MAX_X;
static unsigned int GSL_MAX_Y_ADAPTION = GSL_MAX_Y;


struct gsl_platform_data gsl_data = {

	.x_max = GSL_MAX_X,
	.y_max = GSL_MAX_Y,
	.irqflags = IRQF_TRIGGER_RISING,
	.irq_gpio  = GPIO_TP_IRQ,
	.reset = GPIO_TP_RESET,
};

static struct i2c_board_info __initdata gsl_i2c_device = {
	I2C_BOARD_INFO(GSL_TS_NAME, GSL_TS_ADDR),
	.platform_data = &gsl_data,
};
	
/**********************************************************
Name: gsl_read_interface
Function: read i2c data
input:
output:
***********************************************************/

static int gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{

	int err = 0;
	u8 temp = reg;
	mutex_lock(&gsl_i2c_lock);
	if(temp < 0x80)
	{
		if(temp==0x7c){
			temp =0x78;
			i2c_master_send(client,&temp,1);	
			err = i2c_master_recv(client,&buf[0],4);
			temp = 0x7c;
		}else{
			i2c_master_send(client,&temp,1);	
			err = i2c_master_recv(client,&buf[0],4);
		}
	}
	i2c_master_send(client,&temp,1);
	err = i2c_master_recv(client,&buf[0],num);
	mutex_unlock(&gsl_i2c_lock);
	return err;
}

/**********************************************************
Name: gsl_write_interface
Function: write i2c data
input:
output:
***********************************************************/

static int gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];
	int err;
	u8 tmp_buf[num+1];
	tmp_buf[0] = reg;
	memcpy(tmp_buf + 1, buf, num);
	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = tmp_buf;
	//xfer_msg[0].timing = 400;

	mutex_lock(&gsl_i2c_lock);
	err= i2c_transfer(client->adapter, xfer_msg, 1);
	mutex_unlock(&gsl_i2c_lock);
	return err;	
//	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

/**********************************************************
Name: gsl_load_fw
Function: Loading firmware into touch controller
input:
output:
***********************************************************/

static void gsl_load_fw(struct i2c_client *client,const struct fw_data *GSL_DOWNLOAD_DATA,int data_len)
{
	u8 buf[4] = {0};
	//u8 send_flag = 1;
	u8 addr=0;
	u32 source_line = 0;
	u32 source_len = data_len;//ARRAY_SIZE(GSL_DOWNLOAD_DATA);

	printk("=============gsl_load_fw start==============\n");

	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
	    addr = (u8)GSL_DOWNLOAD_DATA[source_line].offset;
		memcpy(buf,&GSL_DOWNLOAD_DATA[source_line].val,4);
	    gsl_write_interface(client, addr, buf, 4);	
	}
	printk("=============gsl_load_fw end==============\n");
}


/**********************************************************
Name: gsl_start_core
Function: Starting the touch controller 
input:
output:
***********************************************************/

static void gsl_start_core(struct i2c_client *client)
{
	//u8 tmp = 0x00;
	u8 buf[4] = {0};
#if 0
	buf[0]=0;
	buf[1]=0x10;
	buf[2]=0xfe;
	buf[3]=0x1;
	gsl_write_interface(client,0xf0,buf,4);
	buf[0]=0xf;
	buf[1]=0;
	buf[2]=0;
	buf[3]=0;
	gsl_write_interface(client,0x4,buf,4);
	msleep(20);
#endif
	buf[0]=0;
	gsl_write_interface(client,0xe0,buf,4);
#ifdef GSL_ALG_ID
	{
		gsl_DataInit(gsl_config_data_id);
	}
#endif	
}

/**********************************************************
Name: gsl_reset_core
Function: Reset the touch controller 
input:
output:
***********************************************************/

static void gsl_reset_core(struct i2c_client *client)
{
	u8 buf[4] = {0x00};
	
	buf[0] = 0x88;
	gsl_write_interface(client,0xe0,buf,4);
	msleep(5);

	buf[0] = 0x04;
	gsl_write_interface(client,0xe4,buf,4);
	msleep(5);
	
	buf[0] = 0;
	gsl_write_interface(client,0xbc,buf,4);
	msleep(5);
#ifdef GSL_3670_D_EDITION
	gsl_io_control(client);
	msleep(50);
#endif
}

/**********************************************************
Name: gsl_clear_reg
Function: Clear the data that storage in register
input:
output:
***********************************************************/

static void gsl_clear_reg(struct i2c_client *client)
{
	u8 buf[4]={0};
	//clear reg
	buf[0]=0x88;
	gsl_write_interface(client,0xe0,buf,4);
	msleep(20);
	buf[0]=0x3;
	gsl_write_interface(client,0x80,buf,4);
	msleep(5);
	buf[0]=0x4;
	gsl_write_interface(client,0xe4,buf,4);
	msleep(5);
	buf[0]=0x0;
	gsl_write_interface(client,0xe0,buf,4);
	msleep(20);
	//clear reg
}



/**********************************************************
Name: gsl_compatible_id
Function: Check error in probe function
input:
output:
***********************************************************/

static int gsl_compatible_id(struct i2c_client *client)
{
	u8 buf[4];
	int i,err;
	for(i=0;i<5;i++)
	{
		err = gsl_read_interface(client,0xfc,buf,4);
		printk("[tp-gsl] 0xfc = {0x%02x%02x%02x%02x}\n",buf[3],buf[2],
			buf[1],buf[0]);
		if(!(err<0))
		{
			err = 1;		
			break;	
		}
	}
	return err;	
}


/**********************************************************
Name: gsl_hw_init
Function: Hardware initial
input:
output:
********************************************************** /

static void gsl_hw_init(void)
{
	gpio_request(GSL_IRQ_GPIO_NUM,GSL_IRQ_NAME);
	gpio_request(GPIO_TP_RESET,GSL_RST_NAME);
	gpio_direction_output(GPIO_TP_RESET,1);	
	gpio_direction_input(GSL_IRQ_GPIO_NUM);

	msleep(5);
	gpio_set_value(GPIO_TP_RESET,0);
	msleep(10);	
	gpio_set_value(GPIO_TP_RESET,1);
	msleep(20);
	//	
}
*/

/**********************************************************
Name: gsl_sw_init
Function: Software initial
input:
output:
***********************************************************/

static void gsl_sw_init(struct i2c_client *client)
{
	
#ifdef GSL_IDENTY_TP
	u32 tmp;
#else	
	int temp;
#endif

	if(1==ddata->gsl_sw_flag)
		return;
	ddata->gsl_sw_flag = 1;
	
	gpio_set_value(GPIO_TP_RESET, 0);
	msleep(20);
	gpio_set_value(GPIO_TP_RESET, 1);
	msleep(20);	

	gsl_clear_reg(client);
	gsl_reset_core(client);
#ifdef GSL_3670_D_EDITION
	gsl_io_control(client);
#endif

#ifdef GSL_IDENTY_TP
	if(0 == gsl_tp_type)
		gsl_identify_tp(client);

	if(1 == gsl_tp_type){
		tmp = ARRAY_SIZE(GSLX68X_FW_7_0);
		gsl_load_fw(client,GSLX68X_FW_7_0,tmp);
		GSL_MAX_X_ADAPTION = 800;
		GSL_MAX_Y_ADAPTION = 1280;
	}
	else if(2 == gsl_tp_type){
		tmp = ARRAY_SIZE(GSLX68X_FW_7_85);
		gsl_load_fw(client,GSLX68X_FW_7_85,tmp);
		GSL_MAX_X_ADAPTION = 768;
		GSL_MAX_Y_ADAPTION = 1024;
		
	}
	else if(3 == gsl_tp_type){
		tmp = ARRAY_SIZE(GSLX68X_FW_10_1);
		gsl_load_fw(client,GSLX68X_FW_10_1,tmp);
		GSL_MAX_X_ADAPTION = 1280;
		GSL_MAX_Y_ADAPTION = 800;
	}
#else
	{
		temp = ARRAY_SIZE(GSLX68X_FW);
		gsl_load_fw(client,GSLX68X_FW,temp);	
	}
#endif

#ifdef GSL_3670_D_EDITION
	gsl_io_control(client);
#endif

	gsl_start_core(client);
	gsl_reset_core(client);

	gsl_start_core(client);

	ddata->gsl_sw_flag = 0;
}

/**********************************************************
Name: check_mem_data
Function: Check memory data
input:
output:
***********************************************************/

static void check_mem_data(struct i2c_client *client)
{

	u8 read_buf[4]  = {0};
	msleep(30);
	gsl_read_interface(client,0xb0,read_buf,4);
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a 
		|| read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		print_info("0xb4 ={0x%02x%02x%02x%02x}\n",
			read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		gsl_sw_init(client);
	}
}

/**********************************************************
Name: check_mem_data
Function: Check memory data
input:
output:
***********************************************************/
#ifdef GSL_TIMER
static void gsl_timer_check_func(struct work_struct *work)
{	
	u8 buf[4] = {0};
	u32 tmp;
	int i,flag=0;
	static int timer_count;
	if(1==ddata->gsl_halt_flag){
		return;
	}
	gsl_read_interface(ddata->client, 0xb4, buf, 4);
	tmp = (buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|(buf[0]);

	print_info("[pre] 0xb4 = %x \n",ddata->gsl_timer_data);
	print_info("[cur] 0xb4 = %x \n",tmp);
	print_info("gsl_timer_flag=%d\n",ddata->gsl_timer_flag);
	if(0 == ddata->gsl_timer_flag)
	{
		if(tmp== ddata->gsl_timer_data)
		{
			ddata->gsl_timer_flag = 1;
			if(0==ddata->gsl_halt_flag)
			{
				queue_delayed_work(ddata->timer_wq, &ddata->timer_work, 25);
			}
		}
		else
		{
			for(i=0;i<5;i++){
				gsl_read_interface(ddata->client,0xbc,buf,4);
				if(buf[0]==0&&buf[1]==0&&buf[2]==0&&buf[3]==0)
				{
					flag = 1;
					break;
				}
				flag =0;
			}
			if(flag == 0){
				gsl_reset_core(ddata->client);
				gsl_start_core(ddata->client);
			}
			ddata->gsl_timer_flag = 0;
			timer_count = 0;
			if(0 == ddata->gsl_halt_flag)
			{
				queue_delayed_work(ddata->timer_wq, &ddata->timer_work, GSL_TIMER_CHECK_CIRCLE);
			}
		}
	}
	else if(1==ddata->gsl_timer_flag){
		if(tmp==ddata->gsl_timer_data)
		{
			if(0==ddata->gsl_halt_flag)
			{
				timer_count++;
				ddata->gsl_timer_flag = 2;
				gsl_sw_init(ddata->client);
				ddata->gsl_timer_flag = 1;
			}
			if(0 == ddata->gsl_halt_flag && timer_count < 20)
			{
				queue_delayed_work(ddata->timer_wq, &ddata->timer_work, GSL_TIMER_CHECK_CIRCLE);
			}
		}
		else{
			timer_count = 0;
			if(0 == ddata->gsl_halt_flag && timer_count < 20)
			{
				queue_delayed_work(ddata->timer_wq, &ddata->timer_work, GSL_TIMER_CHECK_CIRCLE);
			}
		}
		ddata->gsl_timer_flag = 0;
	}
	ddata->gsl_timer_data = tmp;
}
#endif

#ifdef TPD_PROC_DEBUG
#ifdef GSL_APPLICATION
static int gsl_read_interfacexw(struct i2c_client *client,u32 addr,u8 *buf,u32 num)
{
	u8 tmp_buf[4];
	int i;
	int err;
	u32 temp1=addr;
	for(i=0;i<num/128;i++){
		temp1 = addr+i*0x80;
		tmp_buf[3]=(u8)((temp1/0x80)>>24);
		tmp_buf[2]=(u8)((temp1/0x80)>>16);
		tmp_buf[1]=(u8)((temp1/0x80)>>8);
		tmp_buf[0]=(u8)((temp1/0x80));
		gsl_write_interface(client,0xf0,tmp_buf,4);
		gsl_read_interface(client,temp1%0x80,&buf[i*128],128);
	}
	temp1=addr+i*0x80;
	tmp_buf[3]=(u8)((temp1/0x80)>>24);
	tmp_buf[2]=(u8)((temp1/0x80)>>16);
	tmp_buf[1]=(u8)((temp1/0x80)>>8);
	tmp_buf[0]=(u8)((temp1/0x80));
	gsl_write_interface(client,0xf0,tmp_buf,4);
	err=gsl_read_interface(client,temp1%0x80,&buf[i*128],(num-i*0x80));
	return err;
}
#endif
static int char_to_int(char ch)
{
	if(ch>='0' && ch<='9')
		return (ch-'0');
	else
		return (ch-'a'+10);
}
//
static int gsl_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *ptr = page;
	char temp_data[5] = {0};
	unsigned int tmp=0;
#ifdef GSL_APPLICATION
	unsigned int temp1=0;
	char *buf;
#endif
	if('v'==gsl_read[0]&&'s'==gsl_read[1])
	{
#ifdef GSL_ALG_ID 
		tmp=gsl_version_id();
#else 
		tmp=0x20121215;
#endif
		ptr += sprintf(ptr,"version:%x\n",tmp);
	}
	else if('r'==gsl_read[0]&&'e'==gsl_read[1])
	{
		if('i'==gsl_read[3])
		{
#ifdef GSL_ALG_ID 
			tmp=(gsl_data_proc[5]<<8) | gsl_data_proc[4];
			ptr +=sprintf(ptr,"gsl_config_data_id[%d] = ",tmp);
			if(tmp>=0&&tmp<512)
				ptr +=sprintf(ptr,"%d\n",gsl_config_data_id[tmp]); 
#endif
		}
		else 
		{
			gsl_write_interface(ddata->client,0xf0,&gsl_data_proc[4],4);
			gsl_read_interface(ddata->client,gsl_data_proc[0],temp_data,4);
			ptr +=sprintf(ptr,"offset : {0x%02x,0x",gsl_data_proc[0]);
			ptr +=sprintf(ptr,"%02x",temp_data[3]);
			ptr +=sprintf(ptr,"%02x",temp_data[2]);
			ptr +=sprintf(ptr,"%02x",temp_data[1]);
			ptr +=sprintf(ptr,"%02x};\n",temp_data[0]);
		}
	}
#ifdef GSL_APPLICATION
	else if('a'==gsl_read[0]&&'p'==gsl_read[1]){
		tmp = (unsigned int)(((gsl_data_proc[2]<<8)|gsl_data_proc[1])&0xffff);
		buf=kzalloc(tmp,GFP_KERNEL);
		if(buf==NULL)
			return -1;
		if(3==gsl_data_proc[0]){
			gsl_read_interface(ddata->client,gsl_data_proc[3],buf,tmp);
			memcpy(page,buf,tmp);
		}else if(4==gsl_data_proc[0]){
			temp1=((gsl_data_proc[6]<<24)|(gsl_data_proc[5]<<16)|
				(gsl_data_proc[4]<<8)|gsl_data_proc[3]);
			gsl_read_interfacexw(ddata->client,temp1,buf,tmp);
			memcpy(page,buf,tmp);
		}
		kfree(buf);
		*eof=1;
		return tmp;
	}
#endif
	*eof = 1;
	return (ptr - page);
}
static int gsl_config_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
	u8 buf[8] = {0};
	char temp_buf[CONFIG_LEN];
	char *path_buf;
	int tmp = 0;
	int tmp1 = 0;
	print_info("[tp-gsl][%s] \n",__func__);
	if(count > 512)
	{
		print_info("size not match [%d:%ld]\n", CONFIG_LEN, count);
        	return -EFAULT;
	}
	path_buf=kzalloc(count,GFP_KERNEL);
	if(!path_buf)
	{
		printk("alloc path_buf memory error \n");
		return -1;
	}	
	if(copy_from_user(path_buf, buffer, count))
	{
		print_info("copy from user fail\n");
		goto exit_write_proc_out;
	}
	memcpy(temp_buf,path_buf,(count<CONFIG_LEN?count:CONFIG_LEN));
	print_info("[tp-gsl][%s][%s]\n",__func__,temp_buf);
#ifdef GSL_APPLICATION
	if('a'!=temp_buf[0]||'p'!=temp_buf[1]){
#endif
	buf[3]=char_to_int(temp_buf[14])<<4 | char_to_int(temp_buf[15]);	
	buf[2]=char_to_int(temp_buf[16])<<4 | char_to_int(temp_buf[17]);
	buf[1]=char_to_int(temp_buf[18])<<4 | char_to_int(temp_buf[19]);
	buf[0]=char_to_int(temp_buf[20])<<4 | char_to_int(temp_buf[21]);
	
	buf[7]=char_to_int(temp_buf[5])<<4 | char_to_int(temp_buf[6]);
	buf[6]=char_to_int(temp_buf[7])<<4 | char_to_int(temp_buf[8]);
	buf[5]=char_to_int(temp_buf[9])<<4 | char_to_int(temp_buf[10]);
	buf[4]=char_to_int(temp_buf[11])<<4 | char_to_int(temp_buf[12]);
#ifdef GSL_APPLICATION
	}
#endif
	if('v'==temp_buf[0]&& 's'==temp_buf[1])//version //vs
	{
		memcpy(gsl_read,temp_buf,4);
		printk("gsl version\n");
	}
	else if('s'==temp_buf[0]&& 't'==temp_buf[1])//start //st
	{
	#ifdef GSL_MONITOR
		cancel_delayed_work_sync(&gsl_monitor_work);
	#endif
		gsl_proc_flag = 1;
		gsl_reset_core(ddata->client);
	}
	else if('e'==temp_buf[0]&&'n'==temp_buf[1])//end //en
	{
		msleep(20);
		gsl_reset_core(ddata->client);
		gsl_start_core(ddata->client);
		gsl_proc_flag = 0;
	}
	else if('r'==temp_buf[0]&&'e'==temp_buf[1])//read buf //
	{
		memcpy(gsl_read,temp_buf,4);
		memcpy(gsl_data_proc,buf,8);
	}
	else if('w'==temp_buf[0]&&'r'==temp_buf[1])//write buf
	{
		gsl_write_interface(ddata->client,buf[4],buf,4);
	}
	
#ifdef GSL_ALG_ID
	else if('i'==temp_buf[0]&&'d'==temp_buf[1])//write id config //
	{
		tmp1=(buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4];
		tmp=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		if(tmp1>=0 && tmp1<512)
		{
			gsl_config_data_id[tmp1] = tmp;
		}
	}
#endif
#ifdef GSL_APPLICATION
	else if('a'==temp_buf[0]&&'p'==temp_buf[1]){
		if(1==path_buf[3]){
			tmp=((path_buf[5]<<8)|path_buf[4]);
			gsl_write_interface(ddata->client,path_buf[6],&path_buf[10],tmp);
		}else if(2==path_buf[3]){
			tmp = ((path_buf[5]<<8)|path_buf[4]);
			tmp1=((path_buf[9]<<24)|(path_buf[8]<<16)|(path_buf[7]<<8)
				|path_buf[6]);
			buf[0]=(char)((tmp1/0x80)&0xff);
			buf[1]=(char)(((tmp1/0x80)>>8)&0xff);
			buf[2]=(char)(((tmp1/0x80)>>16)&0xff);
			buf[3]=(char)(((tmp1/0x80)>>24)&0xff);
			buf[4]=(char)(tmp1%0x80);
			gsl_write_interface(ddata->client,0xf0,buf,4);
			gsl_write_interface(ddata->client,buf[4],&path_buf[10],tmp);
		}else if(3==path_buf[3]||4==path_buf[3]){
			memcpy(gsl_read,temp_buf,4);
			memcpy(gsl_data_proc,&path_buf[3],7);
		}
	}
#endif
exit_write_proc_out:
	kfree(path_buf);
	return count;
}
#endif

/**********************************************************
Name: gsl_report_point
Function: report point to Android OS
input:
output:
***********************************************************/

static void gsl_report_point(struct input_dev *idev, struct gsl_touch_info *cinfo)
{
	int i; 
	u32 gsl_point_state = 0;
	u32 temp=0;
	if(cinfo->finger_num>0 && cinfo->finger_num<11)
	{
		ddata->gsl_up_flag = 0;
		gsl_point_state = 0;
	#ifdef GSL_HAVE_TOUCH_KEY
		if(1==cinfo->finger_num)
		{
			if(cinfo->x[0] > ddata->x_max || cinfo->y[0] > ddata->y_max)
			{
				gsl_report_key(idev,cinfo->x[0],cinfo->y[0]);
				return;		
			}
		}
	#endif
		for(i=0;i<cinfo->finger_num;i++)
		{
			gsl_point_state |= (0x1<<cinfo->id[i]);	
//			printk("id = %d, x = %d, y = %d \n",cinfo->id[i], 
//				cinfo->x[i],cinfo->y[i]);

		//	para = cinfo->x[i];
		//	cinfo->x[i] = cinfo->y[i];
		//	cinfo->y[i] = para;
			
		#ifdef GSL_REPORT_POINT_SLOT
			#if 1
			input_mt_slot(idev, cinfo->id[i] - 1);
			input_mt_report_slot_state(idev, MT_TOOL_FINGER, 1);
			input_report_abs(idev, ABS_MT_TOUCH_MAJOR, GSL_PRESSURE);
			input_report_abs(idev, ABS_MT_POSITION_X, cinfo->x[i]);
			input_report_abs(idev, ABS_MT_POSITION_Y, cinfo->y[i]);	
			input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 1);
			#else
			#endif
		#else 
			input_report_key(idev, BTN_TOUCH, 1);
			input_report_abs(idev, ABS_MT_TRACKING_ID, cinfo->id[i]-1);
			input_report_abs(idev, ABS_MT_TOUCH_MAJOR, GSL_PRESSURE);
			input_report_abs(idev, ABS_MT_POSITION_X, cinfo->x[i]);
			input_report_abs(idev, ABS_MT_POSITION_Y, cinfo->y[i]);	
			input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(idev);		
		#endif
		}
	}
	else if(cinfo->finger_num == 0)
	{
		gsl_point_state = 0;
	//	ddata->gsl_point_state = 0;
		if(1 == ddata->gsl_up_flag)
		{
			return;
		}
		ddata->gsl_up_flag = 1;
	#ifdef GSL_HAVE_TOUCH_KEY
		if(ddata->gsl_key_state > 0)
		{
			if(ddata->gsl_key_state < GSL_KEY_NUM+1)
			{
				input_report_key(idev,gsl_key_data[ddata->gsl_key_state - 1].key,0);
				input_sync(idev);
			}
		}
	#endif
	#ifndef GSL_REPORT_POINT_SLOT
		input_report_key(idev, BTN_TOUCH, 0);
		input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
		input_mt_sync(idev);
	#endif
	}

	temp = gsl_point_state & ddata->gsl_point_state;
	temp = (~temp) & ddata->gsl_point_state;
#ifdef GSL_REPORT_POINT_SLOT
	for(i=1;i<6;i++)
	{
		if(temp & (0x1<<i))
		{
			input_mt_slot(idev, i-1);
//			input_report_abs(idev, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(idev, MT_TOOL_FINGER, 0);
		}
	}
#endif	
	ddata->gsl_point_state = gsl_point_state;
	input_sync(idev);
}

#ifdef GSL_IDENTY_TP	
#define GSL_C		100
#define GSL_CHIP_1	0xff800000	//GSL2681_7.00inch
#define GSL_CHIP_2	0xfc026013  //GSL3675_7.85inch
#define GSL_CHIP_3  0xfc000000  //GSL3692_10.1inch

static void SortBubble(int t[],int size)
{
	int temp = 0;
	int m,n;
	for(m=0;m<size;m++)
	{
		for(n=m+1;n<size;n++)
		{
			temp = t[m];
			if (temp>t[n])
			{
				t[m] = t[n];
				t[n] = temp;
			}
		}
	}
}

static unsigned int gsl_count_one(unsigned int flag)
{
	unsigned int tmp=0; 
	int i =0;

	for (i=0 ; i<32 ; i++) {
		if (flag & (0x1 << i)) {
			tmp++;
		}
	}
	return tmp;
}

static int gsl_identify_tp(struct i2c_client *client)
{
	u8 buf[4];
	int err=1;
	int flag=0;
	unsigned int tmp,tmp0,tmp1,tmp2,tmp3;
	unsigned int min_tmp[3];
	u32 num;

identify_tp_repeat:
	gsl_clear_reg(client);
	gsl_reset_core(client);
	num = ARRAY_SIZE(GSL_TP_CHECK_FW);
	gsl_load_fw(client,GSL_TP_CHECK_FW,num);
	gsl_start_core(client);
	msleep(200);
	gsl_read_interface(client,0xb4,buf,4);
	print_info("the test 0xb4 = {0x%02x%02x%02x%02x}\n",buf[3],buf[2],buf[1],buf[0]);

	if (((buf[3] << 8) | buf[2]) > 1) {
		print_info("[TP-GSL][%s] is start ok\n",__func__);
		msleep(20);
		gsl_read_interface(client,0xb8,buf,4);
		tmp = (buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		print_info("the test 0xb8 = {0x%02x%02x%02x%02x}\n",buf[3],buf[2],buf[1],buf[0]);

	/*GSL2681_7.00_inch*/
		tmp1 = gsl_count_one(GSL_CHIP_1^tmp); 
		tmp0 = gsl_count_one((tmp&GSL_CHIP_1)^GSL_CHIP_1); 
		tmp1 += tmp0*GSL_C;  
		print_info("[TP-GSL] tmp1 = %d\n",tmp1);

	/*GSL3675_7.85_inch*/
		tmp2 = gsl_count_one(GSL_CHIP_2^tmp); 
		tmp0 = gsl_count_one((tmp&GSL_CHIP_2)^GSL_CHIP_2); 
		tmp2 += tmp0*GSL_C;
		print_info("[TP-GSL] tmp2 = %d\n",tmp2);

	/*GSL3692_10.1_inch*/
		tmp3 = gsl_count_one(GSL_CHIP_3^tmp); 
		tmp0 = gsl_count_one((tmp&GSL_CHIP_3)^GSL_CHIP_3); 
		tmp3 += tmp0*GSL_C;
		print_info("[TP-GSL] tmp3 = %d\n",tmp3);
	
		if (0xffffffff == GSL_CHIP_1) {
			tmp1=0xffff;
		}
		if (0xffffffff == GSL_CHIP_2) {
			tmp2=0xffff;
		}
		if (0xffffffff == GSL_CHIP_3) {
			tmp3=0xffff;
		}
		print_info("[TP-GSL] tmp1 = %d\n",tmp1); 
		print_info("[TP-GSL] tmp2 = %d\n",tmp2);
		print_info("[TP-GSL] tmp2 = %d\n",tmp3);
		
	/*Find the minmum "tmpX"*/

		min_tmp[0] = tmp1;
		min_tmp[1] = tmp2;
		min_tmp[2] = tmp3;

		SortBubble(min_tmp,3);
		tmp = min_tmp[0];
		
		if(tmp == tmp1){
			gsl_config_data_id = gsl_config_data_id_tp_7_0;
			gsl_tp_type = 1;
		} else if(tmp == tmp2) {
			gsl_config_data_id = gsl_config_data_id_tp_7_85;
			gsl_tp_type = 2;
		} else if(tmp == tmp3) {
			gsl_config_data_id = gsl_config_data_id_tp_10_1;
			gsl_tp_type = 3;
		} 
		err = 1;
	} 
	else {
		flag++;
		if(flag < 3) {
			goto identify_tp_repeat;
		}
		err = 0;
	}
	return err; 
}
#endif


/**********************************************************
Name: gsl_report_work
Function: called in probe function
input:
output:
***********************************************************/

//static void gsl_report_work(struct work_struct *work)
static void gsl_report_work(void)
{
	int rc,tmp;
	u8 buf[44] = {0};
	int tmp1=0;
	struct gsl_touch_info *cinfo = ddata->cinfo;
	struct i2c_client *client = ddata->client;
	struct input_dev *idev = ddata->idev;
	
//	printk("GSL report work\n");

	if(1 == ddata->gsl_sw_flag)
		goto schedule;

#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		goto schedule;
	}
#endif
#ifdef GSL_TIMER 
	if(2==ddata->gsl_timer_flag){
		goto schedule;
	}
#endif

	/* read data from DATA_REG */
	rc = gsl_read_interface(client, 0x80, buf, 44);
	if (rc < 0) 
	{
		dev_err(&client->dev, "[gsl] I2C read failed\n");
		goto schedule;
	}

	if (buf[0] == 0xff) {
		goto schedule;
	}

	cinfo->finger_num = buf[0];
	for(tmp=0;tmp<(cinfo->finger_num>10 ? 10:cinfo->finger_num);tmp++)
	{
		cinfo->y[tmp] = (buf[tmp*4+4] | ((buf[tmp*4+5])<<8));
		cinfo->x[tmp] = (buf[tmp*4+6] | ((buf[tmp*4+7] & 0x0f)<<8));
		cinfo->id[tmp] = buf[tmp*4+7] >> 4;
//		printk("tp-gsl  x = %d y = %d \n",cinfo->x[tmp],cinfo->y[tmp]);
	}
	
//	printk("111 finger_num= %d\n",cinfo->finger_num);
#ifdef GSL_ALG_ID
	cinfo->finger_num = (buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|(buf[0]);
	gsl_alg_id_main(cinfo);
	tmp1=gsl_mask_tiaoping();
	print_info("[tp-gsl] tmp1=%x\n",tmp1);
	if(tmp1>0&&tmp1<0xffffffff)
	{
		buf[0]=0xa;
		buf[1]=0;
		buf[2]=0;
		buf[3]=0;
		gsl_write_interface(client,0xf0,buf,4);
		buf[0]=(u8)(tmp1 & 0xff);
		buf[1]=(u8)((tmp1>>8) & 0xff);
		buf[2]=(u8)((tmp1>>16) & 0xff);
		buf[3]=(u8)((tmp1>>24) & 0xff);
		print_info("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
			tmp1,buf[0],buf[1],buf[2],buf[3]);
		gsl_write_interface(client,0x8,buf,4);
	}
#endif


	//print_info("222 finger_num= %d\n",cinfo->finger_num);
	gsl_report_point(idev,cinfo);
	
schedule:
	return;

}

/**********************************************************
Name: gsl_report_work
Function: called in probe function
input:
output:
***********************************************************/

static int gsl_request_input_dev(struct gsl_ts_data *ddata)

{
	struct input_dev *input_dev;
	int err;
#ifdef GSL_HAVE_TOUCH_KEY
	int i;
#endif
	/*allocate input device*/
	print_info("==input_allocate_device=\n");
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		goto err_allocate_input_device_fail;
	}
	ddata->idev = input_dev;
	input_dev->name = ddata->client->name;
	input_dev->id.bustype =BUS_I2C;
	input_dev->dev.parent= &ddata->client->dev;
	/*set input parameter*/	
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(MT_TOOL_FINGER, input_dev->keybit);//bee
#ifdef GSL_REPORT_POINT_SLOT
	__set_bit(EV_REP, input_dev->evbit);
//	__set_bit(INPUT_PROP_DIRECT,input_dev->propbit);
	input_mt_init_slots(input_dev,10,0);
#else 
	__set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,0,10,0,0);
#endif
	__set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	__set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
#ifdef TOUCH_VIRTUAL_KEYS
	__set_bit(KEY_MENU,  input_dev->keybit);
	__set_bit(KEY_BACK,  input_dev->keybit);
	__set_bit(KEY_HOME,  input_dev->keybit);    
#endif
#ifdef GSL_HAVE_TOUCH_KEY
	for(i=0;i<GSL_KEY_NUM;i++)
	{
		__set_bit(gsl_key_data[i].key, input_dev->keybit);
	}
#endif
	
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, ddata->x_max, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, ddata->y_max, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	input_dev->name = GSL_TS_NAME;		//dev_name(&client->dev)

	/*register input device*/
	err = input_register_device(input_dev);
	if (err) {
		goto err_register_input_device_fail;
	}
	return 0;
err_register_input_device_fail:
	input_free_device(input_dev);			
err_allocate_input_device_fail:
	return err;
}

/**********************************************************
Name: gsl_report_work
Function: called in probe function
input:
output:
***********************************************************/

static irqreturn_t gsl_ts_interrupt_isr(int irq, void *dev_id) {
	return IRQ_WAKE_THREAD;
}

static irqreturn_t gsl_ts_interrupt(int irq, void *dev_id)
{
	struct i2c_client *client = ddata->client;
	print_info("%s:gslX68X_ts_interrupt\n",__func__);
	
	disable_irq_nosync(client->irq);
	gsl_report_work();

	/*
	if (!work_pending(&ddata->work)) 
	{
		queue_work(ddata->wq, &ddata->work);
	}
	*/

	enable_irq(client->irq);
	
	return IRQ_HANDLED;
}

//---------------------------------------------------------------
#endif


//---------------------------------------------------------------
static int gsl_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
//	printk("Martin Debug: %s ++\n", __func__);
	struct gsl_platform_data *pdata = (struct gsl_platform_data *)client->dev.platform_data;
	struct input_dev *input_dev;
	int err = 0;

	print_info("=== gsl_ts_probe ===\n");
	
//	pdata = &gsl_data;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		print_info("=== %s:GSL I2C doesn't work ===\n",__func__);
		goto exit_check_functionality_failed;
	}

	print_info("=== GSL kzalloc ===\n");
	ddata = kzalloc(sizeof(struct gsl_ts_data), GFP_KERNEL);
	if(ddata==NULL){
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}


	ddata->gsl_halt_flag = 0;
	ddata->gsl_sw_flag = 0;
	ddata->gsl_up_flag = 0;
	ddata->gsl_point_state = 0;
	#ifdef GSL_HAVE_TOUCH_KEY
	ddata->gsl_key_state = 0;
	#endif	


	ddata->cinfo = kzalloc(sizeof(struct gsl_touch_info),GFP_KERNEL);
	if(ddata->cinfo == NULL)
	{
		err = -ENOMEM;
		goto exit_alloc_cinfo_failed;
	}
	mutex_init(&gsl_i2c_lock);
	
	ddata->client = client;
	i2c_set_clientdata(client, ddata);
	print_info("=== GSL I2C addr = %x ===\n",client->addr);

	
	err = gsl_compatible_id(client);
	if(err < 0)
	{	
		goto exit_i2c_fail; 
	}
	
	client->irq = gpio_to_irq(pdata->irq_gpio);
	ddata->client = client;
	ddata->pdata = pdata;
	ddata->x_max = pdata->x_max - 1;//GSL_MAX_X;//pdata->x_max;
	ddata->y_max = pdata->y_max - 1;//GSL_MAX_Y;//pdata->y_max;

/*Hardware initial*/
#define hw_init
#ifdef hw_init
	err = gpio_request(pdata->reset, "gsl reset");
	if (err < 0) {
		dev_err(&client->dev, "%s:failed to set gpio reset.\n",
			__func__);
		goto exit_request_reset;
	}

	err = gpio_direction_output(pdata->reset, 1);
	if (err) {
		printk(KERN_ERR "set reset direction error=%d\n", err);
		gpio_free(pdata->reset);
	}

	msleep(50);
	gpio_set_value(GPIO_TP_RESET,0);
	msleep(50);	
	gpio_set_value(GPIO_TP_RESET,1);
	msleep(50);

/*init INTERRUPT pin*/
	err = gpio_request(pdata->irq_gpio, "gsl irq");
	if (err < 0)
		printk(KERN_ERR "Failed to request GPIO%d (gsl int) err=%d\n",	pdata->irq_gpio, err);

	err = gpio_direction_input(pdata->irq_gpio);
	if (err) {
		printk(KERN_ERR "set int direction err=%d\n", err);
		gpio_free(pdata->irq_gpio);
	}
#endif

/*register early suspend*/
#ifdef CONFIG_HAS_EARLYSUSPEND
	ddata->pm.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ddata->pm.suspend = gsl_ts_suspend;
	ddata->pm.resume = gsl_ts_resume;
	register_early_suspend(&ddata->pm);
#endif

/*gsl of software init*/	
#ifdef GSL
	gsl_sw_init(client);
	msleep(20);
	check_mem_data(client);
#endif

	ddata->x_max = GSL_MAX_X_ADAPTION - 1;//GSL_MAX_X;//pdata->x_max;
	ddata->y_max = GSL_MAX_Y_ADAPTION - 1;//GSL_MAX_Y;//pdata->y_max;

	/*request input system*/	
	err = gsl_request_input_dev(ddata);
	if(err < 0)
	{
		goto exit_i2c_transfer_fail;//exit_input_dev_alloc_failed	
	}

#ifdef GSL
	#ifdef GSL_TIMER
		INIT_DELAYED_WORK(&ddata->timer_work, gsl_timer_check_func);
		ddata->timer_wq = create_workqueue("gsl_timer_wq");
		queue_delayed_work(ddata->timer_wq, &ddata->timer_work, GSL_TIMER_CHECK_CIRCLE);
	#endif

	#ifdef TOUCH_VIRTUAL_KEYS
		gsl_ts_virtual_keys_init();
	#endif
	
#ifdef TPD_PROC_DEBUG
/*	gsl_config_proc = proc_create(GSL_CONFIG_PROC_FILE, 0666, NULL,NULL);
	if (gsl_config_proc == NULL)
	{
		print_info("create_proc_entry %s failed\n", GSL_CONFIG_PROC_FILE);
	}
	else
	{
		gsl_config_proc->read_proc = gsl_config_read_proc;
		gsl_config_proc->write_proc = gsl_config_write_proc;
	}
	gsl_proc_flag = 0;*/
#endif
#endif
/*request irq*/
	err = request_threaded_irq(client->irq, gsl_ts_interrupt_isr, gsl_ts_interrupt, pdata->irqflags|IRQF_ONESHOT, client->dev.driver->name, ddata);
	print_info("GSL_request_threaded_irq irq =%d from GPIO %d, flag = %x， name = %s, result = %d",client->irq,pdata->irq_gpio,pdata->irqflags|IRQF_ONESHOT,client->dev.driver->name,err);
	
	if (err < 0) {
		dev_err(&client->dev, "GSL_probe: request irq failed err = %d\n", err);
		goto exit_irq_request_failed;
	}

	return 0;

exit_irq_request_failed:

exit_create_singlethread:
	unregister_early_suspend(&ddata->pm);
	input_unregister_device(ddata->idev);

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
exit_request_reset:
	gpio_free(pdata->reset);

exit_i2c_transfer_fail:
	gpio_free(GPIO_TP_RESET);
	gpio_free(GPIO_TP_IRQ);
exit_i2c_fail:
	i2c_set_clientdata(client, NULL);
	kfree(ddata->cinfo);
exit_alloc_cinfo_failed:
	kfree(ddata);

exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}


static void gsl_ts_suspend(struct early_suspend *handler)
{
	//struct gsl_ts_data *ts = container_of(handler, struct gsl_ts_data, early_suspend);

	u32 tmp;
	struct i2c_client *client = ddata->client;

#ifdef GSL_ALG_ID
		tmp = gsl_version_id(); 
		printk("[tp-gsl]the version of alg_id:%x\n",tmp);
#endif

	//version info
	ddata->gsl_halt_flag = 1;
#ifdef GSL_TIMER	
	cancel_delayed_work_sync(&ddata->timer_work);
	if(2==ddata->gsl_timer_flag){
		return;
	}
#endif	
	disable_irq(client->irq);
	gpio_set_value(GPIO_TP_RESET,0);
}

static void gsl_ts_resume(struct early_suspend *handler)
{
	//struct gsl_ts_data *ts = container_of(handler, struct gsl_ts_data, early_suspend);
	struct i2c_client *client = ddata->client;
	print_info("==gslX68X_ts_resume=\n");
	if(1==ddata->gsl_sw_flag){
		ddata->gsl_halt_flag = 0;
		enable_irq(client->irq);
		return;
	}

#ifdef GSL_TIMER
	if(2==ddata->gsl_timer_flag)
	{
		ddata->gsl_halt_flag=0;
		enable_irq(client->irq);
		return;
	}
#endif

	gpio_set_value(GPIO_TP_RESET, 1);
	msleep(20);
	//mdelay(13);
	gsl_reset_core(client);
	gsl_start_core(client);
	msleep(20);
	check_mem_data(client);
	
	enable_irq(client->irq);
#ifdef GSL_TIMER
	queue_delayed_work(ddata->timer_wq, &ddata->timer_work, GSL_TIMER_CHECK_CIRCLE);
	ddata->gsl_timer_flag = 0;
#endif
	ddata->gsl_halt_flag = 0;
	
}


static int gsl_ts_remove(struct i2c_client *client)
{
	ddata = i2c_get_clientdata(client);

#ifdef GSL_TIMER
		cancel_delayed_work_sync(&ddata->timer_work);
		destroy_workqueue(ddata->timer_wq);
#endif

	input_unregister_device(ddata->idev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ddata->pm);
#endif
	free_irq(client->irq, ddata);

	input_unregister_device(ddata->idev);
	input_free_device(ddata->idev);
	gpio_free(GPIO_TP_RESET);
	gpio_free(GPIO_TP_IRQ);

//	cancel_work_sync(&ddata->work);
	destroy_workqueue(ddata->wq);
	i2c_set_clientdata(client, NULL);
	kfree(ddata->cinfo);
	kfree(ddata);
	
	return 0;
}

#ifdef GSL
static const struct i2c_device_id gsl_ts_id[] = {
	{GSL_TS_NAME, 0},{}
};
#endif

MODULE_DEVICE_TABLE(i2c, gsl_ts_id);

static struct i2c_driver gsl_ts_driver = {
	.probe = gsl_ts_probe,
	.remove = gsl_ts_remove,
	.id_table = gsl_ts_id,
	.driver = {
		   .name = GSL_TS_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init gsl_ts_init(void)
{
//	printk("Martin Debug: %s ++\n", __func__);
	int ret;

	ret = i2c_add_driver(&gsl_ts_driver);
	if (ret) {
		printk(KERN_WARNING "Adding gsl driver failed " \
		       "(errno = %d)\n", ret);
	} else {
		pr_info("Successfully added driver %s\n",	gsl_ts_driver.driver.name);
	}
	return ret;
}

static void __exit gsl_ts_exit(void)
{
	i2c_del_driver(&gsl_ts_driver);
}

static int __init gsl_platform_init(void)
{

	/* schematic's i2c bus start at 0, but ACPI start at 1 */
	int i2c_busnum = 4; /* test on Baytrail CR RVP */

	printk(KERN_INFO "gsl_platform_init %s:\n", __func__);
	

	gsl_i2c_device.irq = gpio_to_irq(GPIO_TP_IRQ); 

	return i2c_register_board_info(i2c_busnum, &gsl_i2c_device, 1);
}

fs_initcall(gsl_platform_init);

module_init(gsl_ts_init);
module_exit(gsl_ts_exit);

MODULE_AUTHOR("<sileadinc>");
MODULE_DESCRIPTION("gsl i2c TouchScreen driver");
MODULE_LICENSE("GPL");
