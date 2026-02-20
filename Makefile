################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
#ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
#endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
PREFIX ?= /usr
DESTDIR ?=

TARGET = ns-agent


PKG_CFLAGS := $(shell pkg-config --cflags glib-2.0 libnm)
PKG_LIBS   := $(shell pkg-config --libs glib-2.0 libnm)

CFLAGS += $(PKG_CFLAGS)
LIBS   += $(PKG_LIBS)


# All Target
all: ns-agent

# Tool invocations
ns-agent: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -ggdb -L/lib/aarch64-linux-gnu -L/usr/lib/aarch64-linux-gnu -o "ns-agent" $(OBJS) $(USER_OBJS) $(LIBS) -lrt
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) ns-agent
	-@echo ' '



install: $(TARGET)
	install -D -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	install -D -m 644 ns-agent.conf $(DESTDIR)/etc/ns/ns-agent.conf
	install -D -m 644 novus-snmpd.conf $(DESTDIR)/etc/snmp/snmpd.conf.d/novus-snmpd.conf
	install -D -m 644 NOVUS-SECURE-MIB_REV1.2.mib $(DESTDIR)/usr/share/snmp/mibs/NOVUS-SECURE-MIB_REV1.2.mib

.PHONY: all clean dependents

-include ../makefile.targets
