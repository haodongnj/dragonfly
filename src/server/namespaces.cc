#include "server/namespaces.h"

#include "base/flags.h"
#include "base/logging.h"
#include "server/engine_shard_set.h"

ABSL_DECLARE_FLAG(bool, cache_mode);

namespace dfly {

using namespace std;

Namespaces* namespaces = nullptr;

Namespace::Namespace() {
  shard_db_slices_.resize(shard_set->size());
  shard_blocking_controller_.resize(shard_set->size());
  shard_set->RunBriefInParallel([&](EngineShard* es) {
    CHECK(es != nullptr);
    ShardId sid = es->shard_id();
    shard_db_slices_[sid] = make_unique<DbSlice>(sid, absl::GetFlag(FLAGS_cache_mode), es);
    shard_db_slices_[sid]->UpdateExpireBase(absl::GetCurrentTimeNanos() / 1000000, 0);
  });
}

DbSlice& Namespace::GetCurrentDbSlice() {
  EngineShard* es = EngineShard::tlocal();
  CHECK(es != nullptr);
  return GetDbSlice(es->shard_id());
}

DbSlice& Namespace::GetDbSlice(ShardId sid) {
  CHECK_LT(sid, shard_db_slices_.size());
  return *shard_db_slices_[sid];
}

BlockingController* Namespace::GetOrAddBlockingController(EngineShard* shard) {
  if (!shard_blocking_controller_[shard->shard_id()]) {
    shard_blocking_controller_[shard->shard_id()].reset(new BlockingController(shard));
  }

  return shard_blocking_controller_[shard->shard_id()].get();
}

BlockingController* Namespace::GetBlockingController(ShardId sid) {
  return shard_blocking_controller_[sid].get();
}

Namespaces::~Namespaces() {
  shard_set->RunBriefInParallel([&](EngineShard* es) {
    CHECK(es != nullptr);
    for (auto& ns : namespaces_) {
      ns.second.shard_db_slices_[es->shard_id()].reset();
    }
  });
}

void Namespaces::Init() {
  DCHECK(default_namespace_ == nullptr);
  default_namespace_ = &GetOrInsert("");
}

bool Namespaces::IsInitialized() const {
  return default_namespace_ != nullptr;
}

Namespace& Namespaces::GetDefaultNamespace() const {
  return *default_namespace_;
}

Namespace& Namespaces::GetOrInsert(std::string_view ns) {
  std::lock_guard guard(mu_);

  return namespaces_[ns];
}

}  // namespace dfly
