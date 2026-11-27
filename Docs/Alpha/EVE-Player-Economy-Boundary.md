# EVE evidence: player assets and the shared economy

## Decision for SSW

Player-owned ships, fittings, stored equipment, cargo, skins, skills and wallet remain in the player record. The standalone economy owns NPC inventories, budgets, mines, production and freight. The client presents those domains together but does not copy the whole pilot into the NPC universe. A future player/account server can own the player record independently. Player ownership does not imply client authority in an online deployment.

The earlier whole-pilot migration requirement is withdrawn. Uncommitted whole-pilot migration code was removed after this clarification; no real save was migrated or modified. The native service remains disconnected from live play until the trade bridge, NPC projection and single-advancement gate are verified.

## Static artifact evidence

REA opened the installed code.ccp as a ZIP and inventoried 13,170 file occurrences, with no integrity contradictions in that inventory. Archive SHA-256: 328170f160fca4d355e70912e1b1ceda7781ac38416074a03c1ddf9af5e28581. REA inventory parent evidence: ev_f4bd2cc03774b73b67448de842f5d3a958f9089f2f7966fae34e68f00aea2bfb; member inventory evidence: ev_a04e7eb9934178ffb462609df081989a0362ed8a4923fab29e2e6db8f15acc78.

The built-in REA PYC reader supports newer Python; the existing local xdis reader was used explicitly for these Python 2.7 modules without importing or executing them. Five modules and 278 nested code objects/functions were indexed. Source metadata identifies V24.02, not a claim that this matches the current EVE release. Raw disassembly, hashes and REA inventory evidence are retained locally in Saved/Analysis/Phase5-Player-Economy-Boundary. Disassembler stack annotations are aids to interpretation, not recovered original source.

| Inspected module/function | Direct static observation | Implication for SSW |
| --- | --- | --- |
| services/marketsvc.pyj: GetMarketProxy, BuyStuff, BuyMulti, SellMulti, CancelOrder | Obtains marketProxy; submits PlaceBuyOrder, BuyMultipleItems, PlaceMultiSellOrder and CancelCharOrder. Requests include station/type/quantity/price and personal/corporation context. | UI submits bounded trade requests; it does not directly adjust shared stock. |
| marketsvc.pyj: OnOwnOrdersChanged | Invalidates character/type/location order caches, emits an escrow-change event, and can blink the market-orders button. | Apply targeted change events after accepted results; avoid rebuilding unrelated panels. |
| walletSvc.pyj: startup, OnAccountChange, _OnCharAccountChange | Retrieves cash balance from the remote account service; account updates include ownerID and balance. Personal changes update local UI signals. | Wallet belongs to the player domain; displayed values are a projection of that domain's authority. |
| environment/invCache.pyj: GetInventoryFromId, OnItemChange | Obtains inventories through inventoryMgr, tracks item/owner/location changes, removes stale ownership entries and broadcasts item changes. | Persist item identity, owner, location and compartment separately. Ownership changes are explicit transactions. |
| assetholding_svc.pyj: remote_inventory | Resolves a remote inventoryAssetHoldingMgr. The other methods inspected concern diagnostic/validation toggles. | Corroborates a remote inventory boundary; these diagnostic toggles are not gameplay features to copy. |

These are client-side logical service boundaries. They do not prove CCP's physical databases, microservice deployment, locking implementation, transaction protocol or NPC economic algorithms. No EVE server code was recovered, and no EVE source is being copied into SSW.

## Public CCP corroboration

- [Asset location IDs](https://docs.esi.evetech.net/docs/asset_location_id.html) documents parent item IDs, stations, structures and active-ship context. This is an archived official reference; it now points to the current developer site.
- [Buy and sell orders](https://support.eveonline.com/hc/en-us/articles/203218932-Buy-and-Sell-Orders) describes brokered orders, including immediate trades, station/range constraints, matching and remaining quantities.
- [Corporation orders and deliveries](https://support.eveonline.com/hc/en-us/articles/203280351-Corporation-Orders-and-Market-Deliveries) distinguishes ownership and station-specific delivery access.
- [Net worth](https://support.eveonline.com/hc/en-us/articles/115002091569-Total-Net-Worth-Indicator) distinguishes owned assets, wallet funds and escrow. An appraisal is not spendable balance.

These sources corroborate game semantics. They do not disclose the internal commit protocol.

## SSW transaction design (our implementation, not a recovered CCP protocol)

1. Obtain an authoritative quote for NPC stock at an accessible location. A displayed price is not a stock reservation.
2. Prepare the trade on the NPC service: reserve stock for a purchase, or NPC budget and receiving capacity for a sale. Do not create a player wallet there.
3. Persist the player's side in a local outbox/escrow record, including the receipt ID and destination. Pending goods/funds cannot be spent twice.
4. Settle once on the NPC service. Its durable receipt is the recovery source if the reply is lost.
5. Apply the result once to player assets, then persist the applied receipt and publish inventory/wallet changes. If the native side cancelled, release/refund player escrow instead.
6. On restart reconcile unfinished outbox records before allowing dependent actions. Do not infer success from a timeout or give refunds for already-committed trades.

The initial native prepare/settle/cancel handlers use the trusted local player adapter boundary. A playerEscrowId is a recovery reference, not cryptographic proof of funds. Public multiplayer must use an authenticated player/gameplay authority with server-verifiable authorization; a client-supplied escrow reference alone is insufficient. SSW costs, included GST, progression and balance remain SSW policy.

## Initial implementation scope

The native external-trade module now implements preparation, cancellation, settlement and status with caller-owned player assets kept outside the NPC world. Reservations and NPC budget escrow persist with the world. Preparations expire after five minutes of economy time, freeing resources; the eventual player outbox must reconcile cancellation/refund or committed delivery. Exact request contents are retained with receipts so another operation cannot reuse an ID. These mechanisms are SSW design choices, not claims about CCP transaction internals. The live UI/outbox, NPC projection and launcher activation remain open gates.
