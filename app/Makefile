# ======================================================
#  USER-SPACE APP MAKEFILE
# ======================================================

CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -g
INCLUDES := -I../driver/include

SRC_DIR  := src
BIN_DIR  := bin

SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LIBS   := $(shell sdl2-config --libs) -lSDL2_ttf

all: $(BIN_DIR)/test_control $(BIN_DIR)/stream_interface

# ------------------------------------------------------
#  test_control
# ------------------------------------------------------
$(BIN_DIR)/test_control: $(SRC_DIR)/test_control.o
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(SRC_DIR)/test_control.o: $(SRC_DIR)/test_control.c ../driver/include/ioctl_cmds.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------------------------------
#  stream_interface
# ------------------------------------------------------
$(BIN_DIR)/stream_interface: $(SRC_DIR)/stream_interface.o
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(SDL2_LIBS)

$(SRC_DIR)/stream_interface.o: $(SRC_DIR)/stream_interface.c ../driver/include/ioctl_cmds.h
	$(CC) $(CFLAGS) $(INCLUDES) $(SDL2_CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o
	rm -f $(BIN_DIR)/test_control $(BIN_DIR)/stream_interface

.PHONY: all clean
