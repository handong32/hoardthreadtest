void * 
run_test (void * arg)
{
  register unsigned int i;
  register unsigned long request_size = size;
  register uint64_t total_iterations = iteration_count;
  int tid = *((int *) arg);
  struct timeval start, end, null, elapsed, adjusted;

#ifdef __EBBRT_BM__
  bar->Wait();
#else
  pthread_barrier_wait (&barrier);
#endif

#if 0
  /* Time a null loop.  We'll subtract this from the final malloc loop
     results to get a more accurate value. */

  gettimeofday (&start, NULL);

  for (i = 0; i < total_iterations; i++)
    {
      register void *buf;
      buf = dummy (i);
      buf = dummy (i);
    }

  gettimeofday (&end, NULL);
  null.tv_sec = end.tv_sec - start.tv_sec;
  null.tv_usec = end.tv_usec - start.tv_usec;
  if (null.tv_usec < 0)
    {
      null.tv_sec--;
      null.tv_usec += USECSPERSEC;
    }
#else
  null.tv_sec = 0;
  null.tv_usec = 0;
#endif

  /* Run the real malloc test */ 
  gettimeofday (&start, NULL);

  {
    void ** buf = (void **) malloc(sizeof(void *) * total_iterations);
    
    for (i = 0; i < total_iterations; i++)
      {
	buf[i] = malloc (request_size);
      }
    
    for (i = 0; i < total_iterations; i++)
      {
	free (buf[i]);
      }
  }

  gettimeofday (&end, NULL);
  elapsed.tv_sec = end.tv_sec - start.tv_sec;
  elapsed.tv_usec = end.tv_usec - start.tv_usec;
  if (elapsed.tv_usec < 0)
    {
      elapsed.tv_sec--;
      elapsed.tv_usec += USECSPERSEC;
    }

  /*          * Adjust elapsed time by null loop time          */
  adjusted.tv_sec = elapsed.tv_sec - null.tv_sec;
  adjusted.tv_usec = elapsed.tv_usec - null.tv_usec;
  if (adjusted.tv_usec < 0)
    {
      adjusted.tv_sec--;
      adjusted.tv_usec += USECSPERSEC;
    }

#ifdef __EBBRT_BM__
  bar->Wait();
#else
  pthread_barrier_wait (&barrier);
#endif
  
  unsigned int pt = tid;
  executionTime[pt % thread_count] = adjusted.tv_sec + adjusted.tv_usec / 1000000.0;
  //  printf ("Thread %u adjusted timing: %d.%06d seconds for %d requests" " of %d bytes.\n", pt, adjusted.tv_sec, adjusted.tv_usec, total_iterations, request_size);

  return NULL;
}

void *
dummy (unsigned i)
{
  return NULL;
}
