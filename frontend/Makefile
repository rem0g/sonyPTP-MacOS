
OUT ?= ../out
OUT_DIR := $(OUT)/bin
LIB_INC := ../libcameracontrolptp/include/

CC := g++
CFLAGS := -Wall -g
override INCLUDES += -I$(LIB_INC)
override INCLUDES += -I/opt/homebrew/include
SYS := $(shell $(CC) -dumpmachine)

TARGET := $(OUT_DIR)/control
SRC_DIR := sources
SCRIPTS_DIR := scripts
OBJ_DIR := .obj
SOURCES := main.cpp command.cpp serverclient.cpp websocket_server.cpp websocket_integration.cpp sony_device_finder.cpp
ifneq (, $(findstring linux, $(SYS)))
# Linux
	SOURCES += socket.cpp
else ifneq (, $(findstring cygwin, $(SYS)))
# Cygwin
	SOURCES += socket_cygwin.cpp
else ifneq (, $(findstring darwin, $(SYS)))
# macOS - use the same socket implementation as Linux
	SOURCES += socket.cpp
else
	$(error Unsupported system: $(SYS))
endif
SCRIPTS := $(notdir $(wildcard $(SCRIPTS_DIR)/*.sh))
SCRIPTS := $(addprefix $(OUT_DIR)/, $(SCRIPTS))

OBJECTS := $(addprefix $(OBJ_DIR)/, $(SOURCES:.cpp=.o))
DEPENDS := $(patsubst %.o,%.d, $(OBJECTS))

all: $(OUT_DIR) $(OBJ_DIR) $(TARGET) $(SCRIPTS)

objs: $(OBJ_DIR) $(OBJECTS)

clean:
	rm -rf $(TARGET) $(OBJ_DIR) $(SCRIPTS)

$(TARGET): $(OBJECTS)
	$(CC) -L$(OUT)/lib -L/opt/homebrew/lib -o $@ $^ -lcameracontrolptp -lusb-1.0 -lstdc++ -lssl -lcrypto -Wl,-rpath,@executable_path/../lib

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(OUT_DIR)/%.sh : $(SCRIPTS_DIR)/%.sh
	cp $< $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(OBJ_DIR)/%.d : $(SRC_DIR)/%.cpp
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	$(CC) -MM $(CFLAGS) $(INCLUDES) $< \
	| sed 's/$(notdir $*).o/$(subst /,\/,$(patsubst %.d,%.o,$@) $@)/' > $@ ; \
	[ -s $@ ] || rm -f $@

-include $(DEPENDS)

.PHONY: all clean
