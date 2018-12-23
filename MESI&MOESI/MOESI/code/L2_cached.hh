//defining methods for l2 cache.

L2_cache::L2_cache(int core){
		core_id=core;
		hits=0;
		miss=0;
		coherent_miss=0;

		MtoO=0;
		MtoI=0;
		EtoM=0;
		EtoS=0;
		EtoI=0;
		StoI=0;
		StoM=0;
		OtoM=0;
		OtoI=0;

		int i;
		for(i=0;i<L2_number_of_sets;i++){
			set[i]=new Set(L2_asso,L2);
		}
	}

	
	void L2_cache::attach_l1(L1_cache* temp){
		l1=temp;
	}

	
	void L2_cache::attach_bus(Bus* temp){
		bus=temp;
	}


	
	void L2_cache::invalidate(unsigned long long baddr){
		unsigned long long setno=get_set(baddr,SET_L2);
		if(set[setno]->is_there(baddr)){
		set[setno]->remove(baddr);
		l1->invalid(baddr);
	}
	}

	
	bool L2_cache::is_present_in_state(unsigned long long baddr,int state){
		unsigned long long setno=get_set(baddr,SET_L2);
		Block temp=set[setno]->is_there_l2(baddr);
		int t_state=temp.state;
		if(t_state==state){
			return true;
		}else{
			return false;
		}
	}

	
	void L2_cache::set_state(unsigned long long baddr,int s){
		unsigned long long setno=get_set(baddr,SET_L2);
		Block *tptr=set[setno]->is_there_ptr(baddr);
		if(tptr==NULL){
			cout<<"something is wrong with set_state in l2 cache\n";
		}
		tptr->state=s;
	}


	void L2_cache::touch(unsigned long long baddr,int operation){
		//cout<<"touch l2 cache "<<core_id<<"\n";
		unsigned long long setno=get_set(baddr,SET_L2);
		Block *temp=set[setno]->is_there_ptr(baddr);
		//cout<<"problem 2\n";
		//problem
		if(temp!=NULL){
			hits++;

			set[setno]->touch(baddr,operation);
			int t_state=temp->state;
			if(operation==WRITE){
				if(t_state==M){
					//do nothing
				}else if(t_state==E){
					temp->state=M;
					EtoM++;//-------
				}else if(t_state==O){
					temp->state=M;
					bus->inform_handler(baddr,core_id);//added.
					OtoM++;//--------
				}else if(t_state==S){
					//now some logic
					temp->state=M;
					bus->inform_handler(baddr,core_id);
					StoM++;//--------
				}
			}
			
		}else{
			miss++;
			//coherency counting logic
			if(!(coherent_table.find(baddr)==coherent_table.end())){// if baddr is present in the coherent table.
				coherent_miss++;
				//removing the entry from the coherent_table
				coherent_table.erase(baddr);
			}
			




			//now we have to bring this into the cache.
			if(set[setno]->current_count < set[setno]->assosiativity){
				Block b=Block(baddr,S);
				if(operation==READ){
					if(bus->share_handler(baddr,core_id)){
						//there is sharing
						b.state=S;
					}else{
						//there is no sharing
						b.state=E;
					}
					//----------
					bus->touch(baddr,operation);
					set[setno]->lowlevel_insert(b,operation);

				}else{
					//the operation is write
					b.state=M;
					bus->touch(baddr,operation);
					bus->inform_handler(baddr,core_id);
					set[setno]->lowlevel_insert(b,operation);
				}
			}else{
				Block lru_block=set[setno]->find_lru();
				set[setno]->remove_lru();
				l1->invalid(lru_block.baddress);
				Block b=Block(baddr,S);
				if(operation==READ){
					
					//---------
					if(bus->share_handler(baddr,core_id)){
						//there is sharing
						b.state=S;
					}else{
						//there is no sharing
						b.state=E;
					}
					//----------
					bus->touch(baddr,operation);
					set[setno]->lowlevel_insert(b,operation);

				}else{
					//the operation is write
					b.state=M;
					bus->touch(baddr,operation);
					bus->inform_handler(baddr,core_id);
					set[setno]->lowlevel_insert(b,operation);
				}
				
			}
		}//miss condition ends
	}//method ends.  //i think it is correct.