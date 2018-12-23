#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<stdint.h>
#include<map>
#include<iostream>
#include<math.h>
#include<pthread.h>
#include<set>

#define BLOCK_LOG_SIZE 6
#define NUM_CPUS 8

#define L1_SETS 64
#define L1_WAYS 8
#define L1_BLOCK_SIZE 64

#define L2_SETS 512*2
#define L2_WAYS 8
#define L2_BLOCK_SIZE 64

#define LLC_SETS NUM_CPUS*512*2
#define LLC_WAYS 16
#define LLC_BLOCK_SIZE 64

#define invalid 0xffffffffffffffff

#define request 0
#define response 1

enum mesi_states{M,E,S,I};
enum message{BusRd,BusRdx,Flush};

struct packet
{
int req_reply_flag; //0 indicates request and 1 indicates reply
int core_id; //indicates the core id of requester or responder
enum mesi_states state;// MESI state of requester/replier
enum message msg; //indicates read or write request
unsigned long long addr; //block address
int s_bit; //0 indicates no sharers and 1 indicates sharers
int seen_count; //used to keep track of how many cores have seen the message
};

typedef struct packet packet;


void initialize();
int search(int core_id, std::string cache_level, unsigned long long addr);
int getsetno(unsigned long long addr, int NUM_SETS);
void insert(int core_id, std::string cache_level, unsigned long long addr, int flag);
void update(int core_id, std::string cache_level, unsigned long long addr, int flag);
int find_lru_victim(int core_id, std::string cache_level, int set_no);
void writeback(std::string cache_level, unsigned long long addr);
void backinvalidate_upperlevel(int core_id, std::string cache_level, unsigned long long addr);
void display(std::string cache_level);
unsigned long gettag(unsigned long long addr, int NUM_SETS);
void coherence_call(int core_id,enum mesi_states s,unsigned long long addr,int flag);
void update_state(int core_id,enum message msg,unsigned long long addr,packet * p);
void writethrough(int core_id,std::string cache_level, unsigned long long addr);
void bus_arbitration();
int inclusive_sanity_check();

struct block
{
unsigned long long addr;
int valid;
int dirty;
unsigned long long lru;
enum mesi_states state;
};
typedef struct block block;

struct cpu
{
block ** l1;
block ** l2;
};
typedef struct cpu cpu;

typedef struct sys
{
cpu core[NUM_CPUS];
block ** llc;
}sys;

sys s1;

unsigned long long lru_counter = 0;
unsigned long long l2_hit_count[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long l2_miss_count[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long l1_hit_count[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long l1_miss_count[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long coherence_miss_count[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long llc_hit_count = 0;
unsigned long long llc_miss_count = 0;
unsigned long long coherence_invalidation = 0;
unsigned long long m_to_i[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long e_to_i[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long s_to_i[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long m_to_s[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long e_to_s[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long e_to_m[NUM_CPUS] = {0,0,0,0,0,0,0,0};
unsigned long long s_to_m[NUM_CPUS] = {0,0,0,0,0,0,0,0};

std::set<unsigned long long> coherence_bckp[NUM_CPUS];
std::set<unsigned long long>::iterator it;
std::pair<std::set<unsigned long long>::iterator,bool> ret;

packet *** bus = (packet ***) malloc(2*sizeof(packet**));

void initialize()
{
for(int i=0;i<NUM_CPUS;i++)
{
	s1.core[i].l1 = (block **) malloc(L1_SETS*sizeof(block*));
 	s1.core[i].l2 = (block **) malloc(L2_SETS*sizeof(block*));

	for(int j=0;j<L1_SETS;j++)
	{
		s1.core[i].l1[j] = (block*) malloc(L1_WAYS*sizeof(block));
		for(int k=0;k<L1_WAYS;k++)
		{
			s1.core[i].l1[j][k].addr = 0;
			s1.core[i].l1[j][k].valid = 0;
			s1.core[i].l1[j][k].dirty = 0;
			s1.core[i].l1[j][k].lru = 0;
			s1.core[i].l1[j][k].state = I;
		}
	}

	for(int j=0;j<L2_SETS;j++)
	{
		s1.core[i].l2[j] = (block*) malloc(L2_WAYS*sizeof(block));
		for(int k=0;k<L2_WAYS;k++)
		{
			s1.core[i].l2[j][k].addr = 0;
			s1.core[i].l2[j][k].valid = 0;
			s1.core[i].l2[j][k].dirty = 0;
			s1.core[i].l2[j][k].lru = 0;
			s1.core[i].l2[j][k].state = I;
		}
	}	
}
	
	s1.llc =  (block **) malloc(LLC_SETS*sizeof(block*));
	for(int j=0;j<LLC_SETS;j++)
	{
		s1.llc[j] =  (block*) malloc(LLC_WAYS*sizeof(block));
		for(int k=0;k<LLC_WAYS;k++)
		{
			s1.llc[j][k].addr = 0;
			s1.llc[j][k].valid = 0;
			s1.llc[j][k].dirty = 0;
			s1.llc[j][k].lru = 0;
			s1.llc[j][k].state = I;

		}

	}

}


int main(int argc, char * argv[])
{
	FILE *fp;
	uint64_t addr,elem,block_addr;
	uint32_t count=0;
	uint32_t tid;
	int flag,found=0;
	bus[request] = (packet**)malloc(1*sizeof(packet*));
	bus[response] = (packet**)malloc(NUM_CPUS*sizeof(packet*));
	if(argc!=3)
	{
	printf("Pass file name and count of records to read\n");
	exit(0);
	}

	fp = fopen(argv[1],"rb");
	assert(fp!=NULL);
	count = atoi(argv[2]);

	initialize();

	while(count>0)
	{
	fread(&elem,sizeof(uint64_t),1,fp);
        flag = elem & 0x1;
        addr = elem >> 4;
        tid = (elem>> 1) & 0x7;
	//tid = tid % 4;
	assert(tid<8);
	block_addr = addr >> BLOCK_LOG_SIZE;


	if(!search(tid,"L1",block_addr))
	{
	l1_miss_count[tid]+=1;
	if(!search(tid,"L2",block_addr))
	{
		l2_miss_count[tid]+=1;
		it = coherence_bckp[tid].find(block_addr);
		if(it!=coherence_bckp[tid].end())
			{
				coherence_miss_count[tid]+=1;
				coherence_bckp[tid].erase(it);
			}
				
		if(!search(tid,"LLC",block_addr))
		{		
			llc_miss_count+=1;
			insert(tid,"LLC",block_addr,flag);
			insert(tid,"L2",block_addr,flag);
			insert(tid,"L1",block_addr,flag);
		}
		else
		{			
			llc_hit_count+=1;
			update(tid,"LLC",block_addr,flag);
			insert(tid,"L2",block_addr,flag);
			insert(tid,"L1",block_addr,flag);				
		}
	
	}
	else
	{
		l2_hit_count[tid]+=1;
		update(tid,"L2",block_addr,flag);
		insert(tid,"L1",block_addr,flag); //miss in in L1 but hit in L2
	}
	}
	else
	{
	l1_hit_count[tid]+=1;
	update(tid,"L1",block_addr,flag);
	}		
		
	count = count-1;
	}

fclose(fp);
printf("Reading finished\n");

for (int i=0;i<NUM_CPUS;i++)
{
printf("Core_id:%d\n",i);
printf("L1_HITS:%llu\n",l1_hit_count[i]);
printf("L1_MISSES:%llu\n",l1_miss_count[i]);
printf("L2_HITS:%llu\n",l2_hit_count[i]);
printf("L2_MISSES:%llu\n",l2_miss_count[i]);
printf("m_to_i:%llu\n", m_to_i[i]);
printf("e_to_i:%llu\n", e_to_i[i]);
printf("s_to_i:%llu\n", s_to_i[i]);
printf("m_to_s:%llu\n", m_to_s[i]);
printf("e_to_s:%llu\n", e_to_s[i]);
printf("e_to_m:%llu\n", e_to_m[i]);
printf("s_to_m:%llu\n", s_to_m[i]);
printf("coherence misses:%llu\n",coherence_miss_count[i]);
}
printf("LLC_HITS:%llu\n",llc_hit_count);
printf("LLC_MISSES:%llu\n",llc_miss_count);
printf("Coherence Invalidations: %llu\n",coherence_invalidation);
assert(inclusive_sanity_check());

return 0;
}


int getsetno(unsigned long long addr, int NUM_SETS)
{

int log_bits = 0;
int setno = -1 ; 
log_bits = (int) log2(NUM_SETS);
setno = addr & (NUM_SETS-1);	

return setno;
}

int search(int core_id, std::string cache_level, unsigned long long addr)
{

int set_no = -1;
int i=0;
int found = 0;
if(cache_level.compare("L1") == 0)
{
	set_no = getsetno(addr,L1_SETS);
	assert(set_no != -1);
	for(i=0;i<L1_WAYS;i++)
	{
		if((s1.core[core_id].l1[set_no][i].addr == addr) && (s1.core[core_id].l1[set_no][i].valid == 1))
		{
		 found = 1;
		 break;
		}

	}

}
if(cache_level.compare("L2") == 0)
{
	set_no = getsetno(addr,L2_SETS);
	assert(set_no != -1);
	for(i=0;i<L2_WAYS;i++)
	{		
		if((s1.core[core_id].l2[set_no][i].addr == addr) && (s1.core[core_id].l2[set_no][i].valid == 1))
		{
		 found = 1;
		 break;
		}
	}

}

if(cache_level.compare("LLC") == 0)
{
	set_no = getsetno(addr,LLC_SETS);
	assert(set_no != -1);
	for(i=0;i<LLC_WAYS;i++)
	{
		if((s1.llc[set_no][i].addr == addr) && (s1.llc[set_no][i].valid == 1))
		{
		 found = 1;
		 break;
		}

	}
}

return found;
}


void insert(int core_id, std::string cache_level, unsigned long long addr, int flag)
{

int set_no = -1;
int i;
int way;
int sbit_set_flag = 0;

if(cache_level.compare("L1") == 0)
{
	set_no = getsetno(addr,L1_SETS);
	assert(set_no != -1);
	for(i=0;i<L1_WAYS;i++)
	{
		if(s1.core[core_id].l1[set_no][i].valid == 0)
		{
			way = i;
			break;
		}
	}
	if(i == L1_WAYS)
	{
		way = find_lru_victim(core_id,"L1",set_no);
	} 
	
	if(flag) //write access
	{
		s1.core[core_id].l1[set_no][way].addr = addr;
		s1.core[core_id].l1[set_no][way].valid = 1;
		s1.core[core_id].l1[set_no][way].dirty = 1;
		s1.core[core_id].l1[set_no][way].state = M; //coherence is maintained in L2
	}
	else //read access
	{
		s1.core[core_id].l1[set_no][way].addr = addr;
		s1.core[core_id].l1[set_no][way].valid = 1;
		s1.core[core_id].l1[set_no][way].dirty = 0;
		s1.core[core_id].l1[set_no][way].state = S; //coherence is maintained in L2 due

	}
	for(i=0;i<L1_WAYS;i++)
	{
	s1.core[core_id].l1[set_no][i].lru+=1; 
	}
	s1.core[core_id].l1[set_no][way].lru = 0;

}

if(cache_level.compare("L2") == 0)
{
	set_no = getsetno(addr,L2_SETS);
	assert(set_no != -1);
	for(i=0;i<L2_WAYS;i++)
	{
		if(s1.core[core_id].l2[set_no][i].valid == 0)
		{
			way = i;
			break;
		}
	}
	if(i == L2_WAYS)
	{
		way = find_lru_victim(core_id,"L2",set_no);
		backinvalidate_upperlevel(core_id,"L1",s1.core[core_id].l2[set_no][way].addr);//to ensure inclusive of L1 and L2
	
	}
	
	if(s1.core[core_id].l2[set_no][way].dirty == 1)
	{
		writeback("LLC",s1.core[core_id].l2[set_no][way].addr);
	
	}


	if(flag) //write access
	{
		s1.core[core_id].l2[set_no][way].addr = addr;
		s1.core[core_id].l2[set_no][way].valid = 1;
		s1.core[core_id].l2[set_no][way].dirty = 1;
		coherence_call(core_id,I,addr,flag);
		for(int i=0;i<NUM_CPUS;i++)
		{
		if(bus[response][i]!=NULL && bus[response][i]->req_reply_flag==1) 
			coherence_invalidation+=1;
		}
		s1.core[core_id].l2[set_no][way].state = M;
	}
	else //read access
	{
		s1.core[core_id].l2[set_no][way].addr = addr;
		s1.core[core_id].l2[set_no][way].valid = 1;
		s1.core[core_id].l2[set_no][way].dirty = 0;
		coherence_call(core_id,I,addr,flag);
		for(int i=0;i<NUM_CPUS-1;i++)
		{
		if(bus[response][i]!=NULL && bus[response][i]->s_bit == 1)
			{
			s1.core[core_id].l2[set_no][way].state = S;
			sbit_set_flag = 1;
			}
		}

		if(!sbit_set_flag)
			{
			
			s1.core[core_id].l2[set_no][way].state = E;
			}
		

	}

	for(i=0;i<L2_WAYS;i++)
	{
	s1.core[core_id].l2[set_no][i].lru+=1; 
	}
	s1.core[core_id].l2[set_no][way].lru = 0;
	
	//bus[response] = NULL; //ensures that reponse from bus is removed

}

if(cache_level.compare("LLC") == 0)
{
	set_no = getsetno(addr,LLC_SETS);
	assert(set_no != -1);
	for(i=0;i<LLC_WAYS;i++)
	{
		if(s1.llc[set_no][i].valid == 0)
		{
			way = i;
			break;
		}
	}
	if(i == LLC_WAYS)
	{
		way = find_lru_victim(core_id,"LLC",set_no);
		for(int i=0; i<NUM_CPUS; i++)
		{	 
		backinvalidate_upperlevel(i,"L2",s1.llc[set_no][way].addr);//to ensure inclusive of LLC and L2
		backinvalidate_upperlevel(i,"L1",s1.llc[set_no][way].addr);//to ensure inclusive of LLC and L1
		}
	}
	
	if(flag) //write access
	{
		s1.llc[set_no][way].addr = addr;
		s1.llc[set_no][way].valid = 1;
		s1.llc[set_no][way].dirty = 1;
		s1.llc[set_no][way].state = M; //coherence state is irrelevant for LLC
	}
	else //read access
	{
		s1.llc[set_no][way].addr = addr;
		s1.llc[set_no][way].valid = 1;
		s1.llc[set_no][way].dirty = 0;
		s1.llc[set_no][way].state = S; //coherence state is irrelevant for LLC

	}

	for(i=0;i<LLC_WAYS;i++)
	{
	s1.llc[set_no][i].lru+=1; 
	}
	s1.llc[set_no][way].lru = 0;


}

}

int find_lru_victim(int core_id, std::string cache_level, int set_no)
{
int way;
int i;
int lru=0;
if(cache_level.compare("L1") == 0)
{
	for(i=0;i<L1_WAYS;i++)
	{
		if (s1.core[core_id].l1[set_no][i].lru>=lru)
			{
				lru = s1.core[core_id].l1[set_no][i].lru;
				way = i;		
			}		
	}
}

if(cache_level.compare("L2") == 0)
{
	for(i=0;i<L2_WAYS;i++)
	{
		if (s1.core[core_id].l2[set_no][i].lru>=lru)
			{
				lru = s1.core[core_id].l2[set_no][i].lru;
				way = i;		
			}		
	}
}


if(cache_level.compare("LLC") == 0)
{
	for(i=0;i<LLC_WAYS;i++)
	{
		if (s1.llc[set_no][i].lru>=lru)
			{
				lru = s1.llc[set_no][i].lru;
				way = i;		
			}		
	}
}
return way;
}


void writethrough(int core_id,std::string cache_level, unsigned long long addr)
{

int set_no = -1;
int i,j;
int reply_length = 0;
int got_addr = 0;
int way = 99;
if(cache_level.compare("L2") == 0)
{
	set_no = getsetno(addr,L2_SETS);
	assert(set_no != -1);
	for(i=0;i<L2_WAYS;i++)
	{
	if(s1.core[core_id].l2[set_no][i].addr == addr)
	{
		got_addr = 1;
		way = i;
		if(s1.core[core_id].l2[set_no][i].state == M)
		{
			coherence_call(core_id,M,addr,1);
			s1.core[core_id].l2[set_no][i].dirty = 1;
			for(j=0;j<NUM_CPUS;j++)
			{
			 if(bus[response][j]!=NULL && bus[response][j]->req_reply_flag ==1)
				reply_length+=1;
			}
			
			assert(reply_length==0); //sanity check, no block should be in M,E,S state
		}
		else if(s1.core[core_id].l2[set_no][i].state == S)
		{
			coherence_call(core_id,S,addr,1);
			s1.core[core_id].l2[set_no][i].dirty = 1;
			s1.core[core_id].l2[set_no][i].state = M;
			s_to_m[core_id]+=1;
		}
		
		else if(s1.core[core_id].l2[set_no][i].state == E)
		{
			coherence_call(core_id,E,addr,1);
			s1.core[core_id].l2[set_no][i].dirty = 1;
			s1.core[core_id].l2[set_no][i].state = M;
			e_to_m[core_id]+=1;
			for(j=0;j<NUM_CPUS;j++)
			{
			 if(bus[response][j]!=NULL && bus[response][j]->req_reply_flag ==1)
				reply_length+=1;
			}
			assert(reply_length==0); //sanity check, no block should be in M,E,S state

		}

		break;
	}

	}
	
	assert(got_addr!=0); // Sanity check to make sure writethrough block is present in L2
	assert(way!=99);

	for(i=0;i<L2_WAYS;i++)
	{
	s1.core[core_id].l2[set_no][i].lru+=1;
	}
	s1.core[core_id].l2[set_no][way].lru = 0;

}

}
void writeback(std::string cache_level, unsigned long long addr)
{
int set_no = -1;
int i=0;
if(cache_level.compare("LLC") == 0)
{
	set_no = getsetno(addr,LLC_SETS);
	assert(set_no != -1);

	for(i=0;i<LLC_WAYS;i++)
	{
		if(s1.llc[set_no][i].addr == addr)
		{
		 	s1.llc[set_no][i].dirty = 1; 
			break;
		}

	}

}
}


void update(int core_id, std::string cache_level, unsigned long long addr, int flag)
{
int set_no = -1;
int i;
int way;
int reply_length = 0;
if(cache_level.compare("L1") == 0)
{
	set_no = getsetno(addr,L1_SETS);
	assert(set_no != -1);
	for(i=0;i<L1_WAYS;i++)
	{
		if(s1.core[core_id].l1[set_no][i].addr == addr)
		{
			way = i;
			break;
		}
	}
	if(i == L1_WAYS)
	{
		printf("should not come here\n");
		exit(0);
	}
	
	if(flag) //write, update lru value and dirty bit
	{
		s1.core[core_id].l1[set_no][way].dirty = 1;
		writethrough(core_id,"L2",addr); //L1 is writethrough cache

	}
	for(i=0;i<L1_WAYS;i++)
	{
	s1.core[core_id].l1[set_no][i].lru+=1;
	}
	s1.core[core_id].l1[set_no][way].lru = 0;

}

if(cache_level.compare("L2") == 0)
{
	set_no = getsetno(addr,L2_SETS);
	assert(set_no != -1);
	for(i=0;i<L2_WAYS;i++)
	{
		if(s1.core[core_id].l2[set_no][i].addr == addr)
		{
			way = i;
			break;
		}
	}
	if(i == L2_WAYS)
	{
		printf("should not come here\n");
		exit(0);
	}

	if(flag) //write
	{
		if(s1.core[core_id].l2[set_no][way].state == M)
		{
			coherence_call(core_id,M,addr,flag);
			s1.core[core_id].l2[set_no][way].dirty = 1;
			for(int i;i<NUM_CPUS;i++)
			{
			 if(bus[response][i]!=NULL && bus[response][i]->req_reply_flag ==1)
				reply_length+=1;
			}
			
			assert(reply_length==0); //sanity check, no block should be in M,E,S state
		}
		else if(s1.core[core_id].l2[set_no][way].state == S)
		{
			coherence_call(core_id,S,addr,flag);
			s1.core[core_id].l2[set_no][way].dirty = 1;
			s1.core[core_id].l2[set_no][way].state = M;
			s_to_m[core_id]+=1;
		}
		
		else if(s1.core[core_id].l2[set_no][way].state == E)
		{
			coherence_call(core_id,E,addr,flag);
			s1.core[core_id].l2[set_no][way].dirty = 1;
			s1.core[core_id].l2[set_no][way].state = M;
			e_to_m[core_id]+=1;
			for(int i;i<NUM_CPUS;i++)
			{
			 if(bus[response][i]!=NULL && bus[response][i]->req_reply_flag ==1)
				reply_length+=1;
			}
			assert(reply_length==0); //sanity check, no block should be in M,E,S state

		}

	}
	for(i=0;i<L2_WAYS;i++)
	{
	s1.core[core_id].l2[set_no][i].lru+=1;
	}
	s1.core[core_id].l2[set_no][way].lru = 0;
		

}

if(cache_level.compare("LLC") == 0)
{
	set_no = getsetno(addr,LLC_SETS);
	assert(set_no != -1);
	for(i=0;i<LLC_WAYS;i++)
	{
		if(s1.llc[set_no][i].addr == addr)
		{
			way = i;
			break;
		}
	}
	if(i == LLC_WAYS)
	{
		printf("should not come here\n");
		exit(0);
	}
	
	if(flag) //write
	{
		s1.llc[set_no][way].dirty = 1;
	}
	
	for(i=0;i<LLC_WAYS;i++)
	{
	s1.llc[set_no][i].lru+=1;
	}
	s1.llc[set_no][way].lru = 0;

}

}


void backinvalidate_upperlevel(int core_id, std::string cache_level, unsigned long long addr)
{
int set_no = -1;
int i=0;
int found = 0;

if(cache_level.compare("L1") == 0)
{
	set_no = getsetno(addr,L1_SETS);
	assert(set_no != -1);

	for(i=0;i<L1_WAYS;i++)
	{
		if((s1.core[core_id].l1[set_no][i].addr == addr) && (s1.core[core_id].l1[set_no][i].valid == 1))
		{
		 	s1.core[core_id].l1[set_no][i].valid = 0; 
			s1.core[core_id].l1[set_no][i].state = I;
			break;
		}

	}

}
if(cache_level.compare("L2") == 0)
{
	set_no = getsetno(addr,L2_SETS);
	assert(set_no != -1);

	for(i=0;i<L2_WAYS;i++)
	{
		if((s1.core[core_id].l2[set_no][i].addr == addr) && (s1.core[core_id].l2[set_no][i].valid == 1))
		{
		 	if(s1.core[core_id].l2[set_no][i].dirty ==1)
				writeback("LLC",s1.core[core_id].l2[set_no][i].addr);

			s1.core[core_id].l2[set_no][i].valid = 0;
			s1.core[core_id].l2[set_no][i].state = I; 
			break;
		}

	}

}


}

void coherence_call(int core_id,enum mesi_states s,unsigned long long addr,int flag)
{
enum message m;

if(flag)
  m = BusRdx;
else
  m = BusRd;

packet *  p = (packet *)malloc(sizeof(packet));
p->req_reply_flag = 0;
p->core_id = core_id;
p->state = s;
p->msg = m;
p->addr = addr;
p->s_bit = 0;
p->seen_count = 0;
bus[request][0] = p;
bus_arbitration();

if(bus[request][0]->seen_count == NUM_CPUS-1)
	free(bus[request][0]);
else
	assert(0);

}

void bus_arbitration()
{
packet *p = bus[request][0];
int index = 0;

for(int i = 0;i< NUM_CPUS;i++)
{
	bus[response][i] = NULL;
	if(i!=p->core_id)
	{
		if(search(i,"L2",p->addr))
		{
		bus[response][index] = (packet *) malloc(sizeof(packet));
		bus[response][index]->s_bit = 0;
		update_state(i,p->msg,p->addr,bus[response][index]);
		p->seen_count+=1;
		index +=1;
	
		}	
	
	else
		{
		p->seen_count+=1;

		}

	}
}


}


void update_state(int core_id,enum message msg,unsigned long long addr, packet * p)
{

int set_no = -1;


if(msg == BusRdx)
{
	set_no = getsetno(addr,L2_SETS);
	assert(set_no != -1);
	for(int i=0;i<L2_WAYS;i++)
	{
		if (s1.core[core_id].l2[set_no][i].addr == addr)
		{
		  if(s1.core[core_id].l2[set_no][i].state == M)
			{
			p->req_reply_flag = 1;
			p->core_id = core_id;
			p->state = M;
			p->msg = Flush;
			p->addr = addr;
			p->s_bit = 0;
			p->seen_count = 0;
			s1.core[core_id].l2[set_no][i].state = I;
			s1.core[core_id].l2[set_no][i].valid = 0;
			ret = coherence_bckp[core_id].insert(addr);
			assert(ret.second != false);
			backinvalidate_upperlevel(core_id,"L1",s1.core[core_id].l2[set_no][i].addr);
			m_to_i[core_id]+=1;
			}
		else if(s1.core[core_id].l2[set_no][i].state == E)
			{
			p->req_reply_flag = 1;
			p->core_id = core_id;
			p->state = E;
			p->msg = Flush;
			p->addr = addr;
			p->s_bit = 0;
			p->seen_count = 0;
			s1.core[core_id].l2[set_no][i].state = I;
			s1.core[core_id].l2[set_no][i].valid = 0;
			ret = coherence_bckp[core_id].insert(addr);
			assert(ret.second != false);
			backinvalidate_upperlevel(core_id,"L1",s1.core[core_id].l2[set_no][i].addr);
			e_to_i[core_id]+=1;
			}
		else if(s1.core[core_id].l2[set_no][i].state == S)
			{
			p->req_reply_flag = 1;
			p->core_id = core_id;
			p->state = S;
			p->msg = Flush;
			p->addr = addr;
			p->s_bit = 0;
			p->seen_count = 0;
			s1.core[core_id].l2[set_no][i].state = I;
			s1.core[core_id].l2[set_no][i].valid = 0;
			ret = coherence_bckp[core_id].insert(addr);
			assert(ret.second != false);
			backinvalidate_upperlevel(core_id,"L1",s1.core[core_id].l2[set_no][i].addr);
			s_to_i[core_id]+=1;
			}
			break;
		}
	}

}	

else if (msg == BusRd)

{
	set_no = getsetno(addr,L2_SETS);
	assert(set_no != -1);

	for(int i=0;i<L2_WAYS;i++)
	{
		if (s1.core[core_id].l2[set_no][i].addr == addr)
		{
		  if(s1.core[core_id].l2[set_no][i].state == M)
			{
			p->req_reply_flag = 1;
			p->core_id = core_id;
			p->state = M;
			p->msg = Flush;
			p->addr = addr;
			p->s_bit = 1;
			p->seen_count = 1;
			s1.core[core_id].l2[set_no][i].state = S;
			m_to_s[core_id]+=1;
			}
		else if(s1.core[core_id].l2[set_no][i].state == E)
			{
			p->req_reply_flag = 1;
			p->core_id = core_id;
			p->state = E;
			p->msg = Flush;
			p->addr = addr;
			p->s_bit = 1;
			p->seen_count = 1;
			s1.core[core_id].l2[set_no][i].state = S;
			e_to_s[core_id]+=1;
			}
		else if(s1.core[core_id].l2[set_no][i].state == S)
			{
			p->req_reply_flag = 1;
			p->core_id = core_id;
			p->state = S;
			p->msg = Flush;
			p->addr = addr;
			p->s_bit = 1;
			p->seen_count = 1;
			}
		break;
		}
	}


}

}


int inclusive_sanity_check()
{

int sanity_flag = 1;

for(int core_id=0; core_id<NUM_CPUS ; core_id++)
{
	for(int set_no=0; set_no<L1_SETS; set_no++)
	{
		for(int way=0; way<L1_WAYS; way++)
		{
			if(s1.core[core_id].l1[set_no][way].valid)
			{
			if(!search(core_id,"L2",s1.core[core_id].l1[set_no][way].addr))
				sanity_flag = 0;
			if(!search(core_id,"LLC",s1.core[core_id].l1[set_no][way].addr))
				sanity_flag = 0;
			}
		}

	}

	for(int set_no=0; set_no<L2_SETS; set_no++)
	{
		for(int way=0; way<L2_WAYS; way++)
		{
			if(s1.core[core_id].l2[set_no][way].valid)
			{
			if(!search(core_id,"LLC",s1.core[core_id].l2[set_no][way].addr))
				sanity_flag = 0;
			}
	

		}

	}

}

return sanity_flag;

}



void display(std::string cache_level)
{

if(cache_level.compare("L1")==0)
{
for(int i=0;i<NUM_CPUS;i++)
{
printf("core id=%d\n",i);
for(int j=0;j<L1_SETS;j++)
{
printf("set no:%d\n",j);
for(int k=0;k<L1_WAYS;k++)
{
printf("way no:%d\n",k);
printf("address= %llu\n",s1.core[i].l1[j][k].addr);
printf("valid= %d\n",s1.core[i].l1[j][k].valid);
printf("dirty= %d\n",s1.core[i].l1[j][k].dirty);
printf("lru= %llu\n",s1.core[i].l1[j][k].lru);
printf("state= %d\n",s1.core[i].l1[j][k].state);
}
printf("===============================\n");
}
}
}


if(cache_level.compare("L2")==0)
{
for(int i=0;i<NUM_CPUS;i++)
{
printf("core id=%d\n",i);
for(int j=0;j<L2_SETS;j++)
{
printf("set no:%d\n",j);
for(int k=0;k<L2_WAYS;k++)
{
printf("way no:%d\n",k);
printf("address= %llu\n",s1.core[i].l2[j][k].addr);
printf("valid= %d\n",s1.core[i].l2[j][k].valid);
printf("dirty= %d\n",s1.core[i].l2[j][k].dirty);
printf("lru= %llu\n",s1.core[i].l2[j][k].lru);
printf("state= %d\n",s1.core[i].l2[j][k].state);
}
printf("===============================\n");
}
}
}

for(int i=0;i<NUM_CPUS;i++)
{
printf("core = %d\n",i);
printf("%p\n",s1.core[i].l1);
printf("%p\n",s1.core[i].l2);
}

}
