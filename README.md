DEFAULT WALLET PASSWORD
Private key:  PW5Hx7Uq1Sf4EQ32xqdLx6Nr21Nmn6pd8Wa6xcthsyCYqFrtD6xkZ
Public key: EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA

KATA-ACCOUNTING WALLET PASSWORD
Private key: PW5HyJ11ydx7ksVrt1jdCQs2q8pJqQ2PTL9h5wjzxxLu2E2TsBH8k
Public key:  EOS6TzVLNnhLitYdZELXJPjRJ7bDFfXnhGAKsKHBH4biREmwvb5m7

 - How to Build -
   - set env EOSIO_CDT_ROOT=<eosio.cdt dir>/build   # such as /Users/timothy.banks/workspace/eosio.cdt/build
   export EOSIO_CDT_ROOT=/Users/timothy.banks/Work/eosio.cdt/build
   - set env EOSIO_ROOT   #where eos, eosio.contract being installed such as /Users/timothy.banks/workspace
   export EOSIO_ROOT=/Users/timothy.banks/Work
   - mkdir build
   - cd build
   - cmake ..
   - make

reset && eosio-cpp -abigen accounting.cpp -o accounting/accounting.wasm

cd ~/Work/katas/accounting 
ps -ef | grep nodeos | awk -F " " '{ if ($8 ~ /nodeos$/) system("kill " $2)}'
cd "/Users/timothy.banks/Library/Application Support/eosio/nodeos"
rm -rf config
rm -rf data
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::http_plugin --plugin eosio::history_plugin --plugin eosio::history_api_plugin --filter-on="*" --access-control-allow-origin='*' --contracts-console --http-validate-host=false --verbose-http-errors >> nodeos.log 2>&1 &
cd ~/Work/katas/accounting 

cleos wallet unlock
cleos create account eosio eosio.token EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA
cleos create account eosio accounting EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA
cleos create account eosio alice EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA
cleos create account eosio bob EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA
cleos create account eosio jane EOS8UdgHjwvkx8ugiuDitKoSMW8BnZB5g9KPTKhtuR6QSauBsTTPA

execute eosio.token commands [https://developers.eos.io/welcome/latest/getting-started/smart-contract-development/deploy-issue-and-transfer-tokens]
cleos set contract eosio.token eosio.token -p eosio.token@active 
cleos push action eosio.token create '["eosio.token", "1000000000.0000 SYS"]' -p eosio.token@active
cleos push action eosio.token issue '["eosio.token", "2000000.0000 SYS", "memo"]' -p eosio.token@active
cleos push action eosio.token transfer '["eosio.token", "alice", "1000000.0000 SYS", "memo"]' -p eosio.token@active
cleos push action eosio.token transfer '["eosio.token", "bob", "1000000.0000 SYS", "memo"]' -p eosio.token@active

cleos set contract alice accounting 
cleos push action alice acctreset '[]' -p alice
cleos push action alice acctreg '[]' -p alice 
cleos push action alice acctcreate '[savings]' -p alice
cleos push action alice acctcreate '[vacation]' -p alice
cleos push action alice acctlist '[]' -p alice
cleos push action alice accttransfer '[sys, savings, "500000.0000 SYS"]' -p alice
cleos push action alice acctlist '[]' -p alice

cleos set contract bob accounting 
cleos push action bob acctreset '[]' -p bob
cleos push action bob acctreg '[]' -p bob 
cleos push action bob acctcreate '[savings]' -p bob
cleos push action bob acctcreate '[vacation]' -p bob
cleos push action bob acctlist '[]' -p bob
cleos push action bob accttransfer '[sys, savings, "500000.0000 SYS"]' -p bob
cleos push action bob acctlist '[]' -p bob

cleos push action eosio.token transfer '[alice, bob, "1000000.0000 SYS", "memo"]' -p alice
cleos push action eosio.token transfer '[bob, alice, "500000.0000 SYS", "memo"]' -p bob
cleos push action eosio.token transfer '[alice, bob, "1000000.0000 SYS", "memo"]' -p alice





