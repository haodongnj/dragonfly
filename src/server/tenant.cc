#include "server/tenant.h"

#include "base/flags.h"
#include "base/logging.h"
#include "server/engine_shard_set.h"

ABSL_DECLARE_FLAG(bool, cache_mode);

namespace dfly {

using namespace std;

Tenants* tenants = new Tenants();

Tenant::Tenant() {
  shard_db_slices_.resize(shard_set->size());
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

Tenants::Tenants() {
  default_tenant_ = &GetOrInsert("");
}

Tenant& Tenants::GetDefaultTenant() {
  return *default_tenant_;
}

Tenant& Tenants::GetOrInsert(std::string_view tenant) {
  return tenants_[tenant];
}

}  // namespace dfly
