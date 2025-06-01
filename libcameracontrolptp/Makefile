
ROOT_DIR ?= .
OUT ?= ../out
OUT_DIR := ${OUT}/lib

CC := g++
SYS := $(shell $(CC) -dumpmachine)

ifneq (, $(findstring linux, $(SYS)))
# Linux
	tg_so = ${OUT_DIR}/libcameracontrolptp.so
else ifneq (, $(findstring cygwin, $(SYS)))
# Cygwin
	tg_so = ${OUT_DIR}/libcameracontrolptp.dll
else ifneq (, $(findstring darwin, $(SYS)))
# macOS
	tg_so = ${OUT_DIR}/libcameracontrolptp.dylib
else
	$(error Unsupported system: $(SYS))
endif

override INCLUDES += -I${ROOT_DIR}/include
override INCLUDES += -I${ROOT_DIR}/ports

sources_usb:= ${ROOT_DIR}/ports/ports_usb_impl.cpp
lib_usb:= $(shell pkg-config --libs libusb-1.0)
# For macOS, we need the parent directory of libusb-1.0
usb_cflags:= -I/opt/homebrew/include

sources_so :=
sources_so += ${sources_usb}
sources_so += ${ROOT_DIR}/ports/ports_ptp_impl.cpp
sources_so += ${ROOT_DIR}/sources/socc_ptp.cpp
sources_so += ${ROOT_DIR}/sources/parser.cpp
OBJ_DIR := .obj
OBJECTS := $(addprefix $(OBJ_DIR)/, $(notdir $(sources_so:.cpp=.o)))

.PHONY: clean

all: ${OUT_DIR} $(OBJ_DIR) ${tg_so}

objs: $(OBJ_DIR) $(OBJECTS)

${OUT_DIR}:
	mkdir -p ${OUT_DIR}

${tg_so}: $(OBJECTS)
	${CC} $^ -shared -o $@ ${lib_usb} -pthread -lstdc++ -install_name @rpath/$(notdir ${tg_so})

$(OBJ_DIR)/%.o : **/%.cpp
	$(CC) $(CFLAGS) -fPIC -g $(INCLUDES) $(usb_cflags) -o $@ -c $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf ${tg_so} $(OBJ_DIR)
