#include <linux/kernel.h>
#include <linux/sched.h>



asmlinkage long sys_hello(void)
{

	printk("Hello World\n");
	return 0;

}

asmlinkage long sys_hello2(void)
{

	printk("Hello this is the second system call\n");
	return 0;

}

asmlinkage long sys_hello3(void)
{

	printk("Hello this is the third system call\n");
	return 0;
}

asmlinkage long sys_hello4(void)
{
	printk("Hello this is the fourth system call\n");
	return 0;
}
