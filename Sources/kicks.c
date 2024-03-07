/* @(#)$Id: kicks.c,v 1.5 1997/07/18 07:55:04 cvs Exp $ */

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

void kick(char *source,char *chanarg,char *args)
{
	char buffer[500] = "";
	char nick[80] = "";
	char kicknum[6] = "";
	char channel[CHANNELNAME_LENGTH] = "";
	char *comment;
	int found=0;
	achannel *chan;
	auser *user;

	if(*args=='#'){
		GetnWord(0,args,channel, CHANNELNAME_LENGTH);
		GetWord(1,args,nick);
		comment=ToWord(2,args);
	}else{
		GetWord(0,args,nick);
		comment=ToWord(1,args);
		strcpy(channel,chanarg);
		GuessChannel(source,channel);
	}

	if(strlen(comment)>200)
		comment[200]='\0';

	if(strcmp(channel,"*") == 0 || !*nick){
		notice(source,"SYNTAX: kick [#channel] <nick|pattern> [reason]");
		return;
	}

	chan=ToChannel(channel);
	if(!chan||!chan->on){
		if(*source)
			notice(source,replies[RPL_NOTONCHANNEL][L_DEFAULT]);
		return;
	}

	if(*source && (chan->flags&CFL_OPONLY)){
		notice(source,replies[RPL_OPONLY][chan->lang]);
		return;
	}

	if(!chan->AmChanOp){
		if(*source)
			notice(source,replies[RPL_NOTCHANOP][chan->lang]);
		return;
	}

	/* check whether there are wildcards or not.
	 * if there are wildcards, it's a masskick and nick is a match pattern
	 * otherwise, it's an ordinary kick and nick is the nick to kick
	 */
	if(!strpbrk(nick,"*?")){
#ifdef DEBUG
		printf("KICK REQUEST (NO WILDCARDS)\nSOURCE %s\nCHANNEL %s\nTARGET %s\n",
			source,channel,nick);
#endif
		if(*source&&Access(channel,source)<KICK_LEVEL){
			ReplyNotAccess(source,channel);
			return;
		}

		user=ToUserNick(channel,nick);
		if(!user) return;

		// Check if the user is a network service
		if (user->N->mode & LFL_ISSERVICE)
		{
			if (*source)
			{
				sprintf(buffer,replies[RPL_ISSERVICE][chan->lang], user->N->nick);
				notice(source, buffer);
			}
			return;
		}

		if(*comment){
			if(*source && GetNick(source) != NULL)
				sprintf(buffer, "%s K %s %s :(%s) %s\n",
					myYYXXX,channel,user->N->num,GetNick(source),comment);
			else
				sprintf(buffer, "%s K %s %s :%s\n",
					myYYXXX,channel,user->N->num,comment);
		}else{
			if(*source && GetNick(source) != NULL)
				sprintf(buffer, "%s K %s %s :From %s\n",
					myYYXXX,channel,user->N->num,GetNick(source));
			else
				sprintf(buffer, "%s K %s %s :%s\n",
					myYYXXX,channel,user->N->num,mynick);
		}
		sendtoserv(buffer);
		sprintf(buffer,"I KICK %s OFF %s",nick,channel);
		PutLog(buffer);
		onpart(user->N->num, channel);
	} else {
#ifdef DEBUG
		sprintf(buffer, "KICK REQUEST (WITH WILDCARDS)\nSOURCE %s\nCHANNEL %s\nTARGET %s\n",
			source,channel,nick);
		printf("%s", buffer);
		PutLog(buffer);
#endif
		if(*source&&Access(channel,source)<MASS_KICK_LEVEL){
			ReplyNotAccess(source,channel);
			return;
		}

		user=chan->users;
		while(user){
			sprintf(buffer,"%s!%s@%s",user->N->nick,user->N->username,gethost(user->N));

			if(match(buffer,nick)&&(!*source||strcasecmp(user->N->num,source))){

				// Check if the user is a network service
				if (user->N->mode & LFL_ISSERVICE)
				{
					sprintf(buffer,replies[RPL_ISSERVICE][chan->lang], user->N->nick);
					notice(source, buffer);
					return;
				}

				if(*comment){
					if(*source && GetNick(source) != NULL)
						sprintf(buffer, "%s K %s %s :(%s) %s\n",myYYXXX,channel,user->N->num,GetNick(source),comment);
					else
						sprintf(buffer, "%s K %s %s :%s\n",myYYXXX,channel,user->N->num,comment);
				}else{
					if(*source && GetNick(source) != NULL)
						sprintf(buffer, "%s K %s %s :From %s\n",
						        myYYXXX,channel,user->N->num,GetNick(source));
					else
						sprintf(buffer, "%s K %s %s :%s\n",
							myYYXXX,channel,user->N->num,mynick);
				}
				sendtoserv(buffer);
				sprintf(buffer,"I KICK %s OFF %s",user->N->nick,channel);
				strncpy(kicknum, user->N->num, 5);
				user=user->next;
				onpart(kicknum, channel);
				PutLog(buffer);
				found=1;
				continue;
			}
			user=user->next;
		}
		if(*source && !found)
			notice(source,replies[RPL_NOMATCH][chan->lang]);
	}
}
