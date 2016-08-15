#include <assert.h>
#include <stdio.h>
#include <time.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"


#if defined(_WIN32)
#define __WIN32__
#endif

#ifdef __WIN32__
#include  <windows.h>
#include  <conio.h>
#include  <process.h>

#else
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>

#ifndef __SVR4
//extern "C" int pthread_setconcurrency (int) throw();
#include <pthread.h>
#endif


typedef void * LPVOID;
typedef long long LONGLONG;
typedef long DWORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } foo;
  LONGLONG QuadPart;    // In Visual C++, a typedef to _ _int64} LARGE_INTEGER;
} LARGE_INTEGER;
typedef long long _int64;
#ifndef TRUE
enum { TRUE = 1, FALSE = 0 };
#endif
#include <assert.h>
#define _ASSERTE(x) assert(x)
#define _inline inline

void sleepcp(int milliseconds) // cross-platform sleep function
{
    clock_t time_end;
    time_end = clock() + milliseconds * CLOCKS_PER_SEC/1000;
    while (clock() < time_end)
    {
    }
}

void Sleep (long x) 
{
  //  printf ("sleeping for %ld seconds.\n", x/1000);
  //sleep(x/1000);
    //std::this_thread::sleep_for(std::chrono::seconds(x/1000));
    //boost::this_thread::sleep( boost::posix_time::seconds(x/1000) );
    sleepcp(x);
}

void QueryPerformanceCounter (long * x)
{
    //struct timezone tz;
  struct timeval tv;
  gettimeofday (&tv, NULL);
  *x = tv.tv_sec * 1000000L + tv.tv_usec;
}

void QueryPerformanceFrequency(long * x)
{
  *x = 1000000L;
}


#include  <stdio.h>
#include  <stdlib.h>
#include  <stddef.h>
#include  <string.h>
#include  <ctype.h>
#include  <time.h>
#include  <assert.h>

#define _REENTRANT 1
#include <pthread.h>

typedef void * VoidFunction (void *);
void _beginthread (VoidFunction x, int, void * z)
{
  pthread_t pt;
  pthread_attr_t pa;
  pthread_attr_init (&pa);

#if 1//defined(__SVR4)
  pthread_attr_setscope (&pa, PTHREAD_SCOPE_SYSTEM); /* bound behavior */
#endif

  //  printf ("creating a thread.\n");
  int v = pthread_create(&pt, &pa, x, z);
  //  printf ("v = %d\n", v);
}
#endif


#if 0
static char buf[65536];

#define malloc(v) &buf
#define free(p) 
#endif

#undef CPP

/* Test driver for memory allocators           */
/* Author: Paul Larson, palarson@microsoft.com */
#define MAX_THREADS     100
#define MAX_BLOCKS  20000000

int volatile  stopflag=FALSE ;       

struct lran2_st {
  long x, y, v[97];
};

int     TotalAllocs=0 ;

typedef struct thr_data {

  int    threadno ;
  int    NumBlocks ;
  int    seed ;

  int    min_size ;
  int    max_size ;

  char * *array ;
  int    *blksize ;
  int     asize ;

  unsigned long    cAllocs ;
  unsigned long    cFrees ;
  int    cThreads ;
  unsigned long    cBytesAlloced ;

  volatile int finished ;
  struct lran2_st rgen ;

} thread_data;

void runthreads(long sleep_cnt, int min_threads, int max_threads, 
		int chperthread, int num_rounds) ;
void runloops(long sleep_cnt, int num_chunks ) ;
static void warmup(char **blkp, int num_chunks );
static void * exercise_heap( void *pinput) ;
static void lran2_init(struct lran2_st* d, long seed) ;
static long lran2(struct lran2_st* d) ;
ULONG CountReservedSpace() ;
 
char **          blkp = new char *[MAX_BLOCKS] ;
int *           blksize = new int[MAX_BLOCKS] ;
long            seqlock=0 ;
struct lran2_st rgen ;
int             min_size=10, max_size=500 ;
int             num_threads ;
ULONG           init_space ;

extern  int   cLockSleeps ;
extern  int   cAllocedChunks ;
extern  int   cAllocedSpace ;
extern  int   cUsedSpace ;
extern  int   cFreeChunks ;
extern  int   cFreeSpace ;

int cChecked=0 ;

int main (int argc, char *argv[])
{
  int          min_threads, max_threads ;
  int          num_rounds ;
  int          chperthread ;
  unsigned     seed=12345 ;
  int          num_chunks=10000;
  long sleep_cnt;

  if (argc > 7) {
    sleep_cnt = atoi(argv[1]);
    min_size = atoi(argv[2]);
    max_size = atoi(argv[3]);
    chperthread = atoi(argv[4]);
    num_rounds = atoi(argv[5]);
    seed = atoi(argv[6]);
    max_threads = atoi(argv[7]);
    min_threads = max_threads;
    printf ("sleep = %ld, min = %d, max = %d, per thread = %d, num rounds = %d, seed = %d, max_threads = %d, min_threads = %d\n",
	    sleep_cnt, min_size, max_size, chperthread, num_rounds, seed, max_threads, min_threads);
    goto DoneWithInput;
  }

#if defined(_MT) || defined(_REENTRANT)
  //#ifdef _MT
  printf( "\nMulti-threaded test driver \n") ;
#else
  printf( "\nSingle-threaded test driver \n") ;
#endif
#ifdef CPP
  printf("C++ version (new and delete)\n") ;
#else
  printf("C version (malloc and free)\n") ;
#endif
  printf("runtime (sec): ") ;
  scanf ("%ld", &sleep_cnt);

  printf("chunk size (min,max): ") ;
  scanf("%d %d", &min_size, &max_size ) ;
#if defined(_MT) || defined(_REENTRANT)
  //#ifdef _MT
  printf("threads (min, max):   ") ; 
  scanf("%d %d", &min_threads, &max_threads) ;
  printf("chunks/thread:  ") ; scanf("%d", &chperthread ) ;
  printf("no of rounds:   ") ; scanf("%d", &num_rounds ) ;
  num_chunks = max_threads*chperthread ;
#else 
  printf("no of chunks:  ") ; scanf("%d", &num_chunks ) ;
#endif
  printf("random seed:    ") ; scanf("%d", &seed) ;

 DoneWithInput:

  if( num_chunks > MAX_BLOCKS ){
    printf("Max %d chunks - exiting\n", MAX_BLOCKS ) ;
    return(1) ;
  }

#ifndef __WIN32__
#ifdef __SVR4
  pthread_setconcurrency (max_threads);
#endif
#endif

  lran2_init(&rgen, seed) ;

  runthreads(sleep_cnt, min_threads, max_threads, chperthread, num_rounds) ;


#ifdef _DEBUG
  _cputs("Hit any key to exit...") ;	(void)_getch() ;
#endif

  return 0;

} /* main */

void runthreads(long sleep_cnt, int min_threads, int max_threads, int chperthread, int num_rounds)
{
  thread_data *de_area = new thread_data[max_threads] ;
  thread_data *pdea;
  int           nperthread ;
  int           sum_threads ;
  unsigned long  sum_allocs ;
  unsigned long  sum_frees ;
  double        duration ;
  long ticks_per_sec ;
  long start_cnt, end_cnt ;
  _int64        ticks ;
  double        rate_1=0, rate_n ;
  double        reqd_space ;
  ULONG         used_space ;
  int           prevthreads ;
  int           i ;

  QueryPerformanceFrequency( &ticks_per_sec ) ;

  pdea = &de_area[0] ;
  memset(&de_area[0], 0, sizeof(thread_data)) ;

  prevthreads = 0 ;
  for(num_threads=min_threads; num_threads <= max_threads; num_threads++ )
    {

      warmup(&blkp[prevthreads*chperthread], (num_threads-prevthreads)*chperthread );

      nperthread = chperthread ;
      stopflag   = FALSE ;
		
      for(i=0; i< num_threads; i++){
	de_area[i].threadno    = i+1 ;
	de_area[i].NumBlocks   = num_rounds*nperthread;
	de_area[i].array       = &blkp[i*nperthread] ;
	de_area[i].blksize     = &blksize[i*nperthread] ;
	de_area[i].asize       = nperthread ;
	de_area[i].min_size    = min_size ;
	de_area[i].max_size    = max_size ;
	de_area[i].seed        = lran2(&rgen) ; ;
	de_area[i].finished    = 0 ;
	de_area[i].cAllocs     = 0 ;
	de_area[i].cFrees      = 0 ;
	de_area[i].cThreads    = 0 ;
	de_area[i].finished    = FALSE ;
	lran2_init(&de_area[i].rgen, de_area[i].seed) ;

	_beginthread(exercise_heap, 0, &de_area[i]) ;  

      }

      QueryPerformanceCounter( &start_cnt) ;

      // printf ("Sleeping for %ld seconds.\n", sleep_cnt);
      Sleep(sleep_cnt * 1000L) ;

      stopflag = TRUE ;

      for(i=0; i<num_threads; i++){
	while( !de_area[i].finished ){
		sched_yield();
	}
      }


      QueryPerformanceCounter( &end_cnt) ;

      sum_frees = sum_allocs =0  ;
      sum_threads = 0 ;
      for(i=0;i< num_threads; i++){
	sum_allocs    += de_area[i].cAllocs ;
	sum_frees     += de_area[i].cFrees ;
	sum_threads   += de_area[i].cThreads ;
	de_area[i].cAllocs = de_area[i].cFrees = 0;
      }

      ticks = end_cnt - start_cnt ;
      duration = (double)ticks/ticks_per_sec ;


      for( i=0; i<num_threads; i++){
	if( !de_area[i].finished )
	  printf("Thread at %d not finished\n", i) ;
      }


      rate_n = sum_allocs/duration ;
      if( rate_1 == 0){
	rate_1 = rate_n ;
      }
		
      reqd_space = (0.5*(min_size+max_size)*num_threads*chperthread) ;
      // used_space = CountReservedSpace() - init_space;
      used_space = 0;
      
      printf ("Throughput = %8.0f operations per second.\n", sum_allocs / duration);

      Sleep(5000L) ; // wait 5 sec for old threads to die

      prevthreads = num_threads ;

      printf ("Done sleeping...\n");

    }
  delete [] de_area;
}


static void * exercise_heap( void *pinput)
{
  thread_data  *pdea;
  int           cblks=0 ;
  int           victim ;
  long          blk_size ;
  int           range ;

  if( stopflag ) return 0;

  pdea = (thread_data *)pinput ;
  pdea->finished = FALSE ;
  pdea->cThreads++ ;
  range = pdea->max_size - pdea->min_size ;

  /* allocate NumBlocks chunks of random size */
  for( cblks=0; cblks<pdea->NumBlocks; cblks++){
    victim = lran2(&pdea->rgen)%pdea->asize ;
#ifdef CPP
    delete pdea->array[victim] ;
#else
    free(pdea->array[victim]) ;
#endif
    pdea->cFrees++ ;

    if (range == 0) {
      blk_size = pdea->min_size;
    } else {
      blk_size = pdea->min_size+lran2(&pdea->rgen)%range ;
    }
#ifdef CPP
    pdea->array[victim] = new char[blk_size] ;
#else
    pdea->array[victim] = (char *) malloc(blk_size) ;
#endif

    pdea->blksize[victim] = blk_size ;
    assert(pdea->array[victim] != NULL) ;

    pdea->cAllocs++ ;

		/* Write something! */

		volatile char * chptr = ((char *) pdea->array[victim]);
		*chptr++ = 'a';
		volatile char ch = *((char *) pdea->array[victim]);
		*chptr = 'b';

    
		if( stopflag ) break ;
  }

  //  	printf("Thread %u terminating: %d allocs, %d frees\n",
  //		      pdea->threadno, pdea->cAllocs, pdea->cFrees) ;
  pdea->finished = TRUE ;

  if( !stopflag ){
      //printf("start another thread\n");
      exercise_heap((void*)pdea);
      //_beginthread(exercise_heap, 0, pdea) ;
  } else {
    printf ("thread stopping.\n");
  }
  pthread_exit (NULL);
  return 0;
}

static void warmup(char **blkp, int num_chunks )
{
  int     cblks ;
  int     victim ;
  int     blk_size ;
  LPVOID  tmp ;


  for( cblks=0; cblks<num_chunks; cblks++){
    if (min_size == max_size) {
      blk_size = min_size;
    } else {
      blk_size = min_size+lran2(&rgen)%(max_size-min_size) ;
    }
#ifdef CPP
    blkp[cblks] = new char[blk_size] ;
#else
    blkp[cblks] = (char *) malloc(blk_size) ;
#endif
    blksize[cblks] = blk_size ;
    assert(blkp[cblks] != NULL) ;
  }

  /* generate a random permutation of the chunks */
  for( cblks=num_chunks; cblks > 0 ; cblks--){
    victim = lran2(&rgen)%cblks ;
    tmp = blkp[victim] ;
    blkp[victim]  = blkp[cblks-1] ;
    blkp[cblks-1] = (char *) tmp ;
  }

  for( cblks=0; cblks<4*num_chunks; cblks++){
    victim = lran2(&rgen)%num_chunks ;
#ifdef CPP
    delete blkp[victim] ;
#else
    free(blkp[victim]) ;
#endif

    if (max_size == min_size) {
      blk_size = min_size;
    } else {
      blk_size = min_size+lran2(&rgen)%(max_size - min_size) ;
    }
#ifdef CPP
    blkp[victim] = new char[blk_size] ;
#else
    blkp[victim] = (char *) malloc(blk_size) ;
#endif
    blksize[victim] = blk_size ;
    assert(blkp[victim] != NULL) ;
  }
} 

// =======================================================

/* lran2.h
 * by Wolfram Gloger 1996.
 *
 * A small, portable pseudo-random number generator.
 */

#ifndef _LRAN2_H
#define _LRAN2_H

#define LRAN2_MAX 714025l /* constants for portable */
#define IA	  1366l	  /* random number generator */
#define IC	  150889l /* (see e.g. `Numerical Recipes') */

//struct lran2_st {
//    long x, y, v[97];
//};

static void
lran2_init(struct lran2_st* d, long seed)
{
  long x;
  int j;

  x = (IC - seed) % LRAN2_MAX;
  if(x < 0) x = -x;
  for(j=0; j<97; j++) {
    x = (IA*x + IC) % LRAN2_MAX;
    d->v[j] = x;
  }
  d->x = (IA*x + IC) % LRAN2_MAX;
  d->y = d->x;
}

static 
long lran2(struct lran2_st* d)
{
  int j = (d->y % 97);

  d->y = d->v[j];
  d->x = (IA*d->x + IC) % LRAN2_MAX;
  d->v[j] = d->x;
  return d->y;
}

#undef IA
#undef IC

#endif
#pragma GCC diagnostic pop

