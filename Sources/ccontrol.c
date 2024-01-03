
#include "h.h"

/* Triggered by onnick() and checking if the user matches the configuration options. 
If yes, passes login command. */

void ccontrolLogin (char *num, char *nick, char *username, char *site)
{
	char buffer[200];
	char temp[20];

	// Checking if the user matches the configuration options.
	if (strcasecmp(nick, UWORLD) || strcasecmp(username, UWORLD_USER) || strcasecmp(site, UWORLD_HOST))
		return;

	// We have a positive match. Passing login command.
	sprintf(buffer, "%s P %s :LOGIN %s %s\n", mynum, num, UWORLD_USERNAME, UWORLD_PASSWORD);
	sendtoserv(buffer);

	// Setting UWORLD's servernum
	strcpy(temp, num);
	temp[2] = '\0';
	strcpy(uworldservernum, temp);
	printf("UWORLD SERVER DETECTED with numYY %s\n", uworldservernum);
}
