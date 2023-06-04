#include <iostream>
#include <sdsl/bit_vectors.hpp>

using namespace std;
using namespace sdsl;

const int L=5000000; //size of the chunks array
sdsl::bit_vector* iB[10]; //array of bit vectors
sdsl::bit_vector_il<>* iL[10]; //interleaved IL vector
sdsl::rank_support_il<>* iL_rank[10]; //data structure for the fast 'rank' operation over the IL vector
unsigned char A[5][L]; //sequence of chunk arrays for different levels

template<typename TypeIn, typename TypeOut>
std::vector<TypeOut> read_data_binary(const std::string &filename, bool first_is_size = true, size_t max_size = std::numeric_limits<TypeIn>::max()) {
    try {
        auto openmode = std::ios::in | std::ios::binary;
        if (!first_is_size)
            openmode |= std::ios::ate;

        std::fstream in(filename, openmode);
        in.exceptions(std::ios::failbit | std::ios::badbit);

        size_t size;
        if (first_is_size)
            in.read((char *) &size, sizeof(size_t));
        else {
            size = static_cast<size_t>(in.tellg() / sizeof(TypeIn));
            in.seekg(0);
        }
        size = std::min(max_size, size);

        cout<<"size="<<size;
        std::vector<TypeIn> data(size);
        in.read((char *) data.data(), size * sizeof(TypeIn));

        if constexpr (std::is_same<TypeIn, TypeOut>::value)
            return data;

        return std::vector<TypeOut>(data.begin(), data.end());
    }
    catch (std::ios_base::failure &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << std::strerror(errno) << std::endl;
        exit(1);
    }
}

// Reading the j-th sequence of chunks into the array A[j]
int readA(int j) {
char fnameA[10]="a0.bin";
auto openmode = std::ios::in | std::ios::binary;
    fnameA[1]=j+48;
    std::fstream in(fnameA, openmode);
    cout<<"^^^"<<fnameA<<"^";
    int size;
    in.read((char *) &size, 4);
    size>>=1;   // comment this line for b=8
    cout<<"Asize="<<size<<endl;
    in.read((char *) A[j], size );
    return size;
}

// Generating structure for integer retrieval.
// Bitstreams read from files bits0.bin, bits1.bin, ...
// Chunks read from files a0.bin, a1. bin, ...
void decodeVbyte_prepare() {
char fnameB[10]="bits0.bin";
int j=0,s,sa=0,sb=0,sr=0;
auto dataRead = read_data_binary<uint32_t, uint32_t>(fnameB);
    do {
        sa+=readA(j);
        s=dataRead.size();
        iB[j] = new bit_vector(dataRead[s-1]+1,0);
        for(size_t i=0; i<s;i++)
            (*iB[j])[dataRead[i]] = 1;
        iL[j] = new bit_vector_il<>(*iB[j]);
        sb+=sdsl::size_in_bytes(*iL[j]);
        iL_rank[j] = new rank_support_il<>(iL[j]);
        sr+=size_in_bytes(*iL_rank[j]);
        j++;
        fnameB[4]=j+48;
        dataRead = read_data_binary<uint32_t, uint32_t>(fnameB);
    } while(dataRead.size());
    iB[j] = new bit_vector(s,0);
    iL[j] = new bit_vector_il<>(*iB[j]);
    sb+=size_in_bytes(*iL[j]);
    sa+=readA(j);
    cout<<"sa="<<sa<<" sb="<<sb<<" sr="<<sr<<" total size="<<sa+sb+sr<<endl;
}

// i-th element retrieval from the DAC structure, b=4
int decodeVbyte(int i) {
int b=(*iL[0])[i],p=(A[0][i>>1]>>((i&1)<<2))&0xf,j=1,t=4;
   while(b) {
       i=(*iL_rank[j-1])(i);
       b=(*iL[j])[i];
       p=p+(((A[j][i>>1]>>((i&1)<<2))&0xf)<<(j*4)); // p=p+((A[j][i])<<(j*8)); for b=8
       j++;
    };
    return p;
}

// Getting the average speed of integer retrieval
// testsize - number of experiments; Count - maximal index of an integer to be retrieved
void testVbyte(int64_t testsize, uint32_t Count) {
uint64_t* testPosSelect = new uint64_t[testsize];
std::random_device rd;     // only used once to initialise (seed) engine
std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister)
std::uniform_int_distribution<uint64_t> uniSelect(1,Count); // guaranteed unbiased uniformly distributed integers between 1 and bitmapSetBitsCount, inclusive
std::chrono::high_resolution_clock::time_point start;
std::chrono::high_resolution_clock::time_point stop;
int64_t sum4=0;
    // Generating random integer indices
    for(uint64_t i=0;i <testsize;i++) {
        testPosSelect[i] = uniSelect(rng);
    }
    // Test integer retrieval
    start = std::chrono::high_resolution_clock::now();
    for(uint64_t i =0; i<testsize;i++){
        sum4 += decodeVbyte(testPosSelect[i]);
    }
    stop = std::chrono::high_resolution_clock::now();
    std::cout<<endl<<"==================="<<endl;
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>( stop - start ).count() / double(testsize) << '\t' << sum4;


}

//-----------------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    decodeVbyte_prepare();
    testVbyte(atoi(argv[1]),atoi(argv[2]));
    return 0;
}
