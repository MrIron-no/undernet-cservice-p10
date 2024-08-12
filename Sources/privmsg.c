/* @(#)$Id: privmsg.c,v 1.22 2000/10/24 16:04:24 seks Exp $ */

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

void privmsg(char *source, char *target, char *body)
{
  register auser *user;
  register achannel *chan;
  register char *ptr;
  char global[] = "*";

#ifdef DEBUG
  printf("PRIVMSG from %s to %s with body %s\n", source, target, body);
#endif

  if ((*target == '#' || *target == '$') && strchr(target, '*'))
  {
#ifdef DEBUG
    printf("Received WALL... ignoring.. \n");
#endif
    return;
  }

  /* CTCP */
  if (body[1] == '\001' && (ptr = strchr(body + 2, '\001')) != NULL)
  {
    if (*target == '#' || *target == '&')
    {
      if (CheckFlood(source, target, strlen(ToWord(1, body + 1))))
	return;
    }

    *ptr = '\0';
    if (!IsIgnored(source) &&
      !CheckPrivateFlood(source, strlen(body), "CTCP-"))
      parse_ctcp(source, target, body + 2);
  }

  /* PRIVMSG TO A CHANNEL */
  else if (*target == '#' || *target == '&')
  {
    /* PRIVMSG #blah@channels.undernet.org ?? */
    if (!(chan = ToChannel(target)))
      return;
    chan->lastact = now;
    user = ToUser(target, source);
    if (!user)
      return;	/* not on channel.. happens if not +n */
    user->lastact = now;

    if (CheckFlood(source, target, strlen(ToWord(1, body + 1))))
      return;

    if (!strncmp(body + 1, COMMAND_PREFIX, strlen(COMMAND_PREFIX)))
      parse_command(source, target, target, body + strlen(COMMAND_PREFIX) + 1);

    /* PRIVATE PRIVMSG */
  }
  else
  {
    if (!IsIgnored(source) && !CheckPrivateFlood(source, strlen(body), "MSG-"))
    {
#ifdef FAKE_UWORLD
      if (strcmp(target, ufakeYYXXX) == 0)
      {
	parse_uworld_command(source, body + 1);
      }
      else
#endif
	parse_command(source, target, global, body + 1);
    }
  }
}

void parse_command(char *source, char *target, char *channel, char *commandline)
{
  char buffer[1024] = "";
  char command[80] = "";
  char global[] = "*";
  register aluser *user;

  GetWord(0, commandline, command);

#ifdef DEBUG
  printf("PARSING COMMAND: %s\nCOMMAND: %s\nTARGET: %s\nARGS: %s\nSOURCE: %s\n",
    commandline, command, target, ToWord(1, commandline), source);
#endif

  user = ToLuser(source);

  /* all commands must come from a user */
  if (user == NULL)
    return;

  if (strcasecmp(command, "pass") && strcasecmp(command, "login") &&
    strcasecmp(command, "newpass"))
  {
    sprintf(buffer, "COMMAND FROM %s!%s@%s on %s: %s",
      user->nick, user->username, gethost(user),
      channel, commandline);
    PutLog(buffer);
  }
  else
  {
    sprintf(buffer, "COMMAND FROM %s!%s@%s on %s: %s XXXXXXX",
      user->nick, user->username, gethost(user),
      channel, command);
    PutLog(buffer);
  }

  if (!strcasecmp(command, "showcommands"))
      showcommands(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "pass") || !strcasecmp(command, "login"))
      validate(source, target, ToWord(1, commandline));

  else if (!strcasecmp(command, "deauth"))
      DeAuth(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "die") && Access(global, source) >= LEVEL_DIE)
      quit(ToWord(1, commandline), 0);

  else if (!strcasecmp(command, "restart") && Access(global, source) >= LEVEL_DIE)
      restart(ToWord(1, commandline));	/* added by Kev; restarts */

  else if (!strcasecmp(command, "search"))
      SearchChan(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "join"))
      join(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "part"))
	part(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "op"))
	op(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "deop"))
	deop(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "banlist"))
	showbanlist(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "kick"))
	kick(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "invite"))
	invite(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "topic"))
	topic(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "adduser"))
	AddUser(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "remuser"))
	RemoveUser(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "modinfo"))
	ModUserInfo(source, target, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "newpass"))
	ChPass(source, target, ToWord(1, commandline));

  else if (!strcasecmp(command, "set"))
	SetChanFlag(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "access"))
	showaccess(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "suspend"))
	suspend(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "unsuspend"))
	unsuspend(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "saveuserlist"))
	SaveUserList(source, channel);

  else if (!strcasecmp(command, "lbanlist"))
	ShowShitList(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "ban"))
	AddToShitList(source, channel, ToWord(1, commandline), 0);

  else if (!strcasecmp(command, "unban"))
	RemShitList(source, channel, ToWord(1, commandline), 0);

  else if (!strcasecmp(command, "cleanbanlist"))
	CleanShitList(source, ToWord(1, commandline));

  else if (!strcasecmp(command, "addchan"))
	AddChan(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "remchan"))
	RemChan(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "savedefs"))
	SaveDefs(source);

  else if (!strcasecmp(command, "loaddefs"))
	LoadDefs(source);

  else if (!strcasecmp(command, "savebanlist"))
	SaveShitList(source, channel);

  else if (!strcasecmp(command, "loadbanlist"))
	LoadShitList(source);

  else if (!strcasecmp(command, "status"))
	showstatus(source, channel, ToWord(1, commandline));

#ifndef HIS_SERVERNAME
  else if (!strcasecmp(command, "map"))
	showmap(source);
#endif

  else if (!strcasecmp(command, "help"))
	showhelp(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "chaninfo"))
	ShowChanInfo(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "motd"))
	showmotd(source);

  else if (!strcasecmp(command, "isreg"))
        isreg(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "core"))
	dumpcore(source);

#ifdef RUSAGE_SELF
  else if (!strcasecmp(command, "rusage"))
	show_rusage(source);
#endif

  else if (!strcasecmp(command, "showignore"))
	ShowIgnoreList(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "remignore"))
	AdminRemoveIgnore(source, channel, ToWord(1, commandline));

#ifndef OPERCMD_DISABLE
  else if (!strcasecmp(command, "calmdown"))
	CalmDown(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "operjoin"))
	OperJoin(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "operpart"))
	OperPart(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "clearmode"))
	ClearMode(source, channel, ToWord(1, commandline));
#endif

  else if (!strcasecmp(command, "purge"))
	purge(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "register"))
	RegChan(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "verify"))
	verify(source, ToWord(1, commandline));

#ifdef UPGRADE
  else if (!strcasecmp(command, "upgrade"))
	upgrade(source, ToWord(1, commandline));
#endif

  else if (!strcasecmp(command, "random"))
	RandomChannel(source);

  else if (!strcasecmp(command, "say"))
	Say(source, ToWord(1, commandline));

  else if (!strcasecmp(command, "servnotice"))
	ServNotice(source, ToWord(1, commandline));

  else if (!strcasecmp(command, "fuck"))
	notice(source, "This command is obsolete");

#ifdef DEBUG
  else if (!strcasecmp(command, "db"))
	db_test(source, channel, ToWord(1, commandline));
#endif

#ifdef DOHTTP
  else if (!strcasecmp(command, "rehash"))
	read_http_conf(source);
#endif
#ifdef FAKE_UWORLD
  else if (!strcasecmp(command, "uworld"))
	Uworld_switch(source, channel, ToWord(1, commandline));

  else if (!strcasecmp(command, "opersuspend"))
	OperSuspend(source, ToWord(1, commandline));
#endif

#ifdef DEBUG
  else if (!strcasecmp(command, "showusers"))
	showusers(ToWord(1, commandline));

  else if (!strcasecmp(command, "showchannels"))
	showchannels();
#endif

#ifdef NICKSERV
      else if (!strcasecmp(command, "nickserv"))
	nserv_nickserv(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "addnick"))
	nserv_addnick(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "remnick"))
	nserv_remnick(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "addmask"))
	nserv_addmask(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "remmask"))
	nserv_remmask(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "nickinfo"))
	nserv_nickinfo(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "identify"))
	nserv_identify(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "ghost"))
	nserv_ghost(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "nicknewpass"))
	nserv_nicknewpass(source, ToWord(1, commandline));

      else if (!strcasecmp(command, "nicknewemail"))
	nserv_nicknewemail(source, ToWord(1, commandline));
#endif
#if defined(DOHTTP) && !defined(HTTP_EXT_DISABLE)
  else if (!strcasecmp(command, "dccme"))
	DccMe(source, ToWord(1, commandline));
#endif
}

void parse_ctcp(char *source, char *target, char *body)
{
  char func[80] = "";
  char buffer[1024] = "";
  char tmp[80] = "";

  GetWord(0, body, func);
  body = ToWord(1, body);

  if (strcasecmp(func, "ACTION"))
  {
    sprintf(buffer, "CTCP %s from %s [%s]", func, GetNick(source), body);
    PutLog(buffer);
  }

  if (match(func, "PING"))
  {
    sprintf(buffer, "\001PING %s\001", body);
    notice(source, buffer);
  }
  else if (match(func, "TIME"))
  {
    strcpy(tmp, ctime(&now));
    *strchr(tmp, '\n') = '\0';
    sprintf(buffer, "\001TIME %s\001", tmp);
    notice(source, buffer);
  }
  else if (match(func, "ITIME"))
  {
    sprintf(buffer, "\001ITIME @%03ld\001",
      1000 * ((now + 3600) % 86400) / 86400);
    notice(source, buffer);
  }
  else if (match(func, "VERSION"))
  {
    sprintf(buffer, "\001VERSION %s\001", VERSION);
    notice(source, buffer);
  }
  else if (match(func, "GENDER"))
  {
    notice(source, "\001GENDER I'm a male bot! Are you a pretty young bottesse?\001");
  }
}
