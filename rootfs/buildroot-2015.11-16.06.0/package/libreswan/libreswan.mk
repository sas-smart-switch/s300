################################################################################
#
# libreswan
#
################################################################################

LIBRESWAN_VERSION = 3.17
LIBRESWAN_SITE = http://download.libreswan.org
LIBRESWAN_LICENSE = GPLv2+, BSD-3c
LIBRESWAN_LICENSE_FILES = COPYING LICENSE

LIBRESWAN_DEPENDENCIES = host-bison host-flex gmp iproute2 libnss
LIBRESWAN_MAKE_OPTS = ARCH=$(BR2_ARCH) CC="$(TARGET_CC)" \
	USERCOMPILE="$(TARGET_CFLAGS) $(if $(BR2_STATIC_LIBS),,-fPIE)" \
	USERLINK="$(TARGET_LDFLAGS) $(if $(BR2_STATIC_LIBS),,-fPIE)" \
	INC_USRLOCAL=/usr

# these option`s goal is KLIPS IPsec + OCF HW/SW crypto, for netkey use strongswan
LIBRESWAN_MAKE_OPTS += USE_DNSSEC=false \
			USE_KLIPS=true \
			HAVE_OPENSSL=true \
			USE_NETKEY=false \
			USE_LDAP=false \
			USE_EXTRACRYPTO=false \
			USE_XAUTHPAM=false \
			USE_LIBCAP_NG=false \
			USE_NM=false \
			USE_MAST=false

ifeq ($(BR2_PACKAGE_LIBCURL),y)
LIBRESWAN_DEPENDENCIES += libcurl
LIBRESWAN_MAKE_OPTS += USE_LIBCURL=true
else
LIBRESWAN_MAKE_OPTS += USE_LIBCURL=false
endif

ifeq ($(BR2_PACKAGE_OPENSSL),y)
LIBRESWAN_DEPENDENCIES += openssl
LIBRESWAN_MAKE_OPTS += HAVE_OPENSSL=true
endif

define LIBRESWAN_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE1) -C $(@D) \
		$(LIBRESWAN_MAKE_OPTS) base
endef

define LIBRESWAN_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE1) -C $(@D) \
		$(LIBRESWAN_MAKE_OPTS) DESTDIR=$(TARGET_DIR) INITSYSTEM=sysvinit install-base
endef

$(eval $(generic-package))
