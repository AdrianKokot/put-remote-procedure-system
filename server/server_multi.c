#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 512
#define BACKLOG_SIZE 5
#define SERVER_PORT 1235

int main(int argc, char **argv)
{
  fd_set readfds, writefds, nextReadFds, getScriptsListFds, getScriptResultFds;
  struct timeval timeout;
  socklen_t clientSocketLength;
  struct sockaddr_in serverAddress, clientAddress;

  char eofs = '\n';

  int serverFd = socket(PF_INET, SOCK_STREAM, 0),
      clientFd,
      on = 1,
      maxFd = serverFd,
      fdCount;

  char buffer[BUFFER_SIZE];
  char errorMessage[BUFFER_SIZE] = "\e[31mERROR\e[0m";

  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

  serverAddress.sin_family = PF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(SERVER_PORT);

  int isPortAlreadyTaken = -1 == bind(serverFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

  if (isPortAlreadyTaken)
  {
    printf("Couldn't open socket.\n");
    return EXIT_FAILURE;
  }

  listen(serverFd, BACKLOG_SIZE);
  // 150.254.32.67

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&nextReadFds);

  FD_ZERO(&getScriptsListFds);
  FD_ZERO(&getScriptResultFds);
  maxFd = serverFd;

  printf("Start\n");

  while (1)
  {
    readfds = nextReadFds;
    FD_SET(serverFd, &readfds);
    timeout.tv_sec = 5 * 60;
    timeout.tv_usec = 0;

    fdCount = select(maxFd + 1, &readfds, &writefds, (fd_set *)0, &timeout);

    if (fdCount == 0)
    {
      printf("\e[34m[TIMEOUT]\e[0m\n");
      continue;
    }

    if (FD_ISSET(serverFd, &readfds))
    {
      fdCount -= 1;
      clientSocketLength = sizeof(clientAddress);

      clientFd = accept(serverFd, (struct sockaddr *)&clientAddress, &clientSocketLength);

      FD_SET(clientFd, &nextReadFds);

      printf("\e[32m[CONNECTED]\e[0m: %s\t\e[32m[FD]\e[0m: %d\n", inet_ntoa((struct in_addr)clientAddress.sin_addr), clientFd);

      if (clientFd > maxFd)
        maxFd = clientFd;
    }

    for (int i = serverFd + 1; i <= maxFd && fdCount > 0; i++)
    {
      if (FD_ISSET(i, &writefds))
      {
        fdCount -= 1;

        if (FD_ISSET(i, &getScriptsListFds))
        {
          int len = 0;

          int fd[2];
          pipe(fd);
          pid_t pid = fork();

          if (pid == 0)
          {
            close(fd[0]);
            dup2(fd[1], 1);
            char *command = "ls";
            char *execArgs[] = {command, "./scripts", "-1", NULL};
            execvp(command, execArgs);
            close(fd[1]);
          }
          else
          {
            close(fd[1]);
            dup2(fd[0], 0);

            int status, num_bytes;
            waitpid(pid, &status, 0);

            char *output_str = malloc(sizeof(char) * 5120);

            while ((num_bytes = read(fd[0], output_str, 5120)) > 0)
            {
              len += num_bytes;
            }

            close(fd[0]);
            int sentCount = 0;
            do
            {
              int temp = write(i, output_str, strlen(output_str));
              sentCount += temp;
              output_str += temp;
            } while (sentCount < len);
          }

          // if(pid == 0) {
          //   dup2(i, 1);
          //   char *command = "ls";
          //   char *args[] = {command, "*.py", "-1", NULL};
          //   execvp(command, args);
          //   exit(1);
          // } else {
          //   int status, num_bytes;
          //   waitpid(pid, &status, 0);
          // }

          // int sentCount = 0;
          // do {
          //     sentCount = write(i, r, strlen(r));
          //     r += sentCount;
          // } while(sentCount < len);

          FD_CLR(i, &getScriptsListFds);
        }
        else if (FD_ISSET(i, &getScriptResultFds))
        {
          char *r = "Wiktor Szymanski\n";
          int len = strlen(r);

          int sentCount = 0;
          do
          {
            int temp = write(i, r + sentCount, strlen(r));
            r += temp;
            sentCount += temp;
          } while (sentCount < len);

          FD_CLR(i, &getScriptResultFds);
        }
        else
        {
          write(i, errorMessage, BUFFER_SIZE);
        }

        close(i);

        FD_CLR(i, &writefds);

        if (i == maxFd)
          while (maxFd > clientFd && !FD_ISSET(maxFd, &nextReadFds) && !FD_ISSET(maxFd, &writefds))
            maxFd--;
      }
      else if (FD_ISSET(i, &readfds))
      {
        fdCount -= 1;
        FD_CLR(i, &nextReadFds);

        int readCount = 0;
        do
        {
          int temp = read(i, buffer + readCount, 1);
          readCount += temp;
        } while (buffer[readCount - 1] != eofs);

        int dataSize = atoi(buffer);
        char *data = malloc(sizeof(char) * dataSize + 1);
        data[dataSize] = '\0';
        int dataReadCount = 0;
        do
        {
          int temp = read(i, data + dataReadCount, dataSize);
          dataReadCount += temp;
        } while (dataReadCount < dataSize);

        printf("\e[32m[FD]\e[0m: %d\t\e[32m[MESSAGE]\e[0m: %s\n", i, data);

        if (strncmp(data, "GET_SCRIPTS", 11) == 0)
        {
          FD_SET(i, &getScriptsListFds);
        }
        // else if (strncmp(buffer, "148084", 6) == 0)
        // {
        //     FD_SET(i, &getScriptResultFds);
        // }

        FD_SET(i, &writefds);
        FD_CLR(i, &readfds);
      }
    }
  }

  close(serverFd);

  return EXIT_SUCCESS;
}
