# name of your application
APPLICATION = lgv-demo

BOARD_WHITELIST := native pba-d-01-kw2x
# If no BOARD is found in the environment, use this default:
BOARD ?= native
# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# mandatory modules
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += emcute
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_sock_udp
USEMODULE += gcoap
USEMODULE += shell
USEMODULE += xtimer
USEMODULE += od
# optional modules, e.g. for debugging
USEMODULE += gnrc_icmpv6_echo
#USEMODULE += shell_commands
#USEMODULE += ps
# mandatory packages
USEPKG += nanocoap
# Add sensor modules, if supported
ifeq ($(BOARD),pba-d-01-kw2x)
	USEMODULE += hdc1000
	USEMODULE += tmp006
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
#CFLAGS += -DDEVELHELP
# get rid of stack corruption and panics
CFLAGS += -DTHREAD_STACKSIZE_MAIN=2048
# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
