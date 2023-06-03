#include <chrono>
#include <time.h>
#include <cstring>
#include <string>
#include <iostream>
#include "constants.h"

const int iter=1000000; // Number of experiments
int rands[iter]; // Indices of random elements to extract

using namespace std;
using namespace constants;

extern void prepare_DA_structures(int,int);
extern void formTrmd1_select();
extern int RMDsearch_rev_delta(uchar*,int);
extern void gen_reverse();
extern uint* read_ranks(string);
extern int encode_rmd_rev(int,uint*);


int main(int argc, char *argv[]) {
    gen_reverse(); //Generating the array of the Reverse MD-code codewords
    formTrmd1_select(); //Creates lookup tables for decoding
    uint* ranks=read_ranks(argv[1]); //Loads sequence of integers from file
    int code_len=encode_rmd_rev(Nwords,ranks); //Creates the Reverse MD-code in 'codes' array
    cout<<endl<<"Text encoded"<<endl;
    cout<<Nwords<<" words in text; "<<diff_words<<" different words\n"<<"Size of RMD code in bytes: "<<code_len<<", RMD average codeword length="<<(float)code_len*8/Nwords<<endl;

    prepare_DA_structures(code_len,Nwords); // Create data structures for direct access

    // Generating random numbers to extract
    srand(time(0));
    for(int i=0;i<iter;i++) {
        rands[i]=rand()%Nwords;
    }

    // Computational experiment
    std::chrono::high_resolution_clock::time_point start,stop;
    start = std::chrono::high_resolution_clock::now();    // time count
    int s=0;
    for(int i=0;i<iter;i++) {
        s+=RMDsearch_rev_delta(codes,rands[i]);
            //Validate extraction correctness
            /*if(ranks[rands[i]]!=s) {
                cout<<"!!!!!="<<i<<" ranks[i]="<<ranks[i]<<" s="<<s;
                cin>>s;
            }*/
        }
    stop = std::chrono::high_resolution_clock::now();   // time count
    auto stand=(double)chrono::duration_cast<std::chrono::nanoseconds>( stop - start ).count() / double(iter); // average execution time
    cout<<endl<<"!"<<s<<"! Time="<<stand;
}
