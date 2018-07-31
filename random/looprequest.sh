#!/bin/bash
index=0
while : ; do
	cleos --wallet-url http://192.168.1.100:6666 -u http://192.168.1.100:8001 push action randomoracle request "{\"owner\":\"requester11a\",\"index\":$index,\"handler\":\"getrandom\"}" -p requester11a
	index=`expr "$index" + 1`
	sleep 1s
done
