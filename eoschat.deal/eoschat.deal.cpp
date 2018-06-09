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

            action proposed_trx_action(
                    permission_level{multisig, N(active)},
                    N(eosio.token),
                    N(transfer),
                    currency::transfer{multisig, executor, quantity, ""});

            transaction trx(time_point_sec(now() + deal_proposal_duration));
            trx.actions.push_back(std::move(proposed_trx_action));

            name proposal_name{multisig};

            deals.emplace(scope_account, [&](auto& deal) {
                deal = {initiator,
                        executor,
                        quantity,
                        conditions,
                        multisig,
                        escrow_agent,
                        proposal_name};
            });

            dispatch_inline(
                    N(eosio.msig),
                    N(propose),
                    std::vector<permission_level>{{initiator, N(active)}},
                    std::make_tuple(
                            initiator,
                            proposal_name,
                            std::vector<permission_level>{{multisig, N(active)}},
                            trx
                    ));
        }

        //@abi action
        void accept(account_name account, uint64_t key) {
            deal_index deals(_self, scope_account);
            auto it = deals.find(key);

            eosio_assert(it != deals.end(), "No table entry for provided key");

            dispatch_inline(
                    N(eosio.msig),
                    N(approve),
                    std::vector<permission_level>{{account, N(active)}},
                    std::make_tuple(
                            it->initiator,
                            it->proposal_name,
                            std::vector<permission_level>{{account, N(active)}}
                    ));
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
            name         proposal_name;

            uint64_t primary_key() const { return multisig; }

            EOSLIB_SERIALIZE(deal_info, (initiator)(executor)(quantity)(conditions)(multisig)(escrow_agent)(proposal_name))
        };

        typedef multi_index<N(deals), deal_info> deal_index;

        static const account_name scope_account = N(eoschat.deal);
        static const uint64_t deal_proposal_duration = 60*60*24*30; //seconds = 30 days
    };

    EOSIO_ABI(deal, (init))
}
