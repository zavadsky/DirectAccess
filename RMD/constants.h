#ifndef CONSTANTS_H
#define CONSTANTS_H

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;

// define your own namespace to hold constants
namespace constants
{
const int L=120000000;
inline unsigned char codes[L];

const int Lmax=32,max_rmd=3900000; // maximum codeword length and maximum number of codewords

//Parameters of RMD-code
inline const int k[20]={0,1,3,30}; //numbers which are not delimiters, the last = biggest delimiter+1
                          // Parameters m1,m2,... of a code are all integers not present in the array k
const int kmax=4,Lmin=3; //number of elements in the array k; length of the shortest codeword
const int state_max=5; //number of states in the decoding automaton

struct ai_state { // State of the decoding automaton
    int st,L,p,res;
};

inline int Nwords,diff_words; // Total number of words and number of different words in text

}
#endif
