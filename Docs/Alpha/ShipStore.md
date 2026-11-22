# Ship Store: purchase, storage and fitting

The existing UI.ShipStorePopupClass / UShipStoreDlg now has Hulls, Skins, Components, Missiles and Drones tabs, an item search, sortable selected rows, live wallet balance, quantity controls and local delivery information. No additional panel registry entry is needed. Each item lists its GST-exclusive base, included tax/fees and final total; the charges cell has a breakdown tooltip. Catalog prices remain final amounts rather than adding a surcharge.

Hulls keep the existing faction, rank, skill, shipyard/OOB-stock and dock restrictions. A training override is capped by the normal catalog price, preventing the old 400,000-credit override from exceeding the new 57,400-credit Stormhawk price. Skins are purchased here as permanent pilot licenses; Player Logbook / Ship Skins now lists owned licenses for preview, application and reset only.

Component, missile and drone purchases are stored at the current dock in the pilot SaveGame. Components I–V are priced from sys.json. Fitting requires a locally stored matching component, keeps SSW's reactor validation, consumes one unit, and charges no second purchase. Removed owned components retain their damage/repair state in storage. Mission-issued parts are not converted into free player salvage. Mining Drone purchases can be transferred to the persistent ship cargo hold, then loaded through existing mining-drone controls. Missile purchases remain in local dock storage until a loadout consumes them. Authored missions draw from a finite carrier/base magazine; Free Flight draws from the pilot's missile stores at that dock. Unchanged loadouts do not consume ammunition twice, and unused rounds return to the same source during an exchange. Carrier-launched AI craft draw from the same magazine and return surviving rounds on recovery or stand-down. Initial stock and capacities are authored in Ships/missile_stores.json. This provides an inventory endpoint for future replenishment missions; it does not author those missions.

Equipment offers and prices are editable in Content/GameData/Ships/store_equipment.json. Hidden/secret weapon variants are not sold. Sale licenses use skins.json and material-instance bindings. Equipment vendors currently offer catalog supplies rather than consuming finite NPC production stock; existing hull stock is still finite. This is an explicit first-pass NPC vendor assumption, not recovered server behavior.

## Fees

Content/GameData/Cargo/transaction_fees.json defines 2% GST, 1% included service fees and a 1% non-refundable order-placement/repricing broker fee (one-credit minimum when enabled). Prices are inclusive: base plus included charges equals the advertised final total. Trade sellers receive the GST-exclusive proceeds; taxes and fees credit real authority/service accounts, with actual wallet deltas and a saved receipt breakdown. Purchases, repairs, insurance premiums, training services, skill books, blueprints, manufacturing installation, NPC consumption/freight settlements and public contract sales use explicit commercial transaction purposes. Mission rewards, starting allowances, indemnity payouts, collateral escrow and refunds are not new taxable sales.

New market orders explicitly enable brokerage/GST. Existing saved orders keep their original terms until repriced. Broker fees are separate from reserved order value, disclosed before placement, and not returned when an order is cancelled. Raw ledger primitives support untaxed internal transfers and fixtures; user-facing services opt into their correct commercial purpose.

## Evidence

REA inspected C:/CCP/EVE Online/tq/code.ccp, SHA-256 328170f160fca4d355e70912e1b1ceda7781ac38416074a03c1ddf9af5e28581 (retained inventory evidence ev_9dcddc637c2f0b0687a0e8924adc437ba0bc88fec4a773213a787051d0ef22b6). Python 2.7 client modules were inspected statically with the xdis fallback reader; nothing recovered was executed. Saved/Analysis/EveShipStore contains marketbase (238 functions), buyThisTypeWindow (50), requirements (6), wallet_balance (20), skins/controller (83), hashes and disassembly. Saved/Analysis/EveTransactionFees contains brokerFee, quote and buyThisTypeWindow.

Observed client paths cover market groups, station/system scope, filters, quantity and total updates, wallet notifications, licensed/unlicensed preview and application failure. The pinned official public SDE was examined for types, groups, marketGroups, dogmaAttributes and skinLicenses; counts/examples/hashes are retained in Saved/Analysis/EveShipStore/public-sde-store-evidence.json. These tables establish published identities, grouping and static fitting/license metadata, not live stock, order matching or authoritative fee logic. SSW's C++ services and fee rates are original adaptations, retaining SSW balance and no subscription gates.

Primary references: https://developers.eveonline.com/static-data ; https://support.eveonline.com/hc/en-us/articles/203218962-Broker-Fee-and-Sales-Tax ; https://support.eveonline.com/hc/en-us/articles/203218932-Buy-and-Sell-Orders .


## Training progression

New careers start with 5,000 credits; existing saved balances are retained. T1 pays 25,000 credits plus a 1,200-credit safe-recovery bonus, and T2 pays 30,000 plus 1,200. Completing both safely and buying the 57,400-credit Stormhawk leaves 5,000 credits before other optional purchases or expenses.
