/* main.c
 *
 * Copyright (C) 2016 Ray Strode
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "chips-application.h"

int main(int   argc,
         char *argv[])
{
  g_autoptr(GtkApplication) app = NULL;
  int status;

  app = g_object_new (CHIPS_TYPE_APPLICATION,
                      "application-id", "org.gnome.Chips",
                      NULL);

  status = g_application_run (G_APPLICATION (app), argc, argv);

  return status;
}
