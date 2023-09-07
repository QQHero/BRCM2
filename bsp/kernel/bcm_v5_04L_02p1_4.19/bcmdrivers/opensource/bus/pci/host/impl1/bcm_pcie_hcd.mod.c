#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=wlcsm";

MODULE_ALIAS("of:N*Tcpci*Cbrcm,bcm-pcie");
MODULE_ALIAS("of:N*Tcpci*Cbrcm,bcm-pcieC*");
MODULE_ALIAS("of:N*Tpci*Cbrcm,bcm-pcie");
MODULE_ALIAS("of:N*Tpci*Cbrcm,bcm-pcieC*");
MODULE_ALIAS("of:N*Tvpci*Cbrcm,bcm-pcie");
MODULE_ALIAS("of:N*Tvpci*Cbrcm,bcm-pcieC*");
MODULE_ALIAS("of:N*Tpci*Cbrcm,bcm963xx-pcie");
MODULE_ALIAS("of:N*Tpci*Cbrcm,bcm963xx-pcieC*");
MODULE_ALIAS("of:N*Tpci*Cbrcm,bcm947xx-pcie");
MODULE_ALIAS("of:N*Tpci*Cbrcm,bcm947xx-pcieC*");
MODULE_ALIAS("of:N*Tvpci*Cbrcm,bcm963xx-vpcie");
MODULE_ALIAS("of:N*Tvpci*Cbrcm,bcm963xx-vpcieC*");
