#include "flutter_webrtc/flutter_web_r_t_c_plugin.h"

#include "flutter_common.h"
#include "flutter_webrtc.h"

const char* kChannelName = "FlutterWebRTC.Method";

namespace flutter_webrtc_plugin {

// A webrtc plugin for windows/linux.
class FlutterWebRTCPluginImpl : public FlutterWebRTCPlugin {
 public:
  static void RegisterWithRegistrar(PluginRegistrar* registrar) {
    auto channel = std::make_unique<MethodChannel>(
        registrar->messenger(), kChannelName,
        &flutter::StandardMethodCodec::GetInstance());

    auto* channel_pointer = channel.get();

    // Uses new instead of make_unique due to private constructor.
    std::unique_ptr<FlutterWebRTCPluginImpl> plugin(
        new FlutterWebRTCPluginImpl(registrar, std::move(channel)));

    channel_pointer->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto& call, auto result) {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  }

  virtual ~FlutterWebRTCPluginImpl() {}

  BinaryMessenger* messenger() { return messenger_; }

  TextureRegistrar* textures() { return textures_; }

 private:
  // Creates a plugin that communicates on the given channel.
  FlutterWebRTCPluginImpl(PluginRegistrar* registrar,
                          std::unique_ptr<MethodChannel> channel)
      : channel_(std::move(channel)),
        messenger_(registrar->messenger()),
        textures_(registrar->texture_registrar()) {
    // Deliberately does NOT construct FlutterWebRTC here. Its base constructor
    // calls LibWebRTC::CreateRTCPeerConnectionFactory(), which initialises the
    // WebRTC audio device module. On a machine with no usable audio backend
    // (no PulseAudio server, no sound card, a confined sandbox, a CI runner)
    // that CHECK-fails in adm_helpers.cc and aborts the entire process during
    // plugin registration - before any Dart code has run, so the app cannot
    // even start. Construct it on first use instead, so a machine without
    // audio can still run everything that is not a call.
  }

  // Called when a method is called on |channel_|;
  void HandleMethodCall(const MethodCall& method_call,
                        std::unique_ptr<MethodResult> result) {
    // handle method call and forward to webrtc native sdk.
    if (!webrtc_) {
      webrtc_ = std::make_unique<FlutterWebRTC>(this);
    }
    auto method_call_proxy = MethodCallProxy::Create(method_call);
    webrtc_->HandleMethodCall(*method_call_proxy.get(),
                              MethodResultProxy::Create(std::move(result)));
  }

 private:
  std::unique_ptr<MethodChannel> channel_;
  std::unique_ptr<FlutterWebRTC> webrtc_;
  BinaryMessenger* messenger_;
  TextureRegistrar* textures_;
};

}  // namespace flutter_webrtc_plugin


void FlutterWebRTCPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  static auto* plugin_registrar = new flutter::PluginRegistrar(registrar);
  flutter_webrtc_plugin::FlutterWebRTCPluginImpl::RegisterWithRegistrar(
      plugin_registrar);
}