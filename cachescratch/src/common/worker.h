// This class just holds arguments to each thread.
class workerArg {
public:

  workerArg() {}

  workerArg (char * obj, int objSize, int repetitions, int iterations)
    : _object (obj),
      _objSize (objSize),
      _iterations (iterations),
      _repetitions (repetitions)
  {}

  char * _object;
  int _objSize;
  int _iterations;
  int _repetitions;
};

extern "C" void * worker (void * arg)
{
  // free the object we were given.
  // Then, repeatedly do the following:
  //   malloc a given-sized object,
  //   repeatedly write on it,
  //   then free it.
  workerArg * w = (workerArg *) arg;
  delete w->_object;
  workerArg w1 = *w;
  for (int i = 0; i < w1._iterations; i++) {
    // Allocate the object.
    char * obj = new char[w1._objSize];
    // Write into it a bunch of times.
    for (int j = 0; j < w1._repetitions; j++) {
      for (int k = 0; k < w1._objSize; k++) {
	obj[k] = (char) k;
	volatile char ch = obj[k];
	ch++;
      }
    }
    // Free the object.
    delete [] obj;
  }
  return NULL;
}
