/*
 * myfunc.c
 *
 *  Created on: Jun 2, 2016
 *      Author: dad
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif	// HAVE_CONFIG_H
#ifdef WINDOZE
#include <intrin.h>
#endif  // WINDOZE
typedef int make_iso_compilers_happy;	// required to prevent ISO warning when SSESUPPORT not defined
#ifdef SSESUPPORT
unsigned int myCpuInfo()
{
	unsigned int mydata = 0;


	   unsigned int index = 1;
	   unsigned int index2 = 0;
	   unsigned int regs[4];
	//    int sum;
#ifdef WINDOZE
	   __cpuid(regs, index);
#else	// WINDOZE
	    __asm__ __volatile__(
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
	        "pushq %%rbx     \n\t" /* save %rbx */
#else
	        "pushl %%ebx     \n\t" /* save %ebx */
#endif
	        "cpuid            \n\t"
	        "movl %%ebx ,%[ebx]  \n\t" /* write the result into output var */
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
	        "popq %%rbx \n\t"
#else
	        "popl %%ebx \n\t"
#endif
	        : "=a"(regs[0]), [ebx] "=r"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
	        : "a"(index), "c"(index2));
#endif	// WINDOZE

	    if ( ((regs[2]>>19)&0x1) && ((regs[2]>>20)&0x1) ) {
	    	mydata++;
	    } else {
	    	return (mydata);
	    }
	    if ( (regs[2]>>28)&0x1 ) {
	    	mydata++;
	    } else {
	    	return (mydata);
	    }

		index = 7;
#ifdef WINDOZE
		__cpuid(regs, index);
#else	// WINDOZE
	    __asm__ __volatile__(
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
	        "pushq %%rbx     \n\t" /* save %rbx */
#else
	        "pushl %%ebx     \n\t" /* save %ebx */
#endif
	        "cpuid            \n\t"
	        "movl %%ebx ,%[ebx]  \n\t" /* write the result into output var */
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
	        "popq %%rbx \n\t"
#else
	        "popl %%ebx \n\t"
#endif
	        : "=a"(regs[0]), [ebx] "=r"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
	        : "a"(index), "c"(index2));
#endif	// WINDOZE
	    if ( (regs[1]>>5)&0x1 ) mydata++;

	return (mydata);
}
#endif	// SSESUPPORT

