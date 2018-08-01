#include <string>
#include <eosiolib/crypto.h>
#include "Random.hpp"

using namespace std;
using namespace eosio;

Random::Random(account_name self)
:contract(self),requests(_self,_self),seeds(_self,_self),config(_self,_self)
{
}

void Random::setmatchnum(uint64_t num)
{
	//set the minimum random seed needed to generate a random number
	require_auth(_self);
	auto config_exists = config.find(_self);
	if(config.end() == config_exists)
		//if this is the first time to setup configure
		config.emplace(_self,[&](Config & c) {
			c.owner = _self;
			c.require_matched_num = num;
		});
	else
		//if this is not the first time
		config.modify(config_exists,_self,[&](Config & c) {
			c.require_matched_num = num;
		});
}

void Random::sendhash(account_name owner, string hash)
{
	//send the hash of the coming random seed
	require_auth(owner);
	auto config_exists = config.find(_self);
	eosio_assert(config.end() != config_exists,"you must call setmatchnum() first!");
	auto seed_exists = seeds.find(owner);
	if(seeds.end() == seed_exists) {
		//if the owner is a new comer
		seeds.emplace(_self,[&](Seed & s) {
			s.owner = owner;
			s.hash = hash;
			s.seed = 0;
			s.accepted = false;
		});
	} else {
		//if the owner is an acquainted
		seeds.modify(seed_exists,_self,[&](Seed & s) {
			s.hash = hash;
			s.seed = 0;
			s.accepted = false;
		});
	}
}

void Random::sendseed(account_name owner, int64_t seed)
{
	require_auth(owner);
	auto config_exists = config.find(_self);
	eosio_assert(config.end() != config_exists,"you must call setmatchnum() first!");
	auto seed_exists = seeds.find(owner);
	eosio_assert(seeds.end() != seed_exists,"you must call sendhash() first!");
	//check whether the seed can match with the hash
	string hash = to_sha256(seed);
	if(hash != seed_exists->hash) {
		//erase the existing seed
		return;
	} else {
		//accept the random seed
		seeds.modify(seed_exists,_self,[&](Seed & s) {
			s.seed = seed;
			s.accepted = true;
		});
	}
	//count how many seeds are accepted
	int match_count = 0;
	for(auto & s : seeds) 
		if(s.accepted) match_count++;
	//if enough seeds with their correct hash have arrived
	if(match_count >= config_exists->require_matched_num) {
		//answer to requests
#ifndef NDEBUG
		print("we got enough seeds (",match_count,") matching to their hashes! gonna send random number...");
#endif
		//apply all seeds and delete them
		int64_t seed = 0;
		for(auto it = seeds.cbegin() ; it != seeds.cend() ; ) {
			//only adopt accepted seeds
			if(it->accepted) seed += it->seed;
			it = seeds.erase(it);
		}
		//calculate a random number according to seeds
		checksum256 cs = {0};
		string ascii = to_string(seed);
		sha256(const_cast<char*>(ascii.data()),ascii.size(),&cs);
		int64_t random = (((int64_t)cs.hash[0] << 56 ) & 0xFF00000000000000U) 
				|  (((int64_t)cs.hash[1] << 48 ) & 0x00FF000000000000U)
				|  (((int64_t)cs.hash[2] << 40 ) & 0x0000FF0000000000U)
				|  (((int64_t)cs.hash[3] << 32 ) & 0x000000FF00000000U)
				|  ((cs.hash[4] << 24 ) & 0x00000000FF000000U)
				|  ((cs.hash[5] << 16 ) & 0x0000000000FF0000U)
				|  ((cs.hash[6] << 8 ) & 0x000000000000FF00U)
				|  (cs.hash[7] & 0x00000000000000FFU);
		//answer to random number requests
		for(auto it = requests.cbegin() ; it != requests.cend() ; ) {
			if(current_time() - it->timestamp >= 3000) {
				//if the request is overdue, it must be answered immediately
				action(
					permission_level{_self,N(active)},
					it->owner,N(it->action),
					std::make_tuple(it->index,random)
				).send();
				//remove answered request
				it = requests.erase(it);
			} else it++;
		}//end for
	}//end if
}

void Random::request(account_name owner,uint64_t index, string handler)
{
	require_auth(owner);
	requests.emplace(_self,[&](auto & r) {
		r.req_key = requests.available_primary_key();
		r.owner = owner;
		r.action = handler;
		r.index = index;
		r.timestamp = current_time();
	});
}

string Random::to_sha256(int64_t word)
{
	checksum256 cs = {0};
	string ascii = to_string(word);
	sha256(const_cast<char*>(ascii.data()),ascii.size(),&cs);
	string retval;
	for(int i = 0 ; i < sizeof(cs.hash) ; i++) {
		char hex[3] = { 0 };
		snprintf(hex, sizeof(hex), "%02x", static_cast<unsigned char>(cs.hash[i]));
		retval += hex;
	}
	return retval;
}

EOSIO_ABI(Random,(setmatchnum)(sendhash)(sendseed)(request))

