
#ifndef TYPEDEFS_H
#define TYPEDEFS_H
/*
 * Default Typedefs
 */


typedef unsigned char	uchar;



typedef unsigned short	ushort;



typedef unsigned int	uint;



typedef unsigned long	ulong;


/* define [u]int8/16/32/64, uintptr */


typedef unsigned char	uint8;



typedef unsigned short	uint16;



typedef unsigned int	uint32;



typedef unsigned long long uint64;



typedef unsigned int	uintptr;



typedef signed char	int8;



typedef signed short	int16;



typedef signed int	int32;

typedef unsigned int	bool;


typedef signed long long int64;

#define INLINE inline



/* define macro values */

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1  /* TRUE */
#endif

#ifndef NULL
#define	NULL	0
#endif

#ifndef OFF
#define	OFF	0
#endif

#ifndef ON
#define	ON	1  /* ON = 1 */
#endif

#define	AUTO	(-1) /* Auto = -1 */

#endif
