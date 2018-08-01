#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <string>
#include <boost/tuple/tuple.hpp>

using namespace std;
using namespace boost;

class Storage {
	static unsigned int maxPayloadSize;
	
	string node_addr;
	unsigned short node_port;
	string wallet_addr;
	unsigned short wallet_port;
	string contract;
public:
	Storage(string node_addr = "localhost", unsigned short node_port = 8000, string wallet_addr = "localhost", unsigned short wallet_port = 6666, string contract_account = "eosfilestore");
	virtual ~Storage();
	bool get(vector<string> txs,string filename);
	boost::tuple<bool,vector<string> > push(string filename);
protected:
	bool compress(string input,string output,bool delorigin = false);
	bool decompress(string input,string output,bool delorigin = false);
	boost::tuple<bool,string> fetchTx(string txid);
	string pushAction(string contract,string action,string json,string account);
};

#endif
