#include <stdio.h>

#define MAX 9

int main()
{
  int i,j;

  printf("    |");
  for(i=1;i<=MAX;i++) {
    printf(" %3d",i);
  }
  printf("\n----+");

  for(i=1;i<=MAX;i++) {
    printf("----");
  }
  printf("\n");

  for(i=1;i<=MAX;i++) {
    printf("%3d |",i);
    for(j=1;j<=MAX;j++) {
      printf(" %3d",i*j);
    }
    printf("\n");
  }

  return 0;
}
