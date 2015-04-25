#
# Copyright (C) 2007-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mac80211

PKG_VERSION:=2012-03-01
PKG_RELEASE:=1
PKG_SOURCE_URL:=http://www.lrc.ic.unicamp.br/~luciano/files
PKG_MD5SUM:=2798fe608c2af674fe21ebc4698c16dd

PKG_SOURCE:=compat-wireless-cogtra-$(PKG_VERSION).tar.bz2
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/compat-wireless-cogtra-$(PKG_VERSION)
PKG_BUILD_PARALLEL:=1

PKG_DRIVERS = \
	ath5k libertas-usb libertas-sd p54-common p54-pci p54-usb p54-spi \
	rt2x00-lib rt2x00-pci rt2x00-usb rt2x00-soc rt2800-lib rt2400-pci \
	rt2500-pci rt2500-usb rt61-pci rt73-usb rt2800-pci rt2800-usb \
	rtl8180 rtl8187 zd1211rw mac80211-hwsim carl9170 b43 b43legacy \
	ath9k-common ath9k ath9k-htc ath net-libipw net-ipw2100 net-ipw2200 \
	mwl8k net-hermes net-hermes-pci net-hermes-plx net-hermes-pcmcia \
	iwl-legacy iwl3945 iwl4965 iwlagn lib80211

PKG_CONFIG_DEPENDS:= \
	CONFIG_PACKAGE_kmod-mac80211 \
	$(patsubst %,CONFIG_PACKAGE_kmod-%,$(PKG_DRIVERS)) \
	CONFIG_PACKAGE_MAC80211_DEBUGFS \
	CONFIG_PACKAGE_ATH_DEBUG \
	CONFIG_PACKAGE_B43_DEBUG \
	CONFIG_PACKAGE_B43_PIO \
	CONFIG_PACKAGE_B43_N_PHY \
	CONFIG_ATH_USER_REGD \

CARL9170_FW_VERSION:=1.9.4

include $(INCLUDE_DIR)/package.mk

WMENU:=Wireless Drivers

define KernelPackage/mac80211/Default
  SUBMENU:=$(WMENU)
  URL:=http://linuxwireless.org/
  MAINTAINER:=Felix Fietkau <nbd@openwrt.org>
  DEPENDS:=@(!(TARGET_avr32||TARGET_ep93xx)||BROKEN) @!LINUX_2_4
endef

define KernelPackage/cfg80211
  $(call KernelPackage/mac80211/Default)
  TITLE:=cfg80211 - wireless configuration API
  DEPENDS+= +wireless-tools +iw +crda
  FILES:= \
	$(PKG_BUILD_DIR)/compat/compat.ko \
	$(PKG_BUILD_DIR)/net/wireless/cfg80211.ko
  AUTOLOAD:=$(call AutoLoad,20,compat cfg80211)
endef

define KernelPackage/cfg80211/description
cfg80211 is the Linux wireless LAN (802.11) configuration API.
endef

define KernelPackage/mac80211
  $(call KernelPackage/mac80211/Default)
  TITLE:=Linux 802.11 Wireless Networking Stack
  DEPENDS+= +kmod-crypto-core +kmod-crypto-arc4 +kmod-crypto-aes +kmod-cfg80211
  KCONFIG:=\
	CONFIG_AVERAGE=y
  FILES:= $(PKG_BUILD_DIR)/net/mac80211/mac80211.ko
  AUTOLOAD:=$(call AutoLoad,21,mac80211)
  MENU:=1
endef

define KernelPackage/mac80211/config
	menu "Configuration"
		depends on PACKAGE_kmod-mac80211

	config PACKAGE_MAC80211_DEBUGFS
		bool "Export mac80211 internals in DebugFS"
		default y
		help
		  Select this to see extensive information about
		  the internal state of mac80211 in debugfs.

		  Say N unless you know you need this.

	endmenu
endef

define KernelPackage/mac80211/description
Generic IEEE 802.11 Networking Stack (mac80211)
endef

PKG_LINUX_FIRMWARE_NAME:=linux-firmware
PKG_LINUX_FIRMWARE_VERSION:=d543c1d98fc240267ee59fff93f7a0f36d9e2fc3
PKG_LINUX_FIRMWARE_SOURCE:=$(PKG_LINUX_FIRMWARE_NAME)-$(PKG_LINUX_FIRMWARE_VERSION).tar.bz2
PKG_LINUX_FIRMWARE_PROTO:=git
# PKG_LINUX_FIRMWARE_SOURCE_URL:=git://git.kernel.org/pub/scm/linux/kernel/git/dwmw2/linux-firmware.git
PKG_LINUX_FIRMWARE_SOURCE_URL:=git://git.infradead.org/users/dwmw2/linux-firmware.git
PKG_LINUX_FIRMWARE_SUBDIR:=$(PKG_LINUX_FIRMWARE_NAME)-$(PKG_LINUX_FIRMWARE_VERSION)

define Download/linux-firmware
  FILE:=$(PKG_LINUX_FIRMWARE_SOURCE)
  URL:=$(PKG_LINUX_FIRMWARE_SOURCE_URL)
  MD5SUM:=$(PKG_LINUX_FIRMWARE_MD5SUM)
  PROTO:=$(PKG_LINUX_FIRMWARE_PROTO)
  VERSION:=$(PKG_LINUX_FIRMWARE_VERSION)
  SUBDIR:=$(PKG_LINUX_FIRMWARE_SUBDIR)
endef
$(eval $(call Download,linux-firmware))

# Prism54 drivers
P54PCIFW:=2.13.12.0.arm
P54USBFW:=2.13.24.0.lm87.arm
P54SPIFW:=2.13.0.0.a.13.14.arm
CARL9170_FW:=carl9170-1.fw

define Download/p54usb
  FILE:=$(P54USBFW)
  URL:=http://daemonizer.de/prism54/prism54-fw/fw-usb
  MD5SUM:=8e8ab005a4f8f0123bcdc51bc25b47f6
endef
$(eval $(call Download,p54usb))

define Download/p54pci
  FILE:=$(P54PCIFW)
  URL:=http://daemonizer.de/prism54/prism54-fw/fw-softmac
  MD5SUM:=ff7536af2092b1c4b21315bd103ef4c4
endef
$(eval $(call Download,p54pci))

define Download/p54spi
  FILE:=$(P54SPIFW)
  URL:=http://daemonizer.de/prism54/prism54-fw/stlc4560
  MD5SUM:=42661f8ecbadd88012807493f596081d
endef
$(eval $(call Download,p54spi))

define Download/carl9170
  FILE:=$(CARL9170_FW)-$(CARL9170_FW_VERSION)
  URL:=http://downloads.openwrt.org/sources/
  MD5SUM:=30e2ae80c33b3008d271556d1a14e3ea
endef
$(eval $(call Download,carl9170))

define KernelPackage/p54/Default
  $(call KernelPackage/mac80211/Default)
  TITLE:=Prism54 Drivers
endef

define KernelPackage/p54/description
  Kernel module for Prism54 chipsets (mac80211)
endef

define KernelPackage/p54-common
  $(call KernelPackage/p54/Default)
  DEPENDS+= @PCI_SUPPORT||@USB_SUPPORT||@TARGET_omap24xx +kmod-mac80211 +kmod-crc-ccitt
  TITLE+= (COMMON)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/p54/p54common.ko
  AUTOLOAD:=$(call AutoLoad,30,p54common)
endef

define KernelPackage/p54-pci
  $(call KernelPackage/p54/Default)
  TITLE+= (PCI)
  DEPENDS+= @PCI_SUPPORT +kmod-p54-common
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/p54/p54pci.ko
  AUTOLOAD:=$(call AutoLoad,31,p54pci)
endef

define KernelPackage/p54-usb
  $(call KernelPackage/p54/Default)
  TITLE+= (USB)
  DEPENDS+= @USB_SUPPORT +kmod-usb-core +kmod-p54-common
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/p54/p54usb.ko
  AUTOLOAD:=$(call AutoLoad,31,p54usb)
endef

define KernelPackage/p54-spi
  $(call KernelPackage/p54/Default)
  TITLE+= (SPI)
  DEPENDS+= @TARGET_omap24xx +kmod-p54-common
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/p54/p54spi.ko
  AUTOLOAD:=$(call AutoLoad,31,p54spi)
endef

NEED_RT2X00_LIB_CRYPTO:=y
NEED_RT2X00_LIB_FIRMWARE:=y
NEED_RT2X00_LIB_EEPROM:=y
NEED_RT2X00_LIB_HT:=y
NEED_RT2X00_LIB_LEDS:=y

define KernelPackage/rt2x00/Default
  $(call KernelPackage/mac80211/Default)
  TITLE:=Ralink Drivers for RT2x00 cards
endef

define KernelPackage/rt2x00-lib
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @(PCI_SUPPORT||USB_SUPPORT||TARGET_ramips) +kmod-mac80211 +kmod-crc-itu-t
  TITLE+= (LIB)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2x00lib.ko
  AUTOLOAD:=$(call AutoLoad,25,rt2x00lib)
  MENU:=1
endef

define KernelPackage/rt2x00-lib/config
	menu "Configuration"
		depends PACKAGE_kmod-rt2x00-lib

	config PACKAGE_RT2X00_LIB_DEBUGFS
		bool "Enable rt2x00 debugfs support"
		depends PACKAGE_MAC80211_DEBUGFS
		help
		  Enable creation of debugfs files for the rt2x00 drivers.
		  These debugfs files support both reading and writing of the
		  most important register types of the rt2x00 hardware.

	config PACKAGE_RT2X00_DEBUG
		bool "Enable rt2x00 debug output"
		help
		  Enable debugging output for all rt2x00 modules

	endmenu
endef

define KernelPackage/rt2x00-pci
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @(PCI_SUPPORT||TARGET_ramips) +kmod-rt2x00-lib +kmod-eeprom-93cx6
  TITLE+= (PCI)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2x00pci.ko
  AUTOLOAD:=$(call AutoLoad,26,rt2x00pci)
endef

define KernelPackage/rt2x00-usb
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @USB_SUPPORT +kmod-rt2x00-lib +kmod-usb-core
  TITLE+= (USB)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2x00usb.ko
  AUTOLOAD:=$(call AutoLoad,26,rt2x00usb)
endef

define KernelPackage/rt2x00-soc
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @TARGET_ramips +kmod-rt2x00-lib
  TITLE+= (SoC)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2x00soc.ko
  AUTOLOAD:=$(call AutoLoad,26,rt2x00soc)
endef

define KernelPackage/rt2800-lib
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @(PCI_SUPPORT||USB_SUPPORT||TARGET_ramips) +kmod-rt2x00-lib +TARGET_ramips:kmod-rt2x00-soc +@DRIVER_11N_SUPPORT
  TITLE+= (rt2800 LIB)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2800lib.ko
  AUTOLOAD:=$(call AutoLoad,27,rt2800lib)
endef

define KernelPackage/rt2400-pci
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @PCI_SUPPORT +kmod-rt2x00-pci
  TITLE+= (RT2400 PCI)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2400pci.ko
  AUTOLOAD:=$(call AutoLoad,27,rt2400pci)
endef

define KernelPackage/rt2500-pci
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @PCI_SUPPORT +kmod-rt2x00-pci
  TITLE+= (RT2500 PCI)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2500pci.ko
  AUTOLOAD:=$(call AutoLoad,27,rt2500pci)
endef

define KernelPackage/rt2500-usb
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @USB_SUPPORT +kmod-rt2x00-usb
  TITLE+= (RT2500 USB)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2500usb.ko
  AUTOLOAD:=$(call AutoLoad,27,rt2500usb)
endef

define KernelPackage/rt61-pci
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @PCI_SUPPORT +kmod-rt2x00-pci
  TITLE+= (RT2x61 PCI)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt61pci.ko
  AUTOLOAD:=$(call AutoLoad,27,rt61pci)
endef

define KernelPackage/rt73-usb
  $(call KernelPackage/rt2x00/Default)
  DEPENDS+= @USB_SUPPORT +kmod-rt2x00-usb
  TITLE+= (RT73 USB)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt73usb.ko
  AUTOLOAD:=$(call AutoLoad,27,rt73usb)
endef

define KernelPackage/rt2800-pci
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @(PCI_SUPPORT||TARGET_ramips) +kmod-rt2x00-pci +kmod-rt2800-lib +kmod-crc-ccitt +TARGET_ramips:kmod-rt2x00-soc
  TITLE+= (RT2860 PCI)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2800pci.ko
  AUTOLOAD:=$(call AutoLoad,28,rt2800pci)
endef

define KernelPackage/rt2800-usb
$(call KernelPackage/rt2x00/Default)
  DEPENDS+= @USB_SUPPORT +kmod-rt2x00-usb +kmod-rt2800-lib +kmod-crc-ccitt
  TITLE+= (RT2870 USB)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rt2x00/rt2800usb.ko
  AUTOLOAD:=$(call AutoLoad,28,rt2800usb)
endef

define KernelPackage/rtl818x/Default
  $(call KernelPackage/mac80211/Default)
  TITLE:=Realtek Drivers for RTL818x devices
  URL:=http://wireless.kernel.org/en/users/Drivers/rtl8187
  DEPENDS+= +kmod-eeprom-93cx6 +kmod-mac80211
endef

define KernelPackage/rtl8180
  $(call KernelPackage/rtl818x/Default)
  DEPENDS+= @PCI_SUPPORT
  TITLE+= (RTL8180 PCI)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rtl818x/rtl8180/rtl8180.ko
  AUTOLOAD:=$(call AutoLoad,27,rtl8180)
endef

define KernelPackage/rtl8187
$(call KernelPackage/rtl818x/Default)
  DEPENDS+= @USB_SUPPORT +kmod-usb-core
  TITLE+= (RTL8187 USB)
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/rtl818x/rtl8187/rtl8187.ko
  AUTOLOAD:=$(call AutoLoad,27,rtl8187)
endef

ZD1211FW_NAME:=zd1211-firmware
ZD1211FW_VERSION:=1.4
define Download/zd1211rw
  FILE:=$(ZD1211FW_NAME)-$(ZD1211FW_VERSION).tar.bz2
  URL:=@SF/zd1211/
  MD5SUM:=19f28781d76569af8551c9d11294c870
endef
$(eval $(call Download,zd1211rw))

define KernelPackage/zd1211rw
  $(call KernelPackage/mac80211/Default)
  TITLE:=Zydas ZD1211 support
  DEPENDS+= @USB_SUPPORT +kmod-usb-core +kmod-mac80211
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/zd1211rw/zd1211rw.ko
  AUTOLOAD:=$(call AutoLoad,60,zd1211rw)
endef

define KernelPackage/ath/config
	menu "Configuration"
		depends on PACKAGE_kmod-ath

	config ATH_USER_REGD
		bool "Force Atheros drivers to respect the user's regdomain settings"
		help
		  Atheros' idea of regulatory handling is that the EEPROM of the card defines
		  the regulatory limits and the user is only allowed to restrict the settings
		  even further, even if the country allows frequencies or power levels that
		  are forbidden by the EEPROM settings.

		  Select this option if you want the driver to respect the user's decision about
		  regulatory settings.

	config PACKAGE_ATH_DEBUG
		bool "Atheros wireless debugging"
		help
		  Say Y, if you want to debug atheros wireless drivers.
		  Right now only ath9k makes use of this.

	endmenu
endef

define KernelPackage/ath
  $(call KernelPackage/mac80211/Default)
  TITLE:=Atheros common driver part
  DEPENDS+= @PCI_SUPPORT||USB_SUPPORT||TARGET_ar71xx +kmod-mac80211
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath.ko
  AUTOLOAD:=$(call AutoLoad,26,ath)
  MENU:=1
endef

define KernelPackage/ath/description
 This module contains some common parts needed by Atheros Wireless drivers.
endef

define KernelPackage/ath5k
  $(call KernelPackage/mac80211/Default)
  TITLE:=Atheros 5xxx wireless cards support
  URL:=http://linuxwireless.org/en/users/Drivers/ath5k
  DEPENDS+= @PCI_SUPPORT||@TARGET_atheros +kmod-ath
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath5k/ath5k.ko
  AUTOLOAD:=$(call AutoLoad,27,ath5k)
endef

define KernelPackage/ath5k/description
 This module adds support for wireless adapters based on
 Atheros 5xxx chipset.
endef

define KernelPackage/ath9k-common
  $(call KernelPackage/mac80211/Default)
  TITLE:=Atheros 802.11n wireless devices (common code for ath9k and ath9k_htc)
  URL:=http://linuxwireless.org/en/users/Drivers/ath9k
  DEPENDS+= @PCI_SUPPORT||USB_SUPPORT||TARGET_ar71xx +kmod-ath +@DRIVER_11N_SUPPORT
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath9k/ath9k_common.ko \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath9k/ath9k_hw.ko
  AUTOLOAD:=$(call AutoLoad,27,ath9k_hw ath9k_common)
endef

define KernelPackage/ath9k
  $(call KernelPackage/mac80211/Default)
  TITLE:=Atheros 802.11n PCI wireless cards support
  URL:=http://linuxwireless.org/en/users/Drivers/ath9k
  DEPENDS+= @PCI_SUPPORT||TARGET_ar71xx +kmod-ath9k-common
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath9k/ath9k.ko
  AUTOLOAD:=$(call AutoLoad,28,ath9k)
endef

define KernelPackage/ath9k/description
This module adds support for wireless adapters based on
Atheros IEEE 802.11n AR5008 and AR9001 family of chipsets.
endef

define KernelPackage/ath9k-htc
  $(call KernelPackage/mac80211/Default)
  TITLE:=Atheros 802.11n USB device support
  URL:=http://linuxwireless.org/en/users/Drivers/ath9k
  DEPENDS+= @USB_SUPPORT +kmod-ath9k-common +kmod-usb-core
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/ath/ath9k/ath9k_htc.ko
  AUTOLOAD:=$(call AutoLoad,55,ath9k_htc)
endef

define KernelPackage/ath9k-htc/description
This module adds support for wireless adapters based on
Atheros USB AR9271 and AR7010 family of chipsets.
endef

define KernelPackage/carl9170
  $(call KernelPackage/mac80211/Default)
  TITLE:=Driver for Atheros AR9170 USB sticks
  DEPENDS:=@USB_SUPPORT +kmod-mac80211 +kmod-ath +kmod-usb-core +kmod-input-core +@DRIVER_11N_SUPPORT
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/ath/carl9170/carl9170.ko
  AUTOLOAD:=$(call AutoLoad,60,carl9170)
endef

define KernelPackage/carl9170/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(DL_DIR)/$(CARL9170_FW)-$(CARL9170_FW_VERSION) $(1)/lib/firmware/$(CARL9170_FW)
endef

define KernelPackage/lib80211
  $(call KernelPackage/mac80211/Default)
  TITLE:=802.11 Networking stack
  FILES:= \
	$(PKG_BUILD_DIR)/net/wireless/lib80211.ko \
	$(PKG_BUILD_DIR)/net/wireless/lib80211_crypt_wep.ko \
	$(PKG_BUILD_DIR)/net/wireless/lib80211_crypt_ccmp.ko \
	$(PKG_BUILD_DIR)/net/wireless/lib80211_crypt_tkip.ko
  AUTOLOAD:=$(call AutoLoad,10, \
	lib80211 \
	lib80211_crypt_wep \
	lib80211_crypt_ccmp \
	lib80211_crypt_tkip \
  )
endef

define KernelPackage/lib80211/description
 Kernel modules for 802.11 Networking stack
 Includes:
 - lib80211
 - lib80211_crypt_wep
 - lib80211_crypt_tkip
 - lib80211_crytp_ccmp
endef

define KernelPackage/libertas-usb
  $(call KernelPackage/mac80211/Default)
  DEPENDS+= @USB_SUPPORT +kmod-cfg80211 +kmod-usb-core +kmod-lib80211 +@DRIVER_WEXT_SUPPORT
  TITLE:=Marvell 88W8015 Wireless Driver
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/libertas/libertas.ko \
	$(PKG_BUILD_DIR)/drivers/net/wireless/libertas/usb8xxx.ko
  AUTOLOAD:=$(call AutoLoad,27,libertas usb8xxx)
endef

define KernelPackage/libertas-sd
  $(call KernelPackage/mac80211/Default)
  DEPENDS+= +kmod-cfg80211 +kmod-lib80211 +@DRIVER_WEXT_SUPPORT
  TITLE:=Marvell 88W8686 Wireless Driver
  FILES:= \
	$(PKG_BUILD_DIR)/drivers/net/wireless/libertas/libertas.ko \
	$(PKG_BUILD_DIR)/drivers/net/wireless/libertas/libertas_sdio.ko
  AUTOLOAD:=$(call AutoLoad,91,libertas libertas_sdio)
endef

define KernelPackage/mac80211-hwsim
  $(call KernelPackage/mac80211/Default)
  TITLE:=mac80211 HW simulation device
  DEPENDS+= +kmod-mac80211
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/mac80211_hwsim.ko
  AUTOLOAD:=$(call AutoLoad,60,mac80211_hwsim)
endef

define KernelPackage/net-libipw
  $(call KernelPackage/mac80211/Default)
  TITLE:=libipw for ipw2100 and ipw2200
  DEPENDS:=@PCI_SUPPORT +kmod-crypto-core +kmod-crypto-arc4 +kmod-crypto-aes +kmod-crypto-michael-mic +kmod-lib80211 +kmod-cfg80211 +@DRIVER_WEXT_SUPPORT
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/ipw2x00/libipw.ko
  AUTOLOAD:=$(call AutoLoad,49,libipw)
endef

define KernelPackage/net-libipw/description
 Hardware independent IEEE 802.11 networking stack for ipw2100 and ipw2200.
endef

IPW2100_NAME:=ipw2100-fw
IPW2100_VERSION:=1.3

define Download/net-ipw2100
  URL:=http://bughost.org/firmware/
  FILE:=$(IPW2100_NAME)-$(IPW2100_VERSION).tgz
  MD5SUM=46aa75bcda1a00efa841f9707bbbd113
endef
$(eval $(call Download,net-ipw2100))

define KernelPackage/net-ipw2100
  $(call KernelPackage/mac80211/Default)
  TITLE:=Intel IPW2100 driver
  DEPENDS:=@PCI_SUPPORT +kmod-net-libipw
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/ipw2x00/ipw2100.ko
  AUTOLOAD:=$(call AutoLoad,50,ipw2100)
endef

define KernelPackage/net-ipw2100/description
 Kernel support for Intel IPW2100
 Includes:
 - ipw2100
endef

IPW2200_NAME:=ipw2200-fw
IPW2200_VERSION:=3.1

define Download/net-ipw2200
  URL:=http://bughost.org/firmware/
  FILE:=$(IPW2200_NAME)-$(IPW2200_VERSION).tgz
  MD5SUM=eaba788643c7cc7483dd67ace70f6e99
endef
$(eval $(call Download,net-ipw2200))

define KernelPackage/net-ipw2200
  $(call KernelPackage/mac80211/Default)
  TITLE:=Intel IPW2200 driver
  DEPENDS:=@PCI_SUPPORT +kmod-net-libipw
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/ipw2x00/ipw2200.ko
  AUTOLOAD:=$(call AutoLoad,50,ipw2200)
endef

define KernelPackage/net-ipw2200/description
 Kernel support for Intel IPW2200
 Includes:
 - ipw2200
endef


define KernelPackage/net-hermes
  $(call KernelPackage/mac80211/Default)
  TITLE:=Hermes 802.11b chipset support
  DEPENDS:=@PCI_SUPPORT||PCMCIA_SUPPORT +kmod-cfg80211 +@DRIVER_WEXT_SUPPORT
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/orinoco/orinoco.ko
  AUTOLOAD:=$(call AutoLoad,50,orinoco)
endef

define KernelPackage/net-hermes/description
 Kernel support for Hermes 802.11b chipsets
endef

define KernelPackage/net-hermes-pci
  $(call KernelPackage/mac80211/Default)
  TITLE:=Intersil Prism 2.5 PCI support
  DEPENDS:=@PCI_SUPPORT +kmod-net-hermes
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/orinoco/orinoco_pci.ko
  AUTOLOAD:=$(call AutoLoad,55,orinoco_pci)
endef

define KernelPackage/net-hermes-pci/description
 Kernel modules for Intersil Prism 2.5 PCI support
endef

define KernelPackage/net-hermes-plx
  $(call KernelPackage/mac80211/Default)
  TITLE:=PLX9052 based PCI adaptor
  DEPENDS:=@PCI_SUPPORT +kmod-net-hermes
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/orinoco/orinoco_plx.ko
  AUTOLOAD:=$(call AutoLoad,55,orinoco_plx)
endef

define KernelPackage/net-hermes-plx/description
 Kernel modules for Hermes in PLX9052 based PCI adaptors
endef

define KernelPackage/net-hermes-pcmcia
  $(call KernelPackage/mac80211/Default)
  TITLE:=Hermes based PCMCIA adaptors
  DEPENDS:=@PCMCIA_SUPPORT +kmod-net-hermes @BROKEN
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/orinoco/orinoco_cs.ko
  AUTOLOAD:=$(call AutoLoad,55,orinoco_cs)
endef

define KernelPackage/net-hermes-pcmcia/description
 Kernel modules for Hermes based PCMCIA adaptors
endef

define KernelPackage/iwlagn
  $(call KernelPackage/mac80211/Default)
  DEPENDS:= +kmod-mac80211 @PCI_SUPPORT
  TITLE:=Intel AGN Wireless support
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/iwlwifi/iwlwifi.ko
  AUTOLOAD:=$(call AutoLoad,60,iwlagn)
  MENU:=1
endef

define KernelPackage/iwlagn/description
 iwlagn kernel module for Intel 5000/5150/1000/6000/6050/6005/6030/100 support
endef

define KernelPackage/iwlagn/config
	menu "Configuration"
		depends on PACKAGE_kmod-iwlagn

	config IWL5000_FW
		bool "Intel 5000 Firmware"
		default y
		help
		  Download and install firmware for:
		    Intel 5000 wireless card into /lib/firmware.

	config IWL5150_FW
		bool "Intel 5150 Firmware"
		default y
		help
		  Download and install firmware for:
		    Intel 5150 wireless card into /lib/firmware.

	config IWL1000_FW
		bool "Intel 1000 Firmware"
		default y
		help
		  Download and install firmware for:
		    Intel 1000 wireless card into /lib/firmware.

	config IWL6000_FW
		bool "Intel 6000 Firmware"
		default y
		help
		  Download and install firmware for:
		    Intel 6000 wireless card into /lib/firmware.

	config IWL6050_FW
		bool "Intel 6050 Firmware"
		default y
		help
		  Download and install firmware for:
		    Intel 6050 wireless card into /lib/firmware.

	config IWL6005_FW
		bool "Intel 6005 Firmware"
		default y
		help
		  Download and install firmware for:
		    Intel 6005 wireless card into /lib/firmware.

	config IWL6030_FW
		bool "Intel 6030 Firmware"
		default y
		help
		  Download and install firmware for:
		    Intel 6030 wireless card into /lib/firmware.

	config IWL100_FW
		bool "Intel 100 Firmware"
		default y
		help
		  Download and install firmware for:
		    Intel 100 wireless card into /lib/firmware.

	endmenu
endef

define KernelPackage/iwl-legacy
  $(call KernelPackage/mac80211/Default)
  DEPENDS:= +kmod-mac80211 @PCI_SUPPORT
  TITLE:=Intel legacy Wireless support
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/iwlegacy/iwl-legacy.ko
  AUTOLOAD:=$(call AutoLoad,60,iwl-legacy)
endef

define KernelPackage/iwl-legacy/description
 iwl-legacy kernel module for legacy Intel wireless support
endef

define KernelPackage/iwl3945
  $(call KernelPackage/mac80211/Default)
  DEPENDS:= +kmod-mac80211 +kmod-iwl-legacy
  TITLE:=Intel iwl3945 Wireless support
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/iwlegacy/iwl3945.ko
  AUTOLOAD:=$(call AutoLoad,61,iwl3945)
endef

define KernelPackage/iwl3945/description
 iwl3945 kernel module for Intel 3945 support
endef

define KernelPackage/iwl4965
  $(call KernelPackage/mac80211/Default)
  DEPENDS:= +kmod-mac80211 +kmod-iwl-legacy
  TITLE:=Intel iwl4965 Wireless support
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/iwlegacy/iwl4965.ko
  AUTOLOAD:=$(call AutoLoad,61,iwl4965)
endef

define KernelPackage/iwl4965/description
 iwl4965 kernel module for Intel 4965 support
endef


define KernelPackage/mwl8k
  $(call KernelPackage/mac80211/Default)
  TITLE:=Driver for Marvell TOPDOG 802.11 Wireless cards
  URL:=http://wireless.kernel.org/en/users/Drivers/mwl8k
  DEPENDS+= @PCI_SUPPORT +kmod-mac80211
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/mwl8k.ko
  AUTOLOAD:=$(call AutoLoad,27,mwl8k)
endef

define KernelPackage/mwl8k/description
 Kernel modules for Marvell TOPDOG 802.11 Wireless cards
endef

#Broadcom firmware
ifneq ($(CONFIG_B43_FW_5_10),)
  PKG_B43_FWV4_NAME:=broadcom-wl
  PKG_B43_FWV4_VERSION:=5.10.56.27.3
  PKG_B43_FWV4_OBJECT:=$(PKG_B43_FWV4_NAME)-$(PKG_B43_FWV4_VERSION)/driver/wl_apsta/wl_prebuilt.o
  PKG_B43_FWV4_SOURCE:=$(PKG_B43_FWV4_NAME)-$(PKG_B43_FWV4_VERSION)_mipsel.tar.bz2
  PKG_B43_FWV4_SOURCE_URL:=http://mirror2.openwrt.org/sources/
  PKG_B43_FWV4_MD5SUM:=3363e3a6b3d9d73c49dea870c7834eac
else
ifneq ($(CONFIG_B43_FW_4_178),)
  PKG_B43_FWV4_NAME:=broadcom-wl
  PKG_B43_FWV4_VERSION:=4.178.10.4
  PKG_B43_FWV4_OBJECT:=$(PKG_B43_FWV4_NAME)-$(PKG_B43_FWV4_VERSION)/linux/wl_apsta.o
  PKG_B43_FWV4_SOURCE:=$(PKG_B43_FWV4_NAME)-$(PKG_B43_FWV4_VERSION).tar.bz2
  PKG_B43_FWV4_SOURCE_URL:=http://mirror2.openwrt.org/sources/
  PKG_B43_FWV4_MD5SUM:=14477e8cbbb91b11896affac9b219fdb
else
  PKG_B43_FWV4_NAME:=broadcom-wl
  PKG_B43_FWV4_VERSION:=4.150.10.5
  PKG_B43_FWV4_OBJECT:=$(PKG_B43_FWV4_NAME)-$(PKG_B43_FWV4_VERSION)/driver/wl_apsta_mimo.o
  PKG_B43_FWV4_SOURCE:=$(PKG_B43_FWV4_NAME)-$(PKG_B43_FWV4_VERSION).tar.bz2
  PKG_B43_FWV4_SOURCE_URL:=http://mirror2.openwrt.org/sources/
  PKG_B43_FWV4_MD5SUM:=0c6ba9687114c6b598e8019e262d9a60
endif
endif
ifneq ($(CONFIG_B43_OPENFIRMWARE),)
  PKG_B43_FWV4_NAME:=broadcom-wl
  PKG_B43_FWV4_VERSION:=5.2
  PKG_B43_FWV4_OBJECT:=openfwwf-$(PKG_B43_FWV4_VERSION)
  PKG_B43_FWV4_SOURCE:=openfwwf-$(PKG_B43_FWV4_VERSION).tar.gz
  PKG_B43_FWV4_SOURCE_URL:=http://www.ing.unibs.it/openfwwf/firmware/
  PKG_B43_FWV4_MD5SUM:=e045a135453274e439ae183f8498b0fa
endif


PKG_B43_FWV3_NAME:=wl_apsta
PKG_B43_FWV3_VERSION:=3.130.20.0
PKG_B43_FWV3_SOURCE:=$(PKG_B43_FWV3_NAME)-$(PKG_B43_FWV3_VERSION).o
PKG_B43_FWV3_SOURCE_URL:=http://downloads.openwrt.org/sources/
PKG_B43_FWV3_MD5SUM:=e08665c5c5b66beb9c3b2dd54aa80cb3

ifeq ($(CONFIG_B43_OPENFIRMWARE),y)
  PKG_B43_FWCUTTER_NAME:=b43-fwcutter
  PKG_B43_FWCUTTER_VERSION:=3e69531aa65b8f664a0ab00dfc3e2eefeb0cb417
  PKG_B43_FWCUTTER_SOURCE:=$(PKG_B43_FWCUTTER_NAME)-$(PKG_B43_FWCUTTER_VERSION).tar.bz2
  PKG_B43_FWCUTTER_PROTO:=git
  PKG_B43_FWCUTTER_SOURCE_URL:=http://git.bu3sch.de/git/b43-tools.git
  PKG_B43_FWCUTTER_SUBDIR:=$(PKG_B43_FWCUTTER_NAME)-$(PKG_B43_FWCUTTER_VERSION)
  PKG_B43_FWCUTTER_OBJECT:=$(PKG_B43_FWCUTTER_NAME)-$(PKG_B43_FWCUTTER_VERSION)/fwcutter/
else
  PKG_B43_FWCUTTER_NAME:=b43-fwcutter
  PKG_B43_FWCUTTER_VERSION:=014
  PKG_B43_FWCUTTER_SOURCE:=$(PKG_B43_FWCUTTER_NAME)-$(PKG_B43_FWCUTTER_VERSION).tar.bz2
  PKG_B43_FWCUTTER_PROTO:=default
  PKG_B43_FWCUTTER_SOURCE_URL:=http://bu3sch.de/b43/fwcutter/
  PKG_B43_FWCUTTER_MD5SUM:=dd63f9149510bd6e01b89314e955083b
  PKG_B43_FWCUTTER_SUBDIR:=b43-fwcutter-$(PKG_B43_FWCUTTER_VERSION)
  PKG_B43_FWCUTTER_OBJECT:=$(PKG_B43_FWCUTTER_NAME)-$(PKG_B43_FWCUTTER_VERSION)/
endif

define Download/b43-common
  FILE:=$(PKG_B43_FWCUTTER_SOURCE)
  URL:=$(PKG_B43_FWCUTTER_SOURCE_URL)
  MD5SUM:=$(PKG_B43_FWCUTTER_MD5SUM)
  PROTO:=$(PKG_B43_FWCUTTER_PROTO)
  VERSION:=$(PKG_B43_FWCUTTER_VERSION)
  SUBDIR:=$(PKG_B43_FWCUTTER_SUBDIR)
endef
$(eval $(call Download,b43-common))

define Download/b43
  FILE:=$(PKG_B43_FWV4_SOURCE)
  URL:=$(PKG_B43_FWV4_SOURCE_URL)
  MD5SUM:=$(PKG_B43_FWV4_MD5SUM)
endef
$(eval $(call Download,b43))

define Download/b43legacy
  FILE:=$(PKG_B43_FWV3_SOURCE)
  URL:=$(PKG_B43_FWV3_SOURCE_URL)
  MD5SUM:=$(PKG_B43_FWV3_MD5SUM)
endef
$(eval $(call Download,b43legacy))

define KernelPackage/b43-common
  $(call KernelPackage/mac80211/Default)
  TITLE:=Generic stuff for Broadcom wireless devices
  URL:=http://linuxwireless.org/en/users/Drivers/b43
  KCONFIG:= \
  	CONFIG_HW_RANDOM=y
  DEPENDS+= +kmod-mac80211 +!(TARGET_brcm47xx||TARGET_brcm63xx):kmod-ssb
endef

define KernelPackage/b43
$(call KernelPackage/b43-common)
  TITLE:=Broadcom 43xx wireless support
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/b43/b43.ko
  AUTOLOAD:=$(call AutoLoad,30,b43)
  MENU:=1
endef

define KernelPackage/b43/config
	menu "Configuration"
		depends on PACKAGE_kmod-b43

	choice
		prompt "b43 firmware version"
		default B43_FW_5_10
		help
		  This option allows you to select the version of the b43 firmware.

	config B43_FW_4_150
		bool "Firmware 410.2160 from driver 4.150.10.5 (stable)"
		help
		  Stable firmware for BCM43xx devices.

		  If unsure, select this.

	config B43_FW_4_178
		bool "Firmware 478.104 from driver 4.178.10.4"
		help
		  Experimental firmware for BCM43xx devices.

		  This firmware is not tested as much as the "stable" firmware.

		  If unsure, select the "stable" firmware.

	config B43_FW_5_10
		bool "Firmware 508.1084 from driver 5.10.56.27"
		help
		  Newer experimental firmware for BCM43xx devices.

		  This firmware is mostly untested. It is needed for some N-PHY devices.

		  If unsure, select the "stable" firmware.

	config B43_OPENFIRMWARE
		bool "Open FirmWare for WiFi networks"
		help
		  Opensource firmware for BCM43xx devices.

		  Do _not_ select this, unless you know what you are doing.
		  The Opensource firmware is not suitable for embedded devices, yet.
		  It does not support QoS, which is bad for AccessPoints.
		  It does not support hardware crypto acceleration, which is a showstopper
		  for embedded devices with low CPU resources.

		  If unsure, select the "stable" firmware.

	endchoice

	config B43_FW_SQUASH
		bool "Remove unnecessary firmware files"
		depends on !B43_OPENFIRMWARE
		default y
		help
		  This options allows you to remove unnecessary b43 firmware files
		  from the final rootfs image. This can reduce the rootfs size by
		  up to 200k.

		  If unsure, say Y.

	config B43_FW_SQUASH_COREREVS
		string "Core revisions to include"
		depends on B43_FW_SQUASH
		default "5,6,7,8,9,10,13,14,15"
		help
		  This is a comma seperated list of core revision numbers.

		  Example (keep files for rev5 only):
		    5

		  Example (keep files for rev5 and rev11):
		    5,11

	config B43_FW_SQUASH_PHYTYPES
		string "PHY types to include"
		depends on B43_FW_SQUASH
		default "G,LP"
		help
		  This is a comma seperated list of PHY types:
		    A  => A-PHY
		    AG => Dual A-PHY G-PHY
		    G  => G-PHY
		    LP => LP-PHY
		    N  => N-PHY

		  Example (keep files for G-PHY only):
		    G

		  Example (keep files for G-PHY and N-PHY):
		    G,N

	config PACKAGE_B43_DEBUG
		bool "Enable debug output and debugfs for b43"
		default n
		help
		  Enable additional debug output and runtime sanity checks for b43
		  and enables the debugfs interface.

		  If unsure, say N.

	config PACKAGE_B43_PIO
		bool "Enable support for PIO transfer mode"
		depends BROKEN
		default n
		help
		  Enable support for using PIO instead of DMA. Unless you have DMA
		  transfer problems you don't need this.

		  If unsure, say N.

	config PACKAGE_B43_PHY_N
		bool "Enable support for N-PHYs"
		select B43_FW_5_10
		depends BROKEN
		default n
		help
		  Enable support for BCM4321 and BCM4322.

		  Currently only 11g speed is available.

		  If unsure, say N.

	endmenu
endef

define KernelPackage/b43/description
Kernel module for Broadcom 43xx wireless support (mac80211 stack) new
endef

define KernelPackage/b43legacy
$(call KernelPackage/b43-common)
  TITLE:=Broadcom 43xx-legacy wireless support
  FILES:=$(PKG_BUILD_DIR)/drivers/net/wireless/b43legacy/b43legacy.ko
  AUTOLOAD:=$(call AutoLoad,30,b43legacy)
  MENU:=1
endef

define KernelPackage/b43legacy/config
	menu "Configuration"
		depends on PACKAGE_kmod-b43legacy

	config B43LEGACY_FW_SQUASH
		bool "Remove unnecessary firmware files"
		default y
		help
		  This options allows you to remove unnecessary b43legacy firmware files
		  from the final rootfs image. This can reduce the rootfs size by
		  up to 50k.

		  If unsure, say Y.

	config B43LEGACY_FW_SQUASH_COREREVS
		string "Core revisions to include"
		depends on B43LEGACY_FW_SQUASH
		default "1,2,3,4"
		help
		  This is a comma seperated list of core revision numbers.

		  Example (keep files for rev4 only):
		    4

		  Example (keep files for rev2 and rev4):
		    2,4

	endmenu
endef

define KernelPackage/b43legacy/description
Kernel module for Broadcom 43xx-legacy wireless support (mac80211 stack) new
endef

BUILDFLAGS:= \
	-I$(PKG_BUILD_DIR)/include \
	$(foreach opt,$(CONFOPTS),-DCONFIG_$(opt)) \
	$(if $(CONFIG_PCI),-DCONFIG_B43_PCI_AUTOSELECT -DCONFIG_B43_PCICORE_AUTOSELECT) \
	$(if $(CONFIG_LEDS_TRIGGERS), -DCONFIG_MAC80211_LEDS -DCONFIG_LEDS_TRIGGERS -DCONFIG_B43_LEDS -DCONFIG_B43LEGACY_LEDS) \
	-DCONFIG_B43_HWRNG -DCONFIG_B43LEGACY_HWRNG \
	$(if $(CONFIG_PCI),-DCONFIG_ATH9K_PCI) \
	$(if $(CONFIG_TARGET_ar71xx),-DCONFIG_ATH9K_AHB) \
	$(if $(CONFIG_PACKAGE_MAC80211_DEBUGFS), -DCONFIG_CFG80211_DEBUGFS -DCONFIG_MAC80211_DEBUGFS -DCONFIG_ATH9K_DEBUGFS -DCONFIG_CARL9170_DEBUGFS -DCONFIG_ATH9K_HTC_DEBUGFS -DCONFIG_ATH5K_DEBUG) \
	$(if $(CONFIG_PACKAGE_ATH_DEBUG),-DCONFIG_ATH_DEBUG -DCONFIG_ATH9K_PKTLOG) \
	-D__CONFIG_MAC80211_RC_DEFAULT=cogtra_ht \
	$(if $(CONFIG_ATH_USER_REGD),-DATH_USER_REGD=1) \
	$(if $(CONFIG_PACKAGE_B43_DEBUG),-DCONFIG_B43_DEBUG) \
	$(if $(CONFIG_PACKAGE_B43_PIO),-DCONFIG_B43_PIO) \
	$(if $(CONFIG_PACKAGE_B43_PHY_N),-DCONFIG_B43_PHY_N) \
	-DCONFIG_B43_SSB \
	$(if $(CONFIG_PACKAGE_RT2X00_LIB_DEBUGFS),-DCONFIG_RT2X00_LIB_DEBUGFS) \
	$(if $(CONFIG_PACKAGE_RT2X00_DEBUG),-DCONFIG_RT2X00_DEBUG) \
	$(if $(NEED_RT2X00_LIB_HT),-DCONFIG_RT2X00_LIB_HT) \
	$(if $(NEED_RT2X00_LIB_CRYPTO),-DCONFIG_RT2X00_LIB_CRYPTO) \
	$(if $(NEED_RT2X00_LIB_FIRMWARE),-DCONFIG_RT2X00_LIB_FIRMWARE) \
	$(if $(NEED_RT2X00_LIB_EEPROM),-DCONFIG_RT2X00_LIB_EEPROM) \
	$(if $(NEED_RT2X00_LIB_LEDS),-DCONFIG_RT2X00_LIB_LEDS) \
	$(if $(CONFIG_PACKAGE_kmod-rt2x00-pci),-DCONFIG_RT2X00_LIB_PCI) \
	$(if $(CONFIG_PACKAGE_kmod-rt2x00-usb),-DCONFIG_RT2X00_LIB_USB) \
	$(if $(CONFIG_PACKAGE_kmod-rt2x00-soc),-DCONFIG_RT2X00_LIB_SOC) \
	$(if $(CONFIG_TARGET_atheros),-DCONFIG_ATH5K_AHB,-DCONFIG_ATH5K_PCI) \
	$(if $(CONFIG_PACKAGE_kmod-iwl3945),-DCONFIG_IWL3945) \
	$(if $(CONFIG_PACKAGE_kmod-iwl4965),-DCONFIG_COMPAT_IWL4965)

MAKE_OPTS:= \
	CROSS_COMPILE="$(KERNEL_CROSS)" \
	ARCH="$(LINUX_KARCH)" \
	EXTRA_CFLAGS="$(BUILDFLAGS)" \
	$(foreach opt,$(CONFOPTS),CONFIG_$(opt)=m) \
	CONFIG_CFG80211=$(if $(CONFIG_PACKAGE_kmod-cfg80211),m) \
	CONFIG_MAC80211=$(if $(CONFIG_PACKAGE_kmod-mac80211),m) \
	CONFIG_MAC80211_RC_ARF=n \
	CONFIG_MAC80211_RC_AARF=n \
	CONFIG_MAC80211_RC_CORA=n \
	CONFIG_MAC80211_RC_COGTRA=n \
	CONFIG_MAC80211_RC_COGTRA_HT=y \
	CONFIG_MAC80211_RC_MINSTREL=n \
	CONFIG_MAC80211_RC_MINSTREL_HT=n \
	CONFIG_MAC80211_LEDS=$(CONFIG_LEDS_TRIGGERS) \
	CONFIG_CFG80211_DEBUGFS=$(if $(CONFIG_PACKAGE_MAC80211_DEBUGFS),y) \
	CONFIG_MAC80211_DEBUGFS=$(if $(CONFIG_PACKAGE_MAC80211_DEBUGFS),y) \
	CONFIG_B43_PCMCIA=n CONFIG_B43_PIO=n \
	CONFIG_B43_PCI_AUTOSELECT=$(if $(CONFIG_PCI),y) \
	CONFIG_B43_PCICORE_AUTOSELECT=$(if $(CONFIG_PCI),y) \
	CONFIG_B43LEGACY_LEDS=$(CONFIG_LEDS_TRIGGERS) \
	CONFIG_B43_LEDS=$(CONFIG_LEDS_TRIGGERS) \
	CONFIG_B43_HWRNG=$(if $(CONFIG_HW_RANDOM),y) \
	CONFIG_B43LEGACY_HWRNG=$(if $(CONFIG_HW_RANDOM),y) \
	CONFIG_B43=$(if $(CONFIG_PACKAGE_kmod-b43),m) \
	CONFIG_B43LEGACY=$(if $(CONFIG_PACKAGE_kmod-b43legacy),m) \
	CONFIG_B43_DEBUG=$(if $(CONFIG_PACKAGE_B43_DEBUG),y) \
	CONFIG_B43_PIO=$(if $(CONFIG_PACKAGE_B43_PIO),y) \
	CONFIG_B43_PHY_N=$(if $(CONFIG_PACKAGE_B43_PHY_N),y) \
	CONFIG_B43_SSB=y \
	CONFIG_ATH_COMMON=$(if $(CONFIG_PACKAGE_kmod-ath),m) \
	CONFIG_ATH_DEBUG=$(if $(CONFIG_PACKAGE_ATH_DEBUG),y) \
	CONFIG_ATH9K_PKTLOG=$(if $(CONFIG_PACKAGE_ATH_DEBUG),y) \
	CONFIG_ATH5K=$(if $(CONFIG_PACKAGE_kmod-ath5k),m) \
	CONFIG_ATH5K_PCI=$(if $(CONFIG_TARGET_atheros),,y) \
	CONFIG_ATH5K_AHB=$(if $(CONFIG_TARGET_atheros),y) \
	CONFIG_ATH5K_DEBUG=$(if $(CONFIG_PACKAGE_MAC80211_DEBUGFS),y) \
	CONFIG_ATH9K=$(if $(CONFIG_PACKAGE_kmod-ath9k),m) \
	CONFIG_ATH9K_PCI=$(CONFIG_PCI) \
	CONFIG_ATH9K_AHB=$(if $(CONFIG_TARGET_ar71xx),y) \
	CONFIG_ATH9K_HTC=$(if $(CONFIG_PACKAGE_kmod-ath9k-htc),m) \
	CONFIG_ATH9K_HTC_DEBUGFS=$(if $(CONFIG_PACKAGE_MAC80211_DEBUGFS),y) \
	CONFIG_ATH9K_HW=$(if $(CONFIG_PACKAGE_kmod-ath9k-common),m) \
	CONFIG_ATH9K_COMMON=$(if $(CONFIG_PACKAGE_kmod-ath9k-common),m) \
	CONFIG_ATH9K_DEBUGFS=$(if $(CONFIG_PACKAGE_MAC80211_DEBUGFS),y) \
	CONFIG_CARL9170=$(if $(CONFIG_PACKAGE_kmod-carl9170),m) \
	CONFIG_CARL9170_DEBUGFS=$(if $(CONFIG_PACKAGE_MAC80211_DEBUGFS),y) \
	CONFIG_COMPAT_ZD1211RW=$(if $(CONFIG_PACKAGE_kmod-zd1211rw),m) \
	CONFIG_P54_COMMON=$(if $(CONFIG_PACKAGE_kmod-p54-common),m) \
	CONFIG_P54_PCI=$(if $(CONFIG_PACKAGE_kmod-p54-pci),m) \
	CONFIG_P54_USB=$(if $(CONFIG_PACKAGE_kmod-p54-usb),m) \
	CONFIG_P54_SPI=$(if $(CONFIG_PACKAGE_kmod-p54-spi),m) \
	CONFIG_P54_SPI_DEFAULT_EEPROM=n \
	CONFIG_RT2X00=$(if $(CONFIG_PACKAGE_kmod-rt2x00-lib),m) \
	CONFIG_RT2X00_LIB=$(if $(CONFIG_PACKAGE_kmod-rt2x00-lib),m) \
	CONFIG_RT2X00_LIB_PCI=$(if $(CONFIG_PACKAGE_kmod-rt2x00-pci),m) \
	CONFIG_RT2X00_LIB_USB=$(if $(CONFIG_PACKAGE_kmod-rt2x00-usb),m) \
	CONFIG_RT2X00_LIB_SOC=$(if $(CONFIG_PACKAGE_kmod-rt2x00-soc),m) \
	CONFIG_RT2X00_LIB_DEBUGFS=$(CONFIG_PACKAGE_RT2X00_LIB_DEBUGFS) \
	CONFIG_RT2X00_LIB_CRYPTO=$(NEED_RT2X00_LIB_CRYPTO) \
	CONFIG_RT2X00_LIB_FIRMWARE=$(NEED_RT2X00_LIB_FIRMWARE) \
	CONFIG_RT2X00_LIB_EEPROM=$(NEED_RT2X00_LIB_EEPROM) \
	CONFIG_RT2X00_LIB_HT=$(NEED_RT2X00_LIB_HT) \
	CONFIG_RT2X00_LIB_LEDS=$(NEED_RT2X00_LIB_LEDS) \
	CONFIG_RT2400PCI=$(if $(CONFIG_PACKAGE_kmod-rt2400-pci),m) \
	CONFIG_RT2500PCI=$(if $(CONFIG_PACKAGE_kmod-rt2500-pci),m) \
	CONFIG_RT2500USB=$(if $(CONFIG_PACKAGE_kmod-rt2500-usb),m) \
	CONFIG_RT61PCI=$(if $(CONFIG_PACKAGE_kmod-rt61-pci),m) \
	CONFIG_RT73USB=$(if $(CONFIG_PACKAGE_kmod-rt73-usb),m) \
	CONFIG_RT2800_LIB=$(if $(CONFIG_PACKAGE_kmod-rt2800-lib),m) \
	CONFIG_RT2800PCI=$(if $(CONFIG_PACKAGE_kmod-rt2800-pci),m) \
	CONFIG_RT2800USB=$(if $(CONFIG_PACKAGE_kmod-rt2800-usb),m) \
	CONFIG_RT2800USB_RT53XX=$(if $(CONFIG_PACKAGE_kmod-rt2800-usb),y) \
	CONFIG_RTL8180=$(if $(CONFIG_PACKAGE_kmod-rtl8180),m) \
	CONFIG_RTL8187=$(if $(CONFIG_PACKAGE_kmod-rtl8187),m) \
	CONFIG_RTL8192CE= \
	CONFIG_RTLWIFI= \
	CONFIG_MAC80211_HWSIM=$(if $(CONFIG_PACKAGE_kmod-mac80211-hwsim),m) \
	CONFIG_PCMCIA= \
	CONFIG_LIBIPW=$(if $(CONFIG_PACKAGE_kmod-net-libipw),m) \
	CONFIG_LIBERTAS=$(if $(CONFIG_PACKAGE_kmod-libertas-sd)$(CONFIG_PACKAGE_kmod-libertas-usb),m) \
	CONFIG_LIBERTAS_CS= \
	CONFIG_LIBERTAS_SPI= \
	CONFIG_COMPAT_LIBERTAS_SDIO=$(if $(CONFIG_PACKAGE_kmod-libertas-sd),m) \
	CONFIG_LIBERTAS_THINFIRM= \
	CONFIG_LIBERTAS_USB=$(if $(CONFIG_PACKAGE_kmod-libertas-usb),m) \
	CONFIG_IPW2100=$(if $(CONFIG_PACKAGE_kmod-net-ipw2100),m) \
	CONFIG_IPW2200=$(if $(CONFIG_PACKAGE_kmod-net-ipw2200),m) \
	CONFIG_NL80211=y \
	CONFIG_LIB80211=$(if $(CONFIG_PACKAGE_kmod-lib80211),m) \
	CONFIG_LIB80211_CRYPT_WEP=$(if $(CONFIG_PACKAGE_kmod-lib80211),m) \
	CONFIG_LIB80211_CRYPT_CCMP=$(if $(CONFIG_PACKAGE_kmod-lib80211),m) \
	CONFIG_LIB80211_CRYPT_TKIP=$(if $(CONFIG_PACKAGE_kmod-lib80211),m) \
	CONFIG_IWLWIFI=$(if $(CONFIG_PACKAGE_kmod-iwlagn),m) \
	CONFIG_IWLWIFI_LEGACY=$(if $(CONFIG_PACKAGE_kmod-iwl-legacy),m) \
	CONFIG_COMPAT_IWL4965=$(if $(CONFIG_PACKAGE_kmod-iwl4965),m) \
	CONFIG_IWL3945=$(if $(CONFIG_PACKAGE_kmod-iwl3945),m) \
	CONFIG_MWL8K=$(if $(CONFIG_PACKAGE_kmod-mwl8k),m) \
	CONFIG_ATMEL= \
	CONFIG_PCMCIA_ATMEL= \
	CONFIG_ADM8211= \
	CONFIG_USB_NET_RNDIS_HOST= \
	CONFIG_USB_NET_RNDIS_WLAN= \
	CONFIG_USB_NET_CDCETHER= \
	CONFIG_USB_USBNET= \
	CONFIG_AT76C50X_USB= \
	CONFIG_WL12XX= \
	CONFIG_EEPROM_93CX6= \
	CONFIG_HERMES=$(if $(CONFIG_PACKAGE_kmod-net-hermes),m) \
	CONFIG_HERMES_CACHE_FW_ON_INIT= \
	CONFIG_PCI_HERMES=$(if $(CONFIG_PACKAGE_kmod-net-hermes-pci),m) \
	CONFIG_PLX_HERMES=$(if $(CONFIG_PACKAGE_kmod-net-hermes-plx),m) \
	CONFIG_PCMCIA_HERMES=$(if $(CONFIG_PACKAGE_kmod-net-hermes-pcmcia),m) \
	CONFIG_HERMES_PRISM= \
	CONFIG_APPLE_AIRPORT= \
	CONFIG_TMD_HERMES= \
	CONFIG_NORTEL_HERMES= \
	CONFIG_PCMCIA_SPECTRUM= \
	CONFIG_ORINOCO_USB= \
	CONFIG_IWM= \
	CONFIG_MWIFIEX= \
	MADWIFI= \
	CONFIG_B44= \
	CONFIG_ATL1= \
	CONFIG_ATL2= \
	CONFIG_ATL1E= \
	CONFIG_ATL1C= \
	CONFIG_BRCMFMAC= \
	CONFIG_BRCMUMAC= \
	CONFIG_BRCMSMAC= \
	KLIB_BUILD="$(LINUX_DIR)" \
	MODPROBE=: \
	KLIB=$(TARGET_MODULES_DIR) \
	KERNEL_SUBLEVEL=$(lastword $(subst ., ,$(KERNEL_PATCHVER))) \
	KBUILD_LDFLAGS_MODULE_PREREQ=

ifeq ($(CONFIG_PACKAGE_kmod-libertas-sd)$(CONFIG_PACKAGE_kmod-libertas-usb)$(CONFIG_PACKAGE_kmod-rt2x00-lib),)
  MAKE_OPTS += CONFIG_COMPAT_KFIFO=
endif

define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)
	$(PKG_UNPACK)
	$(Build/Patch)
	$(TAR) -C $(PKG_BUILD_DIR) -xzf $(DL_DIR)/$(IPW2100_NAME)-$(IPW2100_VERSION).tgz
	$(TAR) -C $(PKG_BUILD_DIR) -xzf $(DL_DIR)/$(IPW2200_NAME)-$(IPW2200_VERSION).tgz
	$(TAR) -C $(PKG_BUILD_DIR) -xjf $(DL_DIR)/$(ZD1211FW_NAME)-$(ZD1211FW_VERSION).tar.bz2
	$(TAR) -C $(PKG_BUILD_DIR) -xjf $(DL_DIR)/$(PKG_LINUX_FIRMWARE_SOURCE)
	rm -rf $(PKG_BUILD_DIR)/include/linux/ssb
	rm -f $(PKG_BUILD_DIR)/include/net/ieee80211.h
endef

ifneq ($(CONFIG_PACKAGE_kmod-cfg80211)$(CONFIG_PACKAGE_kmod-lib80211),)
 define Build/Compile/kmod
	rm -rf $(PKG_BUILD_DIR)/modules
	$(MAKE) $(PKG_JOBS) -C "$(PKG_BUILD_DIR)" $(MAKE_OPTS) all
 endef
endif

define Build/Compile
	$(call Build/Compile/kmod)
endef

define Build/InstallDev
	mkdir -p \
		$(1)/usr/include/mac80211 \
		$(1)/usr/include/mac80211/ath \
		$(1)/usr/include/net/mac80211
	$(CP) $(PKG_BUILD_DIR)/net/mac80211/*.h $(PKG_BUILD_DIR)/include/* $(1)/usr/include/mac80211/
	$(CP) $(PKG_BUILD_DIR)/net/mac80211/rate.h $(1)/usr/include/net/mac80211/
	$(CP) $(PKG_BUILD_DIR)/drivers/net/wireless/ath/*.h $(1)/usr/include/mac80211/ath/
endef

define KernelPackage/libertas-usb/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/usb8388_v9.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/usb8388_v5.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/usb8682.bin \
		$(1)/lib/firmware/
endef

define KernelPackage/libertas-sd/install
	$(INSTALL_DIR) $(1)/lib/firmware/libertas
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/sd8385_helper.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/sd8385.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/sd8686_v9_helper.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/sd8686_v9.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/sd8686_v8_helper.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/sd8686_v8.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/sd8688_helper.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/libertas/sd8688.bin \
		$(1)/lib/firmware/libertas
endef

define KernelPackage/cfg80211/install
	$(INSTALL_DIR) $(1)/lib/wifi
	$(INSTALL_DATA) ./files/lib/wifi/mac80211.sh $(1)/lib/wifi
endef

define KernelPackage/p54-pci/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(DL_DIR)/$(P54PCIFW) $(1)/lib/firmware/isl3886pci
endef

define KernelPackage/p54-usb/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(DL_DIR)/$(P54USBFW) $(1)/lib/firmware/isl3887usb
endef

define KernelPackage/p54-spi/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(DL_DIR)/$(P54SPIFW) $(1)/lib/firmware/3826.arm
endef

define KernelPackage/rt61-pci/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/rt2561.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/rt2561s.bin \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/rt2661.bin \
		$(1)/lib/firmware/
endef

define KernelPackage/rt73-usb/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/rt73.bin $(1)/lib/firmware/
endef

define KernelPackage/rt2800-pci/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/rt2860.bin $(1)/lib/firmware/
endef

define KernelPackage/rt2800-usb/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/rt2870.bin $(1)/lib/firmware/
endef

define KernelPackage/zd1211rw/install
	$(INSTALL_DIR) $(1)/lib/firmware/zd1211
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(ZD1211FW_NAME)/zd1211* $(1)/lib/firmware/zd1211
endef

define KernelPackage/ath9k-htc/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/htc_9271.fw \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/htc_7010.fw \
		$(1)/lib/firmware/
endef

define KernelPackage/mwl8k/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/mwl8k/fmimage_8366_ap-2.fw \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/mwl8k/fmimage_8366.fw \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/mwl8k/helper_8366.fw \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/mwl8k/fmimage_8687.fw \
		$(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/mwl8k/helper_8687.fw \
		$(1)/lib/firmware/
endef

define KernelPackage/net-ipw2100/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/ipw2100-$(IPW2100_VERSION)*.fw $(1)/lib/firmware
endef

define KernelPackage/net-ipw2200/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(IPW2200_NAME)-$(IPW2200_VERSION)/ipw2200*.fw $(1)/lib/firmware
endef

define KernelPackage/iwlagn/install
        $(INSTALL_DIR) $(1)/lib/firmware
ifneq ($(CONFIG_IWL5000_FW),)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-5000-5.ucode $(1)/lib/firmware
endif
ifneq ($(CONFIG_IWL5150_FW),)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-5150-2.ucode $(1)/lib/firmware
endif
ifneq ($(CONFIG_IWL1000_FW),)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-1000-5.ucode $(1)/lib/firmware
endif
ifneq ($(CONFIG_IWL6000_FW),)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-6000-4.ucode $(1)/lib/firmware
endif
ifneq ($(CONFIG_IWL6050_FW),)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-6050-5.ucode $(1)/lib/firmware
endif
ifneq ($(CONFIG_IWL6005_FW),)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-6000g2a-5.ucode $(1)/lib/firmware
endif
ifneq ($(CONFIG_IWL6030_FW),)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-6000g2b-5.ucode $(1)/lib/firmware
endif
ifneq ($(CONFIG_IWL100_FW),)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-100-5.ucode $(1)/lib/firmware
endif
endef

define KernelPackage/iwl3945/install
        $(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-3945-2.ucode $(1)/lib/firmware
endef

define KernelPackage/iwl4965/install
        $(INSTALL_DIR) $(1)/lib/firmware
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_LINUX_FIRMWARE_SUBDIR)/iwlwifi-4965-2.ucode $(1)/lib/firmware
endef

define Build/b43-common
	tar xjf "$(DL_DIR)/$(PKG_B43_FWCUTTER_SOURCE)" -C "$(PKG_BUILD_DIR)"
	patch -p1 -d  "$(PKG_BUILD_DIR)/$(PKG_B43_FWCUTTER_OBJECT)" < ./files/b43-fwcutter-fw-dirname.patch
	$(MAKE) -C "$(PKG_BUILD_DIR)/$(PKG_B43_FWCUTTER_OBJECT)" \
		CFLAGS="-I$(STAGING_DIR_HOST)/include -include endian.h" \
		QUIET_SPARSE=:
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_B43_FWCUTTER_OBJECT)/b43-fwcutter $(STAGING_DIR_HOST)/bin/
ifeq ($(CONFIG_B43_OPENFIRMWARE),y)
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/bin/
	$(MAKE) -C "$(PKG_BUILD_DIR)/$(PKG_B43_FWCUTTER_SUBDIR)/assembler/"
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_B43_FWCUTTER_SUBDIR)/assembler/b43-asm $(STAGING_DIR_HOST)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_B43_FWCUTTER_SUBDIR)/assembler/b43-asm.bin $(STAGING_DIR_HOST)/bin/
endif
	$(INSTALL_BIN) ./files/host_bin/b43-fwsquash.py $(STAGING_DIR_HOST)/bin/
endef

define KernelPackage/b43/install
	rm -rf $(1)/lib/firmware/
	$(call Build/b43-common)
ifeq ($(CONFIG_B43_OPENFIRMWARE),y)
	tar xzf "$(DL_DIR)/$(PKG_B43_FWV4_SOURCE)" -C "$(PKG_BUILD_DIR)"
else
	tar xjf "$(DL_DIR)/$(PKG_B43_FWV4_SOURCE)" -C "$(PKG_BUILD_DIR)"
endif
	$(INSTALL_DIR) $(1)/lib/firmware/
ifeq ($(CONFIG_B43_OPENFIRMWARE),y)
	$(MAKE) -C "$(PKG_BUILD_DIR)/$(PKG_B43_FWV4_OBJECT)/"
	$(INSTALL_DIR) $(1)/lib/firmware/b43-open/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_B43_FWV4_OBJECT)/ucode5.fw $(1)/lib/firmware/b43-open/ucode5.fw
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_B43_FWV4_OBJECT)/b0g0bsinitvals5.fw $(1)/lib/firmware/b43-open/b0g0bsinitvals5.fw
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(PKG_B43_FWV4_OBJECT)/b0g0initvals5.fw $(1)/lib/firmware/b43-open/b0g0initvals5.fw
else
	b43-fwcutter -w $(1)/lib/firmware/ $(PKG_BUILD_DIR)/$(PKG_B43_FWV4_OBJECT)
endif
ifneq ($(CONFIG_B43_FW_SQUASH),)
	b43-fwsquash.py "$(CONFIG_B43_FW_SQUASH_PHYTYPES)" "$(CONFIG_B43_FW_SQUASH_COREREVS)" "$(1)/lib/firmware/b43"
endif
endef

define KernelPackage/b43legacy/install
	$(call Build/b43-common)
	$(INSTALL_DIR) $(1)/lib/firmware/

	b43-fwcutter --unsupported -w $(1)/lib/firmware/ $(DL_DIR)/$(PKG_B43_FWV3_SOURCE)
ifneq ($(CONFIG_B43LEGACY_FW_SQUASH),)
	b43-fwsquash.py "G" "$(CONFIG_B43LEGACY_FW_SQUASH_COREREVS)" "$(1)/lib/firmware/b43legacy"
endif
endef

$(eval $(call KernelPackage,ath5k))
$(eval $(call KernelPackage,lib80211))
$(eval $(call KernelPackage,libertas-usb))
$(eval $(call KernelPackage,libertas-sd))
$(eval $(call KernelPackage,cfg80211))
$(eval $(call KernelPackage,mac80211))
$(eval $(call KernelPackage,p54-common))
$(eval $(call KernelPackage,p54-pci))
$(eval $(call KernelPackage,p54-usb))
$(eval $(call KernelPackage,p54-spi))
$(eval $(call KernelPackage,rt2x00-lib))
$(eval $(call KernelPackage,rt2x00-pci))
$(eval $(call KernelPackage,rt2x00-usb))
$(eval $(call KernelPackage,rt2x00-soc))
$(eval $(call KernelPackage,rt2800-lib))
$(eval $(call KernelPackage,rt2400-pci))
$(eval $(call KernelPackage,rt2500-pci))
$(eval $(call KernelPackage,rt2500-usb))
$(eval $(call KernelPackage,rt61-pci))
$(eval $(call KernelPackage,rt73-usb))
$(eval $(call KernelPackage,rt2800-pci))
$(eval $(call KernelPackage,rt2800-usb))
$(eval $(call KernelPackage,rtl8180))
$(eval $(call KernelPackage,rtl8187))
$(eval $(call KernelPackage,zd1211rw))
$(eval $(call KernelPackage,mac80211-hwsim))
$(eval $(call KernelPackage,ath9k-common))
$(eval $(call KernelPackage,ath9k))
$(eval $(call KernelPackage,ath9k-htc))
$(eval $(call KernelPackage,ath))
$(eval $(call KernelPackage,carl9170))
$(eval $(call KernelPackage,b43))
$(eval $(call KernelPackage,b43legacy))
$(eval $(call KernelPackage,net-libipw))
$(eval $(call KernelPackage,net-ipw2100))
$(eval $(call KernelPackage,net-ipw2200))
$(eval $(call KernelPackage,iwlagn))
$(eval $(call KernelPackage,iwl-legacy))
$(eval $(call KernelPackage,iwl4965))
$(eval $(call KernelPackage,iwl3945))
$(eval $(call KernelPackage,mwl8k))
$(eval $(call KernelPackage,net-hermes))
$(eval $(call KernelPackage,net-hermes-pci))
$(eval $(call KernelPackage,net-hermes-plx))
$(eval $(call KernelPackage,net-hermes-pcmcia))
