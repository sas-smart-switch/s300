################################################################################
#
# fio
#
################################################################################

FIO_VERSION = 2.8
FIO_SITE = https://fossies.org/linux/misc
FIO_LICENSE = GPLv2 + special obligations
FIO_LICENSE_FILES = LICENSE

ifeq ($(BR2_PACKAGE_LIBAIO),y)
FIO_DEPENDENCIES += libaio
endif

define FIO_CONFIGURE_CMDS
	(cd $(@D); ./configure --cc="$(TARGET_CC)" --extra-cflags="$(TARGET_CFLAGS)")
endef

define FIO_BUILD_CMDS
	$(MAKE) -C $(@D)
endef

define FIO_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/fio $(TARGET_DIR)/usr/bin/fio
endef

$(eval $(generic-package))
