#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/uaccess.h>

static int __init lkm_template_init(void) 
{
	pr_info("LKM: Hello World!\n");
	return 0;
}

static void __exit lkm_template_exit(void) 
{
	pr_info("LKM: Goodbye\n");
}

module_init(lkm_template_init);
module_exit(lkm_template_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xx");
MODULE_DESCRIPTION("lkm hello");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
