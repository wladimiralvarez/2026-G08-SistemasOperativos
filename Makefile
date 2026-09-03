# Makefile
#   make          compila y genera ./mishell
#   make clean    borra los objetos y el ejecutable


CC      = gcc

# -g agrega símbolos de depuración para gdb
CFLAGS  = -Wall -Wextra -std=gnu11 -g

TARGET  = mishell
SRCDIR  = src
OBJDIR  = build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
HDRS = $(wildcard $(SRCDIR)/*.h)

all: $(TARGET)

# junta todos los .o en el ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Compilación de cada .c por separado.
# Depende de todos los .h para que al cambiar un header se recompile todo.
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HDRS) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
