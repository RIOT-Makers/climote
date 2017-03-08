# name of your application
APPLICATION = monica

BOARD_WHITELIST := native pba-d-01-kw2x remote-revb
# If no BOARD is found in the environment, use this default:
BOARD ?= native
# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += emcute
# Specify the mandatory networking modules
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_sock_udp
USEMODULE += gcoap
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo
# Required by gcoap example
#USEMODULE += od
#USEMODULE += fmt
# Add also the shell, some shell commands
USEMODULE += shell
#USEMODULE += shell_commands
#USEMODULE += ps
USEMODULE += xtimer
# packages
USEPKG += nanocoap
# Add sensor modules

ifeq ($(BOARD),pba-d-01-kw2x)
	USEMODULE += hdc1000
	USEMODULE += tmp006
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP
CFLAGS += -DTHREAD_STACKSIZE_MAIN=2048

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
