ERSDK_PATH=er_sdk
ifeq ($(CONFIG_NAME), full)
  EXAMPLE_OBJS = $(ERSDK_PATH)/examples/dummy_main.o
endif
ifeq ($(CONFIG_NAME), empty)
  EXAMPLE_OBJS = $(ERSDK_PATH)/examples/dummy_main.o
endif
ifeq ($(CONFIG_NAME), send_http)
  EXAMPLE_OBJS = $(ERSDK_PATH)/examples/exosite_send_http.o
endif
ifeq ($(CONFIG_NAME), send_coap)
  EXAMPLE_OBJS = $(ERSDK_PATH)/examples/exosite_send_coap.o
endif
ifeq ($(CONFIG_NAME), subscribe_http)
  EXAMPLE_OBJS = $(ERSDK_PATH)/examples/exosite_subscribe_http.o
endif
ifeq ($(CONFIG_NAME), subscribe_coap)
  EXAMPLE_OBJS = $(ERSDK_PATH)/examples/exosite_subscribe_coap.o
endif


$(TARGET_NAME)_OBJS += $(EXAMPLE_OBJS)