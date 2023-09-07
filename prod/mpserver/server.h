/****************************************************************************
 * @author: Jordan396 <https://github.com/Jordan396/trivial-twitter-v2>     *
 *                                                                          *
 * This file was heavily adapted from the source code in:                   *
 *   "TCP/IP Sockets in C: Practical Guide for Programmers"                 *
 *   by Michael J. Donahoo and Kenneth L. Calvert                           *
 *   <http://cs.baylor.edu/~donahoo/practical/CSockets/textcode.html>       *
 *                                                                          *
 *   You should have received a copy of the MIT License when cloning this   *
 *   repository. If not, see <https://opensource.org/licenses/MIT>.         *
 ****************************************************************************/
/**
  * @file server.h
  * @author Jordan396
  * @date 13 April 2019
  * @brief Header file for server.c.
  * 
  * Code is documented according to GNOME and Doxygen standards.
  * <https://developer.gnome.org/programming-guidelines/stable/c-coding-style.html.en>
  * <http://www.doxygen.nl/manual/docblocks.html>
  */

#include "tcp_cJSON.h"
#include "mp_util.h"

/**
 * @brief Creates a cJSON object and sends it to the server.
 *
 * @param clntSocket client socket.
 * @return void
 */
void handle_tcp_client(int clntSocket);

const char* get_localtime_str();