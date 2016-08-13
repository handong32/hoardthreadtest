// This class just holds arguments to each thread.
class workerArg {
public:
  workerArg() {}
  workerArg (int objSize, int repetitions, int iterations)
    : _objSize (objSize),
      _iterations (iterations),
      _repetitions (repetitions)
  {}

  int _objSize;
  int _iterations;
  int _repetitions;
};


#if defined(_WIN32)
extern "C" void worker (void * arg)
#else
extern "C" void * worker (void * arg)
#endif
{
  // Repeatedly do the following:
  //   malloc a given-sized object,
  //   repeatedly write on it,
  //   then free it.
  workerArg * w = (workerArg *) arg;
  workerArg w1 = *w;
  for (int i = 0; i < w1._iterations; i++) {
    // Allocate the object.
    char * obj = new char[w1._objSize];
    //    printf ("obj = %p\n", obj);
    // Write into it a bunch of times.
    for (int j = 0; j < w1._repetitions; j++) {
      for (int k = 0; k < w1._objSize; k++) {
#if 0
	volatile double d = 1.0;
	d = d * d + d * d;
#else
	obj[k] = (char) k;
	volatile char ch = obj[k];
	ch++;
#endif
      }
    }
    // Free the object.
    delete [] obj;
  }
#if !defined(_WIN32)
  return NULL;
#endif
}
