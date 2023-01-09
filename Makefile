
OBJS= housesensor.o housesensor_w1.o housesensor_db.o
LIBOJS=

all: housesensor

main: housesensor.o
db: housesensor_db.o

clean:
	rm -f *.o *.a housesensor

rebuild: clean all

%.o: %.c
	gcc -c -g -O -o $@ $<

housesensor: $(OBJS)
	gcc -g -O -o housesensor $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lrt

install:
	if [ -e /etc/init.d/housesensor ] ; then systemctl stop housesensor ; systemctl disable housesensor ; rm -f /etc/init.d/housesensor ; fi
	if [ -e /lib/systemd/system/housesensor.service ] ; then systemctl stop housesensor ; systemctl disable housesensor ; rm -f /lib/systemd/system/housesensor.service ; fi
	mkdir -p /usr/local/bin
	mkdir -p /var/lib/house/sensor
	rm -f /usr/local/bin/housesensor
	cp housesensor /usr/local/bin
	chown root:root /usr/local/bin/housesensor
	chmod 755 /usr/local/bin/housesensor
	cp systemd.service /lib/systemd/system/housesensor.service
	chown root:root /lib/systemd/system/housesensor.service
	touch /etc/default/housesensor
	mkdir -p /etc/house
	touch /etc/house/sensor.config
	systemctl daemon-reload
	systemctl enable housesensor
	systemctl start housesensor

uninstall:
	systemctl stop housesensor
	systemctl disable housesensor
	rm -f /usr/local/bin/housesensor
	rm -f /lib/systemd/system/housesensor.service /etc/init.d/housesensor
	systemctl daemon-reload

purge: uninstall
	rm -rf /etc/house/sensor.config /etc/default/housesensor /var/lib/house/sensor

