#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <vector>
#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>

using namespace std;
using namespace eosio;

class Random : public eosio::contract {
	//@abi table reqtable i64
	struct Request {
		uint64_t req_key;
		account_name owner;
		string action;
		uint64_t index;
		uint64_t timestamp;
		
		uint64_t primary_key() const {return req_key;}
		EOSLIB_SERIALIZE(Request,(req_key)(owner)(action)(index)(timestamp))
	};

	//@abi table seedtable i64
	struct Seed {
		account_name owner;
		string hash;
		uint64_t seed;
		bool accepted;
		
		account_name primary_key() const {return owner;}
		EOSLIB_SERIALIZE(Seed,(owner)(hash)(seed)(accepted))
	};

	//@abi table conftable i64
	struct Config {
		account_name owner;
		uint64_t require_matched_num;

		uint64_t primary_key() const {return owner;}
		EOSLIB_SERIALIZE(Config,(owner)(require_matched_num))
	};

	//tables
	using reqtable = multi_index<N(reqtable),Request>;
	using seedtable = multi_index<N(seedtable),Seed>;
	using conftable = multi_index<N(conftable),Config>;
	
	//member objects
	reqtable requests;
	seedtable seeds;
	conftable config;
	
protected:
	string to_sha256(uint64_t);
	
public:
	Random(account_name name);
	//@abi action
	void setmatchnum(uint64_t num);
	//@abi action
	void sendhash(account_name owner, string hash);
	//@abi action
	void sendseed(account_name owner, uint64_t seed);
	//@abi action
	void reqest(account_name owner,uint64_t index, string handler = "getrandom");
};

#endif
