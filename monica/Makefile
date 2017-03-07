# name of your application
APPLICATION = monica

FEATURES_REQUIRED = periph_i2c

BOARD_WHITELIST := pba-d-01-kw2x remote-revb
# If no BOARD is found in the environment, use this default:
BOARD ?= pba-d-01-kw2x
# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_udp
USEMODULE += gnrc_sock_udp
# Add shell and commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += xtimer
# Add sensor modules

ifeq ($(BOARD),pba-d-01-kw2x)
	USEMODULE += hdc1000
	USEMODULE += tmp006
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
