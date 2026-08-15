#define XBOX_IMPL
#include "xbox.h"

static bool _hasVideoClass = false;

#define FORCE_VIDEO_SINGLETON() \
    do { \
        if (_hasVideoClass) { \
            xbox::logError("Cannot create second class! Pass a reference instead.", "xbox::Video", true); XUnreachable();} \
        else _hasVideoClass = true; \
    } while(0)

#define SINGLETON_GONE() _hasVideoClass = false

xbox::Video::Video() {
    FORCE_VIDEO_SINGLETON();
    initVideo({1080, 1920}, {640, 480, 32, REFRESH_DEFAULT});
}

xbox::Video::Video(const resolution_t &maxResolution) {
    FORCE_VIDEO_SINGLETON();
    initVideo(maxResolution, {640, 480, 32, REFRESH_DEFAULT});
}

xbox::Video::Video(const int width, const int height) {
    FORCE_VIDEO_SINGLETON();
    initVideo({height, width}, {640, 480, 32, REFRESH_DEFAULT});
}

xbox::Video::Video(const VIDEO_MODE &vm) {
    FORCE_VIDEO_SINGLETON();
    initVideo({vm.width, vm.height}, vm);
}

xbox::Video::Video(const resolution_t &maxResolution, const VIDEO_MODE &minimum_vm) {
    FORCE_VIDEO_SINGLETON();
    initVideo(maxResolution, minimum_vm);
}

xbox::Video::~Video() {
    SINGLETON_GONE();
}

xbox::resolution_t xbox::Video::getCurrentResolution() { 
    return resolution; 
}

void xbox::Video::initVideo(const resolution_t &maxResolution, const VIDEO_MODE &minimum_vm) {
    VIDEO_MODE res = getHighestSupportedResolution(minimum_vm, maxResolution);
    XVideoSetMode(res.width, res.height, res.bpp, res.refresh);
    resolution = {res.height, res.width};
}

void xbox::Video::aliasSmoothing(bool enable) {
    if(!softenSet || softenFilter != enable) {
        AvSendTVEncoderOption((PVOID)VIDEO_BASE, VIDEO_ENC_SOFTEN_FILTER, (int)enable, nullptr);
        softenSet = true;
        softenFilter = enable;
    }
}

void xbox::Video::interlaceFilter(int level) {
    if(!flickerSet || (level & 0x07) != flickerLevel) {
        AvSendTVEncoderOption((PVOID)VIDEO_BASE, VIDEO_ENC_FLICKERFILTER, (level & 0x07), nullptr);
        flickerSet = true;
        flickerLevel = level & 0x07;
    }
}

void xbox::Video::waitForVBlank() {
    XVideoWaitForVBlank();
}

void xbox::Video::set(unsigned char *fb) {
    XVideoSetFB(fb);
}

void xbox::Video::flush() {
    asm __volatile__("sfence");
}

void xbox::Video::enable(bool enable) {
    if (encEnabled == enable) return;
    AvSendTVEncoderOption((PVOID)VIDEO_BASE, VIDEO_ENC_VIDEOENABLE, !enable, nullptr);
    encEnabled = enable;
}

bool xbox::Video::enabled() { 
    return encEnabled; 
}

int xbox::Video::memory() { 
    return 1024 * 1024 * 4; 
}

VIDEO_MODE xbox::Video::getHighestSupportedResolution(const VIDEO_MODE &minimum_vm, const resolution_t &maxResolution) {
    VIDEO_MODE vm;
    VIDEO_MODE highest = minimum_vm; 
    void *context = nullptr;
    DWORD maxPixels = 0;

    while (XVideoListModes(&vm, 0, 0, &context)) {
        if (vm.width > maxResolution.width || vm.height > maxResolution.height) continue;
        DWORD currentPixels = (DWORD)vm.width * vm.height;
        if (currentPixels > maxPixels) {
            maxPixels = currentPixels;
            highest = vm;
        }
    }
    return highest;
}