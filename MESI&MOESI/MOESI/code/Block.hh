
class Block{
public:
	unsigned long long baddress;
	//int dirty;
	int state;

	Block(unsigned long long baddr,int s){
		baddress=baddr;
		state=s;
	}
};