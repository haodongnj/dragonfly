// Copyright 2024, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <absl/container/node_hash_map.h>

#include <vector>

#include "server/db_slice.h"
#include "server/tx_base.h"

namespace dfly {

class Tenant {
 public:
  explicit Tenant(ShardId shard_count);

  DbSlice& GetCurrentDbSlice();

  DbSlice& GetDbSlice(ShardId sid);
  const DbSlice& GetDbSlice(ShardId sid) const;

 private:
  std::vector<DbSlice> shard_db_slices_;
};

class Tenants {
 public:
  Tenants();

  Tenant& GetDefaultTenant();
  Tenant& GetOrInsert(std::string_view tenant);
  const Tenant& Get(std::string_view tenant) const;

 private:
  // TODO: when initializing:
  // db_slice_(pb->GetPoolIndex(), GetFlag(FLAGS_cache_mode), this) {
  // db_slice_.UpdateExpireBase(absl::GetCurrentTimeNanos() / 1000000, 0);

  absl::node_hash_map<std::string, Tenant> tenants_;
  Tenant* default_tenant_ = nullptr;
};

extern Tenants* tenants;

}  // namespace dfly
