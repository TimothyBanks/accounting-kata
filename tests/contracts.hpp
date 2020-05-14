#pragma once

#include <eosio/testing/tester.hpp>

namespace eosio { namespace testing {

struct contracts {
   static std::vector<uint8_t> system_wasm() { return read_wasm("/Users/timothy.banks/Work/eosio.contracts/build/contracts/eosio.system/eosio.system.wasm"); }
   static std::vector<char>    system_abi() { return read_abi("/Users/timothy.banks/Work/eosio.contracts/build/contracts/eosio.system/eosio.system.abi"); }
   static std::vector<uint8_t> token_wasm() { return read_wasm("/Users/timothy.banks/Work/eosio.contracts/build/contracts/eosio.token/eosio.token.wasm"); }
   static std::vector<char>    token_abi() { return read_abi("/Users/timothy.banks/Work/eosio.contracts/build/contracts/eosio.token/eosio.token.abi"); }
   static std::vector<uint8_t> msig_wasm() { return read_wasm("/Users/timothy.banks/Work/eosio.contracts/build/contracts/eosio.token/eosio.msig.wasm"); }
   static std::vector<char>    msig_abi() { return read_abi("/Users/timothy.banks/Work/eosio.contracts/build/contracts/eosio.token/eosio.msig.abi"); }

  //  static std::vector<uint8_t> accounting_wasm() { return read_wasm("/Users/timothy.banks/Work/katas/accounting/build/../accounting/accounting.wasm"); }
  //  static std::vector<char>    accounting_abi() { return read_abi("/Users/timothy.banks/Work/katas/accounting/build/../accounting/accounting.abi"); }
   static std::vector<uint8_t> accounting_wasm() { return read_wasm("/Users/timothy.banks/Work/katas/accounting/build/accounting/accounting.wasm"); }
   static std::vector<char>    accounting_abi() { return read_abi("/Users/timothy.banks/Work/katas/accounting/build/accounting/accounting.abi"); }
};

}}
