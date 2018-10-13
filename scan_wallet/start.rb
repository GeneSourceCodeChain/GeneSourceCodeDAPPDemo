require 'json'
require "open-uri"
require 'find'
require "redis"
$redis = Redis.new(:host => 'localhost', :port => 6379)

class ScanBlockChain

	def initialize
    
  end

	def block_number
		post_rpc("eth_blockNumber",[])['result'].to_i(16)
	end

	def balance_of(eth_address)
		post_rpc("eth_getBalance",[eth_address,'latest'])['result'].to_i(16)
	end

	def get_block_tx(block_number)
		post_rpc("eth_getBlockByNumber",[block_number,true])
	end

	def get_tx_body(txhash)
		post_rpc("eth_getTransactionReceipt",[txhash])
	end

	private
		def post_rpc(method_name,params_arr)
			eth_rpc_url = "https://mainnet.infura.io"
			result = `curl -X POST -H 'content-type: application/json' --data '{"jsonrpc":"2.0","method":"#{method_name}","params":#{params_arr},"id":84}' "#{eth_rpc_url}"`
	    JSON.parse result
	  end
end

sbc = ScanBlockChain.new
contract_list = ['0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0']
$redis.set('scan_eth_block_number',6488099) #start block_number
while true
	now_block_number = sbc.block_number.to_i
	old_block_number = $redis.get('scan_eth_block_number').to_i 
	$redis.set('scan_eth_block_number',now_block_number) if !$redis.get('scan_eth_block_number')
	while old_block_number < now_block_number && true
		puts old_block_number
		old_block_number = old_block_number + 1
		tx_list = sbc.get_block_tx("0x"+(old_block_number).to_s(16))['result']['transactions']
		tx_list.each do |tx|
			scan_st = false
			contract_list.each do |contract_address|
				if (tx['input'].include? contract_address.gsub("0x","")) || tx['to'] == contract_address
					scan_st = true
				end
			end
			if scan_st #&& tx['value'] = "0x0"
				tx_json = sbc.get_tx_body(tx['hash'])
				tx_json['result']['logs'].each do |tx_body|
					if tx_body['topics'].size > 2 && (contract_list.include? tx_body['address'])
						file = File.new("#{tx_body['address']}/0x#{tx_body['topics'][2].to_i(16).to_s(16).rjust(40, '0')}", "a+")
						File.open(file, 'a+'){|f| f << "#{tx_json['result']['transactionHash']},0x#{tx_body['topics'][1].to_i(16).to_s(16).rjust(40, '0')},#{tx_body['data'].to_i(16)/1e18}\n"}
						puts "transactionHash:#{tx_json['result']['transactionHash']} | form:0x#{tx_body['topics'][1].to_i(16).to_s(16).rjust(40, '0')} | to:0x#{tx_body['topics'][2].to_i(16).to_s(16).rjust(40, '0')}~~~~#{tx_body['data'].to_i(16)/1e18}"
					end
				end
			end
		end
	end
	$redis.set('scan_eth_block_number',now_block_number)
	sleep 5
end
