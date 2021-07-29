#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <atomicassets.hpp>
#include <atomicdata.hpp>

using namespace eosio;
using namespace std;

#define ATOMIC name("atomicassets")
#define EOSIO name("eosio")

#define CONTRACTN name("clashdomestg")
// -----------------------------------------------

#define RAMFEE 0.995    // Aprox...

CONTRACT clashdomestg : public contract 
{
public:

    using contract::contract;

    ACTION createblend(name authorized_user, name target_collection, int32_t target_template, vector<int32_t> templates_to_mix);
    ACTION delblend(name authorized_account, int32_t target_template);
    ACTION clopenblends();

    [[eosio::on_notify("atomicassets::transfer")]] void blenderize(const name &from, const name &to, vector<uint64_t> asset_ids, const string &memo);
    [[eosio::on_notify("clashdometkn::transfer")]] void transfer(const name &from, const name &to, const asset &quantity, const string &memo);

    clashdomestg(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds),
                                                                        _blenders(get_self(), get_self().value)
    {
    }

private:

    TABLE openblends_s {

        uint64_t account_value;
        name collection;
        name schema_name;
        int32_t target; 
        vector<uint64_t> asset_ids;

        uint64_t primary_key() const {return account_value;}
    };

    typedef multi_index <name("openblends"), openblends_s> openblends_t;
    openblends_t openblends = openblends_t(get_self(), get_self().value); 

    TABLE blender_item {

        name owner;              // Owner account name
        name collection;         // Collection name
        int32_t target;          // Template ID to mint (index)
        vector<int32_t> mixture; // Template IDs <array> to blend

        auto primary_key() const { return target; };
    };
    typedef multi_index<"blenders"_n, blender_item> blender_table;

    // Define tables handlers
    blender_table _blenders;

    static constexpr symbol LUDIO_SYMBOL = symbol(symbol_code("LUDIO"), 4);

    // Private Functions
    /*
        Check if user is authorized to mint NFTs
    */
    bool isAuthorized(name collection, name user) {

        auto itrCollection = atomicassets::collections.require_find(collection.value, "Error 15: No collection with this name exists!");
        bool authorized = false;
        vector<name> authAccounts = itrCollection->authorized_accounts;
        for (auto it = authAccounts.begin(); it != authAccounts.end() && !authorized; it++) {
            if (user == name(*it)) {
                authorized = true;
            }
        }
        return authorized;
    }

    /*
        Call AtomicAssets contract to mint a new NFT
    */
    void mintasset(name collection, name schema, int32_t template_id, name to) {
        
        vector<uint64_t> returning;
        atomicassets::ATTRIBUTE_MAP nodata = {};
        action(
            permission_level{CONTRACTN, name("active")},
            ATOMIC,
            name("mintasset"),
            make_tuple(CONTRACTN, collection, schema, template_id, to, nodata, nodata, returning))
            .send();
    }

    /*
        Call AtomicAssets contract to burn NFTs
    */
    void burnmixture(vector<uint64_t> mixture) {

        for (auto it = mixture.begin(); it != mixture.end(); it++) {
                
            action(
                permission_level{CONTRACTN, name("active")},
                ATOMIC,
                name("burnasset"),
                make_tuple(CONTRACTN, *it))
            .send();
        }
    }
};
