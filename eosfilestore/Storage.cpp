#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/process.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/shared_ptr.hpp>
#include <crypto++/base64.h>
#include "Storage.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;
using namespace boost::process;
using namespace boost::property_tree;
using namespace CryptoPP;

unsigned int Storage::maxPayloadSize = 10000;

Storage::Storage(string addr, unsigned short p, string waddr, unsigned short wp, string contract_account)
:node_addr(addr),node_port(p),wallet_addr(waddr),wallet_port(wp),contract(contract_account)
{
}

Storage::~Storage()
{
}

bool Storage::get(vector<string> txs,string filename)
{
	std::ofstream out(filename + ".zip",ios::binary);
	if(false == out.is_open()) {
		cerr << "can't open output file!"<<endl;
		return false;
	}
	//1)download the chunks and decode with base64
	for(auto & t : txs) {
		//fetch chunk from a transaction 
		bool flag;
		string retval,encoded,decoded;
		boost::tie(flag,encoded) = fetchTx(t);
		if(false == flag) return false;
		//decode base64 into bytearray
		Base64Decoder decoder;
		decoder.Put((byte*)encoded.data(),encoded.size());
		decoder.MessageEnd();
		word64 size = decoder.MaxRetrievable();
		if(size) {
			decoded.resize(size);
			decoder.Get((byte*)decoded.data(),decoded.size());
		}
		//write into file
		out.write(decoded.data(),decoded.size());
	}
	out.close();
	//2)decompress file
	decompress(filename + ".zip",filename,false);
	return true;
}

boost::tuple<bool,vector<string> > Storage::push(string filename)
{
	//1)compress file
	bool retval = compress(filename,filename + ".zip",false);
	if(false == retval) return boost::make_tuple(false,vector<string>());
	//2)slice into chunks and encode with base64
	std::ifstream in(filename + ".zip",ios::binary);
	if(false == in.is_open()) return boost::make_tuple(false,vector<string>());
	int nchunks = ceil(static_cast<float>(file_size(filename + ".zip")) / maxPayloadSize);
	vector<boost::tuple<int,long> > offsets;
	for(int i = 0 ; i < nchunks ; i++) offsets.push_back(boost::make_tuple(i,i * maxPayloadSize));
	random_shuffle(offsets.begin(),offsets.end());
	string decoded(maxPayloadSize,0);
	vector<boost::tuple<int,string> > sequence;
	for(auto & offset : offsets) {
		in.seekg(boost::get<1>(offset),ios::beg);
		//read binary data
		in.read(&decoded[0],maxPayloadSize);
		//encode with base64
		string encoded;
		Base64Encoder encoder;
		encoder.Put((byte*)decoded.data(),decoded.size());
		encoder.MessageEnd();
		word64 size = encoder.MaxRetrievable();
		if(size) {
			encoded.resize(size);
			encoder.Get((byte*)encoded.data(),encoded.size());
		}
		//remove all end of line character
		encoded.erase(remove(encoded.begin(),encoded.end(),'\n'),encoded.end());
		string txid;
		do {
			//send data through abi
			txid = pushAction(contract,"upload",string("{\"chunk\":\"") + encoded + "\"}",contract);
		} while(txid.empty());
		//save the txid
		sequence.push_back(boost::make_tuple(boost::get<0>(offset),txid));
	}
	//sort the sequence according to the index of the chunks
	sort(sequence.begin(),sequence.end(),[&](const boost::tuple<int,string> & a,const boost::tuple<int,string> & b)->bool {
		return boost::get<0>(a) < boost::get<0>(b);
	});
	vector<string> txs;
	for(auto & s : sequence) txs.push_back(boost::get<1>(s));
	return boost::make_tuple(true,txs);
}

bool Storage::compress(string input,string output,bool delorig)
{
	//prepare the zip filter
	filtering_ostream out;
	out.push(zlib_compressor());
	out.push(file_sink(output));
	//open the input file
	std::ifstream in(input,ios::binary);
	if(false == in.is_open()) {
		cerr<<"can't open the specified file!"<<endl;
		return false;
	}
	//generate the output file
	copy(istreambuf_iterator<char>(in),istreambuf_iterator<char>(),ostreambuf_iterator<char>(out));
	if(delorig) remove_all(input);
	return true;
}

bool Storage::decompress(string input,string output,bool delorig)
{
	//prepare the zip filter
	filtering_ostream out;
	out.push(zlib_decompressor());
	out.push(file_sink(output));
	//open the input file
	std::ifstream in(input,ios::binary);
	if(false == in.is_open()) {
		cerr<<"can't open the specified file!"<<endl;
		return false;
	}
	//generate the output file
	copy(istreambuf_iterator<char>(in),istreambuf_iterator<char>(),ostreambuf_iterator<char>(out));
	if(delorig) remove_all(input);
	return true;
}

boost::tuple<bool,string> Storage::fetchTx(string txid)
{
	//get transaction content
	vector<string> arguments;
	arguments.push_back("--print-response");
	arguments.push_back("-u");
	arguments.push_back("http://" + node_addr + ":" + lexical_cast<string>(node_port));
	arguments.push_back("get");
	arguments.push_back("transaction");
	arguments.push_back(txid);
	ipstream out;
	child c(search_path("cleos"),arguments,std_out > out,std_err > null);
	string json, line;
	while(c.running() && getline(out,line)) json += line;
	c.wait();
	if(EXIT_SUCCESS != c.exit_code()) {
		return boost::make_tuple(false,"");
	}
#ifndef NDEBUG
	cout<<"fetching "<<txid<<endl;
#endif
	//parse the json
	ptree pt;
	stringstream ss(json);
	read_json(ss,pt);
	ptree & data = pt.get_child("trx").get_child("trx").get_child("actions").begin()->second.get_child("data");
	string chunk = data.get_child("chunk").get_value<string>();
	return boost::make_tuple(true,chunk);
}

string Storage::pushAction(string contract,string action,string json,string account)
{
	vector<string> arguments;
	arguments.push_back("--print-response");
	arguments.push_back("push");
	arguments.push_back("action");
	arguments.push_back(contract);
	arguments.push_back(action);
	arguments.push_back(json);
	arguments.push_back("-p");
	arguments.push_back(account);
	ipstream out;
	child c(search_path("cleos"),std_err > out,std_out > null);
	string line;
	regex trx_id("\"trx_id\":\\s*\"(.*)\"");
	match_results<string::const_iterator> what;
	string txid;
	while(getline(out,line)) {
		if(regex_search(line,what,trx_id)) {
			txid = string(what[1].begin(),what[1].end());
			break;
		}
	}
	c.wait();
	return txid;
}

