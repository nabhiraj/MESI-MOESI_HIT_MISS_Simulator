//defining methods of Bus


void Bus::attach_l2(L2_cache* temp,int i){	
		l2cache[i]=temp;
	}


void Bus::attach_llc(LLC_cache* temp){
		llc=temp;
	}



void Bus::touch(unsigned long long baddr,int operation){
		//this should first check is there a block in O state in l2 if not then touch in llc should be called
		bool flag=true;
		int i;
		for(i=0;i<NUM_OF_CORE;i++){
			if(l2cache[i]->is_present_in_state(baddr,O)){
				flag=false;
				break;
			}
		}
		if(flag){
			llc->touch(baddr,operation);
		}
	}

	
	bool Bus::share_handler(unsigned long long baddr,int core){//false means no sharing true means there is sharing.
		/*
		share handler can come with following operation.
		1. read
		each cache can be in following sate with respect to the block.
		M S O I E
		*/
		bool returnflag=false;
		int i;
		for(i=0;i<NUM_OF_CORE;i++){//foe each l2cache[i] l2cache pointer
			if(i==core){
				continue;
			}
			if(l2cache[i]->is_present_in_state(baddr,M)){
				returnflag=true;
				l2cache[i]->set_state(baddr,O);
				l2cache[i]->MtoO++;
			}else if(l2cache[i]->is_present_in_state(baddr,E)){
				returnflag=true;
				l2cache[i]->set_state(baddr,S);
				l2cache[i]->EtoS++;
			}else if(l2cache[i]->is_present_in_state(baddr,O)){
				returnflag=true;
				l2cache[i]->set_state(baddr,O);
			}else if(l2cache[i]->is_present_in_state(baddr,S)){
				returnflag=true;
				l2cache[i]->set_state(baddr,S);
			}else if(l2cache[i]->is_present_in_state(baddr,NO_STATE)){
				//do nothing
			}else{
				cout<<"something is wrong shere_handler \n";
			}
		}
		return returnflag;
	}
	
	void Bus::inform_handler(unsigned long long baddr,int core){ //coherence table logic will come in this method.
		/*
		this will be invoked in case of a write
		*/
		int i;
		for(i=0;i<NUM_OF_CORE;i++){
			if(core==i){
				continue;
			}
			if(l2cache[i]->is_present_in_state(baddr,M)){
				l2cache[i]->invalidate(baddr);
				l2cache[i]->MtoI++;
				l2cache[i]->coherent_table.insert(pair<unsigned long long,unsigned long long>(baddr,baddr));
			}else if(l2cache[i]->is_present_in_state(baddr,E)){
				l2cache[i]->invalidate(baddr);
				l2cache[i]->EtoI++;
				l2cache[i]->coherent_table.insert(pair<unsigned long long,unsigned long long>(baddr,baddr));
			}else if(l2cache[i]->is_present_in_state(baddr,O)){
				l2cache[i]->invalidate(baddr);
				l2cache[i]->OtoI++;
				l2cache[i]->coherent_table.insert(pair<unsigned long long,unsigned long long>(baddr,baddr));
			}else if(l2cache[i]->is_present_in_state(baddr,S)){
				l2cache[i]->invalidate(baddr);
				l2cache[i]->StoI++;
				l2cache[i]->coherent_table.insert(pair<unsigned long long,unsigned long long>(baddr,baddr));
			}else if(l2cache[i]->is_present_in_state(baddr,NO_STATE)){
				//do nothing
			}else{
				cout<<"something is wrong shere_handler \n";
			}

		}
	}
	
	void Bus::invalid(unsigned long long baddr){
		int i;
		for(i=0;i<NUM_OF_CORE;i++){
			l2cache[i]->invalidate(baddr);
		}
	}
	

	void Bus::invalid(unsigned long long baddr,int j){ //invalidate the data in all the core except the core number j.
		int i;
		for(i=0;i<NUM_OF_CORE;i++){
			if(i!=j)
			l2cache[i]->invalidate(baddr);
		}
	}