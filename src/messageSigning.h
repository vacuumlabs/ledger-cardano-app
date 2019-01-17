#include "keyDerivation.h"
#include <os.h>

void getTxWitness(path_spec_t* pathSpec,
                  const uint8_t* txHashBuffer, size_t txHashSize,
                  uint8_t* outBuffer, size_t outSize);
