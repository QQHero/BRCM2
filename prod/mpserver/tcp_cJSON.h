/****************************************************************************
 * @author: Jordan396 <https://github.com/Jordan396/tcp_cJSON>              *
 *                                                                          *
 *   You should have received a copy of the MIT License when cloning this   *
 *   repository. If not, see <https://opensource.org/licenses/MIT>.         *
 ****************************************************************************/

/**
  * @file tcp_cJSON.h
  * @author Jordan396
  * @date 18 April 2019
  * @brief Documentation for functions in tcp_cJSON.h.
  *
  * This header file has been created to describe the functions and declare constants.
  * 
  * Credits to the creators of the cJSON library <https://github.com/DaveGamble/cJSON>.
  * 
  * Code is documented according to GNOME and Doxygen standards.
  * <https://developer.gnome.org/programming-guidelines/stable/c-coding-style.html.en>
  * <http://www.doxygen.nl/manual/docblocks.html>
  */

/* Connections */
#define MAX_PENDING 5   /* Maximum outstanding connection requests */
#define MAX_CONC_CONN 5 /* Maximum number of concurrent connections */
#define RCV_BUF_SIZE 32 /* Size of receive buffer */
#define MAX_RESP_SIZE 500

/* Standard libraries */
#define _GNU_SOURCE
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>     /* for sigaction() */
#include <ctype.h>      /* for char validation */
#include <time.h>       /* for waitFor() */
#include <sys/mman.h>   /* to create shared memory across child processes */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <sys/wait.h>   /* for waitpid() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */

/* External libraries */
#include "./cJSON.h"

/**
 * @brief Accepts a cJSON object and sends its string representation over a socket.
 *
 * This function converts a cJSON object to its string representation.
 * It then sends this string to the receiver.
 * 
 * This payload adopts the following structure:
 * The first RCV_BUF_SIZE bytes indicates the size of the actual payload.
 * The remaining bytes contain the actual cJSON string representation payload.
 *
 * @param sock Client socket assigned to the connection.
 * @param jobjToSend cJSON object to be sent.
 * @return int 0 if error occurred, 1 otherwise.
 */
int send_payload(int sock, cJSON *jobjToSend);

/**
 * @brief Receives a send_payload formatted response and saves it to objReceived.
 *
 * The socket listens for a send_payload formatted response.
 * It then saves the response to an objReceived string.
 * 
 * This reponse adopts the following structure:
 * The first RCV_BUF_SIZE bytes indicates the size of the actual payload.
 * The remaining bytes contain the actual cJSON string representation payload.
 *
 * @param sock Client socket assigned to the connection.
 * @param objReceived String to save the response recieved.
 * @return void
 */
void receive_response(int sock, char *objReceived);

/**
 * @brief Prints error message and closes the connection and program.
 *
 * @param errorMessage Error message to be printed.
 * @return void
 */
void die_with_error(char *errorMessage);

/**
 * @brief Prints error message but maintains the connection.
 *
 * @param errorMessage Error message to be printed.
 * @return int 0
 */
int persist_with_error(char *errorMessage);

/**
 * @brief Waits for secs amount of seconds.
 *
 * @param secs Number of seconds to wait for.
 * @return void
 */
void wait_for(unsigned int secs);
