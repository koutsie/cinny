# Makefile for Cinny Desktop
# If pkg-config is not available, use for example the below:
#GTK_CFLAGS := -I/usr/include/gtk-3.0 -I/usr/include/webkit2gtk-4.0
#GTK_LIBS := -lgtk-3 -lwebkit2gtk-4.0

CC := gcc
CFLAGS := -Wno-format -Wno-unused-result -Wall -Wextra -O0 -g3 -Wno-unused-function -Wno-unused-parameter -Wno-deprecated-declarations

GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 webkit2gtk-4.0 gstreamer-1.0)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0 webkit2gtk-4.0 gstreamer-1.0)
TARGET := cinny
SOURCES := main.c
OBJECTS := $(SOURCES:.c=.o)

# Build target
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(GTK_LIBS) -lcurl
	rm -f *.o

%.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: install
install:
	sudo cp cinny /usr/local/bin
