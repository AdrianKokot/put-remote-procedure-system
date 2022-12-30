#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>

#define BUFFER_SIZE 256

char *python_executable = "python3";
char *script_file = "./test-script.py";

int main()
{
  char *args[] = {python_executable, script_file, NULL};

  int fd[2];
  pipe(fd);

  pid_t pid = fork();

  if (pid == 0)
  {
    close(fd[0]);
    dup2(fd[1], 1);
    execvp(python_executable, args);
    close(fd[1]);
  }
  else
  {
    close(fd[1]);
    dup2(fd[0], 0);

    int bytesSum = 0;

    int status, num_bytes;
    waitpid(pid, &status, 0);

    char output_str[BUFFER_SIZE];

    while ((num_bytes = read(fd[0], output_str, BUFFER_SIZE)) > 0)
    {
      bytesSum += num_bytes;
      printf("%s", output_str);
    }

    printf("Read bytes: %d\n", bytesSum);

    close(fd[0]);
  }

  return 0;
}