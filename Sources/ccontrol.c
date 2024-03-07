
#include "h.h"

/* Triggered by onnick() and checking if the user matches the configuration options. 
If yes, passes login command. */

void ccontrolLogin (char *YYXXX, char *nick, char *username, char *site)
{
	char buffer[200] = "";

	// Checking if the user matches the configuration options.
	if (strcasecmp(nick, UWORLD) || strcasecmp(username, UWORLD_USER) || strcasecmp(site, UWORLD_HOST))
		return;

	// We have a positive match. Passing login command.
	sprintf(buffer, "%s P %s :LOGIN %s %s\n", myYYXXX, YYXXX, UWORLD_USERNAME, UWORLD_PASSWORD);
	sendtoserv(buffer);

	// Setting UWORLD's servernum
	strncpy(uworldYY, YYXXX, 2);
	uworldYY[2] = '\0';
	
#ifdef DEBUG
	printf("UWORLD SERVER DETECTED with YY: %s\n", uworldYY);
#endif
}
