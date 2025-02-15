#include "extensions/transport_sockets/tap/config.h"

#include "envoy/config/tap/v3alpha/common.pb.h"
#include "envoy/extensions/transport_sockets/tap/v3alpha/tap.pb.h"
#include "envoy/extensions/transport_sockets/tap/v3alpha/tap.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/config/utility.h"
#include "common/protobuf/utility.h"

#include "extensions/transport_sockets/tap/tap.h"
#include "extensions/transport_sockets/tap/tap_config_impl.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tap {

class SocketTapConfigFactoryImpl : public Extensions::Common::Tap::TapConfigFactory {
public:
  SocketTapConfigFactoryImpl(TimeSource& time_source) : time_source_(time_source) {}

  // TapConfigFactory
  Extensions::Common::Tap::TapConfigSharedPtr
  createConfigFromProto(envoy::config::tap::v3alpha::TapConfig&& proto_config,
                        Extensions::Common::Tap::Sink* admin_streamer) override {
    return std::make_shared<SocketTapConfigImpl>(std::move(proto_config), admin_streamer,
                                                 time_source_);
  }

private:
  TimeSource& time_source_;
};

Network::TransportSocketFactoryPtr UpstreamTapSocketConfigFactory::createTransportSocketFactory(
    const Protobuf::Message& message,
    Server::Configuration::TransportSocketFactoryContext& context) {
  const auto& outer_config = MessageUtil::downcastAndValidate<
      const envoy::extensions::transport_sockets::tap::v3alpha::Tap&>(
      message, context.messageValidationVisitor());
  auto& inner_config_factory = Config::Utility::getAndCheckFactory<
      Server::Configuration::UpstreamTransportSocketConfigFactory>(
      outer_config.transport_socket().name());
  ProtobufTypes::MessagePtr inner_factory_config = Config::Utility::translateToFactoryConfig(
      outer_config.transport_socket(), context.messageValidationVisitor(), inner_config_factory);
  auto inner_transport_factory =
      inner_config_factory.createTransportSocketFactory(*inner_factory_config, context);
  return std::make_unique<TapSocketFactory>(
      outer_config, std::make_unique<SocketTapConfigFactoryImpl>(context.dispatcher().timeSource()),
      context.admin(), context.singletonManager(), context.threadLocal(), context.dispatcher(),
      std::move(inner_transport_factory));
}

Network::TransportSocketFactoryPtr DownstreamTapSocketConfigFactory::createTransportSocketFactory(
    const Protobuf::Message& message, Server::Configuration::TransportSocketFactoryContext& context,
    const std::vector<std::string>& server_names) {
  const auto& outer_config = MessageUtil::downcastAndValidate<
      const envoy::extensions::transport_sockets::tap::v3alpha::Tap&>(
      message, context.messageValidationVisitor());
  auto& inner_config_factory = Config::Utility::getAndCheckFactory<
      Server::Configuration::DownstreamTransportSocketConfigFactory>(
      outer_config.transport_socket().name());
  ProtobufTypes::MessagePtr inner_factory_config = Config::Utility::translateToFactoryConfig(
      outer_config.transport_socket(), context.messageValidationVisitor(), inner_config_factory);
  auto inner_transport_factory = inner_config_factory.createTransportSocketFactory(
      *inner_factory_config, context, server_names);
  return std::make_unique<TapSocketFactory>(
      outer_config, std::make_unique<SocketTapConfigFactoryImpl>(context.dispatcher().timeSource()),
      context.admin(), context.singletonManager(), context.threadLocal(), context.dispatcher(),
      std::move(inner_transport_factory));
}

ProtobufTypes::MessagePtr TapSocketConfigFactory::createEmptyConfigProto() {
  return std::make_unique<envoy::extensions::transport_sockets::tap::v3alpha::Tap>();
}

REGISTER_FACTORY(UpstreamTapSocketConfigFactory,
                 Server::Configuration::UpstreamTransportSocketConfigFactory);

REGISTER_FACTORY(DownstreamTapSocketConfigFactory,
                 Server::Configuration::DownstreamTransportSocketConfigFactory);

} // namespace Tap
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
