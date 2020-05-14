#include <boost/test/unit_test.hpp>
#include "eosio.system_tester.hpp"

#include <chrono>
#include <thread>

using namespace eosio::chain;
using namespace eosio::testing;
using namespace eosio_system;
using namespace fc;

static constexpr auto microseconds_per_milliseconds = 1000; 
static constexpr auto block_produce_time = 500 * microseconds_per_milliseconds;

struct accounting_tester: public tester 
{
  accounting_tester() 
  {
    produce_blocks(2);

    // NOTE: account names have to be 12 chars on real chain
    //

    create_accounts({ N(alice), N(bob), N(jane), N(eosio.token) });
    produce_blocks(2);

    set_code(N(eosio.token), contracts::token_wasm());
    set_abi(N(eosio.token), contracts::token_abi().data());

    set_code(N(alice), contracts::accounting_wasm());
    set_abi(N(alice), contracts::accounting_abi().data());

    set_code(N(bob), contracts::accounting_wasm());
    set_abi(N(bob), contracts::accounting_abi().data());

    set_code(N(jane), contracts::accounting_wasm());
    set_abi(N(jane), contracts::accounting_abi().data());

    produce_blocks();

    // auto verify_abi = [&](const auto& name, auto& account_abi)
    // {
    //     const auto& accnt = control->db().get<account_object,by_name>(name);
    //     abi_def abi;
    //     BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
    //     account_abi.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));
    // };
    // verify_abi(N(alice), alice_abi_ser);
    // verify_abi(N(bob), bob_abi_ser);
    // verify_abi(N(jane), jane_abi_ser);
    // verify_abi(N(eosio.token), tok_abi_ser);

    base_tester::push_action(
      N(eosio.token), 
      N(create), 
      N(eosio.token), 
      mvo()
        ("issuer", "eosio.token")
        ("maximum_supply", SYS_1000000_0000));

    base_tester::push_action( 
      N(eosio.token),
      N(issue), 
      N(eosio.token), 
      mvo()
        ("to", "eosio.token")
        ("quantity", SYS_1000000_0000)
        ("memo", "issue all tokens to token accnt"));

    base_tester::push_action( 
      N(eosio.token), 
      N(transfer), 
      N(eosio.token), 
      mvo()
        ("from", "eosio.token")
        ("to", "alice")
        ("quantity", SYS_2000_0000)
        ("memo", "issue 20000 to alice"));

    base_tester::push_action( 
      N(eosio.token), 
      N(transfer), 
      N(eosio.token), 
      mvo()
        ("from", "eosio.token")
        ("to", "bob")
        ("quantity", SYS_2000_0000)
        ("memo", "issue 2000 to bob"));

    base_tester::push_action( 
      N(eosio.token), 
      N(transfer), 
      N(eosio.token), 
      mvo()
        ("from", "eosio.token")
        ("to", "jane")
        ("quantity", SYS_2000_0000)
        ("memo", "issue 2000 to jane"));

    produce_blocks();
  }

  void register_account(const name& owner) 
  {
    base_tester::push_action(owner, N(acctreg), owner, {});
  }

  void create_category(const name& owner, const std::string& category) 
  {
    base_tester::push_action(owner, N(acctcreate), owner, mvo() ("category", category));
  }

  void create_payment(const name& owner, 
                      const std::string& category, 
                      const name& payee, 
                      const asset& total, 
                      const std::string& memo)
  {
    base_tester::push_action(
      owner, 
      N(acctpay), 
      owner, 
      mvo()
        ("fromcat", category)
        ("payee", payee)
        ("total", total)
        ("memo", memo)
    );
  }  

  void create_deferred_payment(const name& owner, 
                               const std::string& category, 
                               const name& payee, 
                               const asset& total, 
                               const std::string& memo, 
                               uint64_t deferfor)
  {
    base_tester::push_action(
      owner, 
      N(acctdefer), 
      owner, 
      mvo()
        ("fromcat", category)
        ("payee", payee)
        ("total", total)
        ("memo", memo)
        ("deferfor", deferfor)
    );
  }

  void create_recurring_payment(const name& owner, 
                                const std::string& category, 
                                const name& payee, 
                                const asset& total, 
                                const std::string& memo,
                                uint64_t recurevery,
                                uint64_t recurcount,
                                uint64_t deferfor)
  {
    base_tester::push_action(
      owner, 
      N(acctsched), 
      owner, 
      mvo()
        ("fromcat", category)
        ("payee", payee)
        ("total", total)
        ("memo", memo)
        ("recurevery", recurevery)
        ("recurcount", recurcount)
        ("deferfor", deferfor)
    );
  }   

  void process_transfers(const name& owner)
  {
    base_tester::push_action(
      owner, 
      N(process), 
      owner, 
      mvo()
    ); 
  }      

  void cancel_transfer(const name& owner, uint64_t id)
  {
    base_tester::push_action(
      owner, 
      N(transcancel), 
      owner, 
      mvo()
        ("transferid", id)
    ); 
  }                       

  void transfer_fund(const name& owner, const std::string& from, const std::string& to, const asset& amount) 
  {
    base_tester::push_action( 
      owner, 
      N(accttransfer), 
      owner, 
      mvo()
        ("from", from)
        ("to", to)
        ("total", amount));
  }

  void assert_balance(const name& owner, const std::string& category, const asset& quantity)
  {
    static auto counter = uint64_t{0};

    base_tester::push_action(
      owner, 
      N(assertbal),
      owner, 
      mvo()
        ("category", category)
        ("quantity", quantity)
        ("dummy", ++counter)
    );
  }

  void eosio_transfer(const name& acct, const asset& amount)
  {
    base_tester::push_action( 
      N(eosio.token), 
      N(transfer), N(eosio.token), 
      mvo()
        ("from", "eosio.token")
        ("to", acct)
        ("quantity", amount.to_string())
        ("memo", "eosio_transfer 123"));
  };

  // abi_serializer tok_abi_ser;
  // abi_serializer alice_abi_ser;
  // abi_serializer bob_abi_ser;
  // abi_serializer jane_abi_ser;

  const asset SYS_1000000_0000 = asset::from_string("1000000.0000 SYS");
  const asset SYS_2000_0000 = asset::from_string("2000.0000 SYS");
  const asset SYS_0_0000 = asset::from_string("0.0000 SYS");
  const asset SYS_123_1234 = asset::from_string("123.1234 SYS");
  const asset SYS_234_3456 = asset::from_string("234.3456 SYS");
  const asset SYS_3456_1234 = asset::from_string("3456.1234 SYS");
  const asset SYS_1_2345 = asset::from_string("1.2345 SYS");
  const asset SYS_2_3456 = asset::from_string("2.3456 SYS");
  const asset SYS_40_3456 = asset::from_string("40.3456 SYS");
  const asset INVALID_0_0001 = asset::from_string("0.0001 INVALID");
  const asset SYS_NEG_23_5678 = asset::from_string("-23.5678 SYS");
  const asset SYS_NEG_5678_1234 = asset::from_string("-5678.1234 SYS");

  const std::string CAT_DEFAULT = "sys";
  const std::string CAT_RENT = "rent";
  const std::string CAT_VACATION = "vacation";
};

BOOST_FIXTURE_TEST_CASE(account_transfers_cancelled, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  auto defer_for = 1000000;
  auto rent_payment = asset::from_string("100.0000 SYS");
  BOOST_REQUIRE_NO_THROW(create_recurring_payment(N(alice), CAT_RENT, N(bob), rent_payment, "rent payment", defer_for, 1, defer_for));
  BOOST_REQUIRE_NO_THROW(create_deferred_payment(N(alice), CAT_RENT, N(bob), rent_payment, "rent payment", defer_for));
  BOOST_REQUIRE_NO_THROW(cancel_transfer(N(alice), 0));
  BOOST_REQUIRE_NO_THROW(cancel_transfer(N(alice), 1));
  
  produce_blocks(defer_for / block_produce_time + 1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));  

  // no funds should have been transfered
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent)); 
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_recurring_test_invalid_account, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_THROW(create_recurring_payment(N(alice), CAT_RENT, N(john), rent, "rent payment", 1000000, 1, 1000000), fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_recurring_test_invalid_category, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_THROW(create_recurring_payment(N(alice), CAT_RENT, N(bob), rent, "rent payment", 1000000, 1, 1000000), fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_recurring_test_insufficient_funds, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  auto defer_for = 1000000;
  auto rent_payment = asset::from_string("5000.0000 SYS");
  BOOST_REQUIRE_NO_THROW(create_recurring_payment(N(alice), CAT_RENT, N(bob), rent_payment, "rent payment", defer_for, 1, defer_for));
  produce_blocks(defer_for / block_produce_time + 1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));

  // The transfer should not of been processed since there was insufficient funds.
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_recurring_test_processed, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  auto defer_for = 1000000;
  BOOST_REQUIRE_NO_THROW(create_recurring_payment(N(alice), CAT_RENT, N(bob), rent, "rent payment", defer_for, 1, defer_for));
  produce_blocks(defer_for / block_produce_time + 1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));

  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000 + rent));

  BOOST_REQUIRE_NO_THROW(transfer_fund(N(bob), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_recurring_test_not_processed, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  auto defer_for = 1000000;
  BOOST_REQUIRE_NO_THROW(create_recurring_payment(N(alice), CAT_RENT, N(bob), rent, "rent payment", defer_for, 1, defer_for));
  produce_blocks(1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));  // Not enough time has passed to process

  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent)); // The funds should not have been reserved as this is a recurring transfer.
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_RENT, SYS_0_0000));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_recurring_test_processed_all, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  auto defer_for = 1000000;
  auto rent_payment = asset::from_string("100.0000 SYS");
  BOOST_REQUIRE_NO_THROW(create_recurring_payment(N(alice), CAT_RENT, N(bob), rent_payment, "rent payment", defer_for, 1, defer_for));
  produce_blocks(defer_for / block_produce_time + 1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));
  produce_blocks(defer_for / block_produce_time + 1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));
  // The transfer should have completed and been removed at this point.
  produce_blocks(defer_for / block_produce_time + 1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));

  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent - rent_payment - rent_payment));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000 + rent_payment + rent_payment));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_defer_test_invalid_account, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_THROW(create_deferred_payment(N(alice), CAT_RENT, N(john), rent, "rent payment", 1000000), fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_defer_test_invalid_category, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_THROW(create_deferred_payment(N(alice), CAT_RENT, N(john), rent, "rent payment", 1000000), fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_defer_test_insufficient_funds, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  auto rent_payment = asset::from_string("5000.0000 SYS");
  BOOST_REQUIRE_THROW(create_deferred_payment(N(alice), CAT_RENT, N(bob), rent_payment, "rent payment", 1000000), fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_defer_test_processed, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  auto defer_for = 1000000;
  BOOST_REQUIRE_NO_THROW(create_deferred_payment(N(alice), CAT_RENT, N(bob), rent, "rent payment", defer_for));
  produce_blocks(defer_for / block_produce_time + 1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));

  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000 + rent));

  BOOST_REQUIRE_NO_THROW(transfer_fund(N(bob), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_defer_test_not_processed, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  auto defer_for = 1000000;
  BOOST_REQUIRE_NO_THROW(create_deferred_payment(N(alice), CAT_RENT, N(bob), rent, "rent payment", defer_for));
  produce_blocks(1);
  BOOST_REQUIRE_NO_THROW(process_transfers(N(alice)));  // Not enough time has passed to process

  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000)); // The funds should have been reserved.
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_RENT, SYS_0_0000));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_pay_test_invalid_category, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_THROW(create_payment(N(alice), CAT_RENT, N(bob), rent, "rent payment"), fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_pay_test_insufficient_funds, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  BOOST_REQUIRE_THROW(create_payment(N(alice), CAT_RENT, N(bob), rent + rent, "rent payment"), fc::exception);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(account_pay_test, accounting_tester)
try
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  
  auto rent = asset::from_string("500.0000 SYS");
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000 - rent));

  BOOST_REQUIRE_NO_THROW(register_account(N(bob)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(bob), CAT_RENT));

  BOOST_REQUIRE_NO_THROW(create_payment(N(alice), CAT_RENT, N(bob), rent, "rent payment"));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000 + rent));

  BOOST_REQUIRE_NO_THROW(transfer_fund(N(bob), CAT_DEFAULT, CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_RENT, rent));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(bob), CAT_DEFAULT, SYS_2000_0000));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(on_notify_test, accounting_tester)
try 
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(eosio_transfer(N(alice), SYS_234_3456));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_234_3456 + SYS_2000_0000));
} 
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(on_notify_fail_test, accounting_tester)
try 
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_THROW(eosio_transfer(N(alice), SYS_NEG_23_5678), fc::exception);
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_THROW(eosio_transfer(N(alice), INVALID_0_0001), fc::exception);
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_THROW(eosio_transfer(N(alice), SYS_1000000_0000), fc::exception);
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
} 
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(create_acct_test, accounting_tester)
try 
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
} 
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(create_acct_fail_test, accounting_tester)
try 
{
  BOOST_REQUIRE_THROW(register_account(N(accntinvalid)), fc::exception);
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_THROW(register_account(N(alice)), fc::exception);
} 
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(create_cat_test, accounting_tester)
try 
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
} 
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(create_cat_fail_test, accounting_tester)
try 
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_THROW(create_category(N(alice), CAT_RENT), fc::exception);
  BOOST_REQUIRE_THROW(create_category(N(accntinvalid), CAT_VACATION), fc::exception);
} 
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_fund_test, accounting_tester)
try 
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, SYS_123_1234));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, (SYS_2000_0000 - SYS_123_1234)));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_123_1234));
} 
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_fund_fail_test, accounting_tester)
try 
{
  BOOST_REQUIRE_NO_THROW(register_account(N(alice)));

  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));

  BOOST_REQUIRE_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, SYS_1_2345), fc::exception);
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));

  BOOST_REQUIRE_THROW(transfer_fund(N(alice), CAT_RENT, CAT_DEFAULT, SYS_1_2345), fc::exception);
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));

  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_DEFAULT, SYS_1_2345));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));

  BOOST_REQUIRE_THROW(transfer_fund(N(bob), CAT_DEFAULT, CAT_RENT, SYS_40_3456), fc::exception);

  BOOST_REQUIRE_NO_THROW(create_category(N(alice), CAT_RENT));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));

  BOOST_REQUIRE_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, SYS_3456_1234), fc::exception);
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));

  BOOST_REQUIRE_NO_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, SYS_0_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));

  BOOST_REQUIRE_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, SYS_NEG_23_5678), fc::exception);
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));

  BOOST_REQUIRE_THROW(transfer_fund(N(alice), CAT_DEFAULT, CAT_RENT, INVALID_0_0001), fc::exception);
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_DEFAULT, SYS_2000_0000));
  BOOST_REQUIRE_NO_THROW(assert_balance(N(alice), CAT_RENT, SYS_0_0000));
} 
FC_LOG_AND_RETHROW()