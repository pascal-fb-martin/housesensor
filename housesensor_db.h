/* housesensor - A simple home web server for measurements.
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
 * housesensor_db.h - The database of sensors.
 */
void housesensor_db_initialize (int argc, const char **argv);
void housesensor_db_set (const char *driver, const char *device,
                         const char *value, const char *unit);
const char *housesensor_db_device_first (const char *driver);
const char *housesensor_db_device_next (const char *driver);
const char *housesensor_db_option (const char *name);

void housesensor_db_latest (char *buffer, int size);
void housesensor_db_history (char *buffer, int size);

void housesensor_db_background (time_t now);

