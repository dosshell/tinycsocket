#ifndef TCS_MOCK_H_
#define TCS_MOCK_H_

extern "C" {

extern int MOCK_ALLOC_COUNTER;
extern int MOCK_FREE_COUNTER;
extern int MOCK_ALLOC_FAIL_AFTER;
static const int MOCK_FAIL_OFF = -1;
}
#endif
