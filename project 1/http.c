/**
* COMP30023 Computer Systems 2019
* Project 1: Image Tagger
*
* Created by Ziqi Jia on 25/04/19.
* Copyright © 2019 Ziqi Jia. All rights reserved.
*
* References:
* http_request
* 	comp30023 lab6 solution
*
*/

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dict.h"
#include "http.h"

// constants
static char const * const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";
static char const * const HTTP_200_FORMAT_COOKIE = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\
Set-Cookie: username=%s\r\n\r\n";
static char const * const HTTP_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_400_LENGTH = 47;
static char const * const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_404_LENGTH = 45;


// represents the types of method
typedef enum
{
    GET,
    POST,
    UNKNOWN
} METHOD;


/* Make a copy of a string, strip it with all \n and \r, and return the copy */
char *strip_copy(const char *s)
{
    char *p = malloc(strlen(s) + 1);
    if (p)
    {
        char *p2 = p;
        while(*s != '\0')
        {
            if (*s != '\n' && *s != '\r')
            {
                *p2++ = *s++;
            }
            else
            {
                ++s;
            }
        }
        *p2 = '\0';
    }
    return p;
}

/* Handle the http request that is sent from the client, store the client input into the dictionary and send the appropriate html files back to client */
bool handle_http_request(int sockfd, Dict dict)
{

  // try to read the request
  char buff[2049];
  int n = read(sockfd, buff, 2049);
  if (n <= 0)
  {
      if (n < 0)
          perror("read");
      else
          printf("socket %d close the connection\n", sockfd);
      return false;
  }

  //terminate the string
  buff[n] = 0;

  char * curr = buff;

  // parse the method
  METHOD method = UNKNOWN;
  if (strncmp(curr, "GET ", 4) == 0)
  {
      curr += 4;
      method = GET;
  }
  else if (strncmp(curr, "POST ", 5) == 0)
  {
      curr += 5;
      method = POST;
  }
  else if (write(sockfd, HTTP_400, HTTP_400_LENGTH) < 0)
  {
      perror("write");
      return false;
  }

  // sanitise the URI
  while (*curr == '.' || *curr == '/')
      ++curr;
  // assume the only valid request URI is "/" but it can be modified to accept more files
  if (*curr == ' '||*curr == '?')
  {
      // create cookie to store the username, and set the initial http page as 1_intro.html
      char * cookie = strstr(buff, "Cookie: ");
      char * return_ht = "1_intro.html";
      //if the method is GET
      if(method == GET)
      {
        // check whether the game has cookie
        if(cookie)
        {
          cookie = strtok((char *)strdup(strstr(buff, "Cookie: ") + 8), "\r\n");
          // find username
          char * username = strip_copy(strstr(cookie, "username=") + 9);
          // if it has cookie and send start
          if (strstr(buff, "Start"))
          {
              // insert the username into the dictionary as the key, and set the initial value as " ", means start the game
              Dictionary_Insert(dict, username, " ");
              return_ht = "3_first_turn.html";
          }
          // if it has cookie and send null
          else
          {
              //return the start page
              return_ht = "2_start.html";
              char * showtext = malloc(50);
              //insert username in start page
              sprintf(showtext, "<p>%s</p>\r\n", username);
              int username_length = strlen(showtext);
              long added_length = username_length;
              // get the size of the file
              struct stat st;
              stat(return_ht, &st);
              // increase file size to accommodate the username
              long size = st.st_size + added_length;
              n = sprintf(buff, HTTP_200_FORMAT, size);
              // send the header first
              if (write(sockfd, buff, n) < 0)
              {
                  perror("write");
                  return false;
              }
              // read the content of the HTML file
              int filefd = open(return_ht, O_RDONLY);
              n = read(filefd, buff, 2048);
              if (n < 0)
              {
                  perror("read");
                  close(filefd);
                  return false;
              }
              close(filefd);

              // find the start of the form
              char *startform = strstr(buff, "<form") ;
              int pos = startform - buff;
              // move the trailing part backward
              int p1, p2;
              for (p1 = size - 1, p2 = p1 - added_length; p2 >= pos; --p1, --p2)
                  buff[p1] = buff[p2];
              ++p2;
              // copy the username
              strncpy(buff + p2, showtext, username_length);
              if (write(sockfd, buff, size) < 0)
              {
                  perror("write");
                  return false;
              }
              free(username);
              free(showtext);
              free(cookie);
              return true;
          }
        }
        else
        {
          // if no cookie return intro page
          return_ht = "1_intro.html";
        }
      }
      // if the method is POST
      else if(method == POST)
      {
        // check whether the game has has a player
        if(cookie)
        {
            char * cookie = strtok((char*)strdup(strstr(buff, "Cookie: ") + 8), "\r\n");
            char * username = strip_copy(strstr(cookie, "username=") + 9);
            //check if buff have quit, return gameover, and clear the Dictionary
            if (strstr(buff, "Quit"))
            {
                return_ht = "7_gameover.html";
                Dictionary_Clear(dict);
            }
            // else if buff have guess, check the size of dictionary
            else if(strstr(buff, "Guess"))
            {
                // if the size is 0 means no players, quit the game
                if (Dictionary_Size(dict) == 0)
                {
                  return_ht = "7_gameover.html";
                }
                // if the size <2, compare the value in dicionary, if it is " "(initial value), means someone guess right and delete the value, so return 6_endgame.html, else means just one player, another havn't start, so return 5_discarded.html.
                else if (Dictionary_Size(dict) < 2)
                {
                  if (strcmp(Dictionary_Search(dict, username), " "))
                  {
                      return_ht = "6_endgame.html";
                      Dictionary_Delete(dict, username);
                  }
                  else
                      return_ht = "5_discarded.html";
                }
                // if the size is >2, two people play
                else
                {
                  // a flag to represent if the game is end
                  int endflag = 0;
                  // insert new guess
                  char * guess_post = strstr(buff, "keyword=")+8;
                  // if insert nothing, just return true
                  if (*guess_post == '&')
                  {
                      return true;
                  }
                  //take out keywords in guess
                  char * guess = strtok((char *)strip_copy(strstr(buff, "keyword=") + 8), "&");
                  char * showtext = malloc(50);
                  char * new_guess = malloc(50);
                  // if not first time to guess, take out exist keywords and add new word to become a new guess, insert the new guess into html
                  //remove
                  //printf("find: %s\n", Dictionary_Search(dict, username));
                  if (strcmp(Dictionary_Search(dict, username), " "))
                  {
                      sprintf(new_guess, "%s;%s", Dictionary_Search(dict, username), guess);
                      sprintf(showtext, "<p>%s</p>\r\n", new_guess);
                      Dictionary_Insert(dict, username, new_guess);
                  }
                  // if first time to guess, show the keywords, insert keywords into html
                  else
                  {
                      sprintf(showtext, "<p>%s</p>\r\n", guess);
                      Dictionary_Insert(dict, username, guess);
                  }

                  // compare with other player, check if the guess same as the other
                  int count = 0;
                  struct dNode **items = Dictionary_AllItems(dict, &count);
                  for (int i = 0; i<count; i++)
                  {
                    if (strcmp(items[i]->key, username))
                    {
                      if(items[i]->value)
                      {
                        char * value = strdup(items[i]->value);
                        //remove
                        //printf("other value: %s\n",value);
                        char * value_i = strtok(value, ";");
                        while (value_i)
                        {
                            // if have same word, engflag = 1
                            if (!strcmp(value_i, guess))
                            {
                                endflag = 1;
                                break;
                            }
                            value_i = strtok(NULL, ";");
                        }
                        free(value);
                      }
                    }
                  }
                  free(items);
                  // if endflag = 1, return 6_endgame.html, delete the key and values, set the dictionary size back to 1
                  if (endflag)
                  {
                    return_ht = "6_endgame.html";
                    Dictionary_Delete(dict, username);
                  }
                  // else if endflag not equal to zero, return 4_accepted.html, write keyswards in html
                  else
                  {
                    return_ht = "4_accepted.html";
                    int username_length = strlen(showtext);
                    long added_length = username_length;
                    // get the size of the file
                    struct stat st;
                    stat(return_ht, &st);
                    // increase file size to accommodate the username
                    long size = st.st_size + added_length;
                    n = sprintf(buff, HTTP_200_FORMAT, size);
                    // send the header first
                    if (write(sockfd, buff, n) < 0)
                    {
                        perror("write");
                        return false;
                    }
                    // read the content of the HTML file
                    int filefd = open(return_ht, O_RDONLY);
                    n = read(filefd, buff, 2048);
                    if (n < 0)
                    {
                        perror("read");
                        close(filefd);
                        return false;
                    }
                    close(filefd);
                    // find the start of the form
                    char *startform = strstr(buff, "<form") ;
                    int pos = startform - buff;
                    // move the trailing part backward
                    int p1, p2;
                    for (p1 = size - 1, p2 = p1 - added_length; p2 >= pos; --p1, --p2)
                        buff[p1] = buff[p2];
                    ++p2;
                    // copy the username
                    strncpy(buff + p2, showtext, username_length);
                    if (write(sockfd, buff, size) < 0)
                    {
                        perror("write");
                        return false;
                    }
                    free(showtext);
                    free(guess);
                    free(new_guess);
                    return true;
                  }
                }
            }
            free(cookie);
            free(username);
          }
          // if no user, post username, write into html
          else
          {
            if (strstr(buff, "user="))
            {
              char * username = strip_copy(strstr(buff, "user=") + 5);
              return_ht = "2_start.html";
              char * showtext = malloc(50);
              sprintf(showtext, "<p>%s</p>\r\n", username);
              int username_length = strlen(showtext);
              long added_length = username_length;
              // get the size of the file
              struct stat st;
              stat(return_ht, &st);
              // increase file size to accommodate the username
              long size = st.st_size + added_length;
              n = sprintf(buff, HTTP_200_FORMAT_COOKIE, size, username);
              // send the header first
              if (write(sockfd, buff, n) < 0)
              {
                  perror("write");
                  return false;
              }
              // read the content of the HTML file
              int filefd = open(return_ht, O_RDONLY);
              n = read(filefd, buff, 2048);
              if (n < 0)
              {
                  perror("read");
                  close(filefd);
                  return false;
              }
              close(filefd);
              // find the start of the form
              char *startform = strstr(buff, "<form") ;
              int pos = startform - buff;
              // move the trailing part backward
              int p1, p2;
              for (p1 = size - 1, p2 = p1 - added_length; p2 >= pos; --p1, --p2)
                  buff[p1] = buff[p2];
              ++p2;
              // copy the username
              strncpy(buff + p2, showtext, username_length);
              if (write(sockfd, buff, size) < 0)
              {
                  perror("write");
                  return false;
              }
              free(username);
              free(showtext);
              return true;
            }
          }
      }
      else
          // never used, just for completeness
          fprintf(stderr, "no other methods supported");

      // show the return html pages
      // get the size of the file
      struct stat st;
      stat(return_ht, &st);
      //remove
      //printf("return_ht: %s/n",return_ht);
      n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
      // send the header first
      if (write(sockfd, buff, n) < 0)
      {
          perror("write");
          return false;
      }
      // send the file
      int filefd = open(return_ht, O_RDONLY);
      do
      {
          n = sendfile(sockfd, filefd, NULL, 2048);
      }
      while (n > 0);
      if (n < 0)
      {
          perror("sendfile");
          close(filefd);
          return false;
      }
      close(filefd);
    }
    //send 404
    else if (write(sockfd, HTTP_404, HTTP_404_LENGTH) < 0)
    {
        perror("write");
        return false;
    }

    return true;
}
