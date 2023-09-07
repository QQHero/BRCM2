/****************************************************************************
 * @author: Jordan396 <https://github.com/Jordan396/tcp_cJSON>              *
 *                                                                          *
 *   You should have received a copy of the MIT License when cloning this   *
 *   repository. If not, see <https://opensource.org/licenses/MIT>.         *
 ****************************************************************************/

/**
  * @file tcp_cJSON.c
  * @author Jordan396
  * @date 18 April 2019
  * @brief Source code for functions in tcp_cJSON.h.
  *
  * The functions below are designed to facilitate transmission of JSON data over TCP.
  * 
  * Both sending and receiving parties must follow the same protocol, which is:
  *   1. The sender first formats data as a cJSON string.
  *   2. The sender then calls send_payload.
  *   3. send_payload:
  *     a. Sends the size of the payload within the first RCV_BUF_SIZE bytes.
  *     b. It then sends the actual payload.
  *   4. The receiver uses (3a) to determine when the sender has finished sending data.
  * 
  * Credits to the creators of the cJSON library <https://github.com/DaveGamble/cJSON>.
  * 
  * Code is documented according to GNOME and Doxygen standards.
  * <https://developer.gnome.org/programming-guidelines/stable/c-coding-style.html.en>
  * <http://www.doxygen.nl/manual/docblocks.html>
  */

#include "tcp_cJSON.h"

int send_payload(int sock, cJSON *jobjToSend);
void receive_response(int sock, char *objReceived);
void die_with_error(char *errorMessage);
int persist_with_error(char *errorMessage);
void wait_for(unsigned int secs);

/** \copydoc send_payload */
int send_payload(int sock, cJSON *jobjToSend)
{
  char buffer[RCV_BUF_SIZE];
  char *request = cJSON_PrintUnformatted(jobjToSend);
  int requestSize = strlen(request) + 1;

  sprintf(buffer, "%d", requestSize);
  if (send(sock, buffer, RCV_BUF_SIZE, 0) != RCV_BUF_SIZE) {
    cJSON_free(request);
    return persist_with_error("Block size: send() sent a different number of bytes than expected.\n");
  }
    
  if (send(sock, request, requestSize, 0) != requestSize) {
    cJSON_free(request);
    return persist_with_error("Block contents: send() sent a different number of bytes than expected.\n");
  }

  cJSON_free(request);  
  return 1;
}

/** \copydoc receive_response */
void receive_response(int sock, char *objReceived)
{
  int bytesToRecv = 0;
  int responseIdx = 0;
  char buffer[RCV_BUF_SIZE];    /* Buffer for ttweet string */
  char response[MAX_RESP_SIZE]; /* Stores the entire response */

  while (bytesToRecv <= 0)
  {
    recv(sock, buffer, RCV_BUF_SIZE, 0);
    bytesToRecv = atoi(buffer);
    wait_for(3);
  }

  while (bytesToRecv > 0)
  {
    if (bytesToRecv > RCV_BUF_SIZE)
    {
      recv(sock, buffer, RCV_BUF_SIZE, 0);
      strncpy(response + responseIdx, buffer, RCV_BUF_SIZE);
      responseIdx += RCV_BUF_SIZE;
    }
    else
    {
      recv(sock, buffer, bytesToRecv, 0);
      strncpy(response + responseIdx, buffer, bytesToRecv);
      responseIdx += bytesToRecv;
    }
    bytesToRecv -= RCV_BUF_SIZE;
  }
  strncpy(objReceived, response, sizeof(response));
}

/** \copydoc die_with_error */
void die_with_error(char *errorMessage)
{
  perror(errorMessage);
  exit(1);
}

/** \copydoc persist_with_error */
int persist_with_error(char *errorMessage)
{
  perror(errorMessage);
  return 0;
}

/** \copydoc waitFor */
void wait_for(unsigned int secs)
{
  unsigned int retTime = time(0) + secs; // Get finishing time.
  while (time(0) < retTime)
    ; // Loop until it arrives.
}
