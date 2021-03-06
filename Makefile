
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
	if [ -e /etc/init.d/housesensor ] ; then systemctl stop housesensor ; fi
	mkdir -p /usr/local/bin
	mkdir -p /var/lib/house/sensor
	rm -f /usr/local/bin/housesensor /etc/init.d/housesensor
	cp housesensor /usr/local/bin
	cp init.debian /etc/init.d/housesensor
	chown root:root /usr/local/bin/housesensor /etc/init.d/housesensor
	chmod 755 /usr/local/bin/housesensor /etc/init.d/housesensor
	touch /etc/default/housesensor
	mkdir -p /etc/house
	touch /etc/house/sensor.config
	systemctl daemon-reload
	systemctl enable housesensor
	systemctl start housesensor

uninstall:
	systemctl stop housesensor
	systemctl disable housesensor
	rm -f /usr/local/bin/housesensor /etc/init.d/housesensor
	systemctl daemon-reload

purge: uninstall
	rm -rf /etc/house/sensor.config /etc/default/housesensor /var/lib/house/sensor

