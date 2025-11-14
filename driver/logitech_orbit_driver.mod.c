#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xe8213e80, "_printk" },
	{ 0xb27dbdc3, "usb_find_interface" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0xd710adbf, "__kmalloc_noprof" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x4b24f11f, "usb_control_msg" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xc2fefbb5, "kmalloc_caches" },
	{ 0x38395bf3, "__kmalloc_cache_noprof" },
	{ 0x60c9c0b3, "__init_swait_queue_head" },
	{ 0xb8e4e17e, "usb_set_interface" },
	{ 0x0fc8db40, "usb_alloc_urb" },
	{ 0xea78ea35, "usb_alloc_coherent" },
	{ 0xc5ceac9d, "usb_free_urb" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0x056c43c7, "usb_deregister" },
	{ 0xb39b0442, "usb_get_dev" },
	{ 0x9961a343, "usb_register_dev" },
	{ 0xa3aee79a, "_dev_err" },
	{ 0xd272d446, "__fentry__" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x8134d220, "usb_register_driver" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xdefbe19a, "usb_deregister_dev" },
	{ 0x70eca2ca, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xe8213e80,
	0xb27dbdc3,
	0x092a35a2,
	0xd710adbf,
	0xa61fd7aa,
	0x4b24f11f,
	0x092a35a2,
	0xbd03ed67,
	0xc2fefbb5,
	0x38395bf3,
	0x60c9c0b3,
	0xb8e4e17e,
	0x0fc8db40,
	0xea78ea35,
	0xc5ceac9d,
	0xd272d446,
	0x90a48d82,
	0x056c43c7,
	0xb39b0442,
	0x9961a343,
	0xa3aee79a,
	0xd272d446,
	0xd272d446,
	0x8134d220,
	0xcb8b6ec6,
	0xdefbe19a,
	0x70eca2ca,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"_printk\0"
	"usb_find_interface\0"
	"_copy_from_user\0"
	"__kmalloc_noprof\0"
	"__check_object_size\0"
	"usb_control_msg\0"
	"_copy_to_user\0"
	"random_kmalloc_seed\0"
	"kmalloc_caches\0"
	"__kmalloc_cache_noprof\0"
	"__init_swait_queue_head\0"
	"usb_set_interface\0"
	"usb_alloc_urb\0"
	"usb_alloc_coherent\0"
	"usb_free_urb\0"
	"__stack_chk_fail\0"
	"__ubsan_handle_out_of_bounds\0"
	"usb_deregister\0"
	"usb_get_dev\0"
	"usb_register_dev\0"
	"_dev_err\0"
	"__fentry__\0"
	"__x86_return_thunk\0"
	"usb_register_driver\0"
	"kfree\0"
	"usb_deregister_dev\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v046Dp0837d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v046Dp046Dd*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v046Dp08C2d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v046Dp08CCd*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v046Dp0994d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "DCEB132FAE233653F3A1BF8");
