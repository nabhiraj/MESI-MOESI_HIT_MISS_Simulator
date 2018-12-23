#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<list>
using namespace std;
#include "Mac.hh"
#include "Basic_functions.hh"
#include "Block.hh"
#include "Set.hh"
#include "Bus.hh"
#include "L1_cache.hh"
#include "L2_cache.hh"
#include "LLC_cache.hh"
#include "Busd.hh"
#include "L1_cached.hh"
#include "L2_cached.hh"
#include "LLC_cached.hh"

int main(int argc,char* argv[]){
		L1_cache* l1[NUM_OF_CORE];
		L2_cache* l2[NUM_OF_CORE];
		LLC_cache* llc;
		Bus* bus=new Bus();
		llc=new LLC_cache(bus);
		bus->attach_llc(llc);
		int i;
		for(i=0;i<NUM_OF_CORE;i++){
			l1[i]=new L1_cache();
			l2[i]=new L2_cache(i);
			//lets do binding of l2 and l1 here itself
			l2[i]->attach_l1(l1[i]);
			l1[i]->attach_l2(l2[i]);
			l2[i]->attach_bus(bus);
			bus->attach_l2(l2[i],i);
		}
		if(argc!=3){
			cout<<argc<<"\n";
			cout<<"error--- pass file name and count of number of records.\n";
			return 0;
		}
		
		unsigned long long  count;
		count=atoi(argv[2]);
		FILE* fp=fopen(argv[1],"rb");
		
		if(fp==NULL){
			cout<<"something is not right, not getting file pointer. \n";
			return 0;
		}
		unsigned long long elem;
		unsigned long long addr;
		int tid;
		int flag;
		while(count>0){
			fread(&elem,sizeof(unsigned long long),1,fp);
			flag = elem & 0x1;//this is read write flag------------------------- o for read, and  1 for write (10)(11)
			if(flag==0){
				flag=10; //read operation
			}else{
				flag=11;//write operation
			}
			addr = elem >> 4;
			tid = (elem>> 1) & 0x7;
			l1[tid]->touch(get_blockAddr(addr),flag);
			count--;
		}
		fclose(fp);
		//printing the results.................................
		cout<<"number of hits in l1 cache are as follow\n ";
		for(i=0;i<NUM_OF_CORE;i++){
			cout<<"--------------------for core "<<i<<"----------------------------\n";
			cout<<"for l1 hits core "<<i<<" : "<<l1[i]->hits<<"\n";
			cout<<"for l1 miss core "<<i<<" : "<<l1[i]->miss<<"\n\n";
			cout<<"------------------------\n";
			cout<<"for l2 hits core "<<i<<" : "<<l2[i]->hits<<"\n";
			cout<<"for l2 miss core "<<i<<" : "<<l2[i]->miss<<"\n";
			cout<<"coherency miss in l2 :: "<<l2[i]->coherent_miss<<"\n\n\n";
		}
		cout<<"--------------------------llc----------------------------------------\n";
		cout<<"hits in llc : "<<llc->hits<<"\n";
		cout<<"miss in llc : "<<llc->miss<<"\n\n";
		cout<<"state transition in the given simulation------------------------------------------------------------------------ \n";
		for(i=0;i<NUM_OF_CORE;i++){
			cout<<"-------------------------for core "<<i<<"------------------------------------------\n";
			cout<<"for M to O ::"<<l2[i]->MtoO<<endl;
			cout<<"for M to I ::"<<l2[i]->MtoI<<endl;
			cout<<"for O to M ::"<<l2[i]->OtoM<<endl;
			cout<<"for O to I ::"<<l2[i]->OtoI<<endl;
			cout<<"for E to M ::"<<l2[i]->EtoM<<endl;
			cout<<"for E to S ::"<<l2[i]->EtoS<<endl;
			cout<<"for E to I ::"<<l2[i]->EtoI<<endl;
			cout<<"for S to M ::"<<l2[i]->StoM<<endl;
			cout<<"for S to I ::"<<l2[i]->StoI<<endl;
		}
	return 0;
}