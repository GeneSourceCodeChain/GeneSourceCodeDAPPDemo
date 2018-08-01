#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "Storage.hpp"

using namespace std;
using namespace boost;
using namespace boost::program_options;

int main(int argc,char ** argv)
{
	vector<string> args;
	options_description desc;
	desc.add_options()
		("help,h","print this message")
		("get,g",value<vector<string> >(&args)->multitoken(),"download file, format \"--get <transaction list file> <output path>\"")
		("push,p",value<vector<string> >(&args),"upload file, format \"--push <input path>\"");
	variables_map vm;
	store(parse_command_line(argc,argv,desc),vm);
	notify(vm);
	
	if(vm.count("help") || 0 == vm.count("get") && 0 == vm.count("push")) {
		cout<<desc;
		return EXIT_SUCCESS;
	}
	
	Storage storage("192.168.1.100",8000,"192.168.1.100",6666,"eosfilestore");
	
	if(1 == vm.count("get")) {
		if(2 != args.size()) {
			cout<<"invalid number of arguments"<<endl;
			return EXIT_FAILURE;
		}
		std::ifstream in(args[0]);
		if(false == in.is_open()) {
			cout<<"invalid list file!"<<endl;
			return EXIT_FAILURE;
		}
		vector<string> txs;
		while(false == in.eof()) {
			string line;
			getline(in,line);
			trim(line);
			if(line.empty()) continue;
			txs.push_back(line);
		}
		bool retval = storage.get(txs,args[1]);
		if(false == retval) {
			cout<<"failed to get file"<<endl;
			return EXIT_FAILURE;
		}
	} else if(1 == vm.count("push")) {
		bool retval;
		vector<string> txs;
		boost::tie(retval,txs) = storage.push(args[0]);
		if(false == retval) {
			cout<<"failed to push file to blockchain"<<endl;
			return EXIT_FAILURE;
		} else {
			//output a list of transaction number
			std::ofstream out("list.txt");
			for(auto & t : txs) {
				out<<t<<endl;
			}
			out.close();
		}	
	} else {
		cout<<"invalid command!"<<endl;
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

