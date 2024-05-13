// Copyright 2024, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <absl/container/node_hash_map.h>

#include <memory>
#include <string>
#include <vector>

#include "server/db_slice.h"
#include "server/tx_base.h"

namespace dfly {

class Tenant {
 public:
  explicit Tenant();

  DbSlice& GetCurrentDbSlice();

  DbSlice& GetDbSlice(ShardId sid);

 private:
  std::vector<std::unique_ptr<DbSlice>> shard_db_slices_;
};

class Tenants {
 public:
  Tenants();

  Tenant& GetDefaultTenant();
  Tenant& GetOrInsert(std::string_view tenant);

 private:
  absl::node_hash_map<std::string, Tenant> tenants_;
  Tenant* default_tenant_ = nullptr;
};

extern Tenants* tenants;

}  // namespace dfly
