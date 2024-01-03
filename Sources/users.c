
/* @(#)$Id: users.c,v 1.7 1998/11/21 14:58:43 seks Exp $ */

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

int lu_hash(char *num)
{
  register int i, j;

  for (i = 0, j = 0; i < strlen(num); i++)
  {
    j += (unsigned char)num[i];
  }

  return (j % 1000);
}

int su_hash(char *num)
{
  register int i, j;

  for (i = 0, j = 0; i < strlen(num); i++)
    j += (unsigned char)num[i];

  return (j % 100);
}

aluser *ToLuser(char *num)
{
  register aluser *curr;
  curr = Lusers[lu_hash(num)];
  while (curr && strcmp(num, curr->num))
    curr = curr->next;

  return (curr);
}

aluser *ToLuserNick(char *nick)
{
	register aluser *curr;

	for (int i = 0; i < 1000; i++)
	{
		curr = Lusers[i];

		if (curr && !strcmp(nick, curr->nick))
			return (curr);
	}
	return 0;
}

char *GetNickNum(char *nick)
{
  register aluser *user;

  user = ToLuserNick(nick);

  if (user == NULL)
  {
    printf("GetNickNum(): Cannot finder user %s\n", nick);
    return 0;
  }
  else
  {
    return user->num;
  }
}

char *GetNumNick(char *num)
{
  register aluser *user;

  user = ToLuser(num);

  if (user == NULL)
  {
    printf("GetNumNick(): Cannot finder user %s\n", num);
    return 0;
  }
  else
  {
    return user->nick;
  }
}

void onnick(char *source, char *newnick, char *body)
{
  register aluser *user, **u;
  register asuser *suser, **s;
  register aserver *serv;
  char username[80];
  char hostname[80];
  char TS[80];
  char num[10];
  char modes[10];
  register achannelnode *chan;
  register anickchange *curr, *prec;
  char buffer[512];
  int i = 0;
  int isOper = 0;

  /* a new user */
  if (strlen(source) == 2)
  {	/* Message comes from server */

    GetWord(1, body, TS);
    GetWord(2, body, username);
    GetWord(3, body, hostname);
    GetWord(4, body, modes);
    GetWord(6, body, num);

    if (num[0] == ':') // New join
    {
	GetWord(5, body, num);

    } else { // Check modes

	int length = (int)strlen(modes);
	for (i = 0; i < length; i++)
	{
		if (modes[i] == 'o')
		   isOper = LFL_ISOPER;
	}
	i = 0;
    }

    if (!strcasecmp(newnick, mynick))
    {
      PutLog("ERROR: I'm nick collided");
#ifdef DEBUG
      printf("ARGH!!! I'M NICK COLLIDED!\n");
#endif

      if (atol(TS) <= logTS &&
	strcasecmp(username, myuser) &&
	strcasecmp(hostname, mysite))
      {
	NickInUse();
	PutLog(source);
	PutLog(newnick);
	PutLog(body);
      }
      else
      {
	onquit(num);
	return;		/*ignore */
      }
#ifdef BACKUP
    }
    else if (!strcasecmp(newnick, MAIN_NICK))
    {
      return;	/* ignore */
#endif
    }
    else if (ToLuser(num))
    {
#ifdef DEBUG
      printf("ARGH!!! NICK COLLISION\n");
#endif
      onquit(num);
    }

#ifdef FAKE_UWORLD
    if (Uworld_status == 1 && !strcasecmp(newnick, UFAKE_NICK))
    {
      if (atol(TS) <= UworldTS && atol(TS) != 0 &&
	strcasecmp(username, UFAKE_NICK) &&
	strcasecmp(hostname, UFAKE_HOST))
      {
	sprintf(buffer, "%s nick collided", UFAKE_NICK);
	PutLog(buffer);
	Uworld_status = 0;
	KillUworld("nick collision");
	return;		/* ignore if younger */
      }
    }
#endif

    // Check ccontrol login
    ccontrolLogin(num, newnick, username, hostname);

    user = (aluser *) MALLOC(sizeof(aluser));

    user->num = (char *)MALLOC(strlen(num) + 1);
    strcpy(user->num, num);

    user->nick = (char *)MALLOC(strlen(newnick) + 1);
    strcpy(user->nick, newnick);

    user->username = (char *)MALLOC(strlen(username) + 1);
    strcpy(user->username, username);

    user->site = (char *)MALLOC(strlen(hostname) + 1);
    strcpy(user->site, hostname);

    if (*newnick == '+')
      serv = &VirtualServer;
    else
      serv = ToServer(source);

    user->server = serv;

#ifdef DEBUG
    printf("NEW USER: %s!%s@%s (%s) on %s (%s)", newnick, username, hostname, num, serv->name, serv->num);
    if (isOper)
	printf(" (IRC Operator)\n");
    else
	printf("\n");
#endif

    user->time = atol(TS);
    user->mode = isOper;

    user->channel = NULL;
    user->valchan = NULL;

    user->next = Lusers[lu_hash(num)];
    Lusers[lu_hash(num)] = user;

    /* add user in server's userlist
     */
    suser = (asuser *) MALLOC(sizeof(asuser));
    suser->N = user;
    suser->next = serv->users[su_hash(num)];
    serv->users[su_hash(num)] = suser;

#ifdef NICKSERV
    nserv_nick(newnick, user);
#endif
  }
  else
  {	/* nick change */
    GetWord(0, body, TS);

#if 0
    if (!strcasecmp(source, DEFAULT_NICKNAME) &&
      strcasecmp(newnick, DEFAULT_NICKNAME))
    {
      ChNick(DEFAULT_NICKNAME);
    }
#endif
    if (!strcasecmp(newnick, mynick))
    {
#ifdef DEBUG
      printf("ARGH!!! I'M NICK COLLIDED!\n");
#endif
      if (atol(TS + 1) <= logTS)
      {
	NickInUse();
	PutLog(source);
	PutLog(newnick);
	PutLog(body);
      }
      else
      {
	onquit(source);
	return;		/*ignore */
      }
    }

    u = &Lusers[lu_hash(source)];
    while (*u && strcasecmp(source, (*u)->num))
      u = &(*u)->next;
    user = *u;

#ifdef NICKSERV
    nserv_nick(newnick, user);
#endif

    if (user == NULL)
      quit("ERROR! onnick() can't find user", 1);

    s = &user->server->users[su_hash(source)];
    while (*s && strcasecmp((*s)->N->num, user->num))
      s = &(*s)->next;
    suser = *s;

    /* change the nick in memory */

    printf("NICK CHANGE %s (%s) --> %s\n", user->nick, user->num, newnick);

    TTLALLOCMEM -= strlen(user->nick) + 1;
    free(user->nick);
    user->nick = (char *)MALLOC(strlen(newnick) + 1);
    strcpy(user->nick, newnick);

    /* NICK FLOOD PROTECTION */
    /* 1st wipe old nick changes off */
    chan = user->channel;
    while (chan)
    {
      curr = chan->nickhist;
      prec = NULL;

      /* if not on channel.. ignore nick flood pro */
      if (!chan->N->on)
      {
	chan = chan->next;
	continue;	/* yurk.. as bad as a goto ;) */
      }

      while (curr)
      {
	if (curr->time < (now - 15))
	{
	  if (prec)
	  {
	    prec->next = curr->next;
	    TTLALLOCMEM -= sizeof(anickchange);
	    free(curr);
	    curr = prec->next;
	  }
	  else
	  {
	    chan->nickhist = curr->next;
	    TTLALLOCMEM -= sizeof(anickchange);
	    free(curr);
	    curr = chan->nickhist;
	  }
	}
	else
	{
	  prec = curr;
	  curr = curr->next;
	}
      }

      /* now add the new nick change to the history */
      curr = (anickchange *) MALLOC(sizeof(anickchange));
      strcpy(curr->num, source);
      curr->time = now;		/* a lil confusing :( */
      curr->next = chan->nickhist;
      chan->nickhist = curr;

      /* now count the nick changes in history
         if there are more than allowed.. grrrr */
      for (i = 0, curr = chan->nickhist; curr;
	curr = curr->next, i++);

      if (i == chan->N->NickFloodPro && chan->N->NickFloodPro != 0
	&& chan->N->on)
      {
	sprintf(buffer, "%s!%s@%s", user->nick, user->username, user->site);
	notice(source,
	  "### NICK FLOOD PROTECTION ACTIVATED ###");
	sprintf(buffer, "%s %d", GetNumNick(source),
	  NICK_FLOOD_SUSPEND_TIME);
	suspend("", chan->N->name, buffer);
	ban("", chan->N->name, newnick);
      }
      chan = chan->next;
    }
  }
}

void onquit(char *num)
{
  register aluser *user, **u;
  register asuser *suser, **s;
  register avalchan *valchan;

#ifdef DEBUG
  printf("Detected user quit..\n");
#endif
  u = &Lusers[lu_hash(num)];
  while (*u && strcasecmp(num, (*u)->num))
    u = &(*u)->next;

  user = *u;

  if (user == NULL)
  {
    PutLog("ERROR: onquit() can't find user!");
#ifdef HISTORY
    History(NULL);
#endif
    return;
  }

  /* remove from memory */
  while ((valchan = user->valchan) != NULL)
  {
#ifdef DEBUG
    printf("\twas validated on %s\n", valchan->name);
#endif
    valchan->reg->inuse--;
    user->valchan = valchan->next;
    TTLALLOCMEM -= strlen(valchan->name) + 1;
    free(valchan->name);
    TTLALLOCMEM -= sizeof(avalchan);
    free(valchan);
  }
  while (user->channel != NULL)
  {
#ifdef DEBUG
    printf("\twas on %s\n", user->channel->N->name);
#endif
    /* onpart() free's the chan structure
     * we can't do chan=chan->next after the 
     * onpart() call. We must start from the 
     * beginning of the list every time
     */
    onpart(num, user->channel->N->name);
  }

  /* remove user from server's userlist
   */
  s = &user->server->users[su_hash(user->num)];
  while (*s != NULL && strcasecmp((*s)->N->num, user->num))
  {
    s = &(*s)->next;
  }

  if (*s != NULL)
  {
    suser = *s;
    *s = (*s)->next;
    TTLALLOCMEM -= sizeof(asuser);
    free(suser);
  }
  else
  {
    PutLog("ERROR: onquit()  user not found in server's userlist!");
  }

  *u = user->next;
#if 0
  if (!strcasecmp(user->nick, DEFAULT_NICKNAME))
  {
    ChNick(DEFAULT_NICKNAME);
  }
#endif

#ifdef NICKSERV
  nserv_quit(user);
#endif

  TTLALLOCMEM -= strlen(user->num) + 1;
  free(user->num);
  TTLALLOCMEM -= strlen(user->nick) + 1;
  free(user->nick);
  TTLALLOCMEM -= strlen(user->username) + 1;
  free(user->username);
  TTLALLOCMEM -= strlen(user->site) + 1;
  free(user->site);
  TTLALLOCMEM -= sizeof(aluser);
  free(user);
}

void onkill(char *source, char *target, char *comment)
{
  char buffer[200];

  if (!strcasecmp(target, mynum))
  {
#if 0
    /* ignore kill for nick collisions because we
     * already check in onnick() if we're collided.
     * This kill is prolly a lost  kill resulting of
     * another nick collision..
     */
    if (strstr(comment, "older nick overruled") ||
      strstr(comment, "collided yourself"))
    {
      PutLog("ERROR: Nick collision on me?");
      return;
    }
#endif
    sprintf(buffer, "%s SQ %s 0 :killed by %s\n",
      NUMERIC, SERVERNAME, GetNumNick(source));
    sendtoserv(buffer);
    dumpbuff();
    close(Irc.fd);
    Irc.fd = -1;
    if (reconnect(server))
    {
      try_later(server);
    }
#ifdef BACKUP
  }
  else if (!strcasecmp(target, MAIN_NICK))
  {
    quit(MAIN_NICK " is back", 0);
#endif
#ifdef FAKE_UWORLD
  }
  else if (!strcasecmp(target, UFAKE_NICK))
  {
    char buffer[200];
    sprintf(buffer, "%s is KILLED by %s", UFAKE_NICK, GetNumNick(source));
    PutLog(buffer);
    Uworld_status = 0;
    KillUworld("Killed");
#endif
  }
  else
    onquit(target);
}

void onwhois(char *source, char *nick)
{
  register aluser *user;
  register auser *usr;
  register achannelnode *chan;
  char buffer[512];

  user = ToLuserNick(nick);

  if (user == NULL)
  {
    sprintf(buffer, "%s 401 %s %s :No such nick\n", NUMERIC, source, nick);
  }
  else
  {
    sprintf(buffer, "%s 311 %s %s %s %s * :\n", NUMERIC, source, user->nick,
      user->username, user->site);

    sendtoserv(buffer);

    chan = user->channel;
    if (chan != NULL && strcmp(user->nick, "X") && strcmp(user->nick, "W"))
    {
      sprintf(buffer, "%s 319 %s %s :", NUMERIC, source, user->nick);
      while (chan != NULL)
      {
	/* show a channel only if it is
	 * not +s or +p
	 */
	if (!IsSet(chan->N->name, 's', "") &&
	  !IsSet(chan->N->name, 'p', ""))
	{
	  usr = ToUserNick(chan->N->name, nick);
	  if (usr->chanop)
	    strcat(buffer, "@");
	  strcat(buffer, chan->N->name);
	  strcat(buffer, " ");
	}
	chan = chan->next;
	if (strlen(buffer) > 300)
	{
	  strcat(buffer, "\n");
	  sendtoserv(buffer);
	  sprintf(buffer, "%s 319 %s %s :",
	    NUMERIC, source, nick);
	}
      }
      strcat(buffer, "\n");
      sendtoserv(buffer);
    }
    sprintf(buffer, "%s 312 %s %s %s :\n", NUMERIC, source, source, user->server->name);
    sendtoserv(buffer);

    if (user->mode & LFL_ISOPER)
    {
      sprintf(buffer, "%s 313 %s %s :is an IRC Operator\n",
	NUMERIC, source, user->nick);
      sendtoserv(buffer);
    }
  }

  sprintf(buffer, "%s 318 %s :End of /WHOIS list.\n", NUMERIC, source);
  sendtoserv(buffer);
}
