#!/bin/bash

# rebuild the contract
reset && eosio-cpp -abigen src/accounting.cpp -o accounting/accounting.wasm -Iinclude

# reset the blockchain
cd ~/Work/katas/accounting 
ps -ef | grep nodeos | awk -F " " '{ if ($8 ~ /nodeos$/) system("kill " $2)}'
cd "/Users/timothy.banks/Library/Application Support/eosio/nodeos"
rm -rf config
rm -rf data
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::http_plugin --plugin eosio::history_plugin --plugin eosio::history_api_plugin --filter-on="*" --access-control-allow-origin='*' --contracts-console --http-validate-host=false --verbose-http-errors >> nodeos.log 2>&1 &
cd ~/Work/katas/accounting 

# create the accounts
#cleos wallet unlock
cleos create account eosio eosio.token EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA
cleos create account eosio accounting EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA
cleos create account eosio alice EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA
cleos create account eosio bob EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA
cleos create account eosio jane EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA

# execute eosio.token commands [https://developers.eos.io/welcome/latest/getting-started/smart-contract-development/deploy-issue-and-transfer-tokens]
cleos set contract eosio.token eosio.token -p eosio.token@active 
cleos push action eosio.token create '["eosio.token", "1000000000.0000 SYS"]' -p eosio.token@active
cleos push action eosio.token issue '["eosio.token", "2000000.0000 SYS", "memo"]' -p eosio.token@active
cleos push action eosio.token transfer '["eosio.token", "alice", "1000000.0000 SYS", "memo"]' -p eosio.token@active
cleos push action eosio.token transfer '["eosio.token", "bob", "1000000.0000 SYS", "memo"]' -p eosio.token@active

# execute accounting commands for alice
cleos set contract alice accounting 
cleos push action alice acctreset '[]' -p alice
cleos push action alice acctreg '[]' -p alice 
cleos push action alice acctcreate '[savings]' -p alice
cleos push action alice acctcreate '[vacation]' -p alice
cleos push action alice acctlist '[]' -p alice
cleos push action alice accttransfer '[sys, savings, "500000.0000 SYS"]' -p alice
cleos push action alice acctlist '[]' -p alice

# execute the accounting commands for bob
cleos set contract bob accounting 
cleos push action bob acctreset '[]' -p bob
cleos push action bob acctreg '[]' -p bob 
cleos push action bob acctcreate '[savings]' -p bob
cleos push action bob acctcreate '[vacation]' -p bob
cleos push action bob acctlist '[]' -p bob
cleos push action bob accttransfer '[sys, savings, "500000.0000 SYS"]' -p bob
cleos push action bob acctlist '[]' -p bob

# test out a few transfers
cleos push action eosio.token transfer '[alice, bob, "1000000.0000 SYS", "memo"]' -p alice
cleos push action eosio.token transfer '[bob, alice, "500000.0000 SYS", "memo"]' -p bob
cleos push action eosio.token transfer '[alice, bob, "1000000.0000 SYS", "memo"]' -p alice