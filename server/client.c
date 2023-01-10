#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void request(char *addres, int port, char *index)
{
  int fd = socket(PF_INET, SOCK_STREAM, 0);

  struct hostent *addrent;
  struct sockaddr_in addr;

  addrent = gethostbyname(addres);

  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  memcpy(&addr.sin_addr.s_addr, addrent->h_addr, addrent->h_length);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != -1)
  {
    char *msg = malloc(sizeof(char) * 512);

    int msgSize = sprintf(msg, "%ld\n%s", strlen(index), index);

    int sentCount = 0;
    do
    {
      int temp = write(fd, msg, msgSize);
      msg += temp;
      sentCount += temp;
    } while (sentCount < msgSize);

    int readCount = 0;
    char buffer[512];
    do
    {
      int temp = read(fd, buffer + readCount, 1);
      readCount += temp;
    } while (buffer[readCount - 1] != '\n');

    int dataSize = atoi(buffer);
    char *data = malloc(sizeof(char) * dataSize + 1);
    data[dataSize] = '\0';
    int dataReadCount = 0;
    do
    {
      int temp = read(fd, data + dataReadCount, dataSize);
      dataReadCount += temp;
    } while (dataReadCount < dataSize);

    printf("%s\n", data);
  }
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    printf("Brak agrumentow: DOMENA, PORT i NUMER INDEXU\n");
    return 0;
  }

  // while (1)
  // {
  request(argv[1], atoi(argv[2]), argv[3]);
  // }

  return 0;
}