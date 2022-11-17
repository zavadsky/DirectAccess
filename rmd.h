/*This file consists of declarations required
for generating the Reverse Multi-Delimiter codes and encoding*/

#ifndef RMD_H_
# define RMD_H_

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <math.h>
#include <cstring>
#include "constants.h"

using namespace std;
using namespace constants;

namespace rmd_space
{
uint ranks[L];

int cur_byte(0),cur_bit(7);

int nlk[Lmax+8][33]; // array used in decoding
int nl[Lmax]; // nl[i] = number of codewords of length i

uint rmd[7000000]; // Codewords of RMD-code
char* Dict_rmd[3000000]; //Dictionary of words by descending frequency

map<string,int> rmd_map,rmd_map_sorted;
multimap<int,string> freq_rmd;

double entropy;

}

void gen_reverse();
uint* read_ranks(string);
int encode_rmd_rev(int,uint*);
#endif

