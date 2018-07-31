BOOST_PREFIX=/home/ssgene/opt/boost
CXXFLAGS=-I${BOOST_PREFIX}/include `pkg-config --cflags libcrypto++` -O2
LIBS=-L${BOOST_PREFIX}/lib `pkg-config --libs libcrypto++` -lboost_system -lboost_filesystem -lboost_thread -lpthread
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: randseedserver contract

.PHONY: randseedserver contract

randseedserver: 
	make -C randseedserver

contract:
	make -C Random

install: all
	#import key for randomoracle
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 wallet import --private-key 5KHa8jpUFG771Qvxzq2ND7pBmMsBSWLQiWQexh8w5BBeEbUUx3M
	#import keys for seeds
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 wallet import --private-key 5JT4L5vz3HBLd1NisFD1RrM85oeFq7axc58271cKD1J9sBid1bk
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 wallet import --private-key 5JLt7YrPbREMBgo6X5HXK3Vq3Sn1CFJM4JYPdtzWh2bruUBDN6w
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 wallet import --private-key 5JxFc9aMB9f7B2JNQ9oUGv1q7w9TDL8HUMwCFVt5BD8Rga61HjN
	#import keys for requester
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 wallet import --private-key 5JSomFPK8F5sZKN5P4FEVwfACRsv3zKPDveL5w53HCLLiJXViiF
	#create accounts
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 system newaccount eosio.stake randomoracle EOS55MfVEkczBuFkbh9XyGTurk3KSAu9RcA5VTTEbwbcFdsHmhzGK --stake-cpu "10000.0000 SYS" --stake-net "10000.0000 SYS" --buy-ram "10000.0000 SYS" -p eosio.stake
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 system newaccount eosio.stake randseed111a EOS8NfQjUMLoj89yUWz3VpZECGM3HvFXinJbTaifGTt4PX3m3f8Ye --stake-cpu "10000.0000 SYS" --stake-net "10000.0000 SYS" --buy-ram "10000.0000 SYS" -p eosio.stake
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 system newaccount eosio.stake randseed111b EOS6y1ZGm1wst7PVmhhr9fiUeJuVfiba6URdgiXQbkU3k4DP3fHwL --stake-cpu "10000.0000 SYS" --stake-net "10000.0000 SYS" --buy-ram "10000.0000 SYS" -p eosio.stake
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 system newaccount eosio.stake randseed111c EOS7bSXhVoHdipW1GHjPmbB1LZDVvxCbrMCie7DtXdvx8hTjLoUFb --stake-cpu "10000.0000 SYS" --stake-net "10000.0000 SYS" --buy-ram "10000.0000 SYS" -p eosio.stake
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 system newaccount eosio.stake requester11a EOS789Yo4bDT1afi3B9xk47Y8sQd8tqH4jGqc5Bi8BxTJumjr87eM --stake-cpu "10000.0000 SYS" --stake-net "10000.0000 SYS" --buy-ram "10000.0000 SYS" -p eosio.stake
	#set randomoracle contract
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8000 set contract randomoracle Random -p randomoracle
	#set authority
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 set account permission eosdacrandom active '{"threshold":1,"keys":[{"key":"EOS55MfVEkczBuFkbh9XyGTurk3KSAu9RcA5VTTEbwbcFdsHmhzGK","weight":1}],"accounts":[{"permission":{"actor":"eosdacrandom","permission":"eosio.code"},"weight":1}],"waits":[]}' owner -p eosdacrandom -x 10
	#set requester cotract
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 set contract requester11a ./requester -p requester11a

test:
	LD_LIBRARY_PATH=/home/ssgene/opt/boost/lib:${LD_LIBRARY_PATH} ./randseedserver/randseedserver &
	bash looprequest.sh

clean:
	make -C Random clean
	make -C randomseedserver clean
	make -C requester clean

