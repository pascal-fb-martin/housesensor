/* housesensor - A simple home web server for sensor measurement.
 *
 * Copyright 2019, Pascal Martin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *
 * housesensor.c - Main loop of the housesensor program.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "housesensor.h"
#include "housesensor_w1.h"
#include "housesensor_db.h"

#include "echttp_static.h"
#include "houseportalclient.h"


static int use_houseportal = 0;

static void hs_help (const char *argv0) {

    int i = 1;
    const char *help;

    printf ("%s [-h]%s\n", argv0, echttp_help(0));

    printf ("\nGeneral options:\n");
    printf ("   -h:              print this help.\n");

    printf ("\nHTTP options:\n");
    help = echttp_help(i=1);
    while (help) {
        printf ("   %s\n", help);
        help = echttp_help(++i);
    }
    exit (0);
}

static const char *hs_sensor_status (const char *method, const char *uri,
                                     const char *data, int length) {

    echttp_content_type_json ();
    return housesensor_db_latest();
}

static const char *hs_sensor_recent (const char *method, const char *uri,
                                     const char *data, int length) {

    echttp_content_type_json ();
    return housesensor_db_recent();
}

static const char *hs_sensor_history (const char *method, const char *uri,
                                      const char *data, int length) {

    echttp_content_type_json ();
    return housesensor_db_history();
}

static void hs_background (int fd, int mode) {

    static time_t LastRenewal = 0;
    time_t now = time(0);

    housesensor_w1_background (now);
    housesensor_db_background (now);

    if (use_houseportal) {
        static const char *path[] = {"/sensor"};
        if (now >= LastRenewal + 60) {
            if (LastRenewal > 0)
                houseportal_renew();
            else
                houseportal_register (echttp_port(4), path, 1);
            LastRenewal = now;
        }
    }
}

int main (int argc, const char **argv) {

    // These strange statements are to make sure that fds 0 to 2 are
    // reserved, since this application might output some errors.
    // 3 descriptors are wasted if 0, 1 and 2 are already open. No big deal.
    //
    open ("/dev/null", O_RDONLY);
    dup(open ("/dev/null", O_WRONLY));

    int i;
    for (i = 1; i < argc; ++i) {
        if (echttp_option_present("-h", argv[i])) {
            hs_help(argv[0]);
        }
    }

    housesensor_db_initialize (argc, argv);
    housesensor_w1_initialize (argc, argv);

    echttp_default ("-http-service=dynamic");

    echttp_open (argc, argv);
    if (echttp_dynamic_port()) {
        houseportal_initialize (argc, argv);
        use_houseportal = 1;
    }
    echttp_route_uri ("/sensor/status", hs_sensor_status);
    echttp_route_uri ("/sensor/recent", hs_sensor_recent);
    echttp_route_uri ("/sensor/history", hs_sensor_history);
    echttp_static_route ("/sensor/records", "/var/lib/house/sensor");
    echttp_static_route ("/sensor", "/usr/local/share/house/public/sensor");
    if (!use_houseportal)
        echttp_static_route ("/", "/usr/local/share/house/public");
    echttp_background (&hs_background);
    echttp_loop();
}

