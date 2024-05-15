#include "server/tenant.h"

#include "base/flags.h"
#include "base/logging.h"
#include "server/engine_shard_set.h"

ABSL_DECLARE_FLAG(bool, cache_mode);

namespace dfly {

using namespace std;

Tenants* tenants = nullptr;

Tenant::Tenant() {
  shard_db_slices_.resize(shard_set->size());
  shard_blocking_controller_.resize(shard_set->size());
  shard_set->RunBriefInParallel([&](EngineShard* es) {
    ShardId sid = es->shard_id();
    shard_db_slices_[sid] = make_unique<DbSlice>(sid, absl::GetFlag(FLAGS_cache_mode), es);
    shard_db_slices_[sid]->UpdateExpireBase(absl::GetCurrentTimeNanos() / 1000000, 0);
  });
}

DbSlice& Tenant::GetCurrentDbSlice() {
  EngineShard* es = EngineShard::tlocal();
  CHECK(es != nullptr);
  return GetDbSlice(es->shard_id());
}

DbSlice& Tenant::GetDbSlice(ShardId sid) {
  CHECK_LT(sid, shard_db_slices_.size());
  return *shard_db_slices_[sid];
}

BlockingController* Tenant::GetOrAddBlockingController(EngineShard* shard) {
  if (!shard_blocking_controller_[shard->shard_id()]) {
    shard_blocking_controller_[shard->shard_id()].reset(new BlockingController(shard));
  }

  return shard_blocking_controller_[shard->shard_id()].get();
}

BlockingController* Tenant::GetBlockingController(ShardId sid) {
  return shard_blocking_controller_[sid].get();
}

Tenants::Tenants() {
}

void Tenants::Reset() {
  std::lock_guard guard(mu_);
  for (auto& tenant : tenants_) {
    shard_set->RunBriefInParallel([&](EngineShard* es) {
      tenant.second.GetCurrentDbSlice().UpdateExpireBase(TEST_current_time_ms - 1000, 0);
    });
  }
}

void Tenants::Init() {
  CHECK(default_tenant_ == nullptr);
  default_tenant_ = &GetOrInsert("");
}

Tenant& Tenants::GetDefaultTenant() const {
  return *default_tenant_;
}

Tenant& Tenants::GetOrInsert(std::string_view tenant) {
  std::lock_guard guard(mu_);

  return tenants_[tenant];
}

}  // namespace dfly
