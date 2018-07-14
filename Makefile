PKGCONFIG ?= pkg-config
CFLAGS=-ggdb -Werror=implicit-function-declaration
GLIB=`$(PKGCONFIG) --libs --cflags glib-2.0 gio-2.0 gio-unix-2.0`
THINGYMCCONFIG=-I../thingymcconfig/include/

.PHONY: clean

thingymcconfig-app: thingymcconfig-app.o
	$(CC) $(CFLAGS) $(GLIB) $(THINGYMCCONFIG) -o $@ $^

thingymcconfig-app.o: thingymcconfig-app.c
	$(CC) $(CFLAGS) $(GLIB) $(THINGYMCCONFIG) -c -o $@ $<

clean:
	-rm thingymcconfig-app *.o
