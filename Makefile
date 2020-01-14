include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
 
PKG_NAME:=bbp-k
PKG_RELEASE:=0.1
 
include $(INCLUDE_DIR)/package.mk

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \
	-DVERSION=$(PKG_RELEASE) --verbose

MAKE_OPTS:=$(KERNEL_MAKE_FLAGS) \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	CONFIG_BBP_K=m

define KernelPackage/bbp-k
	SUBMENU:=Other modules
	TITLE:=bbp-k
	FILES:=$(PKG_BUILD_DIR)/bbp-k.ko
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS) modules
endef

$(eval $(call KernelPackage,bbp-k))