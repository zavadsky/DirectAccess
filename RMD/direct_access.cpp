#include "direct_access.h"

using namespace direct_access;

// Find the leftmost '1' bit of 32-bit value
int highest_bit(int x){
uint i=1<<31,j=32;
    while(!(i&x) && j>0) {
        i>>=1;
        j--;
    }
    return j;
}

//=========== Create lookup tables for fast RMD-code decoding ============
void pack_select(int L,int x,tabsrmd1 v, ai_state s) {
    PTR[x]=state_max*L/Tsize-state_max+s.st+1;
    if(L==Tsize) {//first block
        OUT_[x]=v.p2;
        RMD_N[x]=v.p3==-1;
    } else {
        OUT_[x]=v.p1;
        RMD_N[x]=v.p2==-1; // 0 results
    }
}

void formTrmd_inner1_select(ai_state state) {
tabsrmd1 v;
ai_state a=state;
uint i,y,j;
	for(uint x=0;x<pow2size;x++) {
        for(i=1,y=0,j=(pow2size>>1);j>0;i<<=1,j>>=1)
            if(i&x)
                y|=j;
		state=a;
		v.p1=v.p2=v.p3=v.p4=-1;
		for(uint k=(pow2size>>1);k>0;k>>=1) {
			state=a_step24infty(state,y&k); // The function depends on code parameters
			if(state.res!=-1) {
				if(v.p1==-1)
					v.p1=state.res;
				else
					if(v.p2==-1)
						v.p2=state.res;
                v.n++;
			}
		}
		if(v.p1==-1)
			v.p1=state.p;
		else
			if(v.p2==-1)
				v.p2=state.p;
            else
                if(v.p3==-1)
					v.p3=state.p;
        pack_select(a.L+Tsize,a.L==0?x:(state_max*a.L/Tsize-state_max+a.st+1)*pow2size+x,v,state);
	}
}

void formTrmd1_select() {
    formTrmd_inner1_select({0,0,0,-1});
	for(int a=0;a<state_max;a++)
		for(int L=Tsize;L<Lmax;L+=Tsize) {
			ai_state state={a,L,0,-1};
			formTrmd_inner1_select(state);
		}
    cout<<"\n Decoding table formed.\n";
}
//=====================================================

//=== Sequential processing of an RMD-bitstream ===========
uchar get_bit_rev(uchar u){
uchar j=0;
    for(uchar i=1;!(u&i) && i>0;i<<=1,j++);
    return j;
}

// Get the number (s.n) and bit positions of RMD-codewords (s.b[i]) in the 8 rightmost bits of a 12-bit value
// Little endian machine assumed. memory: aaaa aaaa bbbb -> x = bbbb aaaa aaaa
struct rmd_s get_rmd_numbers_rev(ushort x) {
ushort i=1;
int j,m,t=1,q=0;
struct rmd_s s={0,{10,10,10}};
    do {
        for(;x&i;i<<=1);
        t=get_bit_rev(i);
        i<<=1;
        for(j=0;x&i;i<<=1,j++);
        for(m=0;m<kmax && j!=k[m];m++);
        if(m==kmax && t<8) {
            s.n++;
            s.b[q++]=t;
        }
    } while(i<=128);
    return s;
}

// n12[i] contains: 1) number of codewords starting in 8 higher bits of a 12-bit value (lowest 2 bits)
// 2) starting positions of codewords in the 8-bit value (higher 6 bits)
void get_numbers12() {
struct rmd_s s;
    for(ushort i=0;i<0xfff;i++) {
        s=get_rmd_numbers_rev(i);
        n12[i]=s.n;
        if(s.b[1]==10) {
            n12[i]|=s.b[0]<<5;
            n12[i]|=s.b[0]<<2;
        } else
            if(s.b[2]==10) {
                n12[i]|=s.b[1]<<5;
                n12[i]|=s.b[0]<<2;
            } else {
                n12[i]|=s.b[2]<<5;
                n12[i]|=s.b[1]<<2;
                if(s.b[1]==4 && s.b[0]==0)
                    n12[i]|=4;
             }
    }
}
//=====================================================================

// Create Level 1 structures
void create_L1_rev(int Len,uchar* codes) {
int t,k=0,i;
    for(int i=0;i<Len;i++) {
        t=get_rmd_numbers_rev(*((ushort*)(codes+i))&0xfff).n;
        for(int j=0;j<t;j++) {
            A1[k++]=i;
        }
    }
    L1_delta[0]=L1[0]=0;
    for(i=1;i*L1_size<Nwords;i++) {
        L1[i]=A1[i*L1_size];
        for(t=1;A1[i*L1_size-t]==A1[i*L1_size];t++);
        L1_delta[i]=t-1;
        L1_ent[i-1]=(double)(L1[i]-L1[i-1])*L2_size/(L1_size+L1_delta[i-1]-L1_delta[i]);
    }
    L1_ent[i-1]=(double)(A1[Nwords-1]-L1[i-1])*L2_size/(Len-L1[i-1]);
cout<<"\n L1 structures created.\n";
}

// Compress the sequence of 2-byte values L2ranks[n][0..L2_count] into the bitstream of bit_size-bit values
void pack_L2(int n,int bit_size) {
int cur_bit=1<<7,cur_byte=0;
    L2_packed[n] = (uchar*)calloc(((L2_count*bit_size)>>3)+1,1);
    for(int j=0;j<L2_count;j++) {
        ushort val=L2ranks[n][j];
        for(int i=1<<(bit_size-1);i>0;i>>=1) {
            if(val&i)
                L2_packed[n][cur_byte]+=cur_bit;
            if(cur_bit>1)
                cur_bit>>=1;
            else {
                cur_bit=1<<7;
                cur_byte++;
            }
        }
    }
}

// Create Level 2 structures. Return their total size
int create_L2(int Nwords) {
int t,max,mn,sz=0,global_max=0,M_prev,M=0;
    for(int i=0;i<L1_count;i++) {
        L2ranks[i][0]=0;
        max=-100;
        mn=100;
        M_prev=M;
        M=min(L2_count,(Nwords-L1_size*i)>>L2_bit);
        for(int j=0;j<M;j++) {
            int regress=L1[i]+L1_ent[i]*j;
            L2ranks[i][j]=A1[i*L1_size+j*L2_size]-regress;
            if(L2ranks[i][j]>max)
                max=L2ranks[i][j];
            if(L2ranks[i][j]<mn)
                mn=L2ranks[i][j];
        }
        L1[i]+=mn;
        for(int j=0;j<M;j++) {
           int regress=L1[i]+L1_ent[i]*j;
            L2ranks[i][j]=A1[i*L1_size+j*L2_size]-regress;
            if(L2ranks[i][j]<0)
               L2ranks[i][j]=0;
            t=1;
            if(i*L1_size+j*L2_size+L2ranks[i][j]+mn>0)
                for(;A1[i*L1_size+j*L2_size-t]==A1[i*L1_size+j*L2_size];t++);
            L2delta[i][j]=t;
            L2delta_bit[i][j>>2]+=t<<((j&3)<<1);
            if(i>0 && j==0)
                L2delta_bit[i-1][M_prev>>2]+=t<<((M_prev&3)<<1);
            range[i]=highest_bit(max-mn+1);
        }
        if(range[i]>global_max)
            global_max=range[i];
        sz+=range[i]*L2_count;
        pack_L2(i,range[i]);
   }
   cout<<endl<<"Level 2 structures created. =="<<endl;
   return sz;
}

// Facade for all direct access structures creation
void prepare_DA_structures(int code_len,int Nwords) {
    create_L1_rev(code_len,codes);
    int sz=create_L2(Nwords);
    int l1s=sizeof(L2_packed)+sizeof(L1)+sizeof(range)+sizeof(L1_ent),const_s=sizeof(n12)+sizeof(RMD_N)+sizeof(PTR)+sizeof(OUT_);
    cout<<endl<<"L1 size="<<l1s<<" L2 size="<<(int)sz/8+sizeof(L2delta_bit)<<" const size="<<const_s<<" Total size="<<l1s+sz/8+const_s+sizeof(L2delta_bit)<<endl;
    get_numbers12();
}

//Main algorithm
//Extract and decode the t-th element in the RMD-encoded bitstream *codes
int RMDsearch_rev_delta(uchar* codes,int t) {
int l1n=t>>L1_bit,l2n=t&L1_mask;//number of word inside L1 block
int l3n=l2n>>L2_bit,byte,bit,i; //number of L2 block inside L1 block
ushort v;
uint32_t out=0;
int l3t=(l2n&L2_mask)+DELTA_ab(l1n,l3n);
uchar w;
int words=0,k=0;
if(l3t<=L2_size2) {
    // Left-to-right search
    int bit_size=range[l1n],bit_n=l3n*bit_size,pow2s=1<<bit_size;
    ushort sbyte=*(ushort*)(L2_packed[l1n]+(bit_n>>3));
        sbyte=(sbyte<<8)|(sbyte>>8);
        if((bit_n&0x7)>16-bit_size) {
            int bits3=(8-((bit_n+bit_size)&0x7));
            uchar part3=L2_packed[l1n][((bit_n+bit_size)>>3)]>>bits3;
            sbyte=(sbyte<<((bit_n+bit_size)&0x7))+part3;
        } else
            sbyte>>=(16-(bit_n&0x7)-bit_size);
    i=L1[l1n]+L1_ent[l1n]*l3n+(sbyte&(pow2s-1));
        if(l3t==1) {
            w=n12[*((ushort*)(codes+i))&0xfff];
            if((w&3)!=3)
                {byte=i; bit=((w&0x1c)>>2);}
            else
                {byte=i; bit=((w&4)>>2)^1;}
        } else {
                while(words<l3t) {
                    w=n12[*((ushort*)(codes+i++))&0xfff];
                    words+=w&3;
                }
                i--;
                byte=i;
                if(words==l3t)
                     { bit=(w>>5);}
                else
                    if(words==l3t+1) {
                        int x=(w&0x1c)>>2;
                        if(x>4)
                            x=4;
                        bit=x;
                    } else
                        {bit=((w&4)>>2)^1;}
            }
    } else {
        // Right-to-left search
        l3n++;
        int bit_size=range[l1n],bit_n=l3n*bit_size,pow2s=1<<bit_size;
        ushort sbyte=*(ushort*)(L2_packed[l1n]+(bit_n>>3));
            sbyte=(sbyte<<8)|(sbyte>>8);
            if((bit_n&0x7)>16-bit_size) {
                int bits3=(8-((bit_n+bit_size)&0x7));
                uchar part3=L2_packed[l1n][((bit_n+bit_size)>>3)]>>bits3;
                sbyte=(sbyte<<((bit_n+bit_size)&0x7))+part3;
            } else
                sbyte>>=(16-(bit_n&0x7)-bit_size);
        // target inside L2 block
            int i=L1[l1n]+L1_ent[l1n]*l3n+(sbyte&(pow2s-1))-1;
            words=L2_size-DELTA_ab(l1n,l3n)+DELTA_ab(l1n,l3n-1);
            while(words>=l3t) {
                w=n12[*((ushort*)(codes+i--))&0xfff];
                words-=w&3;
            }
            i++;
            while(words<l3t) {
                w=n12[*((ushort*)(codes+i++))&0xfff];
                words+=w&3;
            }
            i--;
            byte=i;
            if(words==l3t)
                     { bit=(w>>5);}
                else
                    if(words==l3t+1) {
                        int x=(w&0x1c)>>2;
                        if(x>4)
                            x=4;
                        bit=x;
                    } else
                        {bit=((w&4)>>2)^1;}
    }
uint64_t val=(*((uint64_t*)(codes+byte)))>>bit;
uchar ptr=0;
    do {
        v=ptr*pow2size+(val&Tmask);
        ptr=PTR[v];
        out+=OUT_[v];
        val>>=Tsize;
    } while(RMD_N[v]);
    return out;
}

