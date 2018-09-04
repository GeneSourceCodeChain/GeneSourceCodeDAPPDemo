#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include "wrapper.hpp"

using namespace std;
using namespace boost;
using namespace boost::program_options;

int main(int argc,char ** argv)
{
	vector<string> args;
	int sale_key;
	options_description desc;
	desc.add_options()
		("help,h","print current message")
		("create_wallet",value<vector<string> >(&args),"create wallet, format: <account>")
		("get_user_data",value<vector<string> >(&args),"get user data, format: <account>")
		("query_order_list",value<int>(&sale_key)->implicit_value(-1),"query order list, format: [<sale_key>]")
		("create_order",value<vector<string> >(&args)->multitoken(),"create an order, format: <account> <price> <type> <id>")
		("transfer",value<vector<string> >(&args)->multitoken(),"transfer money, format: <from> <to> <quantity> <memo>")
		("revoke_order",value<vector<string> >(&args),"revoke an order, format: <sale id>")
		("adventure",value<vector<string> >(&args)->multitoken(),"adventure, format: <account> <elf_id> <adv_id> <random>")
		("fairy_feed",value<vector<string> >(&args)->multitoken(),"feed, format: <account> <elf_id> <gene_id>")
		("fairy_evolution",value<vector<string> >(&args)->multitoken(),"evolution, format: <account> <elf_id> <random>")
		("fairy_amalgamation",value<vector<string> >(&args)->multitoken(),"amalgamation, format: <account> <elf1_id> <elf2_id> <random>")
		("fairy_create",value<vector<string> >(&args)->multitoken(),"create elf, format: <account>")
		("check_trx",value<vector<string> >(&args),"get transaction, format: <txid>")
		("check_actions",value<vector<string> >(&args)->multitoken(),"get actions, format: <account> <from> <to>")
		("get_advruletable","get adventure rule table, format:")
		("get_evoruletable","get evolution rule table, format:")
		("get_attrruletable","get attribute rule table, format:")
		("add_advrule",value<vector<string> >(&args),"add adventure rule, format: \"[<required level>,<price>,<minimum reward exp>,<maximum>,[<bonus gene segments>],[<powers>],<enabled>]\"")
		("add_evorule",value<vector<string> >(&args),"add evolution rule, format: \"[[<required gene segments>],<evolve to>,<power>,[<bonus genes>],<enabled>]\"")
		("add_attrrule",value<vector<string> >(&args),"add attribute rule, format: \"[<genre_id>,<capabilities threshold>]\"")
		("update_advrule",value<vector<string> >(&args),"update adventure rule, format: \"[<adv_id>,<required level>,<price>,<minimum reward exp>,<maximum>,[<bonus gene segments>],[<powers>],<enabled>]\"")
		("update_evorule",value<vector<string> >(&args),"update evolution rule, format: \"[<evo_id>,[<required gene segments>],<evolve to>,<power>,[<bonus genes>],<enabled>]\"")
		("update_attrrule",value<vector<string> >(&args),"update attribute rule, format: \"[<attr_id>,<genre_id>,<capabilities threshold>]\"")
		("remove_advrule",value<vector<string> >(&args),"remove adventure rule, format: <adventure rule id>")
		("remove_evorule",value<vector<string> >(&args),"remove evolution rule, format: <evolution rule id>")
		("remove_attrrule",value<vector<string> >(&args),"remove attribute rule, format: <attribute rule id>")
		("set_status",value<vector<string> >(&args)->multitoken(),"set fairy status, format: <account> <elf id> <status>")
		("remove_fairy",value<vector<string> >(&args)->multitoken(),"remove fairy, format: <account> <elf id>")
		("reset",value<vector<string> >(&args),"reset account, format: <account>");
	variables_map vm;
	store(parse_command_line(argc,argv,desc),vm);
	notify(vm);

	Wrapper wrapper("http://192.168.1.100",8001,"http://192.168.1.100",6666);
	if(1 == vm.count("help")) {
		cout<<desc;
		return EXIT_SUCCESS;
	} else if(1 == vm.count("create_wallet")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.create_account(args[0])<<endl;
	} else if(1 == vm.count("get_user_data")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.get_account_possessions(args[0])<<endl;
	} else if(1 == vm.count("query_order_list")) {
		cout<<wrapper.get_pending_orders(sale_key)<<endl;
	} else if(1 == vm.count("create_order")) {
		if(4 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account> <price> <type> <id>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.add_pending_order(args[0],args[1],lexical_cast<int>(args[2]),lexical_cast<int>(args[3]))<<endl;
	} else if(1 == vm.count("transfer")) {
		if(4 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <from> <to> <quantity> <memo>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.transfer(args[0],args[1],args[2],args[3])<<endl;
	} else if(1 == vm.count("revoke_order")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <sale id>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.revoke_order(lexical_cast<int>(args[0]))<<endl;
	} else if(1 == vm.count("adventure")) {
		if(4 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account> <elf id> <adv id> <random>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.adventure(args[0],lexical_cast<int>(args[1]),lexical_cast<int>(args[2]),lexical_cast<unsigned long>(args[3]))<<endl;
	} else if(1 == vm.count("fairy_feed")) {
		if(3 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account> <elf id> <gene id>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.feed(args[0],lexical_cast<int>(args[1]),lexical_cast<int>(args[2]))<<endl;
	} else if(1 == vm.count("fairy_evolution")) {
		if(3 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account> <elf id> <random>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.evolve(args[0],lexical_cast<int>(args[1]),lexical_cast<unsigned long>(args[2]))<<endl;
	} else if(1 == vm.count("fairy_amalgamation")) {
		if(4 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account> <elf1 id> <elf2 id> <random>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.synthesize(args[0],lexical_cast<int>(args[1]),lexical_cast<int>(args[2]),lexical_cast<unsigned long>(args[3]));
	} else if(1 == vm.count("fairy_create")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.newelf(args[0])<<endl;
	} else if(1 == vm.count("check_trx")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <txid>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.checkTrx(args[0])<<endl;
	} else if(1 == vm.count("check_actions")) {
		if(3 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account> <from> <to>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.getActions(args[0],lexical_cast<int>(args[1]),lexical_cast<int>(args[2]) + 1)<<endl;
	} else if(1 == vm.count("get_advruletable")) {
		cout<<wrapper.getAdvRuleTable()<<endl;
	} else if(1 == vm.count("get_evoruletable")) {
		cout<<wrapper.getEvoRuleTable()<<endl;
	} else if(1 == vm.count("get_attrruletable")) {
		cout<<wrapper.getAttrRuleTable()<<endl;
	} else if(1 == vm.count("add_advrule")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <adventure rule>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.addAdvRule(args[0])<<endl;
	} else if(1 == vm.count("add_evorule")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <evolution rule>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.addEvoRule(args[0])<<endl;
	} else if(1 == vm.count("update_attrrule")) {
	       	if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <attribute rule>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.updateAttrRule(args[0])<<endl;
	} else if(1 == vm.count("update_advrule")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <adventure rule>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.updateAdvRule(args[0])<<endl;
	} else if(1 == vm.count("update_evorule")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <evolution rule>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.updateEvoRule(args[0])<<endl;
	} else if(1 == vm.count("update_attrrule")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <attribute rule>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.updateAttrRule(args[0])<<endl;
	} else if(1 == vm.count("remove_advrule")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <adventure rule id>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.rmAdvRule(lexical_cast<int>(args[0]))<<endl;
	} else if(1 == vm.count("remove_evorule")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <evolution rule id>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.rmEvoRule(lexical_cast<int>(args[0]))<<endl;
	} else if(1 == vm.count("remove_attrrule")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <attribute rule id>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.rmAttrRule(lexical_cast<int>(args[0]))<<endl;
	} else if(1 == vm.count("set_status")) {
		if(3 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account> <elf_key> <value>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.setStatus(args[0],lexical_cast<int>(args[1]),lexical_cast<int>(args[2]))<<endl;
	} else if(1 == vm.count("remove_fairy")) {
		if(2 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account> <elf_key>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.removeElf(args[0],lexical_cast<int>(args[1]))<<endl;
	} else if(1 == vm.count("reset")) {
		if(1 != args.size()) {
			cout<<"Usage: "<<argv[0]<<" <account>"<<endl;
			return EXIT_FAILURE;
		}
		cout<<wrapper.reset(args[0])<<endl;
	} else {
		cout<<"invalid command"<<endl;
		cout<<desc;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

