# Background economy checkpoint

The accelerated Janus integration test runs 360 native-worker steps of 60 simulated seconds without a world, actors or open panels. It uses the authored mining, cargo and refinery catalogs, finite initial market stock, and abstract freight. A unique temporary SaveGame is written and reloaded halfway through; real pilot saves are untouched.

Observed focused result: two tests passed. Six simulated hours completed 20 refinery batches and 18 freight deliveries, with 22,591 history samples and 21,437 settled trades. Runtime was about 4.19 seconds. Market trading includes authored startup inventory; those trades are not evidence that every item was manufactured during this run. This is not a six-hour wall-clock packaged-client soak.

A regression exposed starvation in TrafficAI: an empty sector retained the oldest service timestamp when scheduling produced no journey. Dispatch now records examination even when there is no eligible trade. Producers in unloaded sectors consequently receive a turn without fabricated transit traffic.

## Additional EVE evidence

Static inspection of the supplied code.ccp archive selected industry/storage.pyj, modifiers.pyj, activities/copying.pyj, activities/reaction.pyj, grpc/message_bus/JobMessenger.pyj, facilitySvc.pyj and npsStationTaxModifier.pyj. The existing local Python 2.7 reader decoded bytecode; this is not original source recovery or CCP server-code access. Raw evidence remains in Saved/Analysis/Phase5-Background-Economy-REA.

The client describes multi-product results, licensed blueprint-copy runs, material/time/cost modifiers, structured job notifications, facility cache invalidation and remotely supplied station taxes. SSW already has multi-output jobs, authored lanes, provenance and inventory reservations. These observations support those designs but do not establish CCP NPC production, budgets or pricing algorithms. SSW autonomous budgets, stock consumption, production and freight remain explicitly SSW policies informed by public static relationships, with SSW balance.

The pinned public SDE build is 3714173; it is a historical snapshot, not a claim of the latest release or a matching client version. Archive SHA-256: 328170f160fca4d355e70912e1b1ceda7781ac38416074a03c1ddf9af5e28581.
