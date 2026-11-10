// stdbool.h -- C99's bool, for c4lc, which has no _Bool: a word
#ifndef C4DUKE_STDBOOL_H
#define C4DUKE_STDBOOL_H 1
#define bool  int
#define true  1
#define false 0
#define __bool_true_false_are_defined 1
#endif
