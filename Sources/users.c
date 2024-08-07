
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
  while (curr && (strcmp(num, curr->num) != 0))
    curr = curr->next;

  return (curr);
}

aluser *ToLuserNick(char *nick)
{
	register aluser *curr;
	int found = 0;

	for (int i = 0; i < 1000; i++)
	{
		curr = Lusers[i];

		while (curr)
		{
			if (strcasecmp(nick, curr->nick) == 0)
			{
				found = 1;
				break;
			}
			curr = curr->next;
		}

		if (found)
			break;
	}
	return (curr);
}

char *GetYYXXX(char *nick)
{
  register aluser *user;

  user = ToLuserNick(nick);

  if (user == NULL)
  {
#ifdef DEBUG
    printf("GetYYXXX(): Cannot find user %s\n", nick);
#endif
    return NULL;
  }
  else
  {
    return user->num;
  }
}

char *GetNick(char *num)
{
  register aluser *user;

  user = ToLuser(num);

  if (user == NULL)
  {
#ifdef DEBUG
    printf("GetNick(): Cannot find user %s\n", num);
#endif
    return NULL;
  }
  else
  {
    return user->nick;
  }
}

char *gethost(aluser *user)
{
	if ((user->mode & LFL_REGISTERED) && (user->mode & LFL_ISMODEX))
		return user->hiddenhost;
	else
		return user->site;
}

void onaccount(char *source, char *target, char *body)
{
  register aluser *user;
  char account[80] = "";
  char hiddenhost[80] = "";

  // Fetching arguments.
  GetWord(0, body, account);

  user = ToLuser(target);
  if (user == NULL)
    quit("ERROR! onaccount() can't find user", 1);

#ifdef DEBUG
  /* change the account in memory */
  printf("ACCOUNT %s (%s) --> %s\n", user->nick, user->num, account);
#endif

  user->account = (char *)MALLOC(strlen(account) + 1);
  strcpy(user->account, account);

  user->mode |= LFL_REGISTERED;

  if (user->mode & LFL_ISMODEX)
  {
	sprintf(hiddenhost, "%s%s", user->account, HIDDEN_HOST_SUFFIX);

	// Set hiddenhost in memory
	user->hiddenhost = (char *)MALLOC(strlen(hiddenhost) + 1);
	strcpy(user->hiddenhost, hiddenhost);
  }
}

void onnick(char *source, char *newnick, char *body)
{
  register aluser *user, *tempuser, **u;
  register asuser *suser, **s;
  register aserver *serv;
  char username[80] = "";
  char hostname[80] = "";
  char buffer[512] = "";
  char TS[10] = "";
  char YYXXX[6] = "";
  char modes[10] = "";
  char temp[20] = "";
  char account[80] = "";
  char hiddenhost[80] = "";
  char mode = 0;
  register achannelnode *chan;
  register anickchange *curr, *prec;
  int i = 0;
  int numPos = 5;

  /* a new user */
  if (strlen(source) == 2)
  {	/* Message comes from server */

    GetWord(1, body, TS);
    GetWord(2, body, username);
    GetWord(3, body, hostname);
    GetWord(4, body, modes);

    if (modes[0] == '+')
    {
      numPos++;

	    for (i = 0; i < strlen(modes); i++)
	    {
		    if (modes[i] == 'o')
			    mode |= LFL_ISOPER;
		    else if (modes[i] == 'k')
			    mode |= LFL_ISSERVICE;
		    else if (modes[i] == 'x')
			    mode |= LFL_ISMODEX;
		    else if (modes[i] == 'r')
		    {
			    numPos++;
			    mode |= LFL_REGISTERED;
			    GetWord(5, body, account);
		    	GetWord(6, body, temp);

          // If there is an account_id, delete it.
          for (int j = 0; j < strlen(account) + 1; j++ )
            if (account[j] == ':') account[j] = '\0';
	    	}
	    }
    }

    GetWord(numPos, body, YYXXX);

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
	onquit(YYXXX);
	return;		/*ignore */
      }
#ifdef BACKUP
    }
    else if (!strcasecmp(newnick, MAIN_NICK))
    {
      return;	/* ignore */
#endif
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

    // Checking if we have a numeric collision
    if (ToLuser(YYXXX))
    {
#ifdef DEBUG
      printf("ARGH!!! NUMERIC COLLISION\n");
#endif
      tempuser = ToLuser(YYXXX);
      sprintf(buffer, "ERROR onnick(): Numeric collision. Existing user: %s (%s) (lu_hash: %d). New user %s (arg: %s) (lu_hash: %d)\n", tempuser->nick, tempuser->num, lu_hash(tempuser->num), newnick, body, lu_hash(YYXXX));
      PutLog(buffer); 
      onquit(YYXXX);
    }

#ifdef UWORLD
    // Check ccontrol login
    ccontrolLogin(YYXXX, newnick, username, hostname);
#endif

	  TTLALLOCMEM += sizeof(aluser);
    user = (aluser *) calloc(1, sizeof(aluser));

    strcpy(user->num, YYXXX);

    user->nick = (char *)MALLOC(strlen(newnick) + 1);
    strcpy(user->nick, newnick);

    user->username = (char *)MALLOC(strlen(username) + 1);
    strcpy(user->username, username);

    user->site = (char *)MALLOC(strlen(hostname) + 1);
    strcpy(user->site, hostname);

    user->account = NULL;
    user->hiddenhost = NULL;

    if (mode & LFL_REGISTERED)
    {
	    user->account = (char *)MALLOC(strlen(account) + 1);
	    strcpy(user->account, account);
    }

    if ((mode & LFL_ISMODEX) && (mode & LFL_REGISTERED))
    {
	    snprintf(hiddenhost, 80, "%s%s", user->account, HIDDEN_HOST_SUFFIX);

	    user->hiddenhost = (char *)MALLOC(strlen(hiddenhost) + 1);
	    strcpy(user->hiddenhost, hiddenhost);
    }

    if (*newnick == '+')
      serv = &VirtualServer;
    else
      serv = ToServer(source);

    user->server = serv;

#ifdef DEBUG
    printf("NEW USER: %s!%s@%s (%s) on %s (%s)", newnick, username, hostname, YYXXX, serv->name, serv->YY);
    if (mode & LFL_ISOPER)
	    printf(" (IRC Operator)");
    if (mode & LFL_ISSERVICE)
	    printf(" (Network Service)");
    if (mode & LFL_REGISTERED)
	    printf(" (account: %s)", account);
    if ((mode & LFL_ISMODEX) && (mode & LFL_REGISTERED))
	    printf(" (hidden host: %s)", user->hiddenhost);
    printf("\n");
#endif

    user->time = atol(TS);
    user->mode = mode;

    user->channel = NULL;
    user->valchan = NULL;

    user->next = Lusers[lu_hash(YYXXX)];
    Lusers[lu_hash(YYXXX)] = user;

    /* add user in server's userlist
     */
    suser = (asuser *) MALLOC(sizeof(asuser));
    suser->N = user;
    suser->next = serv->users[su_hash(YYXXX)];
    serv->users[su_hash(YYXXX)] = suser;

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
    while (*u && strcmp(source, (*u)->num) != 0)
      u = &(*u)->next;
    user = *u;

#ifdef NICKSERV
    nserv_nick(newnick, user);
#endif

    if (user == NULL)
      quit("ERROR! onnick() can't find user", 1);

    s = &user->server->users[su_hash(source)];
    while (*s && strcmp((*s)->N->num, user->num) != 0)
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
	sprintf(buffer, "%s %d", user->nick,
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
  while (*u && strcmp(num, (*u)->num) != 0)
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
  while (*s != NULL && strcmp((*s)->N->num, user->num) != 0)
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

  TTLALLOCMEM -= strlen(user->nick) + 1;
  free(user->nick);
  TTLALLOCMEM -= strlen(user->username) + 1;
  free(user->username);
  TTLALLOCMEM -= strlen(user->site) + 1;
  free(user->site);
  if (user->mode & LFL_REGISTERED)
  {
    TTLALLOCMEM -= strlen(user->account) + 1;
    free(user->account);
  }
  if ((user->mode & LFL_REGISTERED) && (user->mode & LFL_ISMODEX))
  {
    TTLALLOCMEM -= strlen(user->hiddenhost) + 1;
    free(user->hiddenhost);
  }
  TTLALLOCMEM -= sizeof(aluser);
  free(user);
}

void onkill(char *source, char *target, char *comment)
{
  char buffer[200] = "";

  if (!strcasecmp(target, myYYXXX))
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
      myYY, SERVERNAME, GetNick(source));
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
  else if (!strcasecmp(target, ufakeYYXXX))
  {
    char buffer[200];
    sprintf(buffer, "%s is KILLED by %s", UFAKE_NICK, GetNick(source));
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
  char buffer[512] = "";

  user = ToLuserNick(nick);

  if (user == NULL)
  {
    sprintf(buffer, "%s 401 %s %s :No such nick\n", myYY, source, nick);
    sendtoserv(buffer);
  }
  else
  {
    sprintf(buffer, "%s 311 %s %s %s %s * :\n", myYY, source, user->nick,
      user->username, gethost(user));

    sendtoserv(buffer);

    chan = user->channel;
    if (chan != NULL && !(user->mode & LFL_ISSERVICE))
    {
      sprintf(buffer, "%s 319 %s %s :", myYY, source, user->nick);
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
	          myYY, source, nick);
	      }
      }
      strcat(buffer, "\n");
      sendtoserv(buffer);
    }

    if (user->mode & LFL_ISOPER)
    {
      sprintf(buffer, "%s 313 %s %s :is an IRC Operator\n", myYY, source, user->nick);
      sendtoserv(buffer);
    }

    if (user->mode & LFL_REGISTERED)
    {
      sprintf(buffer, "%s 330 %s %s %s :is logged in as\n", myYY, source, user->nick, user->account);
      sendtoserv(buffer);
    }
  }

  sprintf(buffer, "%s 318 %s %s :End of /WHOIS list.\n", myYY, source, nick);
  sendtoserv(buffer);
}
