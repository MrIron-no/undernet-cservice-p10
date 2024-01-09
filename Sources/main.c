
/* @(#)$Id: main.c,v 1.21 2000/04/25 00:04:27 seks Exp $ */

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

#define MAIN
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include "../config.h"
#include "debug.h"
#include "defines.h"
#include "struct.h"
#include "lang.h"
#include "prototypes.h"
#include "events.h"
#include "flags.h"
#include "version.h"

#ifndef SIGCLD
#define SIGCLD SIGCHLD
#endif
#if !defined(__FreeBSD__)
extern int errno;
#endif
char mynick[NICK_LENGTH] = DEFAULT_NICKNAME;
char myuser[USERNAME_LENGTH] = DEFAULT_USERNAME;
char mysite[SITE_LENGTH] = DEFAULT_HOSTNAME;
char myrealname[REALNAME_LENGTH] = DEFAULT_REALNAME;
char mynum[20];
char myuplink[10] = "";
char uworldservernum[10] = "";
char *TmpPtr;
int logfile;
int bursting = 0;
time_t now;
time_t logTS = 0;
time_t TSoffset = 0;
time_t TSonline;
time_t TSconnect;
unsigned long long TTLREADBYTES = 0;
unsigned long long TTLSENTBYTES = 0;
unsigned long TTLALLOCMEM = 0;
unsigned long long HTTPTTLSENTBYTES = 0;
long CurrentSendQ = 0;

char server[SERVER_NAME_LENGTH] = DEFAULT_SERVER;

RegUser *UserList[1000];
ShitUser *ShitList[1000];
aluser *Lusers[1000];
achannel *ChannelList[1000];
adefchan *DefChanList = NULL;
anevent *EventList = NULL;
aserver *ServerList = NULL;
aserver VirtualServer;
dbquery *DBQuery = NULL;
dbsync *DBSync = NULL;
syncchan *SyncChan = NULL;
#ifdef DOHTTP
http_socket *HttpList = NULL;
http_file_pipe *FilePipes = NULL;
#endif
misc_socket *MiscList = NULL;
irc_socket Irc =
{-1, 0, NULL, NULL, NULL};
unsigned long MEM_buffers = 0;

static int Data_files_loaded = 0;

int DB_Save_Status = -1;
char DB_Save_Nick[NICK_LENGTH] = "";

const unsigned int glob_cksum1 = BINCKSUM1;
const unsigned int glob_cksum2 = 0;

#ifdef FAKE_UWORLD
char ufakenum[10];
int Uworld_status = 0;
time_t UworldTS, UworldServTS;
#endif

#ifdef NICKSERV
char nservnum[10];
int NServ_status = 1;
#endif

void rec_sigpipe(int sig)
{
  quit("ERROR: Received SIGPIPE :(", 1);
}

void rec_sigsegv(int sig)
{
  quit("ERROR: Received SIGSEGV :(", 1);
}

void rec_sigbus(int sig)
{
  quit("ERROR: Received SIGBUS :(", 1);
}

void rec_sigterm(int sig)
{
  quit("Oops! I'll be right back...", 0);
}

void rec_sigint(int sig)
{
  restart("Received SIGINT.. that probably meant "
    "\"restart you stupid bot!\" ;)");
}

void rec_sigusr(int sig)
{
  /*fflush(logfile); */
  signal(SIGUSR1, rec_sigusr);
}

void regist(void)
{
  char buffer[512];
  time_t t;
  t = time(NULL);

  /* First clean memory */
  // QuitAll(); 

  bursting = 1;

  /* Then send reg stuff to server.. */
  sprintf(buffer, "PASS :%s\nSERVER %s 1 %ld %ld J10 %s]]] +s :%s\n",
    PASSWORD, SERVERNAME, t, t, NUMERIC, SERVERINFO);
  sendtoserv(buffer);
}

void signon(void)
{
  char buffer[512];
  sprintf(buffer, "%s N %s 1 31337 %s %s %s AAAAAA %s :%s\n", NUMERIC, mynick, myuser, mysite, UMODE, mynum, myrealname);
  sendtoserv(buffer);

#ifdef FAKE_UWORLD
  if (Uworld_status == 1)
    IntroduceUworld();
#endif
#ifdef NICKSERV
  if (NServ_status == 1)
    IntroduceNickserv();
#endif
}

#ifdef FAKE_UWORLD
void IntroduceUworld(void)
{
  char buffer[512];
  UworldServTS = now;
  UworldTS = logTS;	/* force nick collision */

  sprintf(ufakenum, "%sAAA", UFAKE_NUMERIC);
  sprintf(buffer, "%s S %s 2 0 %ld J10 %sAAA +s :%s\n"
    "%s N %s 2 31337 %s %s +o AAAAAA %s :%s\n%s EB\n%s EA\n",
    NUMERIC, UFAKE_SERVER, UworldServTS, UFAKE_NUMERIC, UFAKE_INFO,
    UFAKE_NUMERIC, UFAKE_NICK, UFAKE_NICK, UFAKE_HOST, ufakenum, UFAKE_INFO, 
    UFAKE_NUMERIC, UFAKE_NUMERIC);
  sendtoserv(buffer);
}

void KillUworld(char *msg)
{
  char buffer[200];
  sprintf(buffer, "%s Q :%s\n"
    "%s SQ %s %ld :%s\n",
    ufakenum, msg,
    UFAKE_NUMERIC, SERVERNAME, UworldServTS, msg);
  sendtoserv(buffer);
}
#endif

#ifdef NICKSERV
void IntroduceNickserv(void)
{
  char buffer[512];

  sprintf(nservnum, "%sAAB", NUMERIC);
  sprintf(buffer, "%s N %s 1 31337 %s %s +kd AAAAAA %s :%s\n",
    NUMERIC, NSERV_NICK, NSERV_USER, NSERV_HOST,
    nservnum, NSERV_INFO);
  sendtoserv(buffer);
  NServ_status = 1;
}

void KillNickserv(char *msg)
{
  char buffer[200];
  sprintf(buffer, "%s Q :%s\r\n", nservnum, msg);
  sendtoserv(buffer);
  NServ_status = 0;
}
#endif

int reconnect(char *server)
{
#ifdef BACKUP
  quit("DISCONNECTED", 0);
  /* not reached */
#else
  close(Irc.fd);
  Irc.fd = -1;
#ifdef DEBUG
  printf("Lost connection... sleeping 5 seconds...\n");
#endif
  PutLog("Lost connection somehow.. trying to reconnect in 5 seconds");
  sleep(5);
  QuitAll();
  if (!connection(server))
  {
    regist();
    signon();
    SendBurst();
    return 0;
  }
  else
    return 1;
#endif
}

void try_later(char *server)
{
#ifdef BACKUP
  quit("DISCONNECTED", 0);
#else
  close(Irc.fd);
  QuitAll();
  do
  {
    Irc.fd = -1;
    PutLog("Oh well.. let's try again in one minute");
#ifdef DEBUG
    printf("Unable to keep the connection... sleeping 60 seconds...\n");
#endif
    sleep(60);
  }
  while (connection(server));
  regist();
  signon();
  SendBurst();
#endif
}


void dumpcore(char *source)
{
  int file;
  pid_t pid;
  char global[] = "*";

  if (*source && Access(global, source) < LEVEL_CORE)
  {
    notice(source, "Sorry. Your Access is a little too low for that!");
    return;
  }

  pid = getpid();
  if (fork() >= 0)
  {
    kill(pid, SIGABRT);
  }

  /* rewrite the pid file after the fork()
   */
  alarm(2);
  file = open(PIDFILE, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  alarm(0);
  if (file >= 0)
  {
    char buf[32];
    sprintf(buf, "%ld\n", (long)getpid());
    alarm(2);
    write(file, buf, strlen(buf));
    alarm(0);
    close(file);
  }

  PutLog("Core dumped");

  if (*source)
  {
    notice(source, "Core dumped");
  }
}

#ifdef RUSAGE_SELF
void show_rusage(char *source)
{
  char buffer[512], global[] = "*";
  struct rusage usage;

  if (Access(global, source) < RUSAGE_ACCESS)
  {
    notice(source, "This command is not for you!");
    return;
  }

  getrusage(RUSAGE_SELF, &usage);
  sprintf(buffer, "utime: %ld.%06lds stime: %ld.%06lds",
    (long)usage.ru_utime.tv_sec, (long)usage.ru_utime.tv_usec,
    (long)usage.ru_stime.tv_sec, (long)usage.ru_stime.tv_usec);
  notice(source, buffer);
  sprintf(buffer, "maxrss: %ld ixrss %ld idrss: %ld isrss: %ld",
    (long)usage.ru_maxrss, (long)usage.ru_ixrss, (long)usage.ru_isrss,
    (long)usage.ru_isrss);
  notice(source, buffer);
  sprintf(buffer, "minflt: %ld majflt: %ld nswap: %ld",
    (long)usage.ru_minflt, (long)usage.ru_majflt, (long)usage.ru_nswap);
  notice(source, buffer);
  sprintf(buffer, "inblock: %ld oublock: %ld msgsnd: %ld msgrcv: %ld",
    (long)usage.ru_inblock, (long)usage.ru_oublock, (long)usage.ru_msgsnd,
    (long)usage.ru_msgrcv);
  notice(source, buffer);
  sprintf(buffer, "nsignals: %ld nvcsw: %ld nivcsw: %ld",
    (long)usage.ru_nsignals, (long)usage.ru_nvcsw, (long)usage.ru_nivcsw);
  notice(source, buffer);
}
#endif

int quit(char *msg, int flag)
{
  char buffer[200];
#ifdef HISTORY
  History(NULL);
#endif

  if (Data_files_loaded)
  {
    /*SaveUserList("",NULL); */
    do_cold_sync();	/* save userlist */
    SaveShitList("", NULL);
    SaveDefs("");
#ifdef NICKSERV
    nserv_save();
#endif
    sync();
  }

  if (!msg || !*msg)
    sprintf(buffer, "%s Q :%s\n"
      "%s SQ %s 0 :die request\n",
      mynum, mynick, NUMERIC, SERVERNAME);
  else
    sprintf(buffer, "%s Q :%s\n"
      "%s SQ %s 0 :%s\n",
      mynum, msg, NUMERIC, SERVERNAME, msg); // TODO: This is supposed to send the name of the uplink.

  if (Irc.fd >= 0)
  {
    sendtoserv(buffer);
    dumpbuff();
  }

  sprintf(buffer, "LEAVING (%s)", msg);
  PutLog(buffer);
  PutLog("Closing log file");
  close(logfile);

#ifdef DEBUG_MALLOC
  close_debug_malloc();
#endif

  unlink(PIDFILE);

  if (flag)
    abort();
  else
    exit(0);
}

int restart(char *msg)		/* added by Kev */
{
  char buffer[600];
  int i;

  SaveUserList("", NULL);	/* save necessary data... */
  SaveShitList("", NULL);
  SaveDefs("");
#ifdef NICKSERV
  nserv_save();
#endif
  sync();

  if (!msg || !*msg)	/* send out QUIT/SQUIT stuff... */
    sprintf(buffer, "%s Q :restarting...\n"
      "%s SQ %s 0 :restart request\n",
      mynum, NUMERIC, SERVERNAME);
  else
    sprintf(buffer, "%s Q :%s\n"
      "%s SQ %s 0 :%s\n",
      mynum, msg, NUMERIC, SERVERNAME, msg);

  if (Irc.fd >= 0)
  {	/* clear buffer... */
    sendtoserv(buffer);
    dumpbuff();
  }

  sprintf(buffer, "RESTARTING (%s)", msg);	/* log the restart... */
  PutLog(buffer);
  switch (fork())
  {
  case -1:
    PutLog("Fork error; unable to restart");	/* couldn't fork :/ */
    PutLog("Closing log file");	/* and close the log file... */
    close(logfile);
    break;

  case 0:
    /* No need to write "closing log file". It's already done by parent */
    close(logfile);	/* redundant, I know, but I want to know why it didn't
			   come back.... */
    for (i = 0; i < MAX_CONNECTIONS; i++)
      close(i);		/* close all fds -seks */
    sprintf(buffer, "%s/%s", HOMEDIR, EXEC_FILE);
    execl(buffer, EXEC_FILE, (char *)NULL);	/* and restart */
    break;

  default:
    PutLog("Closing log file");
    close(logfile);
    exit(0);
  }
  exit(-1);
}

void notice(char *target, char *msg)
{
  char buffer[200];
//#ifdef DOHTTP
//  extern chat_notice(char *, char *);
//#endif

#ifdef DOHTTP
  if (*target == '+')
  {
    chat_notice(target, msg);
    return;
  }
#endif

  sprintf(buffer, "%s O %s :%s\n", mynum, target, msg);
  sendtoserv(buffer);
}

void servnotice(char *target, char *msg)
{
  char buffer[200];
  sprintf(buffer, "%s O %s :%s\n", NUMERIC, target, msg);
  sendtoserv(buffer);
}


void broadcast(char *msg, int evenwallop)
{
#ifdef DOHTTP
  extern void chat_sendtoall(char *, char *);
#endif
  char buffer[1024];
  achannel *chan;

  chan = ToChannel(BROADCAST_CHANNEL);
  if (chan != NULL)
  {
    sprintf(buffer, "[%s] %s", mynick, msg);
    servnotice(BROADCAST_CHANNEL, buffer);
  }
  else if (evenwallop)
  {
    sprintf(buffer, "%s WA :%s\n",
      NUMERIC, msg);
    sendtoserv(buffer);
  }
#ifdef DOHTTP
  sprintf(buffer, "[%s] %s", mynick, msg);
  chat_sendtoall(NULL, buffer);
#endif
}

char *ToWord(int nb, char *string)
{
  register char *ptr1 = string;
  register char *ptr2;
  register int i = 0;
  while (i != nb)
  {
    if ((ptr2 = strchr(ptr1, ' ')) != NULL)
    {
      while (*(++ptr2) == ' ');
      ptr1 = ptr2;
    }
    else
      ptr1 = strchr(ptr1, '\0');
    i++;
  }
  return ptr1;
}

void GetWord(int nb, char *string, char *output)
{
  register char *ptr1;
  register char *ptr2;
  register int count = 0;

  ptr1 = ToWord(nb, string);
  ptr2 = output;
  while (*ptr1 && *ptr1 != ' ' && count++ < 75)
    *(ptr2++) = *(ptr1++);
  *ptr2 = 0;
}


char *time_remaining(time_t t)
{
  static char s[80];
  int days, hours, mins, secs;

  days = (int)t / 86400;
  t %= 86400;
  hours = (int)t / 3600;
  t %= 3600;
  mins = (int)t / 60;
  t %= 60;
  secs = (int)t;

  if (days == 0)
    sprintf(s, "%02d:%02d:%02d", hours, mins, secs);
  else if (days == 1)
    sprintf(s, "1 day, %02d:%02d:%02d", hours, mins, secs);
  else
    sprintf(s, "%d days, %02d:%02d:%02d", days, hours, mins, secs);

  return s;
}

void pong(char *who, char *remotets)
{
  char buffer[80];
  time_t offset;

  if (remotets[0] == '!')
    remotets++;

  offset = atol(remotets) - now;

  sprintf(buffer, "%s Z %s !%s %ld %ld\n", NUMERIC, who, remotets, offset, now);
  sendtoserv(buffer);
}

void rpong(char *who, char *args)
{
  char buffer[80];
  register aserver *server;
  server = ToServer(who);

  sprintf(buffer, "%s RO %s %s\n", NUMERIC, server->name, args);
  sendtoserv(buffer);
}

void PutLog(char *text)
{
  static char date[80], buffer[1024];
  strcpy(date, ctime(&now));
  *strchr(date, '\n') = '\0';
  sprintf(buffer, "%s: %s\n", date, text);
  alarm(3);
  write(logfile, buffer, strlen(buffer));
  alarm(0);
}

void checkpid(void)
{
  int file;
  pid_t pid = 0;
  char fmt[5];

  if (sizeof(pid_t) == sizeof(long))
     strcpy(fmt, "%ld");
  else
    strcpy(fmt, "%d");

  alarm(2);
  file = open(PIDFILE, O_RDONLY);
  alarm(0);
  if (file >= 0)
  {
    char buf[32];
    alarm(2);
    if (read(file, buf, 31) >= 0)
    {
      alarm(0);
      sscanf(buf, fmt, &pid);
    }
    else
    {
      alarm(0);
      pid = 0;
    }
    close(file);
  }

  if (pid == 0 || kill(pid, 0) != 0)
  {
    char buf[31];
    alarm(2);
    file = open(PIDFILE, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    alarm(0);
    if (file < 0)
    {
      perror(PIDFILE);
      exit(1);
    }
    sprintf(buf, fmt, getpid());
    strcat(buf, "\n");
    alarm(2);
    if (write(file, buf, strlen(buf)) < 0)
    {
      fprintf(stderr, "%s: %s\n", PIDFILE, strerror(errno));
      exit(1);
    }
    alarm(0);

    if (close(file) < 0)
    {
      fprintf(stderr, "%s: %s\n", PIDFILE, strerror(errno));
      exit(1);
    }
  }
  else
  {
    /* the service is already running
     */
    exit(0);
  }
}

void proc(char *source, char *function, char *target, char *body)
{
  char buffer[200];

//  printf("source: %s, function: %s, target: %s, body: %s\n", source, function, target, body);

  if (!strcmp(source, "SERVER") || !strcmp(function, "S"))
  {
    onserver(source, function, target, body);

  }
  else if (!strcmp(function, "G"))
  {
    pong(source, target);

  }
  else if (!strcmp(function, "RI"))
  {
    rpong(source, body);

  }
  else if (!strcmp(function, "EB"))
  {
    if (strcmp(source, myuplink)) return;

    sprintf(buffer, "%s EA\n", NUMERIC);
    sendtoserv(buffer);

    bursting = 0;
  }
  else if (!strcmp(source, "ERROR") || !strcmp(function, "Y"))
  {
    PutLog(function);
    PutLog(target);
    PutLog(body);
    if (reconnect(server))
    {
      try_later(server);
      return;
    };

  }
  else if (!strcmp(function, "SQ"))
  {
    onsquit(source, target, body);
    if (ServerList == NULL)
    {
      PutLog("received squit from");
      PutLog(source);
      PutLog(body);
      if (reconnect(server))
      {
	try_later(server);
	return;
      }
    }
  }
  else if (!strcmp(function, "P"))
  {
    privmsg(source, target, body);

  }
  else if (!strcmp(function, "O"))
  {	/* only for flood pro */
    onnotice(source, target, body);

  }
  else if (!strcmp(function, "J") || !strcmp(function, "C"))
  {
    onjoin(function, source, target, body);

  }
  else if (!strcmp(function, "B"))
  {
    onburst(source, target, body);

  }
  else if (!strcmp(function, "I"))
  {
    oninvite(source, body);

  }
  else if (!strcmp(function, "L"))
  {
    onpart(source, target);

  }
  else if (!strcmp(function, "D"))
  {
    onkill(source, target, body);

  }
  else if (!strcmp(function, "Q"))
  {
    onquit(source);

  }
  else if (!strcmp(function, "K"))
  {
    onkick(source, target, body);

  }
  else if (!strcmp(function, "M"))
  {
    ModeChange(source, target, body);

  }
  else if (!strcmp(function, "N"))
  {
    onnick(source, target, body);

  }
  else if (!strcmp(function, "T"))
  {
    ontopic(source, target, body);

  }
  else if (!strcmp(function, "WHOIS"))
  {
    onwhois(source, body + 1);

  }
  else if (!strcmp(function, "SE"))
  {
    onsettime(source, target);

  }
  else if (!strcmp(function, "V"))
  {
    showversion(source);
  }
  else if (!strcmp(function, "436"))
  {
    if (!strcasecmp(target, mynick))
    {
      NickInUse();
    }
  }
}

int main(int argc, char **argv)
{
  extern void cksum(char *, unsigned int *, unsigned int *);
  extern void read_conf(char *);
  int i;
  unsigned int sum1, sum2;

#if !defined(DEBUG)
  int pid;
#ifdef TIOCNOTTY
  int fd;
#endif
#endif
#ifdef RLIMIT_CORE
  struct rlimit rlim;
#endif
  char conf[256] = "./cs.conf";

  cksum(argv[0], &sum1, &sum2);
  sprintf(mynum, "%sAAA", NUMERIC);

  if (argc == 3 && !strcmp(argv[1], "-f"))
  {
    strncpy(conf, argv[2], 255);
    conf[255] = '\0';
  }
  else if (argc != 1)
  {
    fprintf(stderr, "usage: %s [-f config_file]\n", argv[0]);
    exit(1);
  }

  read_conf(conf);

  if (chdir(HOMEDIR) < 0)
  {
    perror(HOMEDIR);
    exit(1);
  }

  umask(UMASK);

  now = time(NULL);

  signal(SIGSEGV, rec_sigsegv);
  signal(SIGBUS, rec_sigbus);
  signal(SIGTERM, rec_sigterm);
#ifndef DEBUG
  signal(SIGINT, rec_sigint);
#endif
  signal(SIGCLD, SIG_IGN);
  /*signal(SIGPIPE,rec_sigpipe); */
  signal(SIGPIPE, SIG_IGN);
  signal(SIGUSR1, rec_sigusr);
  signal(SIGALRM, SIG_IGN);
  signal(SIGURG, SIG_IGN);

  /* Make sure RLIMITs are set properly */
#ifdef RLIMIT_CORE
  if (!getrlimit(RLIMIT_CORE, &rlim))
  {
    rlim.rlim_cur = rlim.rlim_max;
    setrlimit(RLIMIT_CORE, &rlim);
  }
#endif
#ifdef RLIMIT_RSS
  if (!getrlimit(RLIMIT_RSS, &rlim))
  {
    rlim.rlim_cur = rlim.rlim_max;
    setrlimit(RLIMIT_RSS, &rlim);
  }
#elif defined(RLIMIT_VMEM)
  if (!getrlimit(RLIMIT_VMEM, &rlim))
  {
    rlim.rlim_cur = rlim.rlim_max;
    setrlimit(RLIMIT_VMEM, &rlim);
  }
#endif

#if !defined(DEBUG)
  /* if the service is not running in DEBUG mode, then
     it'll go in the background                         */

  switch (pid = fork())
  {
  case 0:
    /* this is the child part */
    /* close open streams */
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
#if defined(BSD) || defined(__FreeBSD__)
    setpgrp(0, getpid());
#else
    setpgrp();
#endif
#ifdef TIOCNOTTY
    if ((fd = open("/dev/tty", O_RDWR)) >= 0)
    {
      ioctl(fd, TIOCNOTTY, NULL);
      close(fd);
    }
#endif
    break;
  case -1:
    fprintf(stderr,
      "\n"
      "ERROR: not enough memory to fork()\n"
      "exiting...\n");
    exit(1);
  default:
    /* fork() succeeded.. no problem */
    exit(0);
  }
#endif

  /* Check if the service is already running. This MUST be
   * after any possible fork() because the pid is saved to
   * a file for further reference.
   */
  checkpid();

#ifndef DEBUG
  printf("%s is now running in the background [pid %ld]\n",
    mynick, (long)getpid());
#endif

  if ((logfile = open(LOGFILE, O_WRONLY | O_CREAT | O_APPEND, 0600)) < 0)
  {
    perror("LOGFILE");
    exit(1);
  }

  PutLog("Opening log file");

#ifdef DOHTTP
  open_http();
  read_http_conf("");
#endif

#ifdef DEBUG_MALLOC
  open_debug_malloc();
#endif

  InitEvent();

  /* init userlist & shitlist */
  for (i = 0; i < 1000; i++)
  {
    UserList[i] = NULL;
    ShitList[i] = NULL;
  }
  for (i = 0; i < 1000; i++)
  {
    Lusers[i] = NULL;
    ChannelList[i] = NULL;
  }

  memset(&VirtualServer, 0, sizeof(aserver));
  VirtualServer.name = (char *)malloc(15);
  strcpy(VirtualServer.name, "virtual.server");

  LoadUserList("");
  LoadShitList("");
  LoadDefs("");
#ifdef NICKSERV
  nserv_load();
#endif

  Data_files_loaded = 1;

  /* exit if connection failed on first attempt */
  if (connection(server))
    exit(1);

  /* OK.. connection succeeded now send signon stuff... */
  regist();
  signon();
  SendBurst();

  TSonline = now;

  for (;;)
  {
    if (Irc.fd < 0 || wait_msg() < 0)
    {
      /* lost connection */
      try_later(server);
      continue;
    }
  }

  return 0;
}
