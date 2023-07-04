
OBJS= housesensor.o housesensor_w1.o housesensor_db.o
LIBOJS=

SHARE=/usr/local/share/house

# Local build ---------------------------------------------------

all: housesensor

main: housesensor.o
db: housesensor_db.o

clean:
	rm -f *.o *.a housesensor

rebuild: clean all

%.o: %.c
	gcc -c -Os -o $@ $<

housesensor: $(OBJS)
	gcc -Os -o housesensor $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lrt

# Distribution agnostic file installation -----------------------

install-files:
	mkdir -p /usr/local/bin
	mkdir -p /var/lib/house/sensor
	rm -f /usr/local/bin/housesensor
	cp housesensor /usr/local/bin
	chown root:root /usr/local/bin/housesensor
	chmod 755 /usr/local/bin/housesensor
	mkdir -p $(SHARE)/public/sensor
	chmod 755 $(SHARE) $(SHARE)/public $(SHARE)/public/sensor
	cp public/* $(SHARE)/public/sensor
	chown root:root $(SHARE)/public/sensor/*
	chmod 644 $(SHARE)/public/sensor/*
	mkdir -p /etc/house
	touch /etc/house/sensor.config
	touch /etc/default/housesensor

uninstall:
	rm -rf $(SHARE)/public/sensor
	rm -f /usr/local/bin/housesensor

purge-config:
	rm -rf /etc/house/sensor.config /etc/default/housesensor /var/lib/house/sensor

# Distribution agnostic systemd support -------------------------

install-systemd:
	cp systemd.service /lib/systemd/system/housesensor.service
	chown root:root /lib/systemd/system/housesensor.service
	systemctl daemon-reload
	systemctl enable housesensor
	systemctl start housesensor

uninstall-systemd:
	if [ -e /etc/init.d/housesensor ] ; then systemctl stop housesensor ; systemctl disable housesensor ; rm -f /etc/init.d/housesensor ; fi
	if [ -e /lib/systemd/system/housesensor.service ] ; then systemctl stop housesensor ; systemctl disable housesensor ; rm -f /lib/systemd/system/housesensor.service ; systemctl daemon-reload ; fi

stop-systemd: uninstall-systemd

# Debian GNU/Linux install --------------------------------------

install-debian: stop-systemd install-files install-systemd

uninstall-debian: uninstall-systemd uninstall-files

purge-debian: uninstall-debian purge-config

# Void Linux install --------------------------------------------

install-void: install-files

uninstall-void: uninstall-files

purge-void: uninstall-void purge-config

# Default install (Debian GNU/Linux) ----------------------------

install: install-debian

uninstall: uninstall-debian

purge: purge-debian

