/*****************************************************************************
  *
  * (C) Copyright Broadcom Corporation 2015
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  *
  * You may obtain a copy of the License at
  * http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "broadview.h"
#include "rest.h"
#include "rest_http.h"

/******************************************************************
 * @brief  sends a HTTP 200 message to the client 
 *
 * @param[in]   fd    socket for sending message
 *
 * @retval   BVIEW_STATUS_SUCCESS 
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_200_with_data(int fd, char *buffer, int length)
{
    char *response = "HTTP/1.1 200 OK \r\n"
            "Server: BroadViewAgent (Unix) (Linux) \r\n"
            "Content-Type: text/json \r\n\r\n";
    int bytes_sent =0;
    BVIEW_STATUS  rv = BVIEW_STATUS_SUCCESS;
    int sendbuff =0;

    if (0 > send(fd, response, strlen(response), MSG_MORE))
    {
      rv = BVIEW_STATUS_FAILURE;
    }
    bytes_sent = send(fd, buffer, length, MSG_MORE);
    if (0 > bytes_sent)
    {
      rv = BVIEW_STATUS_FAILURE;
    }
   
 
    while (length - bytes_sent > 0)
    {
       /* Calling continues send with buffer size more than 8K will result in ERROR from
        * kernel so increase kernel buffer size for this FD.
        */
       sendbuff = length;
       setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff));

       buffer = buffer + bytes_sent;
       length = length - bytes_sent;
       bytes_sent = send(fd, buffer, length, 0);
       if (0 > bytes_sent)
       {
         return BVIEW_STATUS_FAILURE;
       }
    }
    return rv;
}

/******************************************************************
 * @brief  sends a HTTP 200 message to the client 
 *
 * @param[in]   fd    socket for sending message
 *
 * @retval   BVIEW_STATUS_SUCCESS 
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_200(int fd)
{
    char *response = "HTTP/1.1 200 OK \r\n"
            "Server: BroadViewAgent (Unix) (Linux) \r\n\r\n";

    if (0 > send(fd, response, strlen(response), 0))
        return BVIEW_STATUS_FAILURE;

    return BVIEW_STATUS_SUCCESS;
}

/******************************************************************
 * @brief  sends a HTTP 404 message to the client 
 *
 * @param[in]   fd    socket for sending message
 *
 * @retval   BVIEW_STATUS_SUCCESS 
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_404(int fd)
{
    char *response = "HTTP/1.1 404 Not Found \r\n"
            "Server: BroadViewAgent (Unix) (Linux) \r\n\r\n "
            "<html> <body> Unsupported </body> </html>";

    if (0 > send(fd, response, strlen(response), 0))
        return BVIEW_STATUS_FAILURE;

    return BVIEW_STATUS_SUCCESS;
}

/******************************************************************
 * @brief  sends a HTTP 400 message to the client 
 *
 * @param[in]   fd    socket for sending message
 *
 * @retval   BVIEW_STATUS_SUCCESS 
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_400(int fd)
{
    char *response = "HTTP/1.1 400 Bad Request \r\n"
            "Server: BroadViewAgent (Unix) (Linux) \r\n\r\n"
            "<html> <body> Bad Request </body> </html>";

   if (0 > send(fd, response, strlen(response), 0))
        return BVIEW_STATUS_FAILURE;

    return BVIEW_STATUS_SUCCESS;
}

/******************************************************************
 * @brief  sends a HTTP 500 message to the client 
 *
 * @param[in]   fd    socket for sending message
 *
 * @retval   BVIEW_STATUS_SUCCESS 
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_500(int fd)
{
    char *response = "HTTP/1.1 500 Internal Server Error \r\n"
            "Server: BroadViewAgent (Unix) (Linux) \r\n\r\n"
            "<html> <body> Internal Server Error </body> </html>";

   if (0 > send(fd, response, strlen(response), 0))
        return BVIEW_STATUS_FAILURE;

    return BVIEW_STATUS_SUCCESS;
}

/******************************************************************
 * @brief  sends an asynchronous report to the client 
 *
 * @param[in]   rest    context for reading configuration
 * @param[in]   buffer  Buffer containing data to be sent
 * @param[in]   length  number of bytes to be sent 
 * 
 * @retval   BVIEW_STATUS_SUCCESS if send is successful
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_async_report(REST_CONTEXT_t *rest, char *buffer, int length)
{
    char *header = "POST /agent_response HTTP/1.1\\r\\n"
            "Host: BVIEW Client\\r\\n"
            "User-Agent: BroadViewAgent\\r\\n"
            "Accept: text/html,application/xhtml+xml,application/xml\\r\\n"
            "Content-Length: %d\\r\\n"
            "\\r\\n";

    char buf[REST_MAX_HTTP_BUFFER_LENGTH] = { 0 };
    int clientFd;
    struct sockaddr_in clientAddr;
    int temp = 0;
    BVIEW_STATUS rv = BVIEW_STATUS_SUCCESS;

    snprintf(buf, REST_MAX_HTTP_BUFFER_LENGTH - 1, header, length);

    /* create socket to send data to */
    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    _REST_ASSERT_NET_ERROR((clientFd != -1), "Error Creating server socket");

    /* setup the socket */
    memset(&clientAddr, 0, sizeof (struct sockaddr_in));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(rest->config.clientPort);

    temp = inet_pton(AF_INET, &rest->config.clientIp[0], &clientAddr.sin_addr);
    _REST_ASSERT_NET_SOCKET_ERROR((temp > 0), "Error Creating server socket",clientFd);

    /* connect to the peer */
    temp = connect(clientFd, (struct sockaddr *) &clientAddr, sizeof (clientAddr));
    _REST_ASSERT_NET_SOCKET_ERROR((temp != -1), "Error connecting to client for sending async reports",clientFd);

    /* send data */
    if (0 > send(clientFd, buf, strlen(buf),MSG_MORE))
      rv = BVIEW_STATUS_FAILURE;

    if (0 > send(clientFd, buffer, length, 0))
      rv = BVIEW_STATUS_FAILURE;
    
    /* close down session */
    close(clientFd);

    return rv;

}

/******************************************************************
 * @brief  sends a HTTP 404 message to the client 
 *
 * @param[in]   fd    socket for sending message
 * @param[in]   buffer  Buffer containing data to be sent
 * @param[in]   length  number of bytes to be sent 
 *
 * @retval   BVIEW_STATUS_SUCCESS if send is successful
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_404_with_data(int fd, char *buffer, int length)
{
    char *response = "HTTP/1.1 404 Not Found \r\n"
            "Server: BroadViewAgent (Unix) (Linux) \r\n\r\n "
             "Content-Type: text/json";

    if (0 > send(fd, response, strlen(response), MSG_MORE))
      return BVIEW_STATUS_FAILURE;

    if (0 > send(fd, buffer, length, 0))
      return BVIEW_STATUS_FAILURE;

    return BVIEW_STATUS_SUCCESS;
}

/******************************************************************
 * @brief  sends a HTTP 400 message to the client 
 *
 * @param[in]   fd    socket for sending message
 * @param[in]   buffer  Buffer containing data to be sent
 * @param[in]   length  number of bytes to be sent 
 *
 * @retval   BVIEW_STATUS_SUCCESS if send is successful
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_400_with_data(int fd, char *buffer, int length)
{
    char *response = "HTTP/1.1 400 Bad Request \r\n"
            "Server: BroadViewAgent (Unix) (Linux) \r\n\r\n"
             "Content-Type: text/json";

    if (0 > send(fd, response, strlen(response), MSG_MORE))
      return BVIEW_STATUS_FAILURE;

    if (0 > send(fd, buffer, length, 0))
      return BVIEW_STATUS_FAILURE;

    return BVIEW_STATUS_SUCCESS;
}

/******************************************************************
 * @brief  sends a HTTP 500 message to the client 
 *
 * @param[in]   fd    socket for sending message
 * @param[in]   buffer  Buffer containing data to be sent
 * @param[in]   length  number of bytes to be sent 
 *
 * @retval   BVIEW_STATUS_SUCCESS if send is successful
 * 
 * @note     
 *********************************************************************/
BVIEW_STATUS rest_send_500_with_data(int fd, char *buffer, int length)
{
    char *response = "HTTP/1.1 500 Internal Server Error \r\n"
            "Server: BroadViewAgent (Unix) (Linux) \r\n\r\n"
             "Content-Type: text/json";

    if (0 > send(fd, response, strlen(response), MSG_MORE))
      return BVIEW_STATUS_FAILURE;

    if (0 > send(fd, buffer, length, 0))
      return BVIEW_STATUS_FAILURE;

    return BVIEW_STATUS_SUCCESS;
}


