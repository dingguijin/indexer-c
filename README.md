# indexer-c

Pure C11 indexer skeleton for DEX pool discovery/state-sync milestones.

## Install dependencies

```bash
sudo apt-get install -y build-essential cmake pkg-config git \
    libcurl4-openssl-dev libhiredis-dev libssl-dev ca-certificates
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

```bash
./build/bin/indexer --chain eth --config config/indexer.toml
```

## Logging

`BOTINDEX_LOG=debug|info|warn|error` (default: `info`). Logs are emitted to stderr.

## Milestone scope

M1 includes:
- CLI config loading for `--chain` and `--config`
- Redis connection + chain DB `SELECT` + `meta:chain_id` sentinel check
- JSON-RPC startup probe (`eth_chainId`) and HTTP polling (`eth_blockNumber`)
- `eth_getBlockByNumber` on new head and `HSET pool:head block/hash/ts`
- Structured logs and graceful SIGINT/SIGTERM shutdown

Deferred to M2+:
- WebSocket/newHeads streaming
- Reorg detection/backfill
- UniV2/UniV3 adapters and pool discovery
- State diff, TVL, and bot-side logic
