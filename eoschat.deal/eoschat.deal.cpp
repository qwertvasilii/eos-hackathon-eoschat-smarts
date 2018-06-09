#include <eosiolib/eosio.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>
#include <string>
#include <vector>

namespace eoschat {

    using namespace eosio;

    class deal : public contract
    {
    public:
        explicit deal(account_name self) : contract(self) {}

        //@abi action
        void init(
                account_name initiator,
                account_name executor,
                asset        quantity,
                std::string  conditions,
                account_name multisig,
                account_name escrow_agent
        ) {
            require_auth(initiator);

            //a lot of checks
            eosio_assert( is_account( executor ),     "executor account does not exist");
            eosio_assert( is_account( escrow_agent ), "escrow agent account does not exist");
            eosio_assert( is_account( multisig ),     "multisig account does not exist");

            eosio_assert(initiator    != executor,     "Initiator and executor are identical");
            eosio_assert(initiator    != escrow_agent, "Initiator and escrow agent are identical");
            eosio_assert(escrow_agent != executor,     "Escrow agent and executor are identical");

            eosio_assert(initiator    != multisig,     "Initiator and multisig are identical");
            eosio_assert(executor     != multisig,     "Executor and multisig are identical");
            eosio_assert(escrow_agent != multisig,     "Escrow agent and multisig are identical");

            //TODO multisig assertion

            eosio_assert(!conditions.empty(), "Conditions are empty");

            // transfer initiator's asset to multisig account
            action act(
                    permission_level{initiator, N(active)},
                    N(eosio.token),
                    N(transfer),
                    currency::transfer{initiator, multisig, quantity, ""}
                    );
            act.send();

            deal_index deals(_self, scope_account);

            auto it = deals.find(multisig);
            eosio_assert(it == deals.end(), "Multisig account reuse");

            deals.emplace(scope_account, [&](auto& deal) {
                deal = {initiator,
                        executor,
                        quantity,
                        conditions,
                        multisig,
                        escrow_agent};
            });

            action proposed_trx_action(
                    permission_level{multisig, N(active)},
                    N(eosio.token),
                    N(transfer),
                    currency::transfer{multisig, executor, quantity, ""});

            transaction trx(time_point_sec(now() + 2592000));
            trx.actions.push_back(std::move(proposed_trx_action));

            dispatch_inline(
                    N(eosio.msig),
                    N(propose),
                    std::vector<permission_level>{{initiator, N(active)}},
                    std::make_tuple(initiator,
                    name{multisig},
                    std::vector<permission_level>{{multisig, N(active)}},
                    trx)
                    );
        }

    private:
        //@abi table deals
        struct deal_info {
            account_name initiator;
            account_name executor;
            asset        quantity;
            std::string  conditions;
            account_name multisig;
            account_name escrow_agent;

            uint64_t primary_key() const { return multisig; }

            EOSLIB_SERIALIZE(deal_info, (initiator)(executor)(quantity)(conditions)(multisig)(escrow_agent))
        };

        typedef multi_index<N(deals), deal_info> deal_index;

        static const account_name scope_account = N(eoschat.deal);
    };

    EOSIO_ABI(deal, (init))
}
