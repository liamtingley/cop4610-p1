SRC := src
OBJ := obj
BIN := bin
EXECUTABLE := shell

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
INCS := -Iinclude/
DIRS := $(OBJ)/ $(BIN)/
EXEC := $(BIN)/$(EXECUTABLE)

CC := gcc
CFLAGS := -g -Wall -std=c99 $(INCS)
LDFLAGS :=

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# -MMD -MP writes a .d file per object so header changes trigger a rebuild.
$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(EXEC)
	$(EXEC)

clean:
	rm -f $(OBJ)/*.o $(OBJ)/*.d $(EXEC)

$(shell mkdir -p $(DIRS))

-include $(DEPS)

.PHONY: run clean all
