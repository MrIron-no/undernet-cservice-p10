#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>

using namespace std;

/* CONFIG */
#define PORT		7358
#define GETPATCHPASS	"TEST"
#define RECPATCHPASS	"BLAH"

int main()
{
    string buffer {} ;
    string PatchContent {} ;
    string pass {} ;

    char msg[1500] {} ;

    int upgradeversion {} ;
    int lastupdate {} ;
    ifstream patchfile ;

    cout << "Enter upgrade version: " ;
    cin >> upgradeversion ;

    // Setup a socket.
    sockaddr_in servAddr ;
    bzero( (char*)&servAddr, sizeof( servAddr ) ) ;
    servAddr.sin_family = AF_INET ;
    servAddr.sin_addr.s_addr = htonl( INADDR_ANY ) ;
    servAddr.sin_port = htons( PORT ) ;

    // Open stream
    int serverSd = socket( AF_INET, SOCK_STREAM, 0 ) ;
    if ( serverSd < 0 )
    {
        cerr << "Error establishing the server socket" << endl ;
        exit( 0 ) ;
    }

    // Bind the socket to its local address
    int bindStatus = bind( serverSd, (struct sockaddr*) &servAddr, sizeof( servAddr ) ) ;

    if( bindStatus < 0 )
    {
        cerr << "Error binding socket to local address" << endl ;
        exit( 0 ) ;
    }

    cout << "Waiting for X/W to connect..." << endl ;

    // Listen for up to two requests at a time
    listen( serverSd, 2 ) ;

    sockaddr_in newSockAddr ;
    socklen_t newSockAddrSize = sizeof( newSockAddr ) ;

    // Accept, create a new socket descriptor to handle the new connection with client
    int newSd = accept( serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize ) ;
    if( newSd < 0 )
    {
        cerr << "Error accepting request from X/W!" << endl ;
        exit( 1 ) ;
    }

    cout << "Connected with X/W!" << endl ;
    struct timeval start1, end1 ;
    gettimeofday( &start1, NULL ) ;

    // Keep track of the amount of data sent and received
    int bytesRead { 0 }, bytesWritten { 0 } ;

    while( 1 )
    {
        cout << "Awaiting handshake..." << endl ;
        memset( &msg, 0, sizeof( msg ) ) ; // Clear the buffer
        bytesRead += recv( newSd, (char*)&msg, sizeof( msg ), 0 ) ;

	// Fethcing incoming data.
	buffer = msg ;
	pass = buffer.substr( 0, buffer.find( '\n' ) ) ;
	lastupdate = stoi( buffer.substr( buffer.find( '\n' ) + 1 ) ) ;

	if ( strcmp( pass.c_str(), GETPATCHPASS ) != 0 )
	{
	    cout << "Wrong password!" << endl ;
	    break ;
	}

    	if ( lastupdate >= upgradeversion )
	{
	    cout << "Current version! Aborting." << endl ;
	    break ;
    	}

    	cout << "Correct password!" << endl ;

        // Checking if patchfile exists.
        buffer = to_string( lastupdate ) ;
        buffer += "-" ;
        buffer += to_string( upgradeversion ) ;
        buffer += ".patch" ;

        patchfile.open( buffer ) ;

        if ( !patchfile )
        {
            cout << "I don't have this patchfile: " << buffer << endl ;
            break ;
        }

	// Sending password and new update version.
        memset( &msg, 0, sizeof( msg ) ) ;
	sprintf( msg, "%s\n%d\r\n", RECPATCHPASS, upgradeversion ) ;
        bytesWritten += send( newSd, (char*)&msg, strlen( msg ), 0 ) ;

	cout << "Ready to send patch" << endl ;

        while ( getline( patchfile, buffer ) )
        {
            PatchContent += buffer ;
            PatchContent += '\n' ;
        }

        cout << PatchContent << endl ;
        bytesWritten += send( newSd, (char*)PatchContent.c_str(), strlen( PatchContent.c_str() ), 0 ) ;

        // Closing file.
        patchfile.close() ;

        cout << "Patchfile sent. Exiting." << endl ;

	// Exiting.
	break ;
    }

    // Close socket descriptors
    gettimeofday( &end1, NULL ) ;
    close( newSd ) ;
    close( serverSd ) ;
    cout << "********Session********" << endl ;
    cout << "Bytes written: " << bytesWritten << " Bytes read: " << bytesRead << endl ;
    cout << "Elapsed time: " << ( end1.tv_sec - start1.tv_sec )
         << " secs" << endl ;
    cout << "Connection closed..." << endl ;

    return 0 ;
}
