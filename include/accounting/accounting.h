#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/name.hpp>
#include <eosio/singleton.hpp>

class [[eosio::contract("accounting")]] accounting 
: public eosio::contract 
{
private:
  static constexpr eosio::name default_category = eosio::name{"sys"};
  static constexpr eosio::symbol_code default_symbol_code = eosio::symbol_code{"SYS"};
  static const eosio::asset default_asset;

  struct [[eosio::table]] system
  {
    uint64_t next_id = 0;

    static uint64_t fetch_add(uint64_t count, accounting& accounting_contract);
  };
  using system_table = eosio::singleton<"system"_n, system>;

  struct transfer
  {
    uint64_t id = {};
    eosio::name from_category;
    eosio::name to_account;
    eosio::asset amount = default_asset;
    std::string memo;

    void swap(transfer& other);

    uint64_t primary_key() const
    {
      return id;
    }

    uint64_t get_secondary_key_1() const
    {
      return from_category.value;
    }

    uint64_t get_secondary_key_2() const
    {
      return to_account.value;
    }

    static transfer create(eosio::name from_category,
                           eosio::name to_account,
                           eosio::asset amount,
                           std::string memo,
                           accounting& accounting_contract);
  };

  // This table will hold all deferred transfers.
  struct [[eosio::table]] deferred_transfer : public transfer
  {
    eosio::microseconds defer_until;  // time (in seconds) from the epoch before this transfer can occur.

    void swap(deferred_transfer& other);
    static deferred_transfer create(eosio::name from_category,
                                    eosio::name to_account,
                                    eosio::asset amount,
                                    std::string memo,
                                    eosio::microseconds defer_for,
                                    accounting& accounting_contract);
  };
  using deferred_transfers_table = eosio::multi_index<"def.trans"_n,
                                                      deferred_transfer,
                                                      eosio::indexed_by<"bycategory"_n, eosio::const_mem_fun<transfer, uint64_t, &transfer::get_secondary_key_1>>,
                                                      eosio::indexed_by<"byaccount"_n, eosio::const_mem_fun<transfer, uint64_t, &transfer::get_secondary_key_2>>>;

  // This table will hold all recurring transfers.
  struct [[eosio::table]] recurring_transfer : public deferred_transfer
  {
    eosio::microseconds recur_every;  // time (in seconds) that recurring transfers should take place.
    uint64_t recur_count = 0; 

    void swap(recurring_transfer& other);
    static recurring_transfer create(eosio::name from_category,
                                     eosio::name to_account,
                                     eosio::asset amount,
                                     std::string memo,
                                     eosio::microseconds recur_every,
                                     uint64_t recur_count,
                                     eosio::microseconds defer_for,
                                     accounting& accounting_contract);
  };
  using recurring_transfers_table = eosio::multi_index<"rec.trans"_n,
                                                       recurring_transfer,
                                                       eosio::indexed_by<"bycategory"_n, eosio::const_mem_fun<transfer, uint64_t, &transfer::get_secondary_key_1>>,
                                                       eosio::indexed_by<"byaccount"_n, eosio::const_mem_fun<transfer, uint64_t, &transfer::get_secondary_key_2>>>;

  // This table will hold all the categories created by a user
  // for partitioning SYS tokens.
  struct [[eosio::table]] category
  {
    eosio::name name; // The name of the category
    eosio::asset available = default_asset; // The number SYS tokens allocated to this category that are available.
    eosio::asset reserved = default_asset; // The number of SYS tokens allocated to this category that are pending payment/transfer.

    uint64_t primary_key() const
    {
      return name.value;
    }
  };
  using categories_table = eosio::multi_index<"categories"_n, category>;

public:
  using eosio::contract::contract;

  accounting(const eosio::name& receiver, const eosio::name& code, const eosio::datastream<const char *>& ds);

  // Registers the account with the accounting contract
  [[eosio::action]]
  void acctreg();

  // Creates a new category for partitioning tokens.
  [[eosio::action]]
  void acctcreate(const eosio::name& category);

  // Transfer tokens from one category to another.
  [[eosio::action]]
  void accttransfer(const eosio::name& from, const eosio::name& to, const eosio::asset& total);

  // Transfers tokens from the given category to the payee account.
  [[eosio::action]]
  void acctpay(const eosio::name& fromcat, 
               const eosio::name& payee, 
               const eosio::asset& total, 
               const std::string& memo);

  // Schedules a deferred payment from the given category to the payee account.
  [[eosio::action]]
  void acctdefer(const eosio::name& fromcat, 
                 const eosio::name& payee, 
                 const eosio::asset& total, 
                 const std::string& memo,
                 uint64_t deferfor);

  // Schedules a recurring transfer from the 'fromcat' category to the 'payee' account.
  [[eosio::action]]
  void acctsched(const eosio::name& fromcat, 
                 const eosio::name& payee, 
                 const eosio::asset& total, 
                 const std::string& memo,
                 uint64_t recurevery,
                 uint64_t recurcount,
                 uint64_t deferfor);

  // Lists deferred/pending transfers for a category
  [[eosio::action]]
  void translist();

  // Processes deferred payments
  [[eosio::action]]
  void processdef();

  // Processes recurring payments
  [[eosio::action]]
  void processrec();

  // Processes deferred and then recurring payments
  [[eosio::action]]
  void process();

  // Cancels a deferred/pending transfers
  [[eosio::action]]
  void transcancel(uint64_t transferid);

  // Lists the categories.
  [[eosio::action]]
  void acctlist();

  // Clears out the databases
  [[eosio::action]]
  void acctreset();

  // Verifies the balance in the given category
  // The dummy parameter is necessary to avoid duplicate transaction exceptions.  It appears that the -f option in cleos would overcome this.
  [[eosio::action]]
  void assertbal(const eosio::name& category, const eosio::asset& quantity, uint64_t dummy);

  // Listens to the transfer action on eosio.token contract.
  [[eosio::on_notify("eosio.token::transfer")]]
  void transferred(const eosio::name& from, const eosio::name& to, const eosio::asset& quantity, const std::string& memo);

  // Listens to the issue action on the eosio.token contract.
  [[eosio::on_notify("eosio.token::issue")]]
  void issued(const eosio::name& to, const eosio::asset& quantity, const std::string& memo);

private:
  template <typename Predicate>
  void execute(const eosio::name& category, const Predicate& p);

  template <typename Predicate>
  void execute(const Predicate& p);

  template <typename Transfer_type>
  bool execute_transfer(const Transfer_type& the_transfer);

  template <typename Transfer_type>
  void send_transfer(const Transfer_type& the_transfer);

  template <typename Transfer_type>
  void check_balance(const Transfer_type& the_transfer);

  template <typename Transfer_type>
  void reserve(const Transfer_type& the_transfer);

private:
  categories_table m_categories;
  deferred_transfers_table m_deferred_transfers;
  recurring_transfers_table m_recurring_transfers;
  system_table m_system;
};