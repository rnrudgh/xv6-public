#include "types.h"
#include "user.h"
#include "stat.h"

void periodic();
void func();

int
main (int argc, char *argv[])
{
  volatile int i;
  printf(1,"alarmtest starting \n");
  alarm(10, periodic);

  for (i = 0; i < 100* 500000; i ++) {
    if ( (i % 250000) == 0)
      write(2, ".", 1);
  }
  exit();
}

void
periodic()
{
  printf(1,"alarm!\n");
}
