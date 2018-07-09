#include "types.h"
#include "user.h"

#define STDIO 1
int
main(int argc, char *argv[])
{
  int fd = open("README",2);

  if(fd < 0 ) {
    printf(STDIO, "ERROR file did not open\n");
    exit();
  }
  dup2(fd, STDIO);

  printf(STDIO, "qoooooooooooooo\n");
  exit();
  
}
