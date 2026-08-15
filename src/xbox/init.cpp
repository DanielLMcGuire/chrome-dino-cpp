#define XBOX_INIT
#include "xbox.h"

static bool _hasBuiltArgs = false;

std::vector<char *> __xbx_buildArgs() {
    if (_hasBuiltArgs) return {};
    static std::string xbeFile(
        reinterpret_cast<const char*>(XeImageFileName->Buffer),
        XeImageFileName->Length
    );
    _hasBuiltArgs = true;
    return { xbeFile.data() };
}

void __xbx_init(void) {
    std::vector<char *> args = __xbx_buildArgs();
    int ret = USER_MAIN(args.size(), args.data());
    if (ret != 0) {
        debugPrint("main exited with non-zero exit code: %d\n", ret);
        BARE_CHECK_VIDEO_CLASS
            // need to check if we are enabled to display error
        { 
            if (!XBOX_VIDEO_M_CLASS->enabled()) 
                XBOX_VIDEO_M_CLASS->enable(true);
        } else 
            // cannot know we are setup for sure
        if (!_hasVideoClass) {
            CREATE_VIDEO_CLASS(640, 480);
            DELETE_VIDEO_CLASS(); // only need to init, deconstruct wont deinit
            constexpr bool vEnabled = true;
            AvSendTVEncoderOption((PVOID)VIDEO_BASE, VIDEO_ENC_VIDEOENABLE, !vEnabled, nullptr);
         }
        xbox::LED::set({xbox::LED::Color::RED, xbox::LED::Color::OFF, xbox::LED::Color::RED, xbox::LED::Color::OFF});
        Sleep(15000);
    }
    DELETE_VIDEO_CLASS();
    xbox::reboot(); // prevent hang by rebooting
}