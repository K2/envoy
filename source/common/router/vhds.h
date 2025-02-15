#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "envoy/config/core/v3alpha/config_source.pb.h"
#include "envoy/config/route/v3alpha/route_components.pb.h"
#include "envoy/config/subscription.h"
#include "envoy/http/codes.h"
#include "envoy/local_info/local_info.h"
#include "envoy/router/rds.h"
#include "envoy/router/route_config_update_receiver.h"
#include "envoy/server/filter_config.h"
#include "envoy/service/discovery/v3alpha/discovery.pb.h"
#include "envoy/singleton/instance.h"
#include "envoy/stats/scope.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/logger.h"
#include "common/init/target_impl.h"
#include "common/protobuf/utility.h"

namespace Envoy {
namespace Router {

#define ALL_VHDS_STATS(COUNTER)                                                                    \
  COUNTER(config_reload)                                                                           \
  COUNTER(update_empty)

struct VhdsStats {
  ALL_VHDS_STATS(GENERATE_COUNTER_STRUCT)
};

class VhdsSubscription : Envoy::Config::SubscriptionCallbacks,
                         Logger::Loggable<Logger::Id::router> {
public:
  VhdsSubscription(RouteConfigUpdatePtr& config_update_info,
                   Server::Configuration::ServerFactoryContext& factory_context,
                   const std::string& stat_prefix,
                   std::unordered_set<RouteConfigProvider*>& route_config_providers,
                   const envoy::config::core::v3alpha::ConfigSource::XdsApiVersion xds_api_version =
                       envoy::config::core::v3alpha::ConfigSource::AUTO);
  ~VhdsSubscription() override { init_target_.ready(); }

  void registerInitTargetWithInitManager(Init::Manager& m) { m.add(init_target_); }

private:
  // Config::SubscriptionCallbacks
  void onConfigUpdate(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>&,
                      const std::string&) override {
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }
  void
  onConfigUpdate(const Protobuf::RepeatedPtrField<envoy::service::discovery::v3alpha::Resource>&,
                 const Protobuf::RepeatedPtrField<std::string>&, const std::string&) override;
  void onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                            const EnvoyException* e) override;
  std::string resourceName(const ProtobufWkt::Any& resource) override {
    return MessageUtil::anyConvert<envoy::config::route::v3alpha::VirtualHost>(resource).name();
  }
  std::string loadTypeUrl();

  RouteConfigUpdatePtr& config_update_info_;
  Stats::ScopePtr scope_;
  VhdsStats stats_;
  std::unique_ptr<Envoy::Config::Subscription> subscription_;
  Init::TargetImpl init_target_;
  std::unordered_set<RouteConfigProvider*>& route_config_providers_;
  envoy::config::core::v3alpha::ConfigSource::XdsApiVersion xds_api_version_;
};

using VhdsSubscriptionPtr = std::unique_ptr<VhdsSubscription>;

} // namespace Router
} // namespace Envoy
