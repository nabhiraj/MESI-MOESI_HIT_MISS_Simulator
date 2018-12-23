//defineing methods of llc cache.

LLC_cache::LLC_cache(Bus* b){
		hits=0;
		miss=0;
		bus=b;
		int i;
		for(i=0;i<LLC_number_of_sets;i++){
			set[i]=new Set(LLC_asso,LLC);
		}
	}


void LLC_cache::touch(unsigned long long baddr,int operation){
		unsigned long long setno=get_set(baddr,SET_LLC);
		if(set[setno]->is_there(baddr)){
			hits++;
			set[setno]->touch(baddr,operation);
		}else{
			miss++;
			if(set[setno]->current_count<set[setno]->assosiativity){
				set[setno]->lowlevel_insert(Block(baddr,NO_STATE),operation);//wrong
			}else{
				Block temp=set[setno]->find_lru();
				set[setno]->remove_lru();
				set[setno]->lowlevel_insert(Block(baddr,NO_STATE),operation);//wrong
				bus->invalid(temp.baddress);
			}
		}
	}