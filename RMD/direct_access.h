/* This file consists of declarations of data structures required
for direct access to elements of integer sequence */

#ifndef DA_H_
# define DA_H_

#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <bitset>
#include <vector>
#include <map>
#include <fstream>
#include <sys/timeb.h>
//#include <windows.h>
#include <functional>
#include <random>
#include "constants.h"

using namespace std;
using namespace constants;

namespace direct_access
{

#define DELTA_ab(l1n,l3n) (((L2delta_bit[l1n][l3n>>2])>>((l3n&3)<<1))&3)

//================ Direct access structures ===========
const int Nwrd=104857601; // Maximal dictionary size and encoded text length
const int Tsize=7,Tmask=(1<<Tsize)-1,pow2size=1<<Tsize,L1_bit=14,L1_size=1<<L1_bit,L1_mask=L1_size-1,L1_count=Nwrd/L1_size+1,L2_bit=6,L2_size=1<<L2_bit,L2_size2=L2_size>>1,L2_mask=L2_size-1,L2_count=L1_size/L2_size+1;
int L1[L1_count],L2ranks[L1_count][L2_count],range[L1_count];
uchar L1_delta[L1_count],L2delta[L1_count][L2_count],L2delta_bit[L1_count][(L2_count>>2)+1];
int L1_ent[L1_count];
uchar* L2_packed[L1_count];
unsigned int A1[L];
uchar n12[4096]; // Array for sequential processing dozens of bits of an RMD-bitstream

//============== RMD-code deocoding ===================
struct tabsrmd1 {  // Element of fast decoding table
	int p1,p2,p3,p4,n;
	struct tabsrmd1* al;
};

const int decode_sz=(int)(state_max*(Lmax-1)/Tsize+1)*pow2size;
uchar PTR[decode_sz],RMD_N[decode_sz];
uint32_t OUT_[decode_sz];
}

//number of codewords (n) and their starting bit positions inside the byte
struct rmd_s {
    uchar n,b[3];
};

extern struct ai_state a_step24infty(ai_state,int);

#endif // DACCESS_H_

