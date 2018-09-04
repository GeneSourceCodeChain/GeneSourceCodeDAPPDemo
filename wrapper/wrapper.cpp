#include <stdexcept>
#include <sstream>
#include <fstream>
#include <limits>
#include <boost/lexical_cast.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include "wrapper.hpp"

using namespace std;
using namespace boost;
using namespace boost::process;

Wrapper::Wrapper(string addr,unsigned short port,string waddr,unsigned short wport)
:cleos_cmd(search_path("cleos").string())
,args({
	"--url",addr + ":" + lexical_cast<string>(port),
	"--wallet-url",waddr + ":" + lexical_cast<string>(wport)
}),
rng(time(0)),
unifrand(rng,boost::random::uniform_int_distribution<long>(0,numeric_limits<long>::max()))
{
	regex wallet("\\s*\"wrapper\\s*(\\*)?\"\\s*");
	regex psw("^\"(.*)\"$");
	//check wether the wrapper wallet exists
	vector<string> arguments1(args);
	arguments1.push_back("wallet");
	arguments1.push_back("list");
	ipstream out;
	child c1(cleos_cmd,arguments1,std_out > out);
	string line;
	match_results<string::const_iterator> what;
	while(getline(out,line))
		if(regex_match(line,what,wallet)) {
			string matched = string(what[1].begin(),what[1].end());
			//check if the existed wallet is opened;
			if(matched == "*") {
				return;
			} else {
				//try to open the wallet
				std::ifstream in("password.txt");
				string password;
				in >> password;
				vector<string> arguments(args);
				arguments.push_back("wallet");
				arguments.push_back("unlock");
				arguments.push_back("-n");
				arguments.push_back("wrapper");
				arguments.push_back("--password");
				arguments.push_back(password);
				child c(cleos_cmd,arguments);
				c.wait();
				return;
			}
		}
	c1.wait();
	//create a wallet for wrapper
	vector<string> arguments2(args);
	arguments2.push_back("wallet");
	arguments2.push_back("create");
	arguments2.push_back("-n");
	arguments2.push_back("wrapper");
	arguments2.push_back("--file");
	arguments2.push_back("wallet.txt");
	child c2(cleos_cmd,arguments2,std_out > out);
	while(getline(out,line))
		if(regex_match(line,what,psw)) {
			password = string(what[1].begin(),what[1].end());
			break;
		}
	c2.wait();
	std::ofstream backup("password.txt");
	backup<<password<<endl;
	if(EXIT_SUCCESS != c2.exit_code()) throw runtime_error("failed to launch cleos!");
	if(password.empty()) throw runtime_error("failed to create a wallet for wrapper!");
}

string Wrapper::create_account(string account)
{
	regex privatekey_expr("Private key:\\s*(.*)");
	regex publickey_expr("Public key:\\s*(.*)");
	//create key pairs
	vector<string> arguments(args);
	arguments.push_back("create");
	arguments.push_back("key");
	arguments.push_back("--to-console");
	ipstream out;
	child c(cleos_cmd,arguments,std_out > out);
	string line,publickey,privatekey;
	match_results<string::const_iterator> what;
	while(c.running() && getline(out,line)) {
		if(regex_match(line,what,privatekey_expr)) privatekey = string(what[1].begin(),what[1].end());
		if(regex_match(line,what,publickey_expr)) publickey = string(what[1].begin(),what[1].end());
	}
	c.wait();
	if(publickey.empty() || privatekey.empty()) {
		ptree retval;
		retval.put("status","failure");
		ptree body;
		body.put("key","");
		retval.add_child("body",body);
		stringstream buffer;
		write_json(buffer,retval);
		return buffer.str();
	} else {
		//import key
		vector<string> arguments1(args);
		arguments1.push_back("wallet");
		arguments1.push_back("import");
		arguments1.push_back("--private-key");
		arguments1.push_back(privatekey);
		arguments1.push_back("-n");
		arguments1.push_back("wrapper");
		child c1(cleos_cmd,arguments1,std_out > null,std_err > null);
		c1.wait();
		if(EXIT_SUCCESS != c1.exit_code()) {
			ptree retval;
			retval.put("status","failure");
			ptree body;
			body.put("tx_hash","");
			retval.add_child("body",body);
			stringstream buffer;
			write_json(buffer,retval);
			return buffer.str();
		}
		//create account
		vector<string> arguments2(args);
		arguments2.push_back("--print-response");
		arguments2.push_back("system");
		arguments2.push_back("newaccount");
		arguments2.push_back("eosio.stake");
		arguments2.push_back(account);
		arguments2.push_back(publickey);
		arguments2.push_back("--stake-cpu");
		arguments2.push_back("1000.0000 SYS");
		arguments2.push_back("--stake-net");
		arguments2.push_back("1000.0000 SYS");
		arguments2.push_back("--buy-ram");
		arguments2.push_back("1000.0000 SYS");
		arguments2.push_back("-p");
		arguments2.push_back("eosio.stake");
		ipstream out;
		child c2(cleos_cmd,arguments2,std_err > out,std_out > null);
		regex trx_id("\"trx_id\":\\s*\"(.*)\"");
		match_results<string::const_iterator> what;
		string line;
		string txid;
		while(getline(out,line))
			if(regex_search(line,what,trx_id)) {
				txid = string(what[1].begin(),what[1].end());
				break;
			}
		c2.wait();
		//generate retval
		ptree retval;
		retval.put("status",((EXIT_SUCCESS == c2.exit_code())?"success":"failure"));
		ptree body;
		body.put("tx_hash",txid);
		retval.add_child("body",body);
		stringstream buffer;
		write_json(buffer,retval);
		return buffer.str();
	}
}

float Wrapper::get_balance(string account)
{
	vector<string> arguments(args);
	arguments.push_back("get");
	arguments.push_back("currency");
	arguments.push_back("balance");
	arguments.push_back("eosio.token");
	arguments.push_back(account);
	arguments.push_back("GENE");
	ipstream out;
	child c(cleos_cmd,arguments,std_out > out,std_err > null);
	string line;
	regex balance("([0-9\\.]+)\\s+GENE");
	match_results<string::const_iterator> what;
	while(c.running() && getline(out,line))
		if(regex_match(line,what,balance)) 
			return lexical_cast<float>(string(what[1].begin(),what[1].end()));
	return 0;
}

ptree Wrapper::getTable(string contract,string scope,string tablename,int from,int to)
{
	if(from != -1 && to != -1) {
		assert(from < to);
		assert(to - from <= 10);
	}
	vector<string> arguments(args);
	arguments.push_back("--print-response");
	arguments.push_back("get");
	arguments.push_back("table");
	if(from >= 0) {
		arguments.push_back("-L");
		arguments.push_back(lexical_cast<string>(from));
	}
	if(to >= 0) {
		arguments.push_back("-U");
		arguments.push_back(lexical_cast<string>(to));
	}
	arguments.push_back(contract);
	arguments.push_back(scope);
	arguments.push_back(tablename);
	ipstream out;
	child c(cleos_cmd,arguments,std_out > out,std_err > null);
	string line,json;
	while(getline(out,line)) json += line;
	c.wait();
	stringstream ss(json);
	ptree retval;
	read_json(ss,retval);
	return retval;
}

ptree Wrapper::getAction(string account,unsigned int from,unsigned int to)
{
	assert(from < to);
	assert(to - from <= 10);
	vector<string> arguments(args);
	arguments.push_back("--print-response");
	arguments.push_back("get");
	arguments.push_back("actions");
	arguments.push_back(account);
	arguments.push_back(lexical_cast<string>(from));
	arguments.push_back(lexical_cast<string>(to - 1));
	ipstream out;
	child c(cleos_cmd,arguments,std_err > out,std_out > null);
	string line,json;
	getline(out,line); //consume one line
	getline(out,line); //consume two lines
	while(getline(out,line) && line != "---------------------") json += line;
	c.wait();
	stringstream ss(json);
	ptree retval;
	read_json(ss,retval);
	return retval;
}

ptree Wrapper::getTableAll(string contract,string scope,string tablename,string keydomain)
{
	bool more = true;
	ptree rows;
	for(int start = -1 ; more ;) {
		ptree retval = getTable(contract,scope,tablename,start,-1);
		for(auto & row : retval.get_child("rows")) rows.push_back(row);
		more = retval.get_child("more").get_value<bool>();
		if(more) start = 1 + retval.get_child("rows").rbegin()->second.get_child(keydomain).get_value<int>();
	}
	ptree retval_all;
	retval_all.add_child("rows",rows);
	return retval_all;
}

ptree Wrapper::getActionAll(string account)
{
	bool more = true;
	ptree actions;
	for(int start = 0 ; more ; start += 10) {
		ptree retval = getAction(account,start,start + 9);
		for(auto & action : retval.get_child("actions")) actions.push_back(action);
		more = (retval.get_child("actions").size())?true:false;
	}
	ptree retval_all;
	retval_all.add_child("actions",actions);
	return retval_all;
}

string Wrapper::getActions(string account,int from,int to)
{
	ptree actions = getAction(account,from,to);
	//return 
	ptree retval;
	retval.put("status","success");
	retval.add_child("body",actions);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::pushAction(string contract,string action,string json,string account)
{
	vector<string> arguments(args);
	arguments.push_back("--print-response");
	arguments.push_back("push");
	arguments.push_back("action");
	arguments.push_back(contract);
	arguments.push_back(action);
	arguments.push_back(json);
	arguments.push_back("-p");
	arguments.push_back(account);
	ipstream out;
	child c(cleos_cmd,arguments,std_err > out,std_out > null);
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

string Wrapper::get_account_possessions(string account)
{
	float balance = get_balance(account);
	//get elf list
	ptree elfs = getTableAll("geneminepool",account,"elftable","elf_key");
	ptree & elfs_list = elfs.get_child("rows");
	//get gene list
	ptree genes = getTableAll("geneminepool",account,"genetable","gene_key");
	ptree & genes_list = genes.get_child("rows");
	//assemble retval
	ptree retval;
	retval.put("status","success");
	ptree body;
	body.put("balance",balance);
	body.add_child("fairy_list",elfs_list);
	body.add_child("gene_list",genes_list);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::get_pending_orders(int sale_key)
{
	//get order list
	ptree orders = getTableAll("geneminepool","geneminepool","saletable","sale_key");
	//loop over order list
	map<string,ptree> genes;
	map<string,ptree> elfs;
	ptree retval;
	retval.put("status","success");
	ptree fairy_list;
	ptree gene_list;
	for(auto & row : orders.get_child("rows")) {
		ptree & order = row.second;
		//if the filter is used
		if(sale_key != -1 && sale_key != order.get_child("sale_key").get_value<int>()) continue;
		switch(order.get_child("type").get_value<int>()) {
			case 0:
				{
					map<string,ptree>::iterator which;
					string account = order.get_child("player").get_value<string>();
					//check whether the account's elfs list is bufferred
					if(elfs.end() == (which = elfs.find(account))) {
						ptree elf = getTableAll("geneminepool",account,"elftable","elf_key");
						elfs.insert(make_pair(account,elf));
						which = elfs.find(account);
#ifndef NDEBUG
						assert(elfs.end() != which);
#endif
					}
					//locate the elf of the current order from account's elf list
					ptree elf;
					for(auto & e : which->second.get_child("rows")) {
						if(order.get_child("item_key").get_value<int>() == e.second.get_child("elf_key").get_value<int>()) {
							elf = e.second;
							break;
						}
					}

					ptree fairy;
					fairy.put("fairy_id",elf.get_child("elf_key").get_value<int>());
					fairy.put("u_key",account);
					fairy.put("genre",elf.get_child("genre").get_value<int>());
					fairy.put("level",elf.get_child("level").get_value<int>());
					fairy.add_child("dna",elf.get_child("dna"));
					fairy.add_child("appearance",elf.get_child("appearance"));
					fairy.add_child("capabilities",elf.get_child("capabilities"));
					fairy.add_child("bgr",elf.get_child("bgr"));
					fairy.add_child("skill",elf.get_child("skills"));
					fairy.put("last_evolve_level",0);
					fairy.put("evolve_count",elf.get_child("evolve_count").get_value<int>());
					fairy.put("value",order.get_child("price").get_value<string>());
					fairy.put("order_id",order.get_child("sale_key").get_value<int>());
					fairy.put("created_at",elf.get_child("birth_datetime").get_value<int>());
					fairy.put("updated_at",
						fmax(
							elf.get_child("last_feed_datetime").get_value<int>(),
							fmax(
								elf.get_child("last_adventure_datetime").get_value<int>(),
								elf.get_child("last_evolve_datetime").get_value<int>()
							)
						)
					);
					ptree item;
					fairy_list.push_back(make_pair("",fairy));
				}
				break;
			case 1:
				{
					map<string,ptree>::iterator which;
					string account = order.get_child("player").get_value<string>();
					//check whether the account's genes list is bufferred
					if(genes.end() == (which = genes.find(account))) {
						ptree gene = getTableAll("geneminepool",account,"genetable","gene_key");
						genes.insert(make_pair(account,gene));
						which = genes.find(account);
#ifndef NDEBUG
						assert(genes.end() != which);
#endif
					}
					ptree gene;
					for(auto & g : which->second.get_child("rows")) {
						if(order.get_child("item_key").get_value<int>() == g.second.get_child("gene_key").get_value<int>()) {
							gene = g.second;
							break;
						}
					}

					ptree giyin;
					giyin.put("gene_id",gene.get_child("gene_key").get_value<int>());
					giyin.put("u_key",account);
					giyin.put("segment_id",gene.get_child("segment_id").get_value<int>());
					giyin.add_child("delta_appearance",gene.get_child("delta_appearance"));
					giyin.add_child("delta_capabilities",gene.get_child("delta_capabilities"));
					giyin.add_child("delta_bgr",gene.get_child("delta_bgr"));
					giyin.add_child("delta_skill",gene.get_child("delta_skills"));
					giyin.put("value",order.get_child("price").get_value<string>());
					giyin.put("order_id",order.get_child("sale_key").get_value<int>());
					giyin.put("created_at",gene.get_child("create_datetime").get_value<int>());
					giyin.put("updated_at",0);
					gene_list.push_back(make_pair("",giyin));
				}
				break;
			default:
				assert("unknown type of item");
		}//end switch
	}//end for
	ptree body;
	body.add_child("fairy_list",fairy_list);
	body.add_child("gene_list",gene_list);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::add_pending_order(string account,string price,int type,int id)
{
	string txid = pushAction(
		"geneminepool",
		"sell",
		"[\"" + account + "\"," + lexical_cast<string>(type) + "," + lexical_cast<string>(id) + ",\"" + price + "\"]",
		account
	);
	ptree retval;
	retval.put("status","success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::transfer(string from,string to,string quantity,string memo)
{
	string txid = pushAction(
		"eosio.token",
		"transfer",
		"{\"from\":\"" + from + "\",\"to\":\"" + to + "\",\"quantity\":\"" + quantity + "\",\"memo\":\"" + memo + "\"}",
		from
	);
	ptree retval;
	retval.put("status",(txid != "")?"success":"failure");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::revoke_order(int sale_id)
{
	//get the owner
	ptree orders = getTable("geneminepool","geneminepool","saletable",sale_id,sale_id + 1);
	int size = distance(orders.get_child("rows").begin(),orders.get_child("rows").end());
	if(size != 1) {
		ptree retval;
		retval.put("status","failure");
		ptree body;
		body.put("tx_hash","");
		retval.add_child("body",body);
		stringstream buffer;
		write_json(buffer,retval);
		return buffer.str();
	}
	ptree & order = orders.get_child("rows").begin()->second;
	string account = order.get_child("player").get_value<string>();
	//redeem the order
	string txid = pushAction(
		"geneminepool",
		"redeem",
		"[\"" + account + "\"," + lexical_cast<string>(sale_id) + "]",
		account
	);
	//return
	ptree retval;
	retval.put("status","success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

ptree Wrapper::_getAdvRuleTable()
{
	return getTableAll("geneminepool","geneminepool","advruletable","adv_key");
}

ptree Wrapper::_getEvoRuleTable()
{
	return getTableAll("geneminepool","geneminepool","evoruletable","evo_key");
}

ptree Wrapper::_getAttrRuleTable()
{
	return getTableAll("geneminepool","geneminepool","attruletable","attr_key");
}

string Wrapper::addAdvRule(unsigned int required_level,string price,unsigned int min_level_reward,unsigned int max_level_reward,vector<boost::tuple<unsigned int,unsigned int> > bonus_genes)
{
	string genes;
	string powers;
	for(int i = 0 ; i < bonus_genes.size() ; i++) {
		genes += lexical_cast<string>(get<0>(bonus_genes[i])) + ((i == bonus_genes.size() - 1)?"":",");
		powers += lexical_cast<string>(get<1>(bonus_genes[i])) + ((i == bonus_genes.size() - 1)?"":",");
	}
	string txid = pushAction(
		"geneminepool",
		"addadvrule",
		string("[\"") + lexical_cast<string>(required_level) + "\",\"" + price + "\"," + lexical_cast<string>(min_level_reward) + "," + lexical_cast<string>(max_level_reward) + ",[" + genes + "],[" + powers + "]]",
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status","success");
	ptree body;
	body.put("tx_bash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::addEvoRule(vector<unsigned int> required_gene,unsigned int evolve_to,unsigned int power,vector<unsigned int> bonus_genes)
{
	string genes;
	string bonus;
	for(int i = 0 ; i < required_gene.size() ; i++) {
		genes += lexical_cast<string>(required_gene[i]) + ((i == required_gene.size() - 1)?"":",");
	}
	for(int i = 0 ; i < bonus_genes.size() ; i++) {
		bonus += lexical_cast<string>(bonus[i]) + ((i == bonus.size() - 1)?"":",");
	}
	string txid = pushAction(
		"geneminepool",
		"addevorule",
		"[[" + genes + "]," + lexical_cast<string>(evolve_to) + "," + lexical_cast<string>(power) + ",[" + bonus + "]]",
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status","success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::addAttrRule(unsigned int genre,unsigned int capthresh)
{
	string txid = pushAction(
		"geneminepool",
		"addattrrule",
		"[" + lexical_cast<string>(genre) + "," + lexical_cast<string>(capthresh) + "]",
		"geneminepool"
	);
	//return 
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::addAttrRule(string attrrule)
{
	string txid = pushAction(
		"geneminepool",
		"addattrrule",
		attrrule,
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::getAdvRuleTable()
{
	stringstream buffer;
	ptree advruletable = _getAdvRuleTable();
	write_json(buffer,advruletable);
	return buffer.str();
}

string Wrapper::getEvoRuleTable()
{
	stringstream buffer;
	ptree evoruletable = _getEvoRuleTable();
	write_json(buffer,evoruletable);
	return buffer.str();
}

string Wrapper::getAttrRuleTable()
{
	stringstream buffer;
	ptree attrruletable = _getAttrRuleTable();
	write_json(buffer,attrruletable);
	return buffer.str();
}

string Wrapper::addAdvRule(string advrule)
{
	string txid = pushAction(
		"geneminepool",
		"addadvrule",
		advrule,
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::rmAdvRule(unsigned int adv_key)
{
	string txid = pushAction(
		"geneminepool",
		"rmadvrule",
		string("[") + lexical_cast<string>(adv_key) + "]",
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::rmAttrRule(unsigned int attr_key)
{
	string txid = pushAction(
		"geneminepool",
		"rmattrrule",
		"[" + lexical_cast<string>(attr_key) + "]",
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::updateAdvRule(string advrule)
{
	string txid = pushAction(
		"geneminepool",
		"updateadvrule",
		advrule,
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::addEvoRule(string evorule)
{
	string txid = pushAction(
		"geneminepool",
		"addevorule",
		evorule,
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::updateAttrRule(string attrrule)
{
	string txid = pushAction(
		"geneminepool",
		"updateattrule",
		attrrule,
		"geneminepool"
	);
	//return 
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::rmEvoRule(unsigned int evo_key)
{
	string txid = pushAction(
		"geneminepool",
		"rmevorule",
		string("[") + lexical_cast<string>(evo_key) + "]",
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::updateEvoRule(string evorule)
{
	string txid = pushAction(
		"geneminepool",
		"updateevorule",
		evorule,
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::adventure(string account,int elf_id,int adv_id,unsigned long random)
{
	string txid = pushAction(
		"geneminepool",
		"adventure",
		"[\"" + account + "\"," + lexical_cast<string>(elf_id) + "," + lexical_cast<string>(adv_id) + "," + lexical_cast<string>(random) + "]",
		"geneminepool"
	);
	//if no such adventure
	ptree retval;
	retval.put("status",((txid.empty())?"failure":"success"));
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::feed(string account,int elf_id,int gene_id)
{
	string txid = pushAction(
		"geneminepool",
		"injectgene",
		"[\"" + account + "\"," + lexical_cast<string>(elf_id) + "," + lexical_cast<string>(gene_id) + "]",
		account
	);
	//return
	ptree retval;
	retval.put("status","success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::evolve(string account,int elf_id,unsigned long random)
{
	string txid = pushAction(
		"geneminepool",
		"evolve",
		"[\"" + account + "\"," + lexical_cast<string>(elf_id) + "," + lexical_cast<string>(random) + "]",
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",((txid.empty())?"failure":"success"));
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::synthesize(string account,int elf1_id,int elf2_id,unsigned long random)
{
	string txid = pushAction(
		"geneminepool",
		"synthesize",
		"[\"" + account + "\"," + lexical_cast<string>(elf1_id) + "," + lexical_cast<string>(elf2_id) + "," + lexical_cast<string>(random) + "]",
		"geneminepool"
	);
	//return
	ptree retval;
	retval.put("status",((txid.empty())?"failure":"success"));
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::newelf(string account)
{
	return transfer(account,"geneminepool","500.0000 GENE","GeneMelee::createelf");
}

string Wrapper::checkTrx(string txid)
{
	vector<string> arguments(args);
	arguments.push_back("get");
	arguments.push_back("transaction");
	arguments.push_back(txid);
	string line;
	string json;
	ipstream out;
	child c(cleos_cmd,arguments,std_out > out);
	while(getline(out,line)) json += line;
	c.wait();
	ptree trx;
	stringstream buffer(json);
	read_json(buffer,trx);
	ptree retval;
	retval.put("status",(EXIT_SUCCESS == c.exit_code())?"success":"failure");
	retval.add_child("body",trx);
	stringstream ss;
	write_json(ss,retval);
	return ss.str();
}

string Wrapper::setStatus(string account,int elf_id,int value)
{
	string txid = pushAction(
		"geneminepool",
		"setstatus",
		"[\"" + account + "\"," + lexical_cast<string>(elf_id) + "," + lexical_cast<string>(value) + "]",
		"geneminepool");
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::removeElf(string account,int elf_id)
{
	string txid = pushAction(
		"geneminepool",
		"rmelf",
		"[\"" + account + "\"," + lexical_cast<string>(elf_id) + "]",
		"geneminepool");
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

string Wrapper::reset(string account)
{
	string txid = pushAction(
		"geneminepool",
		"reset",
		"[\"" + account + "\"]",
		"geneminepool");
	ptree retval;
	retval.put("status",(txid.empty())?"failure":"success");
	ptree body;
	body.put("tx_hash",txid);
	retval.add_child("body",body);
	stringstream buffer;
	write_json(buffer,retval);
	return buffer.str();
}

