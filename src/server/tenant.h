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
#include "util/fibers/synchronization.h"

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
  util::fb2::Mutex mu_{};
  absl::node_hash_map<std::string, Tenant> tenants_ ABSL_GUARDED_BY(mu_);
  Tenant* default_tenant_ ABSL_GUARDED_BY(mu_) = nullptr;
};

extern Tenants* tenants;

}  // namespace dfly
