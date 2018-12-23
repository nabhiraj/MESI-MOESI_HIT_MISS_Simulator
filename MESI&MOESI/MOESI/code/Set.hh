class Set{
public:
	int cache_type;
	int assosiativity;
	int current_count;
	list<Block> lrulist;
	Set(int assosiativity,int c){
		this->assosiativity=assosiativity;
		current_count=0;
		cache_type=c;
	}

	
	void lowlevel_insert(Block b,int operation){
		if(current_count<assosiativity){
			lrulist.push_front(b);
			current_count++;
		}else{
			cout<<"something is wrongy \n";
		}
	}
	void touch(unsigned long long baddr,int operation){
		list<Block> :: iterator it;
		for(it=lrulist.begin();it!=lrulist.end();it++){
			if(it->baddress==baddr){
				Block temp=*it;
				lrulist.erase (it);
				current_count--;
				lowlevel_insert(temp,operation);
				break;
			}
		}
	}
	

	bool is_there(unsigned long long baddr){
		list<Block> :: iterator it;
		for(it=lrulist.begin();it!=lrulist.end();it++){
			if(it->baddress==baddr){
				return true;
			}
		}
		return false;
	}

	
	Block* is_there_ptr(unsigned long long baddr){
		list<Block> :: iterator it;
		for(it=lrulist.begin();it!=lrulist.end();it++){
			if(it->baddress==baddr){
				return &(*it);
			}
		}
		return NULL;
	}

	
	Block find_lru(){
		if(current_count > 0){
		return lrulist.back();
		}else{
		cout<<"there is no victim right now\n";
			}
	}
	
	void remove_lru(){
		lrulist.pop_back();
		current_count--;
		if(current_count<0){
			cout<<"something is wrong whith popping\n";
		}
	}


	
	void remove(unsigned long long baddr){
		list<Block> :: iterator it;
		//getting out side for loop after erasing created some problem i suppose
		for(it=lrulist.begin();it!=lrulist.end();it++){
			if(it->baddress==baddr){
				lrulist.erase (it);
				current_count--;
				break;
			}
		}
	}



	
	Block is_there_l2(unsigned long long baddr){
		list<Block> :: iterator it;
		for(it=lrulist.begin();it!=lrulist.end();it++){
			//cout<<"looping.......\n";
			if(it->baddress==baddr){
				//cout<<"match fount \n";
				return *it;//this will so to stack
			}
	}
	return Block(0,NO_STATE);//this will go to stack
}
};