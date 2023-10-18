#ifndef REGEXP_H
#define REGEXP_H
/* 
 * return value:
 *        -1        error
 *        0        not match
 *        1        matched
 */
int match(char *reg, char *text);

#endif
