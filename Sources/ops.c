/* @(#)$Id: ops.c,v 1.6 1999/04/04 17:00:13 seks Exp $ */

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

void op(char *source, char *chan, char *nicklist)
{
  char channel[CHANNELNAME_LENGTH] = "";
  char nick[80] = "";
  char buffer[512] = "";
  register auser *user;
  register achannel *ch;
  register int i;

  /* if the 1st arg is a channel name.. use it instead of 'chan' */
  if (*nicklist == '#')
  {
    GetnWord(0, nicklist, channel, CHANNELNAME_LENGTH);
    nicklist = ToWord(1, nicklist);
  }
  else
  {
    strncpy(channel, chan, CHANNELNAME_LENGTH - 1);
    channel[CHANNELNAME_LENGTH] = '\0';
    GuessChannel(source, channel);
  }

  if (strcmp(channel, "*") == 0)
  {
    notice(source, "SYNTAX: op <channel> <nick1> [nick2] [nick3] [...]");
    return;
  }

  /* Bad channel.. geez what am I supposed to do with it?? */
  if ((ch = ToChannel(channel)) == NULL || !ch->on)
  {
    notice(source, replies[RPL_NOTONCHANNEL][L_DEFAULT]);
    return;
  }

  if (!ch->AmChanOp)
  {
    notice(source, replies[RPL_NOTCHANOP][ch->lang]);
    return;
  }

  if (*source && (ch->flags & CFL_OPONLY) &&
    *nicklist && strcasecmp(nicklist, GetNick(source)))
  {
    notice(source, replies[RPL_OPSELFONLY][ch->lang]);
    return;
  }

  if (*source && (ch->flags & CFL_NOOP))
  {
    notice(source, replies[RPL_NOOP][ch->lang]);
    return;
  }

#ifdef DEBUG
  printf("OP requested....\nCHANNEL: %s\nNICKS: %s\n", channel, nicklist);
#endif
  if (*source && Access(channel, source) < OP_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  if (!*nicklist)
  {
	  strcpy(nick, GetNick(source));
  }
  else
  {
    GetWord(0, nicklist, nick);
  }

  i = 0;
  while (*nick)
  {
    user = ToUserNick(channel, nick);

#ifdef DEBUG
    if (user)
    {
      printf("USER FOUND!\n");
      printf("nick: %s, num: %s\n", user->N->nick, user->N->num);
      printf("Chanop: %s\n", (user->chanop) ? "YES" : "NO");
    }
    else
    {
      printf("USER %s NOT FOUND ON CHANNEL %s\n", nick, channel);
    }
#endif
    /* the user might be shitlisted.. */

    if (user)
    {
      if (ch->flags & CFL_STRICTOP && Access(channel, user->N->num) < OP_LEVEL)
      {
	if (*source)
	{
	  notice(source, "StrictOp is active and user is not authenticated");
	}
      }
      else if (IsShit(channel, user->N, NULL, NULL) < NO_OP_SHIT_LEVEL)
      {
	if (user && !user->chanop)
	{
	  user->chanop = 1;
	  changemode(channel, "+o", user->N->num, 0);
	  if (*source && strcmp(source, user->N->num) != 0)
	  {
	    sprintf(buffer, replies[RPL_YOUREOPPEDBY][ch->lang], GetNick(source));
	    notice(user->N->num, buffer);
	  }
	}
      }
      else
      {
        if (*source)
        {
	        sprintf(buffer, "%s: %s", nick, replies[RPL_CANTBEOPPED][ch->lang]);
	        notice(source, buffer);
        }
      }
    }
    else if (!user && *source)
    {
      sprintf(buffer, replies[RPL_USERNOTONCHANNEL][ch->lang], nick, channel);
      notice(source, buffer);
    }

    GetWord(++i, nicklist, nick);
  }

  AddEvent(EVENT_FLUSHMODEBUFF, now + MODE_DELAY, channel);
}

void deop(char *source, char *ch, char *nicklist)
{
  char channel[CHANNELNAME_LENGTH] = "";
  char nick[80] = "";
  char buffer[512] = "";
  register auser *user;
  register achannel *chan;
  register int i;

  /* if the 1st arg is a channel name.. use it instead of 'channel' */
  if (*nicklist == '#')
  {
    GetnWord(0, nicklist, channel, CHANNELNAME_LENGTH);
    nicklist = ToWord(1, nicklist);
  }
  else
  {
    strncpy(channel, ch, CHANNELNAME_LENGTH);
    GuessChannel(source, channel);
  }

  if (strcmp(channel, "*") == 0)
  {
    notice(source, "SYNTAX: deop <channel> <nick1> [nick2] [nick3] [...]");
    return;
  }

#ifdef DEBUG
  printf("DEOP requested....\nCHANNEL: %s\nNICKS: %s\n", channel, nicklist);
#endif

  if ((chan = ToChannel(channel)) == NULL || !chan->on)
  {
    notice(source, replies[RPL_NOTONCHANNEL][L_DEFAULT]);
    return;
  }

  if (*source && (chan->flags & CFL_OPONLY))
  {
    notice(source, replies[RPL_OPONLY][chan->lang]);
    return;
  }


  if (!chan->AmChanOp)
  {
    notice(source, replies[RPL_NOTCHANOP][chan->lang]);
    return;
  }

  if (*source && Access(channel, source) < OP_LEVEL)
  {
    ReplyNotAccess(source, channel);
    return;
  }

  if (!*nicklist)
  {
    notice(source, "SYNTAX: deop [channel] <nick1> [nick2] [nick3] [...]");
    return;
  }

  i = 0;
  GetWord(i, nicklist, nick);
  while (*nick)
  {
    user = ToUserNick(channel, nick);
    if (user)
    {

      // We don't deop a network service.
      if (user->N->mode & LFL_ISSERVICE)
      {
        sprintf(buffer, replies[RPL_ISSERVICE][chan->lang], user->N->nick);
        notice(source, buffer);
        return;
      }

      if (user && user->chanop)
      {
	user->chanop = 0;
	changemode(channel, "-o", user->N->num, 0);
	if (*source && strcmp(source, user->N->num) != 0)
	{
	  sprintf(buffer, replies[RPL_YOUREDEOPPEDBY][chan->lang], GetNick(source));
	  notice(user->N->num, buffer);
	}
      }
    }
    GetWord(++i, nicklist, nick);
  }
  AddEvent(EVENT_FLUSHMODEBUFF, now + MODE_DELAY, channel);
}

void massdeop(char *channel)
{
  achannel *chan;
  auser *user;

  chan = ToChannel(channel);
  if (chan == NULL || !chan->on || !chan->AmChanOp)
    return;

  user = chan->users;
  while (user != NULL)
  {
    if (user->chanop && !(user->N->mode & LFL_ISSERVICE))
    {
      changemode(channel, "-o", user->N->num, 0);
      user->chanop = 0;
    }
    user = user->next;
  }
  flushmode(channel);
}
