#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<time.h>

#include<mpi.h>

int ierr, procid, numprocs;
int *nums, *receive_num, *send_num, *tmp;

const int TOTAL_NUM = 1;
const int MAX_NUM = 100;

int comp(const void *elem1, const void* elem2) {
  int f = *((int*)elem1);
  int s = *((int*)elem2);
  if (f > s) return 1;
  if (f < s) return -1;
  return 0;
}

bool _check_numprocs() {
  int count = 0, n = numprocs;
  while (n) {
    if (n & 1) ++count;
    n >>= 1;
  }
  return count == 1;
}

void swap(int *a , int *b) {
  int tmp = *a;
  *a = *b;
  *b = tmp;
}

void _bitonic(int n, int m) {

  if (m < 2) return;
  int pos = procid % m, group = procid / m, half = m / 2;
	int pairid;
  if (n > m)  pairid = (procid < group * m + half) ? procid + half : procid - half;
  else pairid = (group+1)*m - 1 - pos;

  MPI_Status status;
  int i, j, k;

  if (procid < group * m + half) {
    //received
    ierr = MPI_Recv(receive_num, TOTAL_NUM, MPI_INT, pairid, 0, MPI_COMM_WORLD, &status);
    j = 0; //index of nums
    k = 0; //index of receive_num
    for (i = 0; i < TOTAL_NUM; ++i) {
      if (receive_num[k] < nums[j]) {
        tmp[i] = receive_num[k];
        ++k;
      } else {
        tmp[i] = nums[j];
        ++j;
      }
    }

    for (i = 0; i < TOTAL_NUM; ++i) {
      if (j == TOTAL_NUM || (k < TOTAL_NUM && receive_num[k] < nums[j])) {
        send_num[i] = receive_num[k];
        ++k;
      } else {
        send_num[i] = nums[j];
        ++j;
      }
    }
    ierr = MPI_Send(send_num, TOTAL_NUM, MPI_INT, pairid, 0, MPI_COMM_WORLD);

    for (i = 0; i < TOTAL_NUM; ++i) {
      nums[i] = tmp[i];
    }
  } else {
    //send
    ierr = MPI_Send(nums, TOTAL_NUM, MPI_INT, pairid, 0, MPI_COMM_WORLD);
    ierr = MPI_Recv(receive_num, TOTAL_NUM, MPI_INT, pairid, 0, MPI_COMM_WORLD, &status);
    for (i = 0; i < TOTAL_NUM; ++i) {
      nums[i] = receive_num[i];
    }

  }

  _bitonic(n, m/2);
}

int main(int argc, char* argv[]) {

  ierr = MPI_Init(&argc, &argv);
  ierr = MPI_Comm_rank(MPI_COMM_WORLD, &procid);
  ierr = MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

  if (!_check_numprocs()) {
    if (procid == 0) printf("Num of PEs should be power of 2.\n");
    return -1;
  }

  srand(time(NULL) + procid);

  nums = (int*) malloc(sizeof(int)*TOTAL_NUM);
  int i;
  for (i = 0; i < TOTAL_NUM; ++i) nums[i] = rand() % MAX_NUM;

  qsort(nums, TOTAL_NUM, sizeof(*nums), comp);
  //sort local data
  
  receive_num = (int*) malloc(sizeof(int)*TOTAL_NUM);
  send_num = (int*) malloc(sizeof(int)*TOTAL_NUM);
  tmp = (int*) malloc(sizeof(int)*TOTAL_NUM);

  for (i = 2; i <= numprocs; i <<= 1) {
     _bitonic(i, i);
  }
  
  free(nums);
  free(receive_num);
  free(send_num);
  free(tmp);

  return 0;
}
