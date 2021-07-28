#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <atomicassets.hpp>
#include <atomicdata.hpp>

using namespace eosio;
using namespace std;

#define ATOMIC name("atomicassets")
#define EOSIO name("eosio")

/*
   IMPORTANT: Define your smart contract name here
*/
#define CONTRACTN name("clashdomestg")
// -----------------------------------------------

#define RAMFEE 0.995    // Aprox...

CONTRACT clashdomestg : public contract 
{
public:
   using contract::contract;

   /*
      Blend Actions
   */
   ACTION createblend(name authorized_user, name target_collection, int32_t target_template, vector<int32_t> templates_to_mix);
   ACTION delblend(name authorized_account, int32_t target_template);

   /*
      System Actions
   */
   [[eosio::on_notify("atomicassets::transfer")]] void blenderize(name from, name to, vector<uint64_t> asset_ids, string memo);

   clashdomestg(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds),
                                                                    _rambalance(get_self(), get_self().value),
                                                                    _userres(EOSIO, CONTRACTN.value),
                                                                    _pending_ram(get_self(), get_self().value),
                                                                    _blenders(get_self(), get_self().value),
                                                                    _rammarket(EOSIO, EOSIO.value)
   {
   }

private:
   TABLE rambalance_item {
      name collection; // Collection name (Index)
      uint64_t bytes;  // Amount bytes available for this collection

      auto primary_key() const { return collection.value; };
   };
   typedef multi_index<"rambalance"_n, rambalance_item> rambalance_table;

   TABLE blender_item {
      name owner;              // Owner account name
      name collection;         // Collection name
      int32_t target;          // Template ID to mint (index)
      vector<int32_t> mixture; // Template IDs <array> to blend

      auto primary_key() const { return target; };
   };
   typedef multi_index<"blenders"_n, blender_item> blender_table;

   // AUX tables to read eosio smart contract tables and get RAM value

   struct userres_item
   {
      name owner;
      asset net_weight;
      asset cpu_weight;
      int64_t ram_bytes;

      auto primary_key() const { return owner.value; };
   };
   typedef multi_index<"userres"_n, userres_item> user_resources;

   struct connector_item
   {
      asset balance;
      double weight;
   };
   typedef connector_item connector;

   struct exchange_state
   {
      asset supply;
      connector base;
      connector quote;
      auto primary_key() const { return supply.amount; };
   };
   typedef multi_index<"rammarket"_n, exchange_state> rammarket;

   TABLE pending_ram_item
   {
      name owner;
      int64_t ram_bytes;
      asset amount;

      auto primary_key() const { return owner.value; };
   };
   typedef multi_index<"pendingram"_n, pending_ram_item> pending_ram_table;

   // Define tables handlers
   rambalance_table _rambalance;
   user_resources _userres;
   pending_ram_table _pending_ram;
   blender_table _blenders;
   rammarket _rammarket;

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