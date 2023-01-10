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
#define MAX_CONNECTIONS 1000
#define connection_header_eof '\n'
#define script_result_BUFFER_SIZE 5120

struct Connection
{
  int fd;
  char *command;
};

/**
 * Prints the project name and server configuration at STDOUT
 */
void print_start_message();

/**
 * Sends content of the message to file with given file descriptor
 * Response is formatted accordingly to communication protocol
 */
void send_response(int file_descriptor, char *message);

/**
 * Reads content of file with given file descriptor
 * Expects content formatted accordingly to communication protocol
 */
char *read_request(int file_descriptor);

/**
 * Gets list of available python scripts from 'scripts' directory
 */
char *get_available_scripts_list();

/**
 * Executes python script with given name and arguments
 * Expects string formatted 'file.py arg1 arg2'
 */
char *get_script_result(char *command);

/**
 * Gets index of connections where item is assigned to given file descriptor
 */
int get_connection_id(int file_descriptor, struct Connection *connections);

int main(int argc, char **argv)
{
  struct Connection connections[MAX_CONNECTIONS];

  print_start_message();

  fd_set read_fds, write_fds, next_read_fds;

  struct timeval timeout;
  socklen_t client_socket_length;
  struct sockaddr_in server_address, client_address;

  int server_fd = socket(PF_INET, SOCK_STREAM, 0);
  int client_fd, fd_count, on = 1, max_fd = server_fd;

  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

  server_address.sin_family = PF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(SERVER_PORT);

  int isPortAlreadyTaken = -1 == bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));

  if (isPortAlreadyTaken)
  {
    printf("[ERROR]: Couldn't open socket.\n");
    return EXIT_FAILURE;
  }

  listen(server_fd, BACKLOG_SIZE);

  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
  FD_ZERO(&next_read_fds);

  max_fd = server_fd;

  while (1)
  {
    read_fds = next_read_fds;
    FD_SET(server_fd, &read_fds);
    timeout.tv_sec = 5 * 60;
    timeout.tv_usec = 0;

    fd_count = select(max_fd + 1, &read_fds, &write_fds, (fd_set *)0, &timeout);

    if (fd_count == 0)
    {
      printf("[TIMEOUT]\n");
      continue;
    }

    if (FD_ISSET(server_fd, &read_fds))
    {
      fd_count -= 1;
      client_socket_length = sizeof(client_address);

      client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_socket_length);

      FD_SET(client_fd, &next_read_fds);

      printf("[FD]: %d\t[CONNECTED]: %s\n", client_fd, inet_ntoa((struct in_addr)client_address.sin_addr));

      for (int i = 0; i < MAX_CONNECTIONS; i++)
      {
        if (connections[i].fd == 0)
        {
          connections[i].fd = client_fd;
          break;
        }
      }

      if (client_fd > max_fd)
      {
        max_fd = client_fd;
      }
    }

    for (int current_fd = server_fd + 1; current_fd <= max_fd && fd_count > 0; current_fd++)
    {
      if (FD_ISSET(current_fd, &write_fds))
      {
        fd_count -= 1;

        int connectionId = get_connection_id(current_fd, connections);

        if (connectionId == -1)
        {
          printf("[FD]: %d\t[ERROR]: Not found connection in connections table\n\n", current_fd);
          send_response(current_fd, "Internal server error\n");
        }
        else
        {
          if (strncmp(connections[connectionId].command, "GET_SCRIPTS", 11) == 0)
          {
            send_response(current_fd, get_available_scripts_list());
          }
          else if (strncmp(connections[connectionId].command, "EXEC ", 5) == 0)
          {
            send_response(current_fd, get_script_result(connections[connectionId].command + 5));
          }
          else
          {
            send_response(current_fd, "Command doesn't exist\n");
          }

          connections[connectionId].fd = 0;
          free(connections[connectionId].command);
        }

        close(current_fd);

        FD_CLR(current_fd, &write_fds);

        if (current_fd == max_fd)
        {
          while (max_fd > client_fd && !FD_ISSET(max_fd, &next_read_fds) && !FD_ISSET(max_fd, &write_fds))
          {
            max_fd--;
          }
        }
      }
      else if (FD_ISSET(current_fd, &read_fds))
      {
        fd_count -= 1;
        FD_CLR(current_fd, &next_read_fds);

        int connectionId = get_connection_id(current_fd, connections);

        if (connectionId != -1)
        {
          connections[connectionId].command = read_request(current_fd);
        }

        FD_SET(current_fd, &write_fds);
        FD_CLR(current_fd, &read_fds);
      }
    }
  }

  close(server_fd);

  return EXIT_SUCCESS;
}

char *read_request(int fd)
{
  char buffer[BUFFER_SIZE];

  int readCount = 0;
  do
  {
    int temp = read(fd, buffer + readCount, 1);
    readCount += temp;
  } while (buffer[readCount - 1] != connection_header_eof);

  int dataSize = atoi(buffer);
  char *data = malloc(sizeof(char) * dataSize + 1);
  data[dataSize] = '\0';
  int dataReadCount = 0;
  do
  {
    int temp = read(fd, data + dataReadCount, dataSize);
    dataReadCount += temp;
  } while (dataReadCount < dataSize);

  printf("[FD]: %d\t[REQUEST_MESSAGE]: %s\n", fd, data);

  return data;
}

void send_response(int fd, char *message)
{
  int messageSize = strlen(message);

  int bodySize = snprintf(NULL, 0, "%d\n%s", messageSize, message);
  char *str = malloc(sizeof(char) * (bodySize + 1));
  snprintf(str, bodySize + 1, "%d\n%s", messageSize, message);

  printf("[FD]: %d\t[RESPONSE_MESSAGE]: %s\n", fd, str);

  int sentCount = 0;
  do
  {
    int temp = write(fd, str, strlen(str));
    sentCount += temp;
    str += temp;
  } while (sentCount < bodySize);
}

char *get_script_result(char *command)
{
  int fd[2];

  pipe(fd);

  pid_t pid = fork();

  if (pid == 0)
  {
    char script_path[BUFFER_SIZE] = "./scripts/";

    close(fd[0]);
    dup2(fd[1], 1);
    dup2(fd[1], STDERR_FILENO);

    char *exec_args[BUFFER_SIZE] = {NULL};
    exec_args[0] = "python3";

    char *token = strtok(command, " ");

    if (token != NULL)
    {
      strcat(script_path, token);
    }
    else
    {
      strcat(script_path, command);
    }

    exec_args[1] = script_path;

    for (int i = 2; i < BUFFER_SIZE && token != NULL; i++)
    {
      token = strtok(NULL, " ");
      if (token != NULL)
      {
        exec_args[i] = malloc(sizeof(char) * strlen(token));
        strcpy(exec_args[i], token);
      }
    }

    execvp(exec_args[0], exec_args);

    close(fd[1]);

    return "";
  }
  else
  {
    close(fd[1]);
    dup2(fd[0], 0);

    int status;
    waitpid(pid, &status, 0);

    char *output_str = malloc(sizeof(char) * script_result_BUFFER_SIZE);

    read(fd[0], output_str, script_result_BUFFER_SIZE);
    close(fd[0]);

    if (strstr(output_str, "Errno 2") != NULL)
    {
      return strstr(output_str, "[Errno 2] ") + 10;
    }

    return output_str;
  }
}

char *get_available_scripts_list()
{
  int fd[2];

  pipe(fd);

  pid_t pid = fork();

  if (pid == 0)
  {
    close(fd[0]);
    dup2(fd[1], 1);
    dup2(fd[1], STDERR_FILENO);

    char *command = "ls";
    char *exec_args[] = {command, "./scripts", "-1", NULL};

    execvp(command, exec_args);
    close(fd[1]);

    return "";
  }
  else
  {
    close(fd[1]);
    dup2(fd[0], 0);

    int status;
    waitpid(pid, &status, 0);

    char *output_str = malloc(sizeof(char) * script_result_BUFFER_SIZE);

    read(fd[0], output_str, script_result_BUFFER_SIZE);
    close(fd[0]);

    return output_str;
  }
}

int get_connection_id(int fd, struct Connection *connections)
{
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (connections[i].fd == fd)
    {
      return i;
    }
  }

  return -1;
}

void print_start_message()
{
  printf("===================================================================================================================================================\n");
  printf("                                 _                                                  _                                                              \n");
  printf("   _ __   ___  _ __ ___    ___  | |_   ___      _ __   _ __   ___    ___   ___   __| | _   _  _ __   ___      ___   ___  _ __ __   __  ___  _ __   \n");
  printf("  | '__| / _ \\| '_ ` _ \\  / _ \\ | __| / _ \\    | '_ \\ | '__| / _ \\  / __| / _ \\ / _` || | | || '__| / _ \\    / __| / _ \\| '__|\\ \\ / / / _ \\| '__|  \n");
  printf("  | |   |  __/| | | | | || (_) || |_ |  __/    | |_) || |   | (_) || (__ |  __/| (_| || |_| || |   |  __/    \\__ \\|  __/| |    \\ V / |  __/| |     \n");
  printf("  |_|    \\___||_| |_| |_| \\___/  \\__| \\___|    | .__/ |_|    \\___/  \\___| \\___| \\__,_| \\__,_||_|    \\___|    |___/ \\___||_|     \\_/   \\___||_|     \n");
  printf("                                               |_|                                                                                                 \n");
  printf("                                                                                                                                                   \n");
  printf("===================================================================================================================================================\n");
  printf("Configuration:\n");
  printf("\tMessage buffer size:\t\t%d\n", BUFFER_SIZE);
  printf("\tQueue size:\t\t\t%d\n", BACKLOG_SIZE);
  printf("\tMax connections:\t\t%d\n", MAX_CONNECTIONS);
  printf("\tScript result buffer size:\t%d\n", script_result_BUFFER_SIZE);
  printf("\tServer port:\t\t\t%d\n", SERVER_PORT);
  printf("===================================================================================================================================================\n");
}