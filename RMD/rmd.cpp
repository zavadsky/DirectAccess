#include "rmd.h"
#include "constants.h"

using namespace constants;
using namespace rmd_space;

// 1) Generating the Reverse Multi-Delimiter codewords in rmd array. 2) Filling nlk array for the decoding.
void gen_reverse() {
int L,n,i,j,L1_long=-1,L1_long_prev=-1,t=Lmin-1;
uint seq=0;
	for(L=Lmin,n=0;L<Lmax+8 && n<=max_rmd;L++) {
		nl[L]=n;
		for(i=0;L-k[i]>Lmin && i<kmax;i++) {
			if(i==kmax-1 && L1_long==-1)
				L1_long=n;
			nlk[L][k[i]]=n-nl[L-k[i]-1];
			for(j=nl[L-k[i]-1];j<nl[L-k[i]] && n<=max_rmd;j++,n++) {
				seq=0x3FFFFFFF>>(30-k[i]);
				rmd[n]=(rmd[j]<<(k[i]+1))|seq;
			}
		}
		if(L1_long_prev>-1) {
			for(j=L1_long_prev;j<nl[L] && n<=max_rmd;j++,n++)
				rmd[n]=(rmd[j]<<1)|1;
		}
		if(t<kmax && L-1!=k[t]) {
			rmd[n]=(1<<(L-1))-1;
			n++;
		} else
			t++;
		L1_long_prev=L1_long;
		L1_long=-1;
	}
	cout<<"\n RMD code generted.\n";
}

//Mapping text into integers = indices of words in the array ordered by the descending order of their frequences,
//i.e. in the map dic_map<word,index>
void text_to_ranks(map<string,int> dic_map,const char* file_name,unsigned int* ranks){
int i=0,j=0;
string word;
char x;
	ifstream file(file_name);
	while ( ! file.eof() ) {
		file>>word;
		if(dic_map.find(word)!=dic_map.end())
			ranks[i++]=dic_map[word];
		/*if(i%100000==0)
			cout<<i<<" ";*/
    }
	cout<<"\nMapping text to integers done\n";
}

// Build the dictionary of words from the text in the file s, in order of descending frequences
int word_frequences(char* s) {
double pi;
string word;
int size=0,i=0;
ifstream file(s);
    // Create the map <word,frequency> - rmd_map
	while ( ! file.eof() ) {
		file>>word;
		if(rmd_map.find(word)!=rmd_map.end())
			rmd_map[word]++;
		else
			rmd_map.insert(make_pair(word,1));
		size++;
		if(size%1000000==0)
			printf("%d ",size);
	}
	printf("file processed\n");

	// Create the multimap <frequency,word> consisting all different words - freq_rmd
map<string,int> :: iterator it;
multimap<int,string> :: iterator it1,it3;
	entropy=0;
	diff_words=0;
	for(it=rmd_map.begin();it!=rmd_map.end();it++,diff_words++) {
		freq_rmd.insert(make_pair(it->second,it->first));
		// Calculate Shannon entropy
		pi=(double)it->second/size;
		entropy-=pi*(long double)log((long double)pi)/(long double)log((long double)2);
	}

	// Create the map <word,index> which maps words of text to integers according to descending order of their frequences
	// and ordered dictionary Dict_rmd
	int sm=0,k;
	string str;
	for(it1=freq_rmd.end(),it1--,i=0,k=3;it1!=freq_rmd.begin();it1--,i++) {
		rmd_map_sorted.insert(make_pair(it1->second,i));//<word of text,number>
		str=it1->second;
		Dict_rmd[i]=(char*)malloc(str.length());
		strcpy(Dict_rmd[i],str.c_str());

	}
	rmd_map_sorted.insert(make_pair(it1->second,i));//insert freq_rmd.begin()
	str=it1->second;
    Dict_rmd[i]=(char*)malloc(str.length());
	strcpy(Dict_rmd[i],str.c_str());
	printf("map built\n");
	return size;
}

//==================== Encode ===================

// writes a codeword to the end of a code bitstream
// x is a codeword aligned to the right edge of a 32-bit word
void flush_to_byte_rmd(unsigned int x) {
int k(0),p;
unsigned int j,t=32;
// Find the leftmost '1' bit in the last byte of a code. Codeword starts 1 bit left to it
	for(j=1<<t-1,p=t-1;!(x&j) && j;j>>=1,p--);
	for(j<<=1;j>0;j>>=1) {
		if(j&x)
			codes[cur_byte]|=(1<<cur_bit);
		cur_bit=cur_bit==0?7:cur_bit-1;
		if(cur_bit==7) {
			codes[++cur_byte]=0;
		}
	}
}

// Append the value x to bitstream
void flush_to_byte_rev(unsigned int x) {
int k(0),p;
unsigned int j,t=32;
// Find the leftmost '1' bit in the last byte of a code. Codeword starts 1 bit left to it
	for(j=1<<t-1,p=t-1;!(x&j) && j;j>>=1,p--);
	for(j<<=1;j>0;j>>=1) {
		if(j&x)
			codes[cur_byte]|=(1<<(7-cur_bit));
		cur_bit=cur_bit==0?7:cur_bit-1;
		if(cur_bit==7) {
			codes[++cur_byte]=0;
		}
	}
}

// Encode the integer sequence given in 'ranks' array. The result is in 'codes' bitstream
int encode_rmd_rev(int max_rank,uint* ranks) {
int i,k=0;
	cur_byte=0; cur_bit=7; codes[0]=0;
	for(i=0;i<max_rank;i++) {
		flush_to_byte_rev(rmd[ranks[i]]); // write current codeword to the end of a code bitstream
		if(ranks[i]>diff_words)
            diff_words=ranks[i];
	}
	return cur_byte;
}

//============== Decode ==============
// R2-infinity automaton modelling
struct ai_state a_step2infty(ai_state prev,int bit) {
ai_state r={0,prev.L+1,prev.p,-1};
	if(bit==0) {
		switch(prev.st) {
			case 0: r.p=prev.p+nlk[prev.L][0];
					break;
			case 1: r.p=prev.p+nlk[prev.L][1];
					break;
			case 2: r.p=nl[r.L]-1;
					break;
		}
	} else {
		r.st=prev.st<2?prev.st+1:2;
		if(prev.st==1) {
			r.L=3;
			r.res=prev.p;
			r.p=0;
		}
	}
	return r;
}

// R2,4-infinity automaton modelling
struct ai_state a_step24infty(ai_state prev,int bit) {
ai_state r={0,prev.L+1,prev.p,-1};
	if(bit==0) {
		switch(prev.st) {
			case 0: r.p=prev.p+nlk[prev.L][0];
					break;
			case 1: r.p=prev.p+nlk[prev.L][1];
					break;
            case 2: r.L=4;
                    r.res=prev.p;
                    r.p=0;
                    break;
			case 3: r.p=prev.p+nlk[prev.L][3];
					break;
			case 4: r.p=nl[r.L]-1;
					break;

		}
	} else {
		r.st=prev.st<4?prev.st+1:4;
		if(prev.st==3) {
			r.L=5;
			r.res=prev.p;
			r.p=0;
		}
	}
	return r;
}

// Convert integer sequence to text
void write_to_file_rmd(int N,unsigned int* indices,char* fname) {
string word;
	ofstream file(fname),f("frequences.txt");
	for(int i=1;i<=N;i++) {
        string s(Dict_rmd[indices[i]]);
        file<<s<<" ";
	}
	file.close();
}

// Write 32-bit integer sequence to file. The first integer is the number of elements
void serialize_ranks() {
    std::ofstream a("numbers.bin",ios::binary);
    a.write((char*)&Nwords,4);
    cout<<"Serializing."<<endl;
    for(int i=0;i<Nwords;i+=1000000) {
        a.write((char*)(ranks+i),4000000);
        cout<<i/1000000<<" ";
    }
    cout<<"Ranks serialized."<<endl;
}

// Read 32-bit integer sequence from file. The first integer is the number of elements
unsigned int* read_ranks(string fn) {
    std::ifstream a(fn,ios::binary);
    a.read((char*)&Nwords,4);
    cout<<"Serializing."<<endl;
    for(int i=0;i<Nwords;i+=1000000) {
        a.read((char*)(ranks+i),4000000);
        cout<<i/1000000<<" ";
    }
    cout<<"Ranks read. Nwords="<<Nwords<<endl;
    return ranks;
}

