#include <accounting/accounting.h>
#include <eosio.token/eosio.token.hpp>

#include <eosio/print.hpp>

#include <chrono>
#include <functional>
#include <string>
#include <string_view>

eosio::microseconds current_time_ms()
{
  return eosio::microseconds{eosio::current_time_point().time_since_epoch().count()};
}

const eosio::asset accounting::default_asset = eosio::asset{0, eosio::symbol{"SYS", 4}};

accounting::accounting(const eosio::name& receiver, const eosio::name& code, const eosio::datastream<const char *>& ds) 
  : contract{receiver, code, ds},
    m_categories{receiver, receiver.value},
    m_deferred_transfers{receiver, receiver.value},
    m_recurring_transfers{receiver, receiver.value},
    m_system{receiver, receiver.value}
{
}

template <typename Predicate>
void accounting::execute(const eosio::name& category, const Predicate& p)
{
  auto self = get_self();
  eosio::check(eosio::is_account(self), "not a valid eosio account.");

  if (category.value != 0)
  {
    auto it = m_categories.find(category.value);
    eosio::check(it == std::end(m_categories), category.to_string() + " category already exists");
  }

  p(self);
}

template <typename Predicate>
void accounting::execute(const Predicate& p)
{
  execute({}, p);
}

void accounting::acctreg()
{
  auto p = [&](const auto& name)
  {
    m_categories.emplace(name, [&](auto& row)
    {
      row.name = default_category;
      row.available = eosio::token::get_balance(eosio::name("eosio.token"), name, default_symbol_code);
    });
    eosio::print(name.to_string() + " account registered");
  };
  execute(default_category, p);
}

void accounting::acctcreate(const eosio::name& category)
{
  auto p = [&](const auto& name)
  {
    m_categories.emplace(name, [&](auto& row)
    {
      row.name = category;
    });
  };
  execute(category, p);
}

void accounting::accttransfer(const eosio::name& from, const eosio::name& to, const eosio::asset& total)
{
  if (from == to)
  {
    return;
  }

  if (total.amount == 0)
  {
    return;
  }

  auto p = [&](const auto& name)
  {
    auto from_it = m_categories.find(from.value);
    eosio::check(total.amount >= 0, "Negative amounts are not allowed");
    eosio::check(from_it != std::end(m_categories), std::string{"category "} + from_it->name.to_string() + std::string{" does not exist"});
    eosio::check(from_it->available >= total, std::string{"category "} + from_it->name.to_string() + std::string{" has insufficient funds"});

    auto to_it = m_categories.find(to.value);
    eosio::check(to_it != std::end(m_categories), std::string{"category "} + to_it->name.to_string() + std::string{" does not exist"});
    
    m_categories.modify(from_it, name, [&](auto& row)
    {
      row.available -= total;
    });

    m_categories.modify(to_it, name, [&](auto& row)
    {
      row.available += total;
    });
  };
  execute(p);
}

void accounting::acctlist()
{
  auto p = [&](const auto& name)
  {
    auto output = std::string{""};

    for (const auto& category : m_categories)
    {
      output += "[";
      output += std::to_string(category.primary_key());
      output += ", ";
      output += category.name.to_string();
      output += ", ";
      output += category.available.to_string();
      output += "],";
    }
    
    if (!output.empty())
    {
      output.pop_back();
    }

    eosio::print(output);
  };
  execute(p);
}

uint64_t accounting::system::fetch_add(uint64_t count, accounting& accounting_contract)
{
  auto self = accounting_contract.get_self();
  auto system_info = accounting_contract.m_system.get_or_create(self);
  auto next_id = system_info.next_id++;
  accounting_contract.m_system.set(system_info, self);
  return next_id;
}

void accounting::transfer::swap(transfer& other)
{
  if (this == &other)
  {
    return;
  }

  using std::swap;
  std::swap(other.id, id);
  std::swap(other.from_category, from_category);
  std::swap(other.to_account, to_account);
  std::swap(other.amount, amount);
  std::swap(other.memo, memo);
}

accounting::transfer accounting::transfer::create(eosio::name from_category,
                                                  eosio::name to_account,
                                                  eosio::asset amount,
                                                  std::string memo,
                                                  accounting& accounting_contract)
{
  auto next_id = system::fetch_add(1, accounting_contract);
  auto my_transfer = transfer
  {
    .id = next_id,
    .from_category = std::move(from_category),
    .to_account = std::move(to_account),
    .amount = std::move(amount),
    .memo = std::move(memo)
  };
  return my_transfer;
}

void accounting::deferred_transfer::swap(deferred_transfer& other)
{
  if (this == &other)
  {
    return;
  }

  transfer::swap(other);
  using std::swap;
  std::swap(other.defer_until, defer_until);
}

accounting::deferred_transfer accounting::deferred_transfer::create(eosio::name from_category,
                                                                    eosio::name to_account,
                                                                    eosio::asset amount,
                                                                    std::string memo,
                                                                    eosio::microseconds defer_for,
                                                                    accounting& accounting_contract)
{
  auto next_id = system::fetch_add(1, accounting_contract);
  auto my_transfer = deferred_transfer
  {
    {
      .id = next_id,
      .from_category = std::move(from_category),
      .to_account = std::move(to_account),
      .amount = std::move(amount),
      .memo = std::move(memo)
    },
    .defer_until = current_time_ms() + defer_for
  };
  return my_transfer;
}

void accounting::recurring_transfer::swap(recurring_transfer& other)
{
  if (this == &other)
  {
    return;
  }

  deferred_transfer::swap(other);
  using std::swap;
  std::swap(other.recur_every, recur_every);
  std::swap(other.recur_count, recur_count);
}

accounting::recurring_transfer accounting::recurring_transfer::create(eosio::name from_category,
                                                                      eosio::name to_account,
                                                                      eosio::asset amount,
                                                                      std::string memo,
                                                                      eosio::microseconds recur_every,
                                                                      uint64_t recur_count,
                                                                      eosio::microseconds defer_for,
                                                                      accounting& accounting_contract)
{
  auto next_id = system::fetch_add(1, accounting_contract);
  auto my_transfer = recurring_transfer
  {
    {
      {
        .id = next_id,
        .from_category = std::move(from_category),
        .to_account = std::move(to_account),
        .amount = std::move(amount),
        .memo = std::move(memo)
      },
      .defer_until = current_time_ms() + defer_for
    },
    .recur_every = recur_every,
    .recur_count = recur_count
  };
  return my_transfer;
}                                                  

template <typename Transfer_type>
void accounting::check_balance(const Transfer_type& the_transfer)
{
  eosio::check(eosio::is_account(the_transfer.to_account), the_transfer.to_account.to_string() + " not a valid eosio account");

  auto it = m_categories.find(the_transfer.from_category.value);
  eosio::check(it != std::end(m_categories), the_transfer.from_category.to_string() + " is not an available category");

  // Non recurring transfers must be able to reserve funds from the available balance.
  eosio::check(it->available >= the_transfer.amount, it->name.to_string() + " doesn't have sufficient funds available for transfer");
}

template <>
void accounting::check_balance<accounting::recurring_transfer>(const accounting::recurring_transfer& the_transfer)
{
  eosio::check(eosio::is_account(the_transfer.to_account), the_transfer.to_account.to_string() + " not a valid eosio account");

  auto it = m_categories.find(the_transfer.from_category.value);
  eosio::check(it != std::end(m_categories), the_transfer.from_category.to_string() + " is not an available category");
}

template <typename Transfer_type>
void accounting::reserve(const Transfer_type& the_transfer)
{
  m_categories.modify(m_categories.find(the_transfer.from_category.value), get_self(), [&](auto& row)
  {
    row.available -= the_transfer.amount;
    row.reserved += the_transfer.amount;
  });
}

template <>
void accounting::reserve<accounting::recurring_transfer>(const accounting::recurring_transfer& the_transfer)
{
  // recurring payments do not reserve funds.
}

template <typename Transfer_type>
void accounting::send_transfer(const Transfer_type& the_transfer)
{
  // Perform the transfer
  // https://developers.eos.io/manuals/eosio.cdt/latest/how-to-guides/how_to_create_and_use_action_wrappers
  auto self = get_self();
  auto transfer_ = eosio::token::transfer_action{"eosio.token"_n, {self, "active"_n}};
  transfer_.send(self, the_transfer.to_account, the_transfer.amount, the_transfer.memo);

  // This could also be a transaction, which means that if the call fails then
  // the parent transaction would fail too.
}

template <typename Transfer_type>
bool accounting::execute_transfer(const Transfer_type& the_transfer)
{ 
  // Deduct the amount of the transfer from the category.
  auto it = m_categories.find(the_transfer.from_category.value);
  m_categories.modify(it, get_self(), [&](auto& row)
  {
    row.reserved -= the_transfer.amount;
  });

  send_transfer(the_transfer);
  return true;
}

template <>
bool accounting::execute_transfer<accounting::recurring_transfer>(const accounting::recurring_transfer& the_transfer)
{ 
  auto it = m_categories.find(the_transfer.from_category.value);

  // Funds aren't reserved for recurring payments.  We need to make sure that the available balance can handle it.
  if (it->available < the_transfer.amount)
  {
    eosio::print(it->name.to_string() +  " has insufficient funds to process transfer to " + the_transfer.to_account.to_string());
    return false;
  }

  // Deduct the amount of the transfer from the category.
  m_categories.modify(it, get_self(), [&](auto& row)
  {
    row.available -= the_transfer.amount;
  });

  send_transfer(the_transfer);
  return true;
}

void accounting::acctpay(const eosio::name& fromcat, 
                         const eosio::name& payee, 
                         const eosio::asset& total, 
                         const std::string& memo)
{
  auto the_transfer = transfer::create(fromcat, payee, total, memo, *this);
  check_balance(the_transfer);
  reserve(the_transfer);
  execute_transfer(the_transfer);
}                         

void accounting::acctdefer(const eosio::name& fromcat, 
                           const eosio::name& payee, 
                           const eosio::asset& total, 
                           const std::string& memo,
                           uint64_t deferfor)
{
  auto the_transfer = deferred_transfer::create(fromcat, payee, total, memo, eosio::microseconds{static_cast<int64_t>(deferfor)}, *this);
  check_balance(the_transfer);
  reserve(the_transfer);
  m_deferred_transfers.emplace(get_self(), [&](auto& row)
  {
    row.swap(the_transfer);
  });
}                           

void accounting::acctsched(const eosio::name& fromcat, 
                           const eosio::name& payee, 
                           const eosio::asset& total, 
                           const std::string& memo,
                           uint64_t recurevery,
                           uint64_t recurcount,
                           uint64_t deferfor)
{
  auto the_transfer = recurring_transfer::create(fromcat, payee, total, memo, eosio::microseconds{static_cast<int64_t>(recurevery)}, recurcount, eosio::microseconds{static_cast<int64_t>(deferfor)}, *this);
  check_balance(the_transfer);
  reserve(the_transfer);
  m_recurring_transfers.emplace(get_self(), [&](auto& row)
  {
    row.swap(the_transfer);
  });
}                 

void accounting::translist()
{
  auto output = std::string{""};

  auto print_transfers = [&](const auto& transfer_table)
  {
    for (const auto& transfer : transfer_table)
    {
      output += "[";
      output += std::to_string(transfer.primary_key());
      output += ", ";
      output += transfer.from_category.to_string();
      output += ", ";
      output += transfer.to_account.to_string();
      output += ", ";
      output += transfer.amount.to_string();
      output += "],";
    }
  };

  print_transfers(m_deferred_transfers);
  print_transfers(m_recurring_transfers);

  if (!output.empty())
  {
    output.pop_back();
  }

  eosio::print(output);
}

void accounting::process()
{
  processdef();
  processrec();
}

void accounting::processdef()
{
  for (auto it = std::begin(m_deferred_transfers); it != std::end(m_deferred_transfers); )
  {
    eosio::print(std::to_string(it->defer_until.count()) + ", " + std::to_string(current_time_ms().count()));

    if (it->defer_until > current_time_ms())
    {
      // This transfer is still deferred
      ++it;
      continue;
    }

    // Funds should have already been reserved for this transfer.
    execute_transfer(*it);
    it = m_deferred_transfers.erase(it);
  }
}

void accounting::processrec()
{
  auto self = get_self();

  for (auto it = std::begin(m_recurring_transfers); it != std::end(m_recurring_transfers); )
  {
    if (it->defer_until > current_time_ms())
    {
      // This transfer is still deferred.
      ++it;
      continue;
    }

    if (!execute_transfer(*it))
    {
      // Unable to successfully process recurring payment.
      // will try again the next time processing.
      ++it;
      continue;
    }

    if (static_cast<int64_t>(it->recur_count) - 1 < 0)
    {
      it = m_recurring_transfers.erase(it);
      continue;
    }

    m_recurring_transfers.modify(it, self, [&](auto& row)
    {
      --row.recur_count;
      row.defer_until = current_time_ms() + row.recur_every;
    });

    ++it;
  }
}

void accounting::transcancel(uint64_t transferid)
{
  auto it = m_deferred_transfers.find(transferid);
  if (it != std::end(m_deferred_transfers))
  {
    // Release funds that were reserved for this.
    auto cat_it = m_categories.find(it->from_category.value);
    m_categories.modify(cat_it, get_self(), [&](auto& row)
    {
      row.available += it->amount;
      row.reserved -= it->amount;
    });
    m_deferred_transfers.erase(it);
    return;
  }

  auto it2 = m_recurring_transfers.find(transferid);
  if (it2 != std::end(m_recurring_transfers))
  {
    m_recurring_transfers.erase(it2);
  }
}

void accounting::acctreset()
{
  auto clear = [](auto& table)
  {
    for (auto it = std::begin(table); it != std::end(table); )
    {
      it = table.erase(it);
    }
  };

  clear(m_categories);
  clear(m_deferred_transfers);
  clear(m_recurring_transfers);
}

void accounting::assertbal(const eosio::name& category, const eosio::asset& quantity, uint64_t dummy)
{
  auto p = [&](const auto& name)
  {
    auto it = m_categories.find(category.value);
    eosio::check(it != std::end(m_categories), category.to_string() + " does not exist");
    eosio::check(it->available == quantity, it->available.to_string() + " != " + quantity.to_string());
  };
  execute(p);
}

void accounting::transferred(const eosio::name& from, const eosio::name& to, const eosio::asset& quantity, const std::string& memo)
{
  if (quantity.symbol.code() != default_symbol_code)
  {
    // Only interested in SYS tokens
    return;
  }

  if (from == to)
  {
    // Doesn't make sense to transfer the same money to myself.
    return;
  }

  auto self = get_self();

  enum class operation { deposit, deduct };
  auto update = [&](auto op)
  {
    auto it = m_categories.find(default_category.value);

    if (it == std::end(m_categories))
    {
      // account hasn't registered yet
      return;
    }

    m_categories.modify(it, self, [&](auto& row)
    {
      if (op == operation::deposit)
      {
        row.available += quantity;
      }
      else
      {
        eosio::check(row.available >= quantity, "insufficient funds for transfer");
        row.available -= quantity;
      }
    });
  };

  if (from == self)
  {
    update(operation::deduct);
  }
  else
  {
    update(operation::deposit);
  }
}

void accounting::issued(const eosio::name& to, const eosio::asset& quantity, const std::string& memo)
{
  if (quantity.symbol.code() != default_symbol_code)
  {
    // Only interested in SYS tokens
    return;
  }

  auto self = get_self();
  auto it = m_categories.find(default_category.value);

  if (it == std::end(m_categories))
  {
    // account hasn't registered yet
    return;
  }

  m_categories.modify(it, self, [&](auto& row)
  {
    row.available += quantity;
  });
}