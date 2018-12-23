//defining methods for L1 cache

L1_cache::L1_cache(){
		hits=0;
		miss=0;
		int i;
		for(i=0;i<L1_number_of_sets;i++){
			set[i]=new Set(L1_asso,L1);
		}
}


void L1_cache::invalid(unsigned long long baddr){
	unsigned long long setno=get_set(baddr,SET_L1);
		set[setno]->remove(baddr);
}

void L1_cache::attach_l2(L2_cache* c){
	l2cache=c;
}


void L1_cache::touch(unsigned long long baddr,int operation){

		
		unsigned long long setno=get_set(baddr,SET_L1);
		if(set[setno]->is_there(baddr)){
			hits++;
			set[setno]->touch(baddr,operation);
			if(operation==WRITE){
				l2cache->touch(baddr,operation);
			}
		}else{
			miss++;
			//we need to bring the block now.
			if(set[setno]->current_count < set[setno]->assosiativity){
				set[setno]->lowlevel_insert(Block(baddr,S),operation);
				l2cache->touch(baddr,operation);
			}else{
				set[setno]->remove_lru();
				set[setno]->lowlevel_insert(Block(baddr,S),operation);
				l2cache->touch(baddr,operation);
			}

		}
	}