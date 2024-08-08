/* @(#)$Id: servers.c,v 1.9 1998/01/25 18:35:47 seks Exp $ */

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

aserver **FindServer(aserver ** head, char *YY)
{
  register aserver **tmp;

  if (head == NULL || *head == NULL)
    return NULL;

  while (*head != NULL)
  {
    if (strcmp((*head)->YY, YY) == 0)
    {
      return head;
    }
    if ((tmp = FindServer(&(*head)->down, YY)) != NULL)
    {
      return tmp;
    }
    else
    {
      head = &(*head)->next;
    }
  }

  return NULL;
}

aserver **FindServerName(aserver ** head, char *name)
{
  register aserver **tmp;

  if (head == NULL || *head == NULL)
    return NULL;

  while (*head != NULL)
  {
    if (!strcasecmp((*head)->name, name))
    {
      return head;
    }
    if ((tmp = FindServerName(&(*head)->down, name)) != NULL)
    {
      return tmp;
    }
    else
    {
      head = &(*head)->next;
    }
  }

  return NULL;
}

aserver *ToServer(char *YY)
{
  return (*FindServer(&ServerList, YY));
}

void onserver(char *arg1, char *arg2, char *arg3, char *args)
{

// This function is triggered both on SERVER (my uplink) and S (other servers)
// arg1: SERVER, arg2: servername, arg3: 1, args: 1703190230 1703239705 J10 ABP]] +h6 :server description
// arg1: AB, arg2: S, arg3: servername, args: 2 0 1703230760 P10 AKAB] +h6 :server description

  register aserver *head;
  register aserver *tmp;
  register int i;
  char newserver[SERVER_NAME_LENGTH] = "";
  char TS[11] = "";
  char YYXXX[6] = "";
  char YY[3] = "";

  if (strcmp(arg1, "SERVER") == 0)
  {
    strncpy(newserver, arg2, SERVER_NAME_LENGTH);
    GetWord(1, args, TS);
    GetWord(3, args, YYXXX);
    strncpy(YY, YYXXX, 2);
    YY[2] = '\0';

    // Storing uplink in memory.
    strcpy(myuplinkYY, YY);
    strncpy(myuplinkname, arg2, SERVER_NAME_LENGTH);

    head = NULL;
  } 
  else 
  {
    strncpy(newserver, arg3, SERVER_NAME_LENGTH);
    GetWord(2, args, TS);
    GetWord(4, args, YYXXX);
    strncpy(YY, YYXXX, 2);
    YY[2] = '\0';

    head = ToServer(arg1);
  }

#ifdef BACKUP
  if (strcmp(newserver, myYY) == 0)
  {
    quit(MAIN_NICK " is back", 0);
  }
#endif

  tmp = (aserver *) MALLOC(sizeof(aserver));
  tmp->name = (char *)MALLOC(strlen(newserver) + 1);
  strcpy(tmp->name, newserver);
  strcpy(tmp->YY, YY);

  tmp->TS = atol(TS);
  if (head == NULL)
  {
    tmp->next = ServerList;
    ServerList = tmp;
    TSoffset = tmp->TS - now;
#ifdef DEBUG
    printf("New connection with %s (%s): my time: %ld  others' time %ld (%ld)\n",
      tmp->name, tmp->YY, now, tmp->TS, TSoffset);
#endif
  }
  else
  {
    tmp->next = head->down;
    head->down = tmp;
  }
  tmp->up = head;
  tmp->down = NULL;
  for (i = 0; i < 100; i++)
    tmp->users[i] = NULL;
}

void onsquit(char *source, char *theserver, char *args)
{
  register aserver *serv, **s;
  register int i;
  char TS[12] = "";
  char buffer[200] = "";

#ifdef FAKE_UWORLD
  if (!strcasecmp(theserver, UFAKE_SERVER) && Uworld_status == 1)
  {
    GetWord(0, args, TS);
    if (atol(TS) == UworldServTS)
    {
      sprintf(buffer, "%s squitted", UFAKE_NICK);
      PutLog(buffer);
      Uworld_status = 0;
    }
    return;
  }
#endif

  s = FindServerName(&ServerList, theserver);

  if (s == NULL)
  {
    sprintf(buffer, "ERROR: SQUIT unknown server %s (from %s)",
      theserver, source);
    PutLog(buffer);
    return;
  }

  serv = *s;

#ifdef DEBUG
  printf("SQUIT: %s\n", theserver);
#endif

  if (args != NULL)
  {
    GetWord(0, args, TS);
#ifdef DEBUG
    if (s != NULL)
      printf("ConnectTS: %ld SquitTS: %ld\n", serv->TS, atol(TS));
#endif
  }

  if (serv != ServerList && args != NULL && serv->TS != atol(TS))
  {
#ifdef DEBUG
    printf("TS's are different.. ignoring squit!\n");
#endif
    return;
  }

  while (serv->down != NULL)
  {
    onsquit(NULL, serv->down->name, NULL);
  }

  for (i = 0; i < 100; i++)
  {
    while (serv->users[i] != NULL)
    {
      onquit(serv->users[i]->N->num);
    }
  }

  TTLALLOCMEM -= strlen(serv->name) + 1;
  free(serv->name);
  *s = serv->next;
  TTLALLOCMEM -= sizeof(aserver);
  free(serv);
}

#ifndef HIS_SERVERNAME
void showmap(char *source)
{
  int count = 0;
  register aluser *user;

  if (CurrentSendQ > HIGHSENDQTHRESHOLD)
  {
    notice(source, "Cannot process your request at this time. Try again later.");
    return;
  }
  notice(source, SERVERNAME);
  showserv(source, ServerList, &count);
  CheckFloodFlood(source, count);
}

void showserv(char *source, aserver * server, int *count)
{
  static char prefix[80] = "";
  static int offset = 0;
  char buffer[200] = "";
  register asuser *suser;
  register int nbusers = 0, i;

  if (server == NULL)
  {
    return;
  }

  (*count)++;	/* number of servers */

  /* count number of users */
  for (i = 0; i < 100; i++)
  {
    suser = server->users[i];
    while (suser != NULL)
    {
      nbusers++;
      suser = suser->next;
    }
  }

  if (server->next == NULL)
  {
    sprintf(buffer, "%s`-%s  (%d client%s)", prefix, server->name, nbusers, (nbusers != 1) ? "s" : "");
  }
  else
  {
    sprintf(buffer, "%s|-%s  (%d client%s)", prefix, server->name, nbusers, (nbusers != 1) ? "s" : "");
  }
  notice(source, buffer);

  if (server->next != NULL)
    strcpy(prefix + offset, "| ");
  else
    strcpy(prefix + offset, "  ");

  offset += 2;
  showserv(source, server->down, count);
  offset -= 2;
  prefix[offset] = '\0';

  showserv(source, server->next, count);
}
#endif

void onsettime(char *source, char *value)
{
  char buffer[200] = "";

  TSoffset = atol(value) - now;
  sprintf(buffer, "SETTIME from %s (%s) (%ld)", source, value, TSoffset);
  PutLog(buffer);
#ifdef DEBUG
  puts(buffer);
#endif
}

void showversion(char *source)
{
  char buffer[200] = "";

  sprintf(buffer, "%s 351 %s :%s\n", myYY, source, VERSION);
  sendtoserv(buffer);
}

static const char convert2y[] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
  'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
  'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
  'w','x','y','z','0','1','2','3','4','5','6','7','8','9','[',']'
};

const char* inttobase64( char* buf, unsigned int v, unsigned int count )
{
  buf[count] = '\0';
  while (count > 0)
  {
    buf[ --count ] = convert2y[(v & NUMNICKMASK)];
    v >>= NUMNICKLOG;
  }

  return buf;
}