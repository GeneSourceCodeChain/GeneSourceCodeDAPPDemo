#ifndef WRAPPER_HPP
#define WRAPPER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/random.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using namespace std;
using namespace boost::random;
using namespace boost::property_tree;

class Wrapper {
	const string cleos_cmd;
	const vector<string> args;
	string password;
	boost::mt19937 rng;
	boost::variate_generator<boost::mt19937&,boost::random::uniform_int_distribution<long> > unifrand;

protected:
	string nextaccount(string account);
	float get_balance(string account);
	ptree getTable(string contract,string scope,string tablename,int from,int to);
	ptree getTableAll(string contract,string scope,string tablename,string keydomain);
	ptree getAction(string account,unsigned int from,unsigned int to);
	ptree getActionAll(string account);
	string pushAction(string contract,string action,string json,string account);
	ptree _getAdvRuleTable();
	ptree _getEvoRuleTable();
	ptree _getAttrRuleTable();
public:
	Wrapper(string addr,unsigned short port,string waddr,unsigned short wport);
	string create_account(string account);
	string get_account_possessions(string account);
	string get_pending_orders(int sale_key = -1);
	string add_pending_order(string account,string price,int type,int id);
	string transfer(string from,string to,string quantity,string memo);
	string revoke_order(int sale_id);
	string adventure(string account,int elf_id,int adv_id,unsigned long random);
	string feed(string account,int elf_id,int gene_id);
	string evolve(string account,int elf_id,unsigned long random);
	string synthesize(string account,int elf1_id,int elf2_id,unsigned long random);
	string newelf(string account);
	string checkTrx(string txid);
	string addAdvRule(unsigned int required_level,string price,unsigned int min_level_reward,unsigned int max_level_reward,vector<boost::tuple<unsigned int,unsigned int> > bonus_genes);
	string addEvoRule(vector<unsigned int> required_level,unsigned int evolve_to,unsigned int power,vector<unsigned int> bonus_genes);
	string addAttrRule(unsigned int genre,unsigned int capthresh);
	string getActions(string account,int from,int to);
	string getAdvRuleTable();
	string getEvoRuleTable();
	string getAttrRuleTable();
	string addAdvRule(string advrule);
	string addEvoRule(string evorule);
	string addAttrRule(string attrrule);
	string rmAdvRule(unsigned int adv_key);
	string rmEvoRule(unsigned int evo_key);
	string rmAttrRule(unsigned int attr_key);
	string updateAdvRule(string advrule);
	string updateEvoRule(string evorule);
	string updateAttrRule(string attrrule);
	string setStatus(string account,int elf_id,int value);
	string removeElf(string account,int elf_id);
	string reset(string account);
};

#endif

