# Phase F: market commands and transaction queue

27 November 2026, pilot local date. This is the second incremental Phase F migration.

## Evidence and boundary

REA 6.9.0 opened and inventoried the locally installed `C:/CCP/EVE Online/tq/code.ccp` ZIP, SHA-256 `328170f160fca4d355e70912e1b1ceda7781ac38416074a03c1ddf9af5e28581`. Inventory evidence: `ev_f4bd2cc03774b73b67448de842f5d3a958f9089f2f7966fae34e68f00aea2bfb`, summary `ev_9dcddc637c2f0b0687a0e8924adc437ba0bc88fec4a773213a787051d0ef22b6`. This establishes container membership, not running behavior.

REA's PYC inspector does not support Python 2.7. The existing xdis fallback statically decoded four selected modules without importing or executing the target. Evidence, function indexes, complete disassemblies and hashes are retained in `Saved/Analysis/PhaseF-Market-Queues`; REA's inventory bundle is `rea-evidence.json` there. The shipped modules identify V24.02 in their recorded source paths; that does not establish the live service version.

| Module | SHA-256 | Direct static observation |
| --- | --- | --- |
| buySellMultiBase.pyj | 7df0829eaf7839d4ef3b971a3ebab8edef51ff96810ae5afb6d3cca2f9382517 | Per-item validation, order availability, warnings and cancellation entry points. |
| buyMultiFromBase.pyj | 1674553320aaf0f8a756576c87a42f7a9f59913d540e5064528bbcb3565b5dba | `_PerformTrade` separates validated/failed items, calls `marketQuote.BuyMulti`, tracks returned order IDs and checks expiry. `GetValidatedItem` includes station, item, price, quantity, minimum volume and price delta fields. |
| quote.pyj | d9e32105bfe8e943eab18ccd77985ba53814a1b7c578e294f89bfffd8217f250 | Modify/cancel paths and expiry display based on issue date plus duration. |
| orders.pyj | e9070a256073437ab5b058cdb632524a17ea82cbfa083dc5b9b40b5ed9d1fa86 | Own-order change notifications refresh orders; separate buy/sell lists expose modify/cancel actions. |

Inference used for SSW: independently validated requests, per-request outcomes and change notifications are a useful fit for an asynchronous marketplace. These files do not prove the server's internal queue, matching rules or scheduling. No EVE server implementation, prices or balances are copied.

## Implemented SSW behavior

Immediate buys/sells and limit-order placement, modification and cancellation now calculate on the existing dedicated economy worker. NPC crossing preserves the former stock, money, fee and capacity rules. The game-thread adapter performs admission, live cargo projection and durable saves. A typed market result cannot enter the generic economy commit accidentally.

The native FIFO holds at most 16 active/pending requests, executes one at a time, and retains a bounded 32-row recent session history. The Marketplace queue tab shows request, status and result with the normal panel/table styling. A selected queued request can be cancelled. Processing and saved requests cannot be cancelled using this action; an already-open limit order uses the existing order cancellation operation.

Each request freezes its approved quote and receipt. Before dispatch and completion the host checks authority, account/session, physical craft, station, quote, inventory revision and original order book. Changed price/stock/funds/capacity rejects the request without an exchange. The next request snapshots the latest committed state, including previous saves, rather than reusing a batch-wide stale wallet.

Failed persistence restores credits, cargo, order escrow and reservations. The UI is notified only after saved confirmation or rejection; closing it does not discard a valid request. Loading a profile cancels unstarted requests and clears private queue history. Shutdown cancels uncommitted requests and joins the worker. Pending requests are session-only, while executed transactions and open limit orders remain in existing saves.

## Remaining Phase F work

Passive resting-order expiry/matching still runs through the existing simulation adapter. Fleet repair scheduling, hull commerce/insurance, contracts and other inventory adapters still require migration. Full flight/AI movement and narrow-phase damage remain game-thread work; the initial simulation broad phase is already migrated. These boundaries must not be described as a completed three-domain split.

## Verification

Final build and full-suite results are recorded in Phase-F-Execution-Domains.md after completion. Seven focused market/queue tests passed, covering native execution, rollback/replay protection, funded partial fills, escrow repricing/cancellation, save round trips, bounded FIFO behavior, sequential wallet/cargo updates and private history reset.
