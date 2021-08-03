#include <clashdomestg.hpp>

ACTION clashdomestg::createblend(name authorized_user, name target_collection, int32_t target_template, vector<int32_t> templates_to_mix) {

   require_auth(get_self());

   // Does target collection exist?
   auto itrCollection = atomicassets::collections.require_find(target_collection.value, "Error 12: No collection with this name exists!");

   // Get target collection info
   atomicassets::templates_t _templates = atomicassets::templates_t(ATOMIC, target_collection.value);

   // Does the template exist in collection?
   auto itrTemplates = _templates.require_find(target_template, "Error 13: No template with this id exists!");

   // Is this smart contract authorized in target collection?
   check(isAuthorized(target_collection, CONTRACTN), "Error 14: You must add in your collection this contract account as authorized!");

   // Is this user authorized in this colecction?
   check(isAuthorized(target_collection, authorized_user), "Error 15: You are not authorized to use this collection!");

   // Create blend info (if exists, update mixture templates)
   auto itrTarget = _blenders.find(target_template);
   if (itrTarget == _blenders.end()) {
      _blenders.emplace(_self, [&](auto &rec) {
         rec.owner = authorized_user;
         rec.collection = target_collection;
         rec.target = target_template;
         rec.mixture = templates_to_mix;
      });
   } else {
      _blenders.modify(itrTarget, _self, [&](auto &rec) {
         rec.mixture = templates_to_mix;
      });
   }
}

[[eosio::on_notify("atomicassets::transfer")]] void clashdomestg::blenderize(const name &from, const name &to, vector<uint64_t> asset_ids, const string &memo) {

   // ignore NFTs sends by this smart contract. IMPORTANT!
   if (from != CONTRACTN) {

        check(memo.size() < 100, "wrong memo!");
        uint32_t target = stol(memo);

        // Check if blend exists
        auto itrBlender = _blenders.require_find(target, "Error 02: There is no blender for this target!");

        // Check if this smart contract is authorized
        check(isAuthorized(itrBlender->collection, itrBlender->owner), "Error 03: The collection owner has disavowed this account!!");

        // Get id templates from NFTs received
        vector<int32_t> mainMixtures = itrBlender->mixture;
        vector<int32_t> blendTemplates = {};
        atomicassets::assets_t _assets = atomicassets::assets_t(ATOMIC, get_self().value);

        auto itrAsset = _assets.begin();
        // for (auto it = asset_ids.begin(); it != asset_ids.end(); it++)
        for (size_t i = 0; i < asset_ids.size(); i++) {
            // itrAsset = _assets.find(*it);
            itrAsset = _assets.find(asset_ids[i]);
            blendTemplates.push_back(itrAsset->template_id);
        }

        // Check if mixture matches
        sort(blendTemplates.begin(), blendTemplates.end()); // Make sure that vectors are sorted
        sort(mainMixtures.begin(), mainMixtures.end());
        check(blendTemplates == mainMixtures, "Error 05: There is no blender with these mixtures!");

        // Check collecton mint limit and supply
        atomicassets::templates_t _templates = atomicassets::templates_t(ATOMIC, itrBlender->collection.value);
        auto itrTemplate = _templates.require_find(itrBlender->target, "Error 17: No template found!");
        check(itrTemplate->max_supply > itrTemplate->issued_supply || itrTemplate->max_supply == 0, "Error 07: This blender cannot mint more assets for that target!");

        // Check if LUDIO payment has been conducted
        auto openblends_itr = openblends.find(from.value);

        if (openblends_itr == openblends.end()) {

            openblends.emplace(get_self(), [&](auto &_openblends) {
                _openblends.account_value = from.value;
                _openblends.collection = itrBlender->collection;
                _openblends.schema_name = itrTemplate->schema_name;
                _openblends.target = itrBlender->target; 
                _openblends.asset_ids = asset_ids;
                _openblends.nfts_transferred = true;
            });

        } else {

            check(openblends_itr->ludio_paid, "LUDIO hasn't been transferred");

            // All right; let's blend and burn
            mintasset(itrBlender->collection, itrTemplate->schema_name, itrBlender->target, from);
            burnmixture(asset_ids);

            openblends.erase(openblends_itr);
        }
   }
}

ACTION clashdomestg::delblend(name authorized_account, int32_t target_template) {
    
    require_auth(get_self());

    auto itrBlender = _blenders.require_find(target_template, "Error 01: No template with this ID!");

    check(isAuthorized(itrBlender->collection, authorized_account), "Error 16: You are not authorized to delete this blend!");

    _blenders.erase(itrBlender);
}

ACTION clashdomestg::transfer(const name &from, const name &to, const asset &quantity, const string &memo) {

    require_auth(from);

    if (from == _self || to != _self) {
        return;
    }

    // CHECK IF IS IT A VALID LUDIO PAYMENT
    check(quantity.symbol.is_valid(), "invalid quantity");
    check(quantity.symbol == LUDIO_SYMBOL, "only LUDIO tokens allowed");
    check(quantity.amount > 0, "only positive LUDIO transfer allowed");

    if (memo == "clashdome struggle new blend") {

        // TODO: MIRAR Q ES LA CANTIDAD DE LUDIO CORRECTA

        auto openblends_itr = openblends.find(from.value);

        // CHECK IF THE NFTs HAVE BEEN TRANSFERED
        if (openblends_itr == openblends.end()) {

            openblends.emplace(get_self(), [&](auto &_openblends) {
                _openblends.account_value = from.value;
                _openblends.ludio_paid = true;
            });

        } else {

            check(openblends_itr->nfts_transferred, "NFTs haven't been transferred");

            // HACER LA MEZCLA
            mintasset(openblends_itr->collection, openblends_itr->schema_name, openblends_itr->target, from);
            burnmixture(openblends_itr->asset_ids);

            openblends.erase(openblends_itr);
        }
    }
}

void clashdomestg::clopenblends() {
    
    require_auth(get_self());

    auto it = openblends.begin();
    while (it != openblends.end()) {
        it = openblends.erase(it);
    }
}