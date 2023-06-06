/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */

#include <linux/cdev.h>    /* needed for char device driver */
#include <linux/fs.h>      /* needed for device drivers */
#include <linux/uaccess.h> /* needed to copy data to/from user */

#include <linux/device.h> /* needed for sysfs handling */
#include <linux/miscdevice.h>
#include <linux/platform_device.h> /* needed for sysfs handling */

#include <linux/timer.h>
#include <linux/jiffies.h>

#include <linux/gpio.h>
#include <linux/ioport.h>	/* needed for memory region handling */
#include <linux/io.h>		/* needed for mmio handling */

#include <linux/kthread.h>	/* needed for kernel thread management */
#include <linux/wait.h>		/* needed for waitqueues handling */
#include <linux/delay.h>	/* needed for delay fonctions */

#define LED 10

static int led_trigger = 0;

static struct timer_list led_timer;
static unsigned int period;
static int stopped = 0;

static struct task_struct* my_thread[1];
static struct resource* res[1]={[0]=0,};
static long current_temp;

static int freq[] = {2,5,10,20};
static char* mode_names[2] = {"automatic","manual"};
static int mode_index;
static int speed_index;


/*******************************************
* driver func
*******************************************/

long get_temp(void){
    long temp = 0;
    unsigned char* regs[1]={[0]=0,};

    res[0] = request_mem_region (0x01c25000, 0x1000, "allwiner h5 ths");
    if ((res[0] == 0)){
        pr_info ("Error while reserving memory region... %d\n",	res==0);
    }

    regs[0] = ioremap (0x01C25000, 0x1000);
    if (regs[0] == 0) {
		pr_info ("Error while trying to map processor register...\n");
		return -EFAULT;
	}

    temp = -1191 * (int)ioread32(regs[0]+0x80) / 10 + 223000;
	pr_info ("temperature=%ld (%d)\n", temp, ioread32(regs[0]+0x80));

    iounmap (regs[0]);
    if (res[0] != 0) release_mem_region (0x01C25000, 0x1000);
    return temp;
}

int set_mode(const char* buf){
    int i;
    int comp = -1;
    char* current_mode;
     for (i=0; i < 2; i++){
        current_mode = mode_names[i];
        comp = strcmp(current_mode, buf);
        if (comp==-10){
            mode_index = i;
            pr_info ("fandriver - mode changed to %s\n", mode_names[mode_index]);
            return 0;
        }  
    }
    pr_err("fandriver - invalid mode\n");
    return -1;
}

int set_speed(const char* buf){
    int speed, i;
    speed = simple_strtol(buf, 0, 10);
    for (i=0; i < 4; i++){
        if (freq[i]==speed){
            speed_index = i;
            pr_info ("fandriver - speed changed to %d\n", speed);
            return 0;
        }
    }
    pr_err("fandriver - invalid speed %d\n", speed);
    return -1;
}

unsigned int compute_period(void){
    unsigned int p;
    p = (1000 / freq[speed_index]) / 2;
    pr_info ("Period is %d\n", p);
    return p;
}

int compute_speed_index(void){
    
    //temp = get_temp();
    if (current_temp < 3500) return 0;
    else if (current_temp < 4000) return 1;
    else if (current_temp < 4500) return 2;
    else return 3;
}

void timer_callback(struct timer_list * data)
{
    if (stopped) return;

    gpio_set_value(LED, led_trigger);
	led_trigger = led_trigger ? (0):(1);
    
    if (mode_index == 0)
        speed_index = compute_speed_index();
    
    period = compute_period();
    mod_timer(&led_timer, jiffies + msecs_to_jiffies(period));
}

/*******************************************
* Thread func
*******************************************/

static int skeleton_thread_temp (void* data)
{
	wait_queue_head_t queue;
    int status;
	pr_info ("skeleton thread_temp is now active...\n");
	init_waitqueue_head (&queue);
	while(!kthread_should_stop()) {

        current_temp = get_temp();

		status = wait_event_interruptible_timeout 
			(queue, kthread_should_stop(), 1*HZ);
		if (status == -ERESTARTSYS) {
			pr_info ("skeleton thread_2 has been interrupted\n");
			break;
		}
		pr_info ("skeleton thread_temp timout elapsed...\n");
	}
	return 0;
}

/*******************************************
* sysfs func
*******************************************/

ssize_t sysfs_show_mode(struct device* dev,
                       struct device_attribute* attr,
                       char* buf)
{
    sprintf(buf, "%s\n", mode_names[mode_index]);
    return strlen(buf);
}
ssize_t sysfs_store_mode(struct device* dev,
                        struct device_attribute* attr,
                        const char* buf,
                        size_t count)
{
    set_mode(buf);
    return count;
}
DEVICE_ATTR(mode, 0664, sysfs_show_mode, sysfs_store_mode);

ssize_t sysfs_show_speed(struct device* dev,
                       struct device_attribute* attr,
                       char* buf)
{
    sprintf(buf, "%d\n", freq[speed_index]);
    return strlen(buf);
}
ssize_t sysfs_store_speed(struct device* dev,
                        struct device_attribute* attr,
                        const char* buf,
                        size_t count)
{
    if (mode_index == 0)
        pr_info ("fandriver - Cannot change speed in automatic mode\n");
    else
        set_speed(buf);
    return count;
}
DEVICE_ATTR(speed, 0664, sysfs_show_speed, sysfs_store_speed);

static void sysfs_dev_release (struct device *dev)
{
    pr_info ("skeleton - sysfs dev release\n");
}

static struct platform_device platform_device = {
    .name = "fandriver",
    .id = -1,
    .dev.release = sysfs_dev_release
};


static int __init skeleton_init(void)
{
    int status = 0;
	if (status == 0) status = platform_device_register (&platform_device);
    if (status == 0) status = device_create_file (&platform_device.dev, &dev_attr_mode);
 	if (status == 0) status = device_create_file (&platform_device.dev, &dev_attr_speed);

    current_temp = get_temp();

	gpio_request(LED, "LED");
	gpio_direction_output(LED, 0 );
		
    period = compute_period();
    timer_setup(&led_timer, timer_callback, 0);
    mod_timer(&led_timer, jiffies + msecs_to_jiffies(period));

    my_thread[0] = kthread_run (skeleton_thread_temp, 0, "s/thread/temp");

    pr_info("Linux module skeleton loaded\n");
    return 0;
}

static void __exit skeleton_exit(void)
{
    stopped = 1;
    kthread_stop (my_thread[0]);
    del_timer(&led_timer);
    gpio_set_value(LED, 0);
	gpio_free(LED);

	device_remove_file (&platform_device.dev, &dev_attr_mode);
	device_remove_file (&platform_device.dev, &dev_attr_speed);
	platform_device_unregister (&platform_device);
    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Julien Piguet <julien.piguet@master.hes-so.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
