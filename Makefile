
#CROSS_COMPILE ?= arm-linux-
CC=$(CROSS_COMPILE)gcc

TOPDIR = $(shell pwd)
BUILDDIR ?= $(TOPDIR)/build
PREFIX ?= $(TOPDIR)/INSTALL

ifeq ($(CROSS_COMPILE), arm-linux-)
CFLAGS := -O1 -ggdb -Wall -pipe  
else
CFLAGS := -O1 -ggdb -Wall -pipe  -Wno-unused-result -Wno-pointer-sign 
endif
LDFLAGS := 
LIBS :=
INCLUDES=

APP = server client

all: $(APP)

.c.o:
	@$(CC) -c $(CFLAGS)   $< -o $(BUILDDIR)/$@
	
server_obj= server.o socket.o misc.o
server: $(server_obj)
	@cd $(BUILDDIR) && $(CC) $(INCLUDES)  ${CFLAGS}  $^ -o $@  -pthread
	
client_obj=client.o socket.o 
client: $(client_obj)
	@cd $(BUILDDIR) && $(CC) $(INCLUDES)  ${CFLAGS}  $^ -o $@
	
clean:
	@rm -rf $(BUILDDIR)/*
	@rm -rf $(APP) *.o
.PHONY: clean install MKDIR package
