
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
	gcc -g -O -o housesensor $(OBJS) -lhouseportal -lechttp -lcrypto -lrt

package:
	mkdir -p packages
	tar -cf packages/housesensor-`date +%F`.tgz housesensor init.debian Makefile

install:
	if [ -e /etc/init.d/housesensor ] ; then systemctl stop housesensor ; fi
	mkdir -p /usr/local/bin
	rm -f /usr/local/bin/housesensor /etc/init.d/housesensor
	cp housesensor /usr/local/bin
	cp init.debian /etc/init.d/housesensor
	chown root:root /usr/local/bin/housesensor /etc/init.d/housesensor
	chmod 755 /usr/local/bin/housesensor /etc/init.d/housesensor
	touch /etc/default/housesensor
	mkdir -p /etc/housesensor
	touch /etc/housesensor/housesensor.config
	systemctl daemon-reload
	systemctl enable housesensor
	systemctl start housesensor

uninstall:
	systemctl stop housesensor
	systemctl disable housesensor
	systemctl daemon-reload

