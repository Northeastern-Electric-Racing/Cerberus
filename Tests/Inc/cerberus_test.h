#ifndef CERBERUS_TEST_H
#define CERBERUS_TEST_H

/* 
 *  ************** NOTE **************
 *  These test files are specifically 
 *  for unit tests, developers can 
 *  make sure that specific functions 
 *  and APIs meet requirements set out. 
 *  This should NOT be used for actually 
 *  simulating drivers and hardware, 
 *  that is what we are using Renode for
 */

void test_can_handler(void);

#endif // CERBERUS_TEST_H