#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(print()){
    printf(1, "fail\n");
    exit();
  }
  exit();
}
