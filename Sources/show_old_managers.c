/* @(#)$Id: show_old_managers.c,v 1.2 1998/01/08 17:02:18 seks Exp $ */

/* Undernet Channel Service (X)
 * Copyright (C) 1995-2002 Robin Thellend
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * The author can be contact by email at <csfeedback@robin.pfft.net>
 *
 * Please note that this software is unsupported and mostly
 * obsolete. It was replaced by GNUworld/CMaster. See
 * http://gnuworld.sourceforge.net/ for more information.
 */

#include "h.h"
#include "dirent.h"

time_t now;

#define HOWLONG  (30*24*60*60)	/* 30 days */

void fix_user_file(char *file, char *channel)
{
  dbuser dbu;
  int fd;

  fd = open(file, O_RDONLY);
  if (fd < 0)
  {
    fprintf(stderr, "Can't open %s [%s]\n", file, strerror(errno));
    return;
  }

  /* Find at least one 500 seen within the last 30 days */
  while (read(fd, &dbu, sizeof(dbuser)) == sizeof(dbuser))
  {
    if (dbu.header[0] != 0xFF || dbu.header[1] != 0xFF ||
      dbu.footer[0] != 0xFF || dbu.footer[1] != 0xFF)
    {
      continue;
    }
    if (!strcmp(dbu.match, "!DEL!"))
      continue;

    if (dbu.access == 500 && (now - dbu.lastseen) < HOWLONG)
    {
      close(fd);
      return;
    }
  }


  /* Ok.. at this point, there is no channel manager that was seen
   * within 30 days. Show all of them.
   */
  lseek(fd, 0L, SEEK_SET);

  while (read(fd, &dbu, sizeof(dbuser)) == sizeof(dbuser))
  {
    if (dbu.header[0] != 0xFF || dbu.header[1] != 0xFF ||
      dbu.footer[0] != 0xFF || dbu.footer[1] != 0xFF)
    {
      continue;
    }
    if (!strcmp(dbu.match, "!DEL!"))
      continue;

    if (dbu.access != 500)
      continue;

    printf("%-14s %-9s %03d %-26s %.24s\n",
      dbu.channel, dbu.nick, dbu.access, dbu.match, ctime(&dbu.lastseen));
  }
  close(fd);
}

void fix_user_db(void)
{
  DIR *dp;
  struct dirent *ent;
  char dir[300], file[300], channel[80], *ptr;
  int count;

  for (count = 0; count < 1000; count++)
  {
    sprintf(dir, "db/channels/%04X", count);
    dp = opendir(dir);
    if (dp == NULL)
    {
      fprintf(stderr, "Can't read %s [%s]\n", dir, strerror(errno));
      continue;
    }
    while ((ent = readdir(dp)) != NULL)
    {
      if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
	continue;
      strcpy(channel, ent->d_name);
      for (ptr = channel; *ptr; ptr++)
	if (*ptr == ' ')
	  *ptr = '/';
      sprintf(file, "db/channels/%04X/%s", count, ent->d_name);
      fix_user_file(file, channel);
    }
    closedir(dp);
  }
}

int main(void)
{
  if (chdir(HOMEDIR) < 0)
  {
    perror(HOMEDIR);
    exit(1);
  }

  time(&now);

  fix_user_db();

  return 0;
}
